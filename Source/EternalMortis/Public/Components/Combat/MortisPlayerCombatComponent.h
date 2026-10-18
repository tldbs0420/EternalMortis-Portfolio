// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Combat/MortisCombatComponent.h"
#include "MortisPlayerCombatComponent.generated.h"

class AMortisPlayerWeapon;
struct FMortisPlayerWeaponData;

UCLASS()
class ETERNALMORTIS_API UMortisPlayerCombatComponent : public UMortisCombatComponent
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Mortis|Combat")
	AMortisPlayerWeapon* GetPlayerCarriedWeaponByTag(FGameplayTag WeaponTag) const;

	UFUNCTION(BlueprintCallable, Category = "Mortis|Combat")
	AMortisPlayerWeapon* GetPlayerCurrentEquippedWeapon() const;

	UFUNCTION(BlueprintCallable, Category = "Mortis|Combat")
	FMortisPlayerWeaponData GetPlayerCurrentWeaponData() const;

	UFUNCTION(BlueprintCallable, Category = "Mortis|Combat")
	void SetPotionCount(int32 NewPotionCount);

	UFUNCTION(BlueprintCallable, Category = "Mortis|Combat")
	void ResetPotionCount(int32 NewPotionCount = 5);

	UFUNCTION(BlueprintCallable, Category = "Mortis|Combat")
	void UsePotion();

	virtual void OnHitTargetActor(AActor* HitActor, const FHitResult& HitResult) override;
	virtual void OnWeaponPulledFromTargetActor(AActor* InteractedActor) override;

	UPROPERTY(BlueprintReadWrite, Category = "Mortis|Combat")
	int32 PotionCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mortis|Potion")
	int32 PotionUsedCount = 0;

private:
	void BroadcastPotionCountChanged() const;
};
