// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/MortisEnemyCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "MortisDebugHelper.h"
#include "AbilitySystem/MortisAbilitySystemComponent.h"
#include "AbilitySystem/Data/MortisEnemyAbilitySet.h"
#include "Character/Enemy/MortisEnemyData.h"
#include "Components/Combat/MortisEnemyCombatComponent.h"
#include "Components/UI/MortisEnemyUIComponent.h"
#include "AbilitySystem/Attributes/MortisEnemyAttributeSet.h"
#include "Animation/MortisEnemyAnimInstance.h"
#include "BehaviorTree/BehaviorTree.h"

#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Controllers/MortisAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Spawn/MortisEnemySettings.h"
#include "System/MortisEnemyDataSubsystem.h"
#include "System/MortisGameInstance.h"
#include "System/MortisMetaProgressionSubsystem.h"
#include "System/MortisRunStateSubsystem.h"
#include "UI/MortisWidgetBase.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float NormalEnemyHideLastDamageTextDelay = 1.0f;
	constexpr float NormalEnemyHideHealthBarDelay = 0.5f;
}

AMortisEnemyCharacter::AMortisEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UMortisEnemyAttributeSet>(TEXT("MortisAttributeSet")))
{
	static ConstructorHelpers::FClassFinder<UUserWidget> EnemyHealthBarWidgetClass(TEXT("/Game/UI/EnemyHUD/WBP_EnemyHPBar"));

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;

	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	
	EnemyCombatComponent = CreateDefaultSubobject<UMortisEnemyCombatComponent>("EnemyCombatComponent");
	EnemyUIComponent = CreateDefaultSubobject<UMortisEnemyUIComponent>(TEXT("EnemyUIComponent"));

	EnemyHealthBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("EnemyHealthBarWidget"));
	EnemyHealthBarWidgetComponent->SetupAttachment(GetRootComponent());
	EnemyHealthBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	EnemyHealthBarWidgetComponent->SetDrawAtDesiredSize(true);
	EnemyHealthBarWidgetComponent->SetGenerateOverlapEvents(false);
	EnemyHealthBarWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EnemyHealthBarWidgetComponent->SetPivot(FVector2D(0.5f, 1.f));
	EnemyHealthBarWidgetComponent->SetVisibility(true);

	if (EnemyHealthBarWidgetClass.Succeeded())
	{
		EnemyHealthBarWidgetComponent->SetWidgetClass(EnemyHealthBarWidgetClass.Class);
	}

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AMortisEnemyCharacter::InitializeEnemyCharacter()
{
	InitializeEnemyByData();
	RegisterStateTagEvent();
	InitializeEnemyHUD();
}

void AMortisEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ThisClass::PrintRotation, 0.01f, true, -1);
}

void AMortisEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearNormalEnemyHealthBarDeathTimers();
	UnbindNormalEnemyHealthBarScaling();

	Super::EndPlay(EndPlayReason);
}

void AMortisEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	// PrintRotation();
	// bool A = GetCharacterMovement()->bOrientRotationToMovement;
	// bool B = GetCharacterMovement()->bUseControllerDesiredRotation;
	// bool C = bUseControllerRotationYaw;
	// FString DebugString = FString::Printf(TEXT("OrientToMovement: %s, UseControllerRotation: %s, UseControllerYaw: %s"), *Debug::ToString(A), *Debug::ToString(B), *Debug::ToString(C));
	// DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.f, -50.f, 100.f), DebugString, this, FColor::Yellow, 0.f);
	//
}

void AMortisEnemyCharacter::StartDeath()
{
	Super::StartDeath();
	
	if (AMortisAIController* AIC = Cast<AMortisAIController>(GetController()))
	{
		AIC->OnEnemyDead();
	}
	StartNormalEnemyHealthBarDeathSequence();
	// GetMesh()->SetSimulatePhysics(true);
}

void AMortisEnemyCharacter::FinishDeath()
{
	// Drop Items
	if (GetWorld() && GetWorld()->GetGameInstance() && EnemyData)
	{
		if (UMortisRunStateSubsystem* RunStateSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UMortisRunStateSubsystem>())
		{
			RunStateSubsystem->AddGold(FMath::RandRange(EnemyStats.DropData.MinGold, EnemyStats.DropData.MaxGold));
			// MORTIS_LOG("Gold: %d", RunStateSubsystem->GetCurrentGold());
		}
		
		if (UMortisMetaProgressionSubsystem* MetaProgressionSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UMortisMetaProgressionSubsystem>())
		{
			MetaProgressionSubsystem->AddMemoryFragments(FMath::RandRange(EnemyStats.DropData.MinGold, EnemyStats.DropData.MaxGold));
			// MORTIS_LOG("Memory Fragments: %d", MetaProgressionSubsystem->GetMemoryFragments());
		}
	}
	
	Super::FinishDeath();
}

void AMortisEnemyCharacter::InitializeEnemyHUD()
{
	UpdateEnemyHealthBarWidgetLocation();
	UnbindNormalEnemyHealthBarScaling();

	if (!EnemyHealthBarWidgetComponent)
	{
		return;
	}

	EnemyHealthBarWidgetComponent->InitWidget();
	const bool bUseBossHealthBar = EnemyData && EnemyData->bIsBoss;
	EnemyHealthBarWidgetComponent->SetVisibility(!bUseBossHealthBar);

	if (bUseBossHealthBar)
	{
		return;
	}

	UUserWidget* UserWidget = EnemyHealthBarWidgetComponent->GetUserWidgetObject();
	if (!UserWidget || !EnemyUIComponent)
	{
		return;
	}

	if (UMortisWidgetBase* MortisWidget = Cast<UMortisWidgetBase>(UserWidget))
	{
		MortisWidget->BP_BindUIComponent(EnemyUIComponent);
		ActiveEnemyHealthBarWidget = MortisWidget;
		EnemyUIComponent->OnHealthChanged.AddUniqueDynamic(this, &ThisClass::HandleNormalEnemyHealthChanged);

		float CurrentHealth = 0.0f;
		float MaxHealth = 0.0f;
		if (EnemyUIComponent->GetCurrentHealthSnapshot(CurrentHealth, MaxHealth))
		{
			HandleNormalEnemyHealthChanged(CurrentHealth, MaxHealth);
		}
	}
}

void AMortisEnemyCharacter::InitializeEnemyByData()
{
	if (!ensureMsgf(EnemyData, TEXT("%s: Enemy Data is not set"), *GetName()))
	{
		return;
	}

	check(GetCapsuleComponent() && GetMesh() && GetCharacterMovement());
	
	GetCapsuleComponent()->InitCapsuleSize(EnemyData->CapsuleRadius, EnemyData->CapsuleHalfHeight);
	
	USkeletalMesh* SelectedMesh = EnemyData->EnemyMesh;
	if (EnemyData->bEnableVisualVariations && !EnemyData->MeshPool.IsEmpty())
	{
		int32 RandomIndex = FMath::RandRange(0, EnemyData->MeshPool.Num() - 1);
		if (EnemyData->MeshPool[RandomIndex])
		{
			SelectedMesh = EnemyData->MeshPool[RandomIndex];
		}
	}

	GetMesh()->SetSkeletalMesh(SelectedMesh);
	
	if (MortisAbilitySystemComponent)
	{
		MortisAbilitySystemComponent->AddLooseGameplayTag(EnemyData->SkinMaterialTag);
	}
	
	if (EnemyData->bEnableVisualVariations && !EnemyData->MaterialSetPool.IsEmpty())
	{
		int32 RandomIndex = FMath::RandRange(0, EnemyData->MaterialSetPool.Num() - 1);
		ApplyMaterialSet(EnemyData->MaterialSetPool[RandomIndex]);
	}
	
	GetMesh()->SetRelativeScale3D(EnemyData->MeshScale);
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -EnemyData->CapsuleHalfHeight) + EnemyData->MeshOffset);
	GetMesh()->SetRelativeRotation(EnemyData->MeshRotation);

	if (EnemyData->AnimClass)
	{
		GetMesh()->SetAnimInstanceClass(EnemyData->AnimClass);
	}

	if (EnemyData->LinkedAnimLayerClass)
	{
		GetMesh()->LinkAnimClassLayers(EnemyData->LinkedAnimLayerClass);
	}
	
	GetCharacterMovement()->RotationRate = EnemyData->RotationRate;
	GetCharacterMovement()->MaxWalkSpeed = EnemyData->IdleMaxWalkSpeed;
	GetCharacterMovement()->BrakingDecelerationWalking = EnemyData->BrakingDecelerationWalking;

	CharacterAbilitySet = EnemyData->AbilitySet;
	
	// is Game playing
	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		if (EnemyData->AttackPatternData)
		{
			if (EnemyCombatComponent)
			{
				EnemyCombatComponent->SetAttackPattern(EnemyData->AttackPatternData);
			}
		}
	}
	UpdateEnemyHealthBarWidgetLocation();
}

UMortisEnemyData* AMortisEnemyCharacter::GetEnemyData() const
{
	return EnemyData;
}

UMortisCombatComponent* AMortisEnemyCharacter::GetCombatComponent() const
{
	return EnemyCombatComponent;
}

void AMortisEnemyCharacter::OnSpawned(const FGameplayEffectSpecHandle& EffectSpecHandle)
{
	InitializeEnemyCharacter();
	if (AMortisEnemyCharacter* Enemy = Cast<AMortisEnemyCharacter>(GetInstigator()))
	{
		Enemy->OnStartDeath.AddUObject(this, &ThisClass::OnOwnerEnemyDestroyed);
		Enemy->OnDestroyed.AddDynamic(this, &ThisClass::OnOwnerEnemyDestroyed);
	}
}

UMortisEnemyCombatComponent* AMortisEnemyCharacter::GetEnemyCombatComponent() const
{
	return EnemyCombatComponent;
}

float AMortisEnemyCharacter::GetRandomStrafingDistance() const
{
	if (!EnemyData || !EnemyData->PhaseStrafingRanges.Contains(CurrentPhase))
	{
		return 500.f;
	}
	return FMath::RandRange(EnemyData->PhaseStrafingRanges[CurrentPhase].MinDistance, EnemyData->PhaseStrafingRanges[CurrentPhase].MaxDistance);
}

void AMortisEnemyCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	ApplyEnemyStats();
}

void AMortisEnemyCharacter::SetEnemyHealthBarCombatVisibility(bool bShouldShow)
{
	(void)bShouldShow;

	// Deprecated: enemy HUD visibility is now owned by the widget blueprint.
}

void AMortisEnemyCharacter::UpdateEnemyHealthBarWidgetLocation()
{
	if (!EnemyHealthBarWidgetComponent || !GetCapsuleComponent())
	{
		return;
	}

	const float CapsuleHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	EnemyHealthBarWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, CapsuleHalfHeight + EnemyHealthBarHeightOffset));
}

void AMortisEnemyCharacter::ApplyMaterialSet(const FMortisMaterialSet& MaterialSet)
{
	for (int32 i = 0; i < MaterialSet.Materials.Num(); i++)
	{
		if (UMaterialInterface* Material = MaterialSet.Materials[i])
		{
			GetMesh()->SetMaterial(i, Material);
		}
	}
}

#if WITH_EDITOR
void AMortisEnemyCharacter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName ChangedPropertyName = PropertyChangedEvent.GetPropertyName();
	if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, EnemyData))
	{
		if (EnemyData)
		{
			InitializeEnemyByData();
		}
	}

	UpdateEnemyHealthBarWidgetLocation();
}
#endif

void AMortisEnemyCharacter::ApplyEnemyStats()
{
	if (!EnemyData)
	{
		MORTIS_LOG("Enemy Data is null");
		return;
	}
	
	FMortisEnemyStats* EnemyStatPointer = EnemyData->StatRowHandle.GetRow<FMortisEnemyStats>(TEXT("Enemy Stats"));
	if (!EnemyStatPointer)
	{
		MORTIS_LOG("Fail to get Stats");
		return;
	}
	
	EnemyStats = *EnemyStatPointer;
	ApplyFloorScaling(EnemyStats);
	
	if (!EnemyData->AbilitySet)
	{
		return;
	}
	EnemyData->AbilitySet->GiveToEnemy(MortisAbilitySystemComponent, EnemyStats);
}

void AMortisEnemyCharacter::ApplyFloorScaling(FMortisEnemyStats& EnemyStatsToApply) const
{	
	// MORTIS_LOG("");
	if (!GetWorld() || !GetWorld()->GetGameInstance())
	{
		return;
	}
	
	if (EnemyData->bIsBoss)
	{
		return;
	}
	const UMortisRunStateSubsystem* RunStateSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UMortisRunStateSubsystem>();
	const UMortisEnemyDataSubsystem* EnemyDataSubsystem = GetWorld()->GetSubsystem<UMortisEnemyDataSubsystem>();
	if (!RunStateSubsystem || !EnemyDataSubsystem)
	{
		MORTIS_LOG("Fail to get Subsystem");
		return;
	}
	
	const UMortisEnemyScalingData* ScalingData = EnemyDataSubsystem->GetScalingData();
	if (!ScalingData)
	{
		MORTIS_LOG("Fail to get ScalingData");
		return;
	}
	
	float CurrentFloor = RunStateSubsystem->GetCurrentFloor();
	EnemyStatsToApply.BaseDamage *= ScalingData->GetMultiplier(MortisScalingCurveNames::BaseDamage, CurrentFloor);
	EnemyStatsToApply.MaxHealth *= ScalingData->GetMultiplier(MortisScalingCurveNames::MaxHealth, CurrentFloor);
	EnemyStatsToApply.MaxPoise *= ScalingData->GetMultiplier(MortisScalingCurveNames::MaxPoise, CurrentFloor);
}

void AMortisEnemyCharacter::RegisterStateTagEvent()
{
	if (!MortisAbilitySystemComponent)
	{
		MORTIS_LOG("ASC is null");
		return;
	}
	
	MortisAbilitySystemComponent->RegisterGameplayTagEvent(
		MortisGameplayTags::State_Movement_Strafing,
		EGameplayTagEventType::NewOrRemoved
	).AddUObject(this, &ThisClass::OnStrafingStateChanged);

	MortisAbilitySystemComponent->RegisterGameplayTagEvent(
		MortisGameplayTags::State_Movement_Chasing,
		EGameplayTagEventType::NewOrRemoved
	).AddUObject(this, &ThisClass::OnChasingStateChanged);
}

void AMortisEnemyCharacter::OnStrafingStateChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (!EnemyData)
	{
		return;
	}
	bool bStrafing = NewCount > 0;
	// MORTIS_LOG("Strafing %s", NewCount > 0 ? TEXT("Begin") : TEXT("End"));
	// bUseControllerRotationYaw = bStrafing;
	GetCharacterMovement()->bOrientRotationToMovement = !bStrafing;
	GetCharacterMovement()->bUseControllerDesiredRotation = bStrafing;
	GetCharacterMovement()->MaxWalkSpeed = bStrafing ? EnemyData->StrafingMaxWalkSpeed : EnemyData->IdleMaxWalkSpeed;
}

void AMortisEnemyCharacter::OnChasingStateChanged(FGameplayTag Tag, int32 NewCount) const
{
	if (!EnemyData)
	{
		return;
	}
	// MORTIS_LOG("Chasing %s", NewCount > 0 ? TEXT("Begin") : TEXT("End"));
	bool bChasing = NewCount > 0;
	GetCharacterMovement()->MaxWalkSpeed = bChasing ? EnemyData->ChasingMaxWalkSpeed : EnemyData->IdleMaxWalkSpeed;
}

void AMortisEnemyCharacter::UnbindNormalEnemyHealthBarScaling()
{
	if (EnemyUIComponent)
	{
		EnemyUIComponent->OnHealthChanged.RemoveDynamic(this, &ThisClass::HandleNormalEnemyHealthChanged);
	}

	ActiveEnemyHealthBarWidget = nullptr;
}

void AMortisEnemyCharacter::StartNormalEnemyHealthBarDeathSequence()
{
	if (bNormalEnemyDeathHealthBarSequenceStarted || !EnemyHealthBarWidgetComponent)
	{
		return;
	}

	if (EnemyData && EnemyData->bIsBoss)
	{
		return;
	}

	bNormalEnemyDeathHealthBarSequenceStarted = true;
	EnemyHealthBarWidgetComponent->SetVisibility(true);

	if (ActiveEnemyHealthBarWidget)
	{
		ActiveEnemyHealthBarWidget->StartDeathHealthBarSequence();
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		HideNormalEnemyLastDamageText();
		return;
	}

	World->GetTimerManager().SetTimer(
		NormalEnemyHideLastDamageTextTimerHandle,
		this,
		&ThisClass::HideNormalEnemyLastDamageText,
		NormalEnemyHideLastDamageTextDelay,
		false);
}

void AMortisEnemyCharacter::HideNormalEnemyLastDamageText()
{
	if (ActiveEnemyHealthBarWidget)
	{
		ActiveEnemyHealthBarWidget->HideLastDamageText();
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		HideNormalEnemyHealthBar();
		return;
	}

	World->GetTimerManager().SetTimer(
		NormalEnemyHideHealthBarTimerHandle,
		this,
		&ThisClass::HideNormalEnemyHealthBar,
		NormalEnemyHideHealthBarDelay,
		false);
}

void AMortisEnemyCharacter::HideNormalEnemyHealthBar()
{
	if (ActiveEnemyHealthBarWidget)
	{
		ActiveEnemyHealthBarWidget->HideHealthBar();
	}

	if (EnemyHealthBarWidgetComponent)
	{
		EnemyHealthBarWidgetComponent->SetVisibility(false);
	}
}

void AMortisEnemyCharacter::ClearNormalEnemyHealthBarDeathTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(NormalEnemyHideLastDamageTextTimerHandle);
		World->GetTimerManager().ClearTimer(NormalEnemyHideHealthBarTimerHandle);
	}
}

void AMortisEnemyCharacter::OnOwnerEnemyDestroyed(AActor* OwnerEnemy)
{
	FGameplayEventData Data;
	Data.Instigator = OwnerEnemy;
	Data.EventTag = MortisGameplayTags::Event_Dead;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, Data.EventTag, Data);
}

void AMortisEnemyCharacter::HandleNormalEnemyHealthChanged(float NewHealth, float MaxHealth)
{
	(void)NewHealth;

	if (!ActiveEnemyHealthBarWidget || !EnemyUIComponent || EnemyUIComponent->IsBoss())
	{
		return;
	}

	ActiveEnemyHealthBarWidget->ApplyHealthBarWidthScaling(MaxHealth);
}
