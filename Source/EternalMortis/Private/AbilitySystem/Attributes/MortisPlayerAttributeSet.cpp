// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/MortisPlayerAttributeSet.h"

#include "AbilitySystemComponent.h"
#include "Character/Player/MortisPlayerCharacter.h"
#include "Components/UI/MortisPlayerUIComponent.h"
#include "GameplayEffectExtension.h"

#include "MortisDebugHelper.h"

namespace
{
	UMortisPlayerUIComponent* GetPlayerUIComponentFromPlayerAttributeData(const FGameplayEffectModCallbackData& Data)
	{
		const AActor* OwningActor = Data.Target.GetAvatarActor();
		const AMortisPlayerCharacter* PlayerCharacter = Cast<AMortisPlayerCharacter>(OwningActor);

		return PlayerCharacter ? PlayerCharacter->FindComponentByClass<UMortisPlayerUIComponent>() : nullptr;
	}

	UMortisPlayerUIComponent* GetPlayerUIComponentFromPlayerAttributeSet(const UAttributeSet* AttributeSet)
	{
		if (!AttributeSet)
		{
			return nullptr;
		}

		const UAbilitySystemComponent* ASC = AttributeSet->GetOwningAbilitySystemComponent();
		const AActor* AvatarActor = ASC ? ASC->GetAvatarActor() : AttributeSet->GetOwningActor();
		const AMortisPlayerCharacter* PlayerCharacter = Cast<AMortisPlayerCharacter>(AvatarActor);

		return PlayerCharacter ? PlayerCharacter->FindComponentByClass<UMortisPlayerUIComponent>() : nullptr;
	}
}

UMortisPlayerAttributeSet::UMortisPlayerAttributeSet()
{
	InitHealingReceivedBonusRate(0.0f);
	InitIncomingHeal(0.0f);
	InitMaxStamina(1.0f);
	InitCurrentStamina(1.0f);
	InitStaminaRegen(1.0f);
	InitStaminaReductionRate(0.0f);
	InitMaxMana(1.0f);
	InitCurrentMana(1.0f);
	InitManaRegen(1.0f);
	InitManaReductionRate(0.0f);
	InitStrength(10.0f);
	InitDexterity(10.0f);
	InitIntelligence(10.0f);
	InitWeaponDamage(0.0f);
	InitStrCoef(0.0f);
	InitDexCoef(0.0f);
	InitIntCoef(0.0f);

	InitRollStaminaReduction(0.0f);
	InitSkillStaminaReduction(0.0f);
	InitSkillBonus(0.0f);
}

void UMortisPlayerAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetCurrentStaminaAttribute())
	{
		const float NewCurrentStamina = FMath::Clamp(GetCurrentStamina(), 0, GetMaxStamina());
		SetCurrentStamina(NewCurrentStamina);

		if (UMortisPlayerUIComponent* PlayerUIComponent = GetPlayerUIComponentFromPlayerAttributeData(Data))
		{
			PlayerUIComponent->OnStaminaChanged.Broadcast(NewCurrentStamina, GetMaxStamina());
		}
	}
	else if (Data.EvaluatedData.Attribute == GetCurrentManaAttribute())
	{
		const float NewCurrentMana = FMath::Clamp(GetCurrentMana(), 0, GetMaxMana());
		SetCurrentMana(NewCurrentMana);

		if (UMortisPlayerUIComponent* PlayerUIComponent = GetPlayerUIComponentFromPlayerAttributeData(Data))
		{
			PlayerUIComponent->OnManaChanged.Broadcast(NewCurrentMana, GetMaxMana());
		}
	}
	else if (Data.EvaluatedData.Attribute == GetIncomingHealAttribute())
	{
		const float RawHealAmount = GetIncomingHeal();
		SetIncomingHeal(0.0f);

		if (RawHealAmount <= 0.0f)
			return;

		const float OldHealth = GetCurrentHealth();
		const float MaxHealthValue = GetMaxHealth();

		const float HealBonusRate = GetHealingReceivedBonusRate();
		const float FinalHealAmount = RawHealAmount * (1.0f + HealBonusRate);

		const float CapRate = FMath::Clamp(GetRecoverableHealthCapRate(), 0.0f, 1.0f);
		const float RecoverableHealthCap = MaxHealthValue * CapRate;

		float NewHealth = OldHealth + FinalHealAmount;

		NewHealth = FMath::Clamp(NewHealth, 0.0f, RecoverableHealthCap);
		SetCurrentHealth(NewHealth);

		if (UMortisUIComponent* UIComponent = GetPlayerUIComponentFromPlayerAttributeData(Data))
			UIComponent->OnHealthChanged.Broadcast(NewHealth, GetMaxHealth());
	}
}

void UMortisPlayerAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxStaminaAttribute())
	{
		const float NewMaxStamina = FMath::Max(NewValue, 0.0f);

		const float NewCurrentStamina = FMath::Clamp(GetCurrentStamina(), 0.0f, NewMaxStamina);
		if (!FMath::IsNearlyEqual(GetCurrentStamina(), NewCurrentStamina))
		{
			SetCurrentStamina(NewCurrentStamina);
		}

		if (UMortisPlayerUIComponent* PlayerUIComponent = GetPlayerUIComponentFromPlayerAttributeSet(this))
		{
			PlayerUIComponent->OnStaminaChanged.Broadcast(NewCurrentStamina, NewMaxStamina);
		}
	}
	else if (Attribute == GetMaxManaAttribute())
	{
		const float NewMaxMana = FMath::Max(NewValue, 0.0f);

		const float NewCurrentMana = FMath::Clamp(GetCurrentMana(), 0.0f, NewMaxMana);
		if (!FMath::IsNearlyEqual(GetCurrentMana(), NewCurrentMana))
		{
			SetCurrentMana(NewCurrentMana);
		}

		if (UMortisPlayerUIComponent* PlayerUIComponent = GetPlayerUIComponentFromPlayerAttributeSet(this))
		{
			PlayerUIComponent->OnManaChanged.Broadcast(NewCurrentMana, NewMaxMana);
		}
	}
}
