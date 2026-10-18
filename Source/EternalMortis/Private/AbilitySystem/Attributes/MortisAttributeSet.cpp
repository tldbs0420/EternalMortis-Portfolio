// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/MortisAttributeSet.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/UI/MortisUIComponent.h"
#include "GameplayEffectExtension.h"
#include "MortisFunctionLibrary.h"
#include "MortisDebugHelper.h"
#include "MortisGameplayTags.h"

namespace
{
	UMortisUIComponent* GetUIComponentFromAttributeData(const FGameplayEffectModCallbackData& Data)
	{
		const AActor* OwningActor = Data.Target.GetAvatarActor();
		return OwningActor ? OwningActor->FindComponentByClass<UMortisUIComponent>() : nullptr;
	}
	
	UMortisUIComponent* GetUIComponentFromAttributeSet(const UAttributeSet* AttributeSet)
	{
		if (!AttributeSet)
		{
			return nullptr;
		}

		const UAbilitySystemComponent* ASC = AttributeSet->GetOwningAbilitySystemComponent();
		const AActor* AvatarActor = ASC ? ASC->GetAvatarActor() : AttributeSet->GetOwningActor();

		return AvatarActor ? AvatarActor->FindComponentByClass<UMortisUIComponent>() : nullptr;
	}

	float ClampHealthToRecoverableCap(const float Health, const float MaxHealth, const float RecoverableHealthCapRate)
	{
		const float SafeMaxHealth = FMath::Max(MaxHealth, 0.0f);
		const float SafeCapRate = FMath::Clamp(RecoverableHealthCapRate, 0.0f, 1.0f);
		const float RecoverableHealthCap = SafeMaxHealth * SafeCapRate;

		return FMath::Clamp(Health, 0.0f, RecoverableHealthCap);
	}

	bool SetCurrentHealthIfChanged(UMortisAttributeSet* AttributeSet, const float NewHealth)
	{
		if (!AttributeSet || FMath::IsNearlyEqual(AttributeSet->GetCurrentHealth(), NewHealth))
		{
			return false;
		}

		AttributeSet->SetCurrentHealth(NewHealth);
		return true;
	}

	float ClampCurrentHealthToRecoverableCapValue(UMortisAttributeSet* AttributeSet, const float MaxHealth, const float RecoverableHealthCapRate)
	{
		if (!AttributeSet)
		{
			return 0.0f;
		}

		const float NewHealth = ClampHealthToRecoverableCap(
			AttributeSet->GetCurrentHealth(),
			MaxHealth,
			RecoverableHealthCapRate);
		SetCurrentHealthIfChanged(AttributeSet, NewHealth);

		return NewHealth;
	}

	void BroadcastHealthChangedFromAttributeSet(const UMortisAttributeSet* AttributeSet)
	{
		if (!AttributeSet)
		{
			return;
		}

		if (UMortisUIComponent* UIComponent = GetUIComponentFromAttributeSet(AttributeSet))
		{
			UIComponent->OnHealthChanged.Broadcast(AttributeSet->GetCurrentHealth(), AttributeSet->GetMaxHealth());
		}
	}

}

UMortisAttributeSet::UMortisAttributeSet()
{
	InitMaxHealth(1.0f);
	InitCurrentHealth(1.0f);
	InitRecoverableHealthCapRate(1.0f);
	InitBaseDamage(1.0f);
	InitResistAll(0.0f);
}

bool UMortisAttributeSet::ClampCurrentHealthToRecoverableCap()
{
	const float NewHealth = ClampHealthToRecoverableCap(
		GetCurrentHealth(),
		GetMaxHealth(),
		GetRecoverableHealthCapRate());

	return SetCurrentHealthIfChanged(this, NewHealth);
}

void UMortisAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetCurrentHealthAttribute())
	{
		NewValue = ClampHealthToRecoverableCap(NewValue, GetMaxHealth(), GetRecoverableHealthCapRate());
	}
}

void UMortisAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	if (Attribute == GetCurrentHealthAttribute())
	{
		NewValue = ClampHealthToRecoverableCap(NewValue, GetMaxHealth(), GetRecoverableHealthCapRate());
	}
}

void UMortisAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	// 체력 변경
	if (Data.EvaluatedData.Attribute == GetCurrentHealthAttribute())
	{
		const float NewCurrentHealth = ClampCurrentHealthToRecoverableCapValue(this, GetMaxHealth(), GetRecoverableHealthCapRate());

		if (UMortisUIComponent* UIComponent = GetUIComponentFromAttributeData(Data))
		{
			UIComponent->OnHealthChanged.Broadcast(NewCurrentHealth, GetMaxHealth());
		}
	}
	// 최대 체력 변경
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		const float NewMaxHealth = GetMaxHealth();
		const float NewCurrentHealth = ClampCurrentHealthToRecoverableCapValue(this, NewMaxHealth, GetRecoverableHealthCapRate());

		if (UMortisUIComponent* UIComponent = GetUIComponentFromAttributeData(Data))
		{
			UIComponent->OnHealthChanged.Broadcast(NewCurrentHealth, NewMaxHealth);
		}
	}
	// 회복 가능한 체력 상한 변경
	else if (Data.EvaluatedData.Attribute == GetRecoverableHealthCapRateAttribute())
	{
		const float NewCurrentHealth = ClampCurrentHealthToRecoverableCapValue(this, GetMaxHealth(), GetRecoverableHealthCapRate());

		if (UMortisUIComponent* UIComponent = GetUIComponentFromAttributeData(Data))
		{
			UIComponent->OnHealthChanged.Broadcast(NewCurrentHealth, GetMaxHealth());
		}
	}
	// 데미지를 입었을 때
	else if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float OldHealth = GetCurrentHealth();
		const float DamageTaken = GetIncomingDamage();

		const float NewHealth = ClampHealthToRecoverableCap(
			OldHealth - DamageTaken,
			GetMaxHealth(),
			GetRecoverableHealthCapRate());
		SetCurrentHealthIfChanged(this, NewHealth);
		//SetIncomingDamage(0.f);

		if (UMortisUIComponent* UIComponent = GetUIComponentFromAttributeData(Data))
		{
			UIComponent->OnHealthChanged.Broadcast(NewHealth, GetMaxHealth());
			UIComponent->OnDamageTaken.Broadcast(DamageTaken, NewHealth, GetMaxHealth());
		}

		// TODO: 사망 상태 태그 추가
		if (GetCurrentHealth() <= 0.0f)
		{
			if (!Data.Target.HasMatchingGameplayTag(MortisGameplayTags::State_Dead))
				Data.Target.AddLooseGameplayTag(MortisGameplayTags::State_Dead);
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwningActor(), MortisGameplayTags::Event_Dead, FGameplayEventData());
		}
	}
	else if (Data.EvaluatedData.Attribute == GetIncomingPoiseDamageAttribute())
	{
		float NewPoise = FMath::Clamp(GetCurrentPoise() - GetIncomingPoiseDamage(), 0.f, GetMaxPoise());
		SetCurrentPoise(NewPoise);
		SetIncomingPoiseDamage(0.f);
		
		// if Poise == 0, send HitReact Event to target
		if (FMath::IsNearlyZero(NewPoise))
		{
			FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
			FGameplayEventData EventData;
			EventData.Instigator = Context.GetInstigator();
			EventData.Target = Data.Target.GetAvatarActor();
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
				GetOwningActor(),
				MortisGameplayTags::Event_Action_HitReact,
				EventData
			);
			// Reset MaxPoise
			SetCurrentPoise(GetMaxPoise());
		}
	}
}

void UMortisAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxHealthAttribute())
	{
		const float NewMaxHealth = FMath::Max(NewValue, 0.0f);
		float TargetHealth = GetCurrentHealth();

		if (NewValue > OldValue)
		{
			TargetHealth += (NewValue - OldValue);
		}

		const float NewCurrentHealth = ClampHealthToRecoverableCap(TargetHealth, NewMaxHealth, GetRecoverableHealthCapRate());
		SetCurrentHealthIfChanged(this, NewCurrentHealth);
		BroadcastHealthChangedFromAttributeSet(this);
	}

	else if (Attribute == GetRecoverableHealthCapRateAttribute())
	{
		const float NewHealth = ClampHealthToRecoverableCap(GetCurrentHealth(), GetMaxHealth(), NewValue);
		SetCurrentHealthIfChanged(this, NewHealth);
		BroadcastHealthChangedFromAttributeSet(this);
	}
	else if (Attribute == GetCurrentHealthAttribute())
	{
		const float NewHealth = ClampHealthToRecoverableCap(NewValue, GetMaxHealth(), GetRecoverableHealthCapRate());
		SetCurrentHealthIfChanged(this, NewHealth);
		BroadcastHealthChangedFromAttributeSet(this);
	}
}
