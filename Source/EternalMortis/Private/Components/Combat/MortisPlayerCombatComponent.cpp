// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/Combat/MortisPlayerCombatComponent.h"
#include "Character/Player/MortisPlayerCharacter.h"
#include "Items/Weapons/MortisPlayerWeapon.h"
#include "Components/UI/MortisPlayerUIComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "MortisGameplayTags.h"

#include "MortisDebugHelper.h"
#include "Character/Enemy/MortisEnemyCharacter.h"
#include "Types/MortisCombatTypes.h"

AMortisPlayerWeapon* UMortisPlayerCombatComponent::GetPlayerCarriedWeaponByTag(FGameplayTag WeaponTag) const
{
	return Cast<AMortisPlayerWeapon>(GetCharacterCarriedWeaponByTag(WeaponTag));
}

AMortisPlayerWeapon* UMortisPlayerCombatComponent::GetPlayerCurrentEquippedWeapon() const
{
	return Cast<AMortisPlayerWeapon>(GetCharacterCurrentEquippedWeapon());
}

FMortisPlayerWeaponData UMortisPlayerCombatComponent::GetPlayerCurrentWeaponData() const
{
	if (!GetPlayerCurrentEquippedWeapon())
	{
		MORTIS_LOG("Current Weapon is invalid!");
		return FMortisPlayerWeaponData();
	}
	return GetPlayerCurrentEquippedWeapon()->PlayerWeaponData;
}

void UMortisPlayerCombatComponent::SetPotionCount(int32 NewPotionCount)
{
	PotionCount = FMath::Max(0, NewPotionCount);
	BroadcastPotionCountChanged();
}

void UMortisPlayerCombatComponent::ResetPotionCount(int32 NewPotionCount)
{
	PotionCount = FMath::Max(0, NewPotionCount);
	PotionUsedCount = 0;
	BroadcastPotionCountChanged();
}

void UMortisPlayerCombatComponent::UsePotion()
{
	PotionCount--;
	PotionUsedCount++;
	BroadcastPotionCountChanged();
}

void UMortisPlayerCombatComponent::OnHitTargetActor(AActor* HitActor, const FHitResult& HitResult)
{
	if (!AddUniqueOverlappedActor(HitActor))
	{
		return;
	}
    FGameplayEventData Data;
    Data.Instigator = GetOwningPawn();
    Data.Target = HitActor;

	IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(GetOwner());
	if (!AbilitySystemInterface)
	{
		return;
	}
	UAbilitySystemComponent* PlayerASC = AbilitySystemInterface->GetAbilitySystemComponent();
	if (!PlayerASC)
	{
		return;
	}
	
	if (AMortisEnemyCharacter* Enemy = Cast<AMortisEnemyCharacter>(HitActor))
	{
		UAbilitySystemComponent* TargetASC = Enemy->GetAbilitySystemComponent();
		if (!TargetASC)
		{
			return;
		}

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			GetOwningPawn(),
			MortisGameplayTags::Event_Combat_AttackHit,
			Data
		);
		
		FGameplayTag DynamicCueTag = DefaultHitCueTag;
		if (CombatVfxData)
		{
			for (const TPair<FGameplayTag, FGameplayTag>& Pair : CombatVfxData->MaterialToHitCueMap)
			{
				if (TargetASC->HasMatchingGameplayTag(Pair.Key))
				{
					DynamicCueTag = Pair.Value;
					break;
				}
			}
		}
		
		FGameplayCueParameters CueParams;
		CueParams.Location = HitResult.ImpactPoint;
		CueParams.Normal = HitResult.ImpactNormal;
		CueParams.Instigator = GetOwningPawn();
		CueParams.EffectContext = PlayerASC->MakeEffectContext();
		CueParams.EffectContext.AddHitResult(HitResult);
		
		TargetASC->ExecuteGameplayCue(DynamicCueTag, CueParams);
	}
	else if (HitActor->GetRootComponent() && HitActor->GetComponentByClass<UStaticMeshComponent>())
	{
		OnHitEnvironment.Broadcast(HitActor, HitResult, MortisGameplayTags::Data_AttackStyle_Swing);
	}
}

void UMortisPlayerCombatComponent::OnWeaponPulledFromTargetActor(AActor* InteractedActor)
{

}

void UMortisPlayerCombatComponent::BroadcastPotionCountChanged() const
{
	const AMortisPlayerCharacter* PlayerCharacter = Cast<AMortisPlayerCharacter>(GetOwner());
	if (!PlayerCharacter)
	{
		return;
	}

	if (UMortisPlayerUIComponent* PlayerUIComponent = PlayerCharacter->GetPlayerUIComponent())
	{
		PlayerUIComponent->OnPotionCountChanged.Broadcast(PotionCount);
	}
}
