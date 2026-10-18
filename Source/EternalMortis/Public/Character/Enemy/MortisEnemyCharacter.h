// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "MortisEnemyData.h"
#include "Character/MortisCharacterBase.h"
#include "Interfaces/MortisSpawnableInterface.h"
#include "Types/MortisStructTypes.h"
#include "MortisEnemyCharacter.generated.h"

class UMortisEnemyCombatComponent;
class UMortisEnemyData;
class UMortisEnemyUIComponent;
class UMortisWidgetBase;
class UWidgetComponent;
/**
 * 
 */

UCLASS()
class ETERNALMORTIS_API AMortisEnemyCharacter : public AMortisCharacterBase, public IMortisSpawnableInterface
{
	GENERATED_BODY()

public:
	AMortisEnemyCharacter(const FObjectInitializer& ObjectInitializer);
	void InitializeEnemyCharacter();
	
	//~ Begin AActor Interfaces 
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	//~ End AActor Interfaces
	
	//~ Begin AMortisCharacterBase Interfaces
	virtual void StartDeath() override;
	virtual void FinishDeath() override;
	//~ End AMortisCharacterBase Interfaces

	//~ Begin IMortisCombatInterface
	FORCEINLINE virtual UMortisCombatComponent* GetCombatComponent() const override;
	//~ End IMortisCombatInterface

	//~ Begin IMortisSpawnableInterace
	virtual void OnSpawned(const FGameplayEffectSpecHandle& EffectSpecHandle) override;
	//~ End IMortisSpawnableInterace
	
public:
	UMortisEnemyData* GetEnemyData() const;

	void SetEnemyHealthBarCombatVisibility(bool bShouldShow);
	
	UMortisEnemyCombatComponent* GetEnemyCombatComponent() const;
	FORCEINLINE UMortisEnemyUIComponent* GetEnemyUIComponent() const { return EnemyUIComponent; }

	float GetRandomStrafingDistance() const;

protected:
	//~ Begin APawn Interfaces
	virtual void PossessedBy(AController* NewController) override;
	//~ End APawn Interfaces
	
	void InitializeEnemyByData();
	void InitializeEnemyHUD();
	void UpdateEnemyHealthBarWidgetLocation();
	void ApplyMaterialSet(const FMortisMaterialSet& MaterialSet);
	
	UPROPERTY(EditDefaultsOnly, Category = "Mortis|Data")
	TObjectPtr<UMortisEnemyData> EnemyData;

	UPROPERTY(EditDefaultsOnly, Category = "Mortis|Data")
	FMortisEnemyStats EnemyStats;	
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mortis|Component")
	TObjectPtr<UMortisEnemyCombatComponent> EnemyCombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mortis|Component")
	TObjectPtr<UMortisEnemyUIComponent> EnemyUIComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mortis|UI")
	TObjectPtr<UWidgetComponent> EnemyHealthBarWidgetComponent;

	UPROPERTY(Transient)
	TObjectPtr<UMortisWidgetBase> ActiveEnemyHealthBarWidget;

	FTimerHandle NormalEnemyHideLastDamageTextTimerHandle;
	FTimerHandle NormalEnemyHideHealthBarTimerHandle;

	bool bNormalEnemyDeathHealthBarSequenceStarted = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mortis|UI")
	float EnemyHealthBarHeightOffset = 30.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mortis|Component")
	FGameplayTag CurrentPhase = MortisGameplayTags::State_Enemy_Phase_1;
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void ApplyEnemyStats();
	void ApplyFloorScaling(FMortisEnemyStats& EnemyStatsToApply) const;
	void RegisterStateTagEvent();
	void OnStrafingStateChanged(FGameplayTag Tag, int32 NewCount);
	void OnChasingStateChanged(FGameplayTag Tag, int32 NewCount) const;
	void UnbindNormalEnemyHealthBarScaling();
	void StartNormalEnemyHealthBarDeathSequence();
	void HideNormalEnemyLastDamageText();
	void HideNormalEnemyHealthBar();
	void ClearNormalEnemyHealthBarDeathTimers();
	
	UFUNCTION()
	void OnOwnerEnemyDestroyed(AActor* OwnerEnemy);
	
	UFUNCTION()
	void HandleNormalEnemyHealthChanged(float NewHealth, float MaxHealth);
	
	FTimerHandle TimerHandle;
};
