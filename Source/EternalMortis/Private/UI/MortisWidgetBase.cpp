// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MortisWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

namespace
{
	const FName DamageTextWidgetName(TEXT("Text_Damage"));
}

void UMortisWidgetBase::StartDeathHealthBarSequence()
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	BP_StartDeathHealthBarSequence();
}

void UMortisWidgetBase::HideLastDamageText()
{
	BP_HideLastDamageText();

	if (UWidget* DamageTextWidget = WidgetTree ? WidgetTree->FindWidget(DamageTextWidgetName) : nullptr)
	{
		DamageTextWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UMortisWidgetBase::HideHealthBar()
{
	BP_HideHealthBar();
	SetVisibility(ESlateVisibility::Hidden);
}

void UMortisWidgetBase::ApplyHealthBarWidthScaling(float MaxHealth)
{
	USizeBox* BarRoot = WidgetTree ? Cast<USizeBox>(WidgetTree->FindWidget(TEXT("SizeBox_BarRoot"))) : nullptr;
	if (!BarRoot)
	{
		return;
	}

	const float BaseWidth = GetBaseWidthForScaling(BarRoot);
	if (BaseWidth <= 0.0f)
	{
		return;
	}

	BarRoot->SetWidthOverride(BaseWidth * HealthBarScalingSettings.CalculateScale(MaxHealth));
}

float UMortisWidgetBase::GetBaseWidthForScaling(USizeBox* SizeBox)
{
	if (!SizeBox)
	{
		return 0.0f;
	}

	const TWeakObjectPtr<USizeBox> SizeBoxKey(SizeBox);
	if (const float* CachedWidth = ResourceBarBaseWidths.Find(SizeBoxKey))
	{
		return *CachedWidth;
	}

	const float BaseWidth = SizeBox->IsWidthOverride() ? SizeBox->GetWidthOverride() : SizeBox->GetDesiredSize().X;
	ResourceBarBaseWidths.Add(SizeBoxKey, BaseWidth);
	return BaseWidth;
}
