// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MortisPlayerControllerBase.generated.h"

class UUserWidget;

/**
 * 
 */
UCLASS()
class ETERNALMORTIS_API AMortisPlayerControllerBase : public APlayerController
{
	GENERATED_BODY()
	
protected:
	virtual void PlayerTick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mortis|UI")
	float GamepadCursorSpeed = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mortis|UI")
	float GamepadCursorDeadZone = 0.1f;

	UPROPERTY(BlueprintReadOnly, Category = "Mortis|UI")
	FVector2D GamepadCursorInput = FVector2D::ZeroVector;

public:
	UFUNCTION(BlueprintCallable, Category = "Mortis|UI")
	void SetGamepadCursorInput(FVector2D InInput);

	UFUNCTION(BlueprintCallable, Category = "Mortis|UI")
	void ResetGamepadCursorInput();

	UFUNCTION(BlueprintCallable, Category = "Mortis|UI")
	bool ClickHoveredButton();

	UFUNCTION(BlueprintCallable, Category = "Mortis|UI")
	bool AdjustHoveredSlider(float Direction);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mortis|UI")
	float SliderStep = 0.05f;

protected:
	void ApplyMenuInputMode(UUserWidget* FocusWidget);
	void RestoreGameplayInputMode();
};
