// Fill out your copyright notice in the Description page of Project Settings.


#include "Controllers/MortisPlayerController.h"
#include "Types/MortisEnumTypes.h"
#include "Blueprint/UserWidget.h"
#include "Character/Player/MortisPlayerCharacter.h"
#include "Components/Combat/MortisPlayerCombatComponent.h"
#include "Components/InputComponent.h"
#include "Engine/GameInstance.h"
#include "Items/Weapons/MortisPlayerWeapon.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "UI/MortisInGameEscMenuWidget.h"
#include "UI/MortisOptionsMenuWidget.h"
#include "System/MortisMetaProgressionSubsystem.h"
#include "System/MortisRuneDatabaseSubsystem.h"
#include "System/MortisRuneInventorySubsystem.h"
#include "System/MortisRunStateSubsystem.h"
#include "System/MortisWeaponDatabaseSubsystem.h"
#include "System/MortisWeaponInventorySubsystem.h"

namespace
{
	const TCHAR* DefaultInGameOptionsMenuClassPath = TEXT("/Game/UI/MainMenu/WBP_OptionsMenu.WBP_OptionsMenu_C");
	const TCHAR* DefaultRunSettlementWidgetClassPath = TEXT("/Game/UI/InGameUI/WBP_RunSettlement.WBP_RunSettlement_C");

	bool IsManagedUIOpen(const UUserWidget* Widget)
	{
		return IsValid(Widget) && Widget->IsInViewport();
	}

	FLinearColor ResolveWeaponGradeFallbackColor(EMortisWeaponGrade Grade)
	{
		switch (Grade)
		{
		case EMortisWeaponGrade::Rare:
			return FLinearColor(0.38f, 0.64f, 0.98f, 1.0f);
		case EMortisWeaponGrade::Epic:
			return FLinearColor(0.76f, 0.47f, 0.98f, 1.0f);
		case EMortisWeaponGrade::Legendary:
			return FLinearColor(0.95f, 0.72f, 0.25f, 1.0f);
		case EMortisWeaponGrade::Common:
		default:
			return FLinearColor(0.77f, 0.77f, 0.81f, 1.0f);
		}
	}

	FText ResolveRuneGradeText(EMortisRuneGrade Grade)
	{
		switch (Grade)
		{
		case EMortisRuneGrade::Rare:
			return NSLOCTEXT("MortisRunSettlement", "RuneGradeRare", "희귀");
		case EMortisRuneGrade::Epic:
			return NSLOCTEXT("MortisRunSettlement", "RuneGradeEpic", "영웅");
		case EMortisRuneGrade::Legendary:
			return NSLOCTEXT("MortisRunSettlement", "RuneGradeLegendary", "전설");
		case EMortisRuneGrade::Common:
		default:
			return NSLOCTEXT("MortisRunSettlement", "RuneGradeCommon", "일반");
		}
	}
}

void AMortisPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent)
	{
		return;
	}

	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ThisClass::HandleEscapePressed);
	InputComponent->BindKey(EKeys::Gamepad_Special_Left, IE_Pressed, this, &ThisClass::HandleEscapePressed);
}

void AMortisPlayerController::BeginPlay()
{
	Super::BeginPlay();
	RestoreGameplayInputMode();
}

AMortisPlayerController::AMortisPlayerController()
{
	PlayerTeamID = FGenericTeamId(static_cast<uint8>(EMortisTeam::Player));
}

bool AMortisPlayerController::IsInGameMenuOpen() const
{
	return IsValid(ActiveInGameEscMenuWidget)
		|| IsValid(ActiveInGameOptionsMenuWidget)
		|| IsValid(ActiveRunSettlementWidget)
		|| IsManagedUIOpen(CurrentUIWidget);
}

bool AMortisPlayerController::CanOpenUI() const
{
	return !IsManagedUIOpen(CurrentUIWidget);
}

bool AMortisPlayerController::SetCurrentUI(UUserWidget* NewCurrentUI)
{
	if (!IsValid(NewCurrentUI))
	{
		return false;
	}

	if (IsManagedUIOpen(CurrentUIWidget) && CurrentUIWidget != NewCurrentUI)
	{
		return false;
	}

	CurrentUIWidget = NewCurrentUI;
	return true;
}

void AMortisPlayerController::ClearCurrentUI(UUserWidget* UIToClear)
{
	if (!UIToClear || CurrentUIWidget == UIToClear)
	{
		CurrentUIWidget = nullptr;
	}
}

void AMortisPlayerController::CloseCurrentUI()
{
	UUserWidget* UIToClose = CurrentUIWidget;
	CurrentUIWidget = nullptr;

	if (!IsValid(UIToClose))
	{
		return;
	}

	if (UIToClose == ActiveInGameOptionsMenuWidget)
	{
		CloseInGameOptionsMenu(false);
		return;
	}

	if (UIToClose == ActiveInGameEscMenuWidget)
	{
		CloseInGameEscMenu();
		return;
	}

	if (UIToClose == ActiveRunSettlementWidget)
	{
		CloseRunSettlementWidget();
		return;
	}

	UIToClose->RemoveFromParent();
}

bool AMortisPlayerController::IsRunSettlementOpen() const
{
	return IsValid(ActiveRunSettlementWidget);
}

void AMortisPlayerController::ShowRunSettlement(EMortisRunSettlementResult Result)
{
	if (IsValid(ActiveRunSettlementWidget))
	{
		return;
	}

	CloseCurrentUI();
	CloseInGameOptionsMenu(false);
	if (IsValid(ActiveInGameEscMenuWidget))
	{
		UnbindInGameEscMenu();
		ActiveInGameEscMenuWidget->RemoveFromParent();
		ActiveInGameEscMenuWidget = nullptr;
	}

	TSubclassOf<UMortisRunSettlementWidget> ResolvedWidgetClass = ResolveRunSettlementWidgetClass();

	ActiveRunSettlementWidget = CreateWidget<UMortisRunSettlementWidget>(this, ResolvedWidgetClass);
	if (!ActiveRunSettlementWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to create run settlement widget."));
		return;
	}

	ActiveRunSettlementWidget->OnContinueRequested.RemoveDynamic(this, &ThisClass::HandleRunSettlementContinueRequested);
	ActiveRunSettlementWidget->OnContinueRequested.AddDynamic(this, &ThisClass::HandleRunSettlementContinueRequested);
	ActiveRunSettlementWidget->ApplySettlementSnapshot(BuildRunSettlementSnapshot(Result));
	ActiveRunSettlementWidget->AddToViewport(RunSettlementZOrder);
	if (!SetCurrentUI(ActiveRunSettlementWidget))
	{
		CloseRunSettlementWidget();
		return;
	}
	ApplyRunSettlementInputMode();
}

void AMortisPlayerController::ShowRunSettlementCleared()
{
	ShowRunSettlement(EMortisRunSettlementResult::Cleared);
}

void AMortisPlayerController::ShowRunSettlementDefeated()
{
	ShowRunSettlement(EMortisRunSettlementResult::Defeated);
}

void AMortisPlayerController::ShowRunSettlementGaveUp()
{
	ShowRunSettlement(EMortisRunSettlementResult::GaveUp);
}

FGenericTeamId AMortisPlayerController::GetGenericTeamId() const
{
	return PlayerTeamID;
}

void AMortisPlayerController::HandleEscapePressed()
{
	if (IsValid(ActiveRunSettlementWidget))
	{
		return;
	}

	if (IsValid(ActiveInGameOptionsMenuWidget))
	{
		CloseInGameOptionsMenu(true);
		return;
	}

	if (IsValid(ActiveInGameEscMenuWidget))
	{
		CloseInGameEscMenu();
		return;
	}

	// Another screen such as rune unequip already owns visible UI and Escape.
	if (bShowMouseCursor)
	{
		return;
	}

	OpenInGameEscMenu();
}

void AMortisPlayerController::OpenInGameEscMenu()
{
	if (IsValid(ActiveInGameEscMenuWidget))
	{
		if (!SetCurrentUI(ActiveInGameEscMenuWidget))
		{
			return;
		}

		ActiveInGameEscMenuWidget->SetVisibility(ESlateVisibility::Visible);
		ApplyMenuInputMode(ActiveInGameEscMenuWidget);
		return;
	}

	if (!CanOpenUI())
	{
		return;
	}

	TSubclassOf<UMortisInGameEscMenuWidget> ResolvedWidgetClass = InGameEscMenuWidgetClass;
	if (!ResolvedWidgetClass)
	{
		ResolvedWidgetClass = UMortisInGameEscMenuWidget::StaticClass();
	}

	ActiveInGameEscMenuWidget = CreateWidget<UMortisInGameEscMenuWidget>(this, ResolvedWidgetClass);
	if (!ActiveInGameEscMenuWidget)
	{
		return;
	}

	ActiveInGameEscMenuWidget->OnResumeRequested.RemoveDynamic(this, &ThisClass::HandleInGameResumeRequested);
	ActiveInGameEscMenuWidget->OnResumeRequested.AddDynamic(this, &ThisClass::HandleInGameResumeRequested);
	ActiveInGameEscMenuWidget->OnOptionsRequested.RemoveDynamic(this, &ThisClass::HandleInGameOptionsRequested);
	ActiveInGameEscMenuWidget->OnOptionsRequested.AddDynamic(this, &ThisClass::HandleInGameOptionsRequested);
	ActiveInGameEscMenuWidget->OnQuitConfirmed.RemoveDynamic(this, &ThisClass::HandleInGameQuitConfirmed);
	ActiveInGameEscMenuWidget->OnQuitConfirmed.AddDynamic(this, &ThisClass::HandleInGameQuitConfirmed);

	ActiveInGameEscMenuWidget->AddToViewport(InGameMenuZOrder);
	if (!SetCurrentUI(ActiveInGameEscMenuWidget))
	{
		UnbindInGameEscMenu();
		ActiveInGameEscMenuWidget->RemoveFromParent();
		ActiveInGameEscMenuWidget = nullptr;
		return;
	}
	ApplyMenuInputMode(ActiveInGameEscMenuWidget);
}

void AMortisPlayerController::CloseInGameEscMenu()
{
	CloseInGameOptionsMenu(false);

	if (IsValid(ActiveInGameEscMenuWidget))
	{
		ClearCurrentUI(ActiveInGameEscMenuWidget);
		UnbindInGameEscMenu();
		ActiveInGameEscMenuWidget->RemoveFromParent();
		ActiveInGameEscMenuWidget = nullptr;
	}

	RestoreGameplayInputMode();
}

void AMortisPlayerController::OpenInGameOptionsMenu()
{
	if (IsValid(ActiveInGameEscMenuWidget))
	{
		ClearCurrentUI(ActiveInGameEscMenuWidget);
		ActiveInGameEscMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (IsValid(ActiveInGameOptionsMenuWidget))
	{
		if (!SetCurrentUI(ActiveInGameOptionsMenuWidget))
		{
			if (IsValid(ActiveInGameEscMenuWidget))
			{
				ActiveInGameEscMenuWidget->SetVisibility(ESlateVisibility::Visible);
				SetCurrentUI(ActiveInGameEscMenuWidget);
			}
			return;
		}

		ActiveInGameOptionsMenuWidget->RefreshFromSavedSettings();
		ApplyMenuInputMode(ActiveInGameOptionsMenuWidget);
		return;
	}

	TSubclassOf<UMortisOptionsMenuWidget> ResolvedWidgetClass = InGameOptionsMenuWidgetClass;
	if (!ResolvedWidgetClass)
	{
		ResolvedWidgetClass = LoadClass<UMortisOptionsMenuWidget>(nullptr, DefaultInGameOptionsMenuClassPath);
	}
	if (!ResolvedWidgetClass)
	{
		ResolvedWidgetClass = UMortisOptionsMenuWidget::StaticClass();
	}

	ActiveInGameOptionsMenuWidget = CreateWidget<UMortisOptionsMenuWidget>(this, ResolvedWidgetClass);
	if (!ActiveInGameOptionsMenuWidget)
	{
		if (IsValid(ActiveInGameEscMenuWidget))
		{
			ActiveInGameEscMenuWidget->SetVisibility(ESlateVisibility::Visible);
			ApplyMenuInputMode(ActiveInGameEscMenuWidget);
		}
		return;
	}

	ActiveInGameOptionsMenuWidget->OnBackRequested.RemoveDynamic(this, &ThisClass::HandleInGameOptionsBackRequested);
	ActiveInGameOptionsMenuWidget->OnBackRequested.AddDynamic(this, &ThisClass::HandleInGameOptionsBackRequested);
	ActiveInGameOptionsMenuWidget->OnForgetMemoriesApplied.RemoveDynamic(this, &ThisClass::HandleInGameOptionsResetApplied);
	ActiveInGameOptionsMenuWidget->OnForgetMemoriesApplied.AddDynamic(this, &ThisClass::HandleInGameOptionsResetApplied);

	ActiveInGameOptionsMenuWidget->AddToViewport(InGameMenuZOrder + 1);
	if (!SetCurrentUI(ActiveInGameOptionsMenuWidget))
	{
		UnbindInGameOptionsMenu();
		ActiveInGameOptionsMenuWidget->RemoveFromParent();
		ActiveInGameOptionsMenuWidget = nullptr;
		if (IsValid(ActiveInGameEscMenuWidget))
		{
			ActiveInGameEscMenuWidget->SetVisibility(ESlateVisibility::Visible);
			SetCurrentUI(ActiveInGameEscMenuWidget);
			ApplyMenuInputMode(ActiveInGameEscMenuWidget);
		}
		return;
	}
	ActiveInGameOptionsMenuWidget->RefreshFromSavedSettings();
	ApplyMenuInputMode(ActiveInGameOptionsMenuWidget);
}

void AMortisPlayerController::CloseInGameOptionsMenu(bool bReturnToEscMenu)
{
	if (IsValid(ActiveInGameOptionsMenuWidget))
	{
		ClearCurrentUI(ActiveInGameOptionsMenuWidget);
		UnbindInGameOptionsMenu();
		ActiveInGameOptionsMenuWidget->RemoveFromParent();
		ActiveInGameOptionsMenuWidget = nullptr;
	}

	if (bReturnToEscMenu && IsValid(ActiveInGameEscMenuWidget))
	{
		ActiveInGameEscMenuWidget->SetVisibility(ESlateVisibility::Visible);
		SetCurrentUI(ActiveInGameEscMenuWidget);
		ApplyMenuInputMode(ActiveInGameEscMenuWidget);
	}
}

void AMortisPlayerController::ApplyRunSettlementInputMode()
{
	ApplyMenuInputMode(nullptr);
}

void AMortisPlayerController::UnbindInGameEscMenu()
{
	if (!ActiveInGameEscMenuWidget)
	{
		return;
	}

	ActiveInGameEscMenuWidget->OnResumeRequested.RemoveDynamic(this, &ThisClass::HandleInGameResumeRequested);
	ActiveInGameEscMenuWidget->OnOptionsRequested.RemoveDynamic(this, &ThisClass::HandleInGameOptionsRequested);
	ActiveInGameEscMenuWidget->OnQuitConfirmed.RemoveDynamic(this, &ThisClass::HandleInGameQuitConfirmed);
}

void AMortisPlayerController::UnbindInGameOptionsMenu()
{
	if (!ActiveInGameOptionsMenuWidget)
	{
		return;
	}

	ActiveInGameOptionsMenuWidget->OnBackRequested.RemoveDynamic(this, &ThisClass::HandleInGameOptionsBackRequested);
	ActiveInGameOptionsMenuWidget->OnForgetMemoriesApplied.RemoveDynamic(this, &ThisClass::HandleInGameOptionsResetApplied);
}

TSubclassOf<UMortisRunSettlementWidget> AMortisPlayerController::ResolveRunSettlementWidgetClass() const
{
	if (RunSettlementWidgetClass)
	{
		return RunSettlementWidgetClass;
	}

	if (TSubclassOf<UMortisRunSettlementWidget> LoadedClass = LoadClass<UMortisRunSettlementWidget>(nullptr, DefaultRunSettlementWidgetClassPath))
	{
		return LoadedClass;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Run settlement widget class is not set and default widget blueprint could not be loaded: %s. Falling back to the C++ debug widget."),
		DefaultRunSettlementWidgetClassPath);
	return UMortisRunSettlementWidget::StaticClass();
}

FMortisRunSettlementSnapshot AMortisPlayerController::BuildRunSettlementSnapshot(EMortisRunSettlementResult Result) const
{
	FMortisRunSettlementSnapshot Snapshot;
	Snapshot.Result = Result;
	Snapshot.EquippedWeaponName = NSLOCTEXT("MortisRunSettlement", "NoEquippedWeapon", "장착 무기 없음");
	Snapshot.EquippedWeaponColor = FLinearColor(0.58f, 0.54f, 0.50f, 0.78f);

	UGameInstance* GameInstance = GetGameInstance();

	if (GameInstance)
	{
		const UMortisRuneInventorySubsystem* RuneInventorySubsystem = GameInstance->GetSubsystem<UMortisRuneInventorySubsystem>();
		const UMortisRuneDatabaseSubsystem* RuneDatabaseSubsystem = GameInstance->GetSubsystem<UMortisRuneDatabaseSubsystem>();

		const TArray<FMortisRuneInstance>* EquippedRunes = RuneInventorySubsystem ? &RuneInventorySubsystem->GetEquippedRunes() : nullptr;
		// int32 UnlockedRuneSlotCount = RuneInventorySubsystem ? RuneInventorySubsystem->GetSlotCount() : 10;
		const UMortisMetaProgressionSubsystem* MetaProgressionSubsystem = GameInstance->GetSubsystem<UMortisMetaProgressionSubsystem>();
		if (!MetaProgressionSubsystem)
		{
			return Snapshot;
		}
		int UnlockedRuneSlotCount = MetaProgressionSubsystem->GetUnlockedRuneSlotCount();
		for (int32 SlotIndex = 0; SlotIndex < MetaProgressionSubsystem->GetMaxRuneSlotCount(); ++SlotIndex)
		{
			FMortisRunSettlementRuneSlot RuneSlot;
			RuneSlot.bUnlocked = SlotIndex < UnlockedRuneSlotCount;
			const FMortisRuneInstance RuneInstance = EquippedRunes && EquippedRunes->IsValidIndex(SlotIndex)
				? (*EquippedRunes)[SlotIndex]
				: FMortisRuneInstance();

			if (RuneInstance.InstanceId.IsValid())
			{
				RuneSlot.bHasRune = true;
				RuneSlot.RuneInstance = RuneInstance;

				FText SymbolName = NSLOCTEXT("MortisRunSettlement", "RuneNameFallback", "룬");
				if (RuneDatabaseSubsystem)
				{
					if (const FMortisRuneSymbolRow* SymbolRow = RuneDatabaseSubsystem->GetRuneSymbolRow(RuneInstance.SymbolType))
					{
						RuneSlot.DisplayIcon = SymbolRow->Glyph;
						SymbolName = SymbolRow->DisplayName.IsEmpty() ? SymbolName : SymbolRow->DisplayName;
					}

					if (const FMortisRuneSetRow* SetRow = RuneDatabaseSubsystem->GetRuneSetRow(RuneInstance.SetTag))
					{
						RuneSlot.DisplayIconTint = SetRow->GlyphTint;
					}
				}

				RuneSlot.DisplayText = FText::Format(
					NSLOCTEXT("MortisRunSettlement", "RuneSlotTextFormat", "{0} {1}"),
					ResolveRuneGradeText(RuneInstance.Grade),
					SymbolName);
			}

			Snapshot.EquippedRuneSlots.Add(RuneSlot);
		}

		if (const UMortisRunStateSubsystem* RunStateSubsystem = GameInstance->GetSubsystem<UMortisRunStateSubsystem>())
		{
			Snapshot.EarnedGold = RunStateSubsystem->GetRunEarnedGold();
		}

		Snapshot.EarnedMemory = MetaProgressionSubsystem->GetRunEarnedMemoryFragments();

		FMortisWeaponRow WeaponRow;
		bool bHasWeaponRow = false;
		if (const UMortisWeaponInventorySubsystem* WeaponInventorySubsystem = GameInstance->GetSubsystem<UMortisWeaponInventorySubsystem>())
		{
			if (WeaponInventorySubsystem->HasWeapon())
			{
				WeaponRow = WeaponInventorySubsystem->GetCurrentWeapon();
				bHasWeaponRow = true;
			}
		}

		const UMortisWeaponDatabaseSubsystem* WeaponDatabaseSubsystem = GameInstance->GetSubsystem<UMortisWeaponDatabaseSubsystem>();
		if (!bHasWeaponRow && WeaponDatabaseSubsystem)
		{
			if (const UMortisRunStateSubsystem* RunStateSubsystem = GameInstance->GetSubsystem<UMortisRunStateSubsystem>())
			{
				bHasWeaponRow = WeaponDatabaseSubsystem->GetWeaponRowByTag(RunStateSubsystem->GetCurrentWeaponTag(), WeaponRow);
			}
		}

		if (!bHasWeaponRow && WeaponDatabaseSubsystem)
		{
			if (const AMortisPlayerCharacter* PlayerCharacter = Cast<AMortisPlayerCharacter>(GetPawn()))
			{
				if (const UMortisPlayerCombatComponent* PlayerCombatComponent = PlayerCharacter->GetMortisPlayerCombatComponent())
				{
					if (const AMortisPlayerWeapon* EquippedWeapon = PlayerCombatComponent->GetPlayerCurrentEquippedWeapon())
					{
						bHasWeaponRow = WeaponDatabaseSubsystem->GetWeaponRowByClass(EquippedWeapon->GetClass(), WeaponRow);
					}
				}
			}
		}

		if (bHasWeaponRow)
		{
			Snapshot.EquippedWeaponName = WeaponRow.WeaponName.IsEmpty()
				? FText::FromName(WeaponRow.WeaponTag.GetTagName())
				: WeaponRow.WeaponName;
			Snapshot.EquippedWeaponColor = ResolveWeaponGradeFallbackColor(WeaponRow.Grade);
		}
	}

	if (const AMortisPlayerCharacter* PlayerCharacter = Cast<AMortisPlayerCharacter>(GetPawn()))
	{
		if (const UMortisPlayerCombatComponent* PlayerCombatComponent = PlayerCharacter->GetMortisPlayerCombatComponent())
		{
			Snapshot.PotionUsedCount = PlayerCombatComponent->PotionUsedCount;
		}
	}

	return Snapshot;
}

void AMortisPlayerController::CloseRunSettlementWidget()
{
	if (!ActiveRunSettlementWidget)
	{
		return;
	}

	ClearCurrentUI(ActiveRunSettlementWidget);
	ActiveRunSettlementWidget->OnContinueRequested.RemoveDynamic(this, &ThisClass::HandleRunSettlementContinueRequested);
	ActiveRunSettlementWidget->RemoveFromParent();
	ActiveRunSettlementWidget = nullptr;
}

void AMortisPlayerController::HandleInGameResumeRequested()
{
	CloseInGameEscMenu();
}

void AMortisPlayerController::HandleInGameOptionsRequested()
{
	OpenInGameOptionsMenu();
}

void AMortisPlayerController::HandleInGameQuitConfirmed()
{
	ShowRunSettlement(EMortisRunSettlementResult::GaveUp);
}

void AMortisPlayerController::HandleInGameOptionsBackRequested()
{
	CloseInGameOptionsMenu(true);
}

void AMortisPlayerController::HandleInGameOptionsResetApplied()
{
	// Main menu refreshes upgrade data here; in-game Options only needs the reset action itself.
}

void AMortisPlayerController::HandleRunSettlementContinueRequested()
{
	CloseRunSettlementWidget();
	RestoreGameplayInputMode();

	if (!MainMenuLevelName.IsNone())
	{
		UGameplayStatics::OpenLevel(this, MainMenuLevelName);
		return;
	}
}
