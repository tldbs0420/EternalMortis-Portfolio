// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Controllers/MortisPlayerControllerBase.h"
#include "GenericTeamAgentInterface.h"
#include "UI/MortisRunSettlementWidget.h"
#include "MortisPlayerController.generated.h"

class UMortisInGameEscMenuWidget;
class UMortisOptionsMenuWidget;
class UMortisRunSettlementWidget;
class UUserWidget;

/**
 *
 */
UCLASS()
class ETERNALMORTIS_API AMortisPlayerController : public AMortisPlayerControllerBase, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

private:
	FGenericTeamId PlayerTeamID;

protected:
	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mortis|UI|Menu")
	TSubclassOf<UMortisInGameEscMenuWidget> InGameEscMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mortis|UI|Menu")
	TSubclassOf<UMortisOptionsMenuWidget> InGameOptionsMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mortis|UI|Menu")
	int32 InGameMenuZOrder = 1000;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mortis|UI|Menu")
	FName MainMenuLevelName = TEXT("MainMenuTest");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mortis|UI|Run Settlement")
	TSubclassOf<UMortisRunSettlementWidget> RunSettlementWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mortis|UI|Run Settlement")
	int32 RunSettlementZOrder = 1100;

	UPROPERTY(Transient)
	TObjectPtr<UMortisInGameEscMenuWidget> ActiveInGameEscMenuWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMortisOptionsMenuWidget> ActiveInGameOptionsMenuWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMortisRunSettlementWidget> ActiveRunSettlementWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> CurrentUIWidget = nullptr;

public:
	AMortisPlayerController();

	UFUNCTION(BlueprintPure, Category = "Mortis|UI")
	bool IsInGameMenuOpen() const;

	UFUNCTION(BlueprintPure, Category = "Mortis|UI")
	bool CanOpenUI() const;

	UFUNCTION(BlueprintPure, Category = "Mortis|UI")
	UUserWidget* GetCurrentUI() const { return CurrentUIWidget; }

	UFUNCTION(BlueprintCallable, Category = "Mortis|UI")
	bool SetCurrentUI(UUserWidget* NewCurrentUI);

	UFUNCTION(BlueprintCallable, Category = "Mortis|UI")
	void ClearCurrentUI(UUserWidget* UIToClear);

	UFUNCTION(BlueprintCallable, Category = "Mortis|UI")
	void CloseCurrentUI();

	UFUNCTION(BlueprintPure, Category = "Mortis|UI|Run Settlement")
	bool IsRunSettlementOpen() const;

	UFUNCTION(BlueprintCallable, Category = "Mortis|UI|Run Settlement")
	void ShowRunSettlement(EMortisRunSettlementResult Result);

	UFUNCTION(BlueprintCallable, Category = "Mortis|UI|Run Settlement")
	void ShowRunSettlementCleared();

	UFUNCTION(BlueprintCallable, Category = "Mortis|UI|Run Settlement")
	void ShowRunSettlementDefeated();

	UFUNCTION(BlueprintCallable, Category = "Mortis|UI|Run Settlement")
	void ShowRunSettlementGaveUp();

	virtual FGenericTeamId GetGenericTeamId() const override;

private:
	void HandleEscapePressed();

	void OpenInGameEscMenu();
	void CloseInGameEscMenu();

	void OpenInGameOptionsMenu();
	void CloseInGameOptionsMenu(bool bReturnToEscMenu);

	void ApplyRunSettlementInputMode();

	void UnbindInGameEscMenu();
	void UnbindInGameOptionsMenu();

	TSubclassOf<UMortisRunSettlementWidget> ResolveRunSettlementWidgetClass() const;
	FMortisRunSettlementSnapshot BuildRunSettlementSnapshot(EMortisRunSettlementResult Result) const;

	void CloseRunSettlementWidget();

	UFUNCTION()
	void HandleInGameResumeRequested();

	UFUNCTION()
	void HandleInGameOptionsRequested();

	UFUNCTION()
	void HandleInGameQuitConfirmed();

	UFUNCTION()
	void HandleInGameOptionsBackRequested();

	UFUNCTION()
	void HandleInGameOptionsResetApplied();

	UFUNCTION()
	void HandleRunSettlementContinueRequested();
};
