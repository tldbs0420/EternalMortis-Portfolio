// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnExtensionComponentBase.h"
#include "GameplayTagContainer.h"
#include "Items/Weapons/MortisShieldBase.h"
#include "Types/MortisStructTypes.h"
#include "MortisCombatComponent.generated.h"

class UMortisCombatVFXData;
class AMortisWeaponBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCurrentEquippedWeaponChanged, FGameplayTag, NewWeaponTag, AMortisWeaponBase*, NewWeapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHitEnvironmentSignature, AActor*, HitEnvironmentActor, const FHitResult&, HitResult, const FGameplayTag&, AttackStyleTag);

// 무기, 신체 구분 후 따로 처리
// 상세한 부위는 Tag로 처리하면 됨
UENUM(BlueprintType)
enum class EToggleCollisionType : uint8
{
	CurrentWeapon,
	Body,
};
/**
 * 
 */
UCLASS()
class ETERNALMORTIS_API UMortisCombatComponent : public UPawnExtensionComponentBase
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintAssignable, Category = "Mortis|Combat|VFX")
	FOnHitEnvironmentSignature OnHitEnvironment;
	
	UFUNCTION(BlueprintCallable, Category = "Mortis|Combat")
	void RegisterWeapon(FGameplayTag WeaponTag, AMortisWeaponBase* WeaponToRegister, bool bRegisterAsEquippedWeapon);

	UFUNCTION(BlueprintCallable, Category = "Mortis|Combat")
	void RegisterWeaponToSlot(FGameplayTag SlotTag, AMortisWeaponBase* WeaponToRegister, bool bRegisterAsEquippedWeapon);
	
	UFUNCTION(BlueprintCallable, Category = "Mortis|Combat")
	bool UnregisterSpawnedWeapon(FGameplayTag WeaponTag);
	
	bool UnRegisterWeapon(const FGameplayTag& SlotTag);

	UFUNCTION(BlueprintCallable, Category = "Mortis|Combat")
	AMortisWeaponBase* GetCharacterCarriedWeaponByTag(FGameplayTag TagToGet) const;

	UPROPERTY(BlueprintReadWrite, Category = "Mortis|Combat")
	FGameplayTag CurrentEquippedWeaponTag;

	UFUNCTION(BlueprintCallable, Category = "Mortis|Combat")
	AMortisWeaponBase* GetCharacterCurrentEquippedWeapon() const;

	UPROPERTY(BlueprintAssignable, Category = "Mortis|Combat")
	FOnCurrentEquippedWeaponChanged OnCurrentEquippedWeaponChanged;
	
	FORCEINLINE AMortisWeaponBase* GetEquippedWeaponBySlotTag(const FGameplayTag& SlotTag) const { return WeaponBySlotTag.FindRef(SlotTag); }
	
	UFUNCTION(BlueprintCallable, Category = "Mortis|Combat")
	void ToggleDamageCollision(bool bShouldEnable, FGameplayTag TagToToggle, EToggleCollisionType ToggleDamageType = EToggleCollisionType::CurrentWeapon);

	void ClearWeapons(float LifeSpan = 0.f);
	
	// 아래의 두 Functions은 상속한 CombatComponent에서 구현하기
	virtual void OnHitTargetActor(AActor* HitActor, const FHitResult& HitResult);
	virtual void OnWeaponPulledFromTargetActor(AActor* InteractedActor);

	/* Shield Callbacks */
	virtual void OnShieldBeginBlock(AActor* Weapon);
	virtual void OnShieldEndBlock(AActor* Weapon);
	
	/* Attack Trace */
	UPROPERTY(EditDefaultsOnly, Category = "Mortis|AttackTrace")
	TArray<TEnumAsByte<EObjectTypeQuery>> AttackTraceObjectTypes;
	
	UPROPERTY()
	TArray<FMortisAttackTraceConfig> AttackTraceConfigs;
	
	void BeginAttackTrace(const FMortisAttackTraceConfig& Config);
	void UpdateAttackTrace();
	void EndAttackTrace(const FMortisAttackTraceConfig& Config);
	
	FORCEINLINE void SetCurrentSlotTag(const FGameplayTag& NewSlotTag) { CurrentAttackSlot = NewSlotTag; }
	AMortisWeaponBase* GetCurrentWeapon() const;
	
protected:
	virtual void ToggleCurrentEquippedWeaponCollision(bool bShouldEnable, FGameplayTag TagToToggle);
	virtual void ToggleBodyDamageCollision(bool bShouldEnable, FGameplayTag TagToToggle);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mortis|AttackTrace")
	TArray<TObjectPtr<AActor>> OverlappedActors;

	UPROPERTY(EditDefaultsOnly, Category = "Mortis|Combat|VFX")
	FGameplayTag DefaultHitCueTag = MortisGameplayTags::GameplayCue_Combat_Hit_Flesh;
	
	UPROPERTY(EditDefaultsOnly, Category = "Mortis|Combat|VFX")
	TObjectPtr<UMortisCombatVFXData> CombatVfxData;
	
	bool AddUniqueOverlappedActor(AActor* NewActor);
	FORCEINLINE void ClearOverlappedActors() { OverlappedActors.Empty(); }

private:
	UPROPERTY(VisibleAnywhere, Category = "Mortis|Combat")
	TMap<FGameplayTag, TObjectPtr<AMortisWeaponBase>> CharacterWeaponMap;

	UPROPERTY(VisibleAnywhere, Category = "Mortis|Combat")
	TMap<FGameplayTag, TObjectPtr<AMortisWeaponBase>> WeaponBySlotTag;
	
	FGameplayTag CurrentAttackSlot = FGameplayTag::EmptyTag;
	
	/* Attack Trace */
	FName CurrentTraceSocket;
	TArray<FVector> PreviousTraceStartLocations;
	TArray<FVector> PreviousTraceEndLocations;
	bool bIsTracing = false;
	
	FVector GetSocketLocation(const FMortisAttackTraceConfig& Config, bool bStart) const;
};
