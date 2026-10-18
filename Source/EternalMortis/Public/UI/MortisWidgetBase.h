// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/MortisUIStructTypes.h"
#include "MortisWidgetBase.generated.h"

class UMortisUIComponent;
class USizeBox;

/**
 * 
 */
UCLASS()
class ETERNALMORTIS_API UMortisWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Mortis|UI", meta = (DisplayName = "Bind UI Component"))
	void BP_BindUIComponent(UMortisUIComponent* InUIComponent);

	UFUNCTION(BlueprintCallable, Category = "Mortis|UI|Death Sequence")
	void StartDeathHealthBarSequence();

	UFUNCTION(BlueprintCallable, Category = "Mortis|UI|Death Sequence")
	void HideLastDamageText();

	UFUNCTION(BlueprintCallable, Category = "Mortis|UI|Death Sequence")
	void HideHealthBar();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Mortis|UI|Death Sequence", meta = (DisplayName = "Start Death Health Bar Sequence"))
	void BP_StartDeathHealthBarSequence();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Mortis|UI|Death Sequence", meta = (DisplayName = "Hide Last Damage Text"))
	void BP_HideLastDamageText();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Mortis|UI|Death Sequence", meta = (DisplayName = "Hide Health Bar"))
	void BP_HideHealthBar();

	UFUNCTION(BlueprintCallable, Category = "Mortis|UI|Resource Bar Scaling")
	void ApplyHealthBarWidthScaling(float MaxHealth);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mortis|UI|Resource Bar Scaling")
	FMortisResourceBarScalingSettings HealthBarScalingSettings;

protected:
	float GetBaseWidthForScaling(USizeBox* SizeBox);

private:
	TMap<TWeakObjectPtr<USizeBox>, float> ResourceBarBaseWidths;
};
