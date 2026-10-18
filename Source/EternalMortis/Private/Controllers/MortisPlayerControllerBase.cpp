// Fill out your copyright notice in the Description page of Project Settings.


#include "Controllers/MortisPlayerControllerBase.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Widget.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"

void AMortisPlayerControllerBase::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (!bShowMouseCursor)
	{
		GamepadCursorInput = FVector2D::ZeroVector;
		return;
	}

	if (GamepadCursorInput.IsNearlyZero(GamepadCursorDeadZone))
	{
		return;
	}

	int32 ViewportX = 0;
	int32 ViewportY = 0;
	GetViewportSize(ViewportX, ViewportY);

	if (ViewportX <= 0 || ViewportY <= 0)
	{
		return;
	}

	float MouseX = 0.f;
	float MouseY = 0.f;

	if (!GetMousePosition(MouseX, MouseY))
	{
		SetMouseLocation(ViewportX / 2, ViewportY / 2);
		return;
	}

	const float NewX = FMath::Clamp(
		MouseX + GamepadCursorInput.X * GamepadCursorSpeed * DeltaTime,
		0.f,
		static_cast<float>(ViewportX - 1)
	);

	const float NewY = FMath::Clamp(
		MouseY - GamepadCursorInput.Y * GamepadCursorSpeed * DeltaTime,
		0.f,
		static_cast<float>(ViewportY - 1)
	);

	SetMouseLocation(FMath::RoundToInt(NewX), FMath::RoundToInt(NewY));
}

void AMortisPlayerControllerBase::SetGamepadCursorInput(FVector2D InInput)
{
	GamepadCursorInput = InInput;
}

void AMortisPlayerControllerBase::ResetGamepadCursorInput()
{
	GamepadCursorInput = FVector2D::ZeroVector;
}

bool AMortisPlayerControllerBase::ClickHoveredButton()
{
	if (!bShowMouseCursor)
	{
		return false;
	}

	TArray<UUserWidget*> FoundWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(
		GetWorld(),
		FoundWidgets,
		UUserWidget::StaticClass(),
		false
	);

	UButton* HoveredButton = nullptr;
	UComboBoxString* HoveredComboBox = nullptr;

	for (UUserWidget* UserWidget : FoundWidgets)
	{
		if (!IsValid(UserWidget) || !UserWidget->WidgetTree)
		{
			continue;
		}

		UserWidget->WidgetTree->ForEachWidget(
			[&HoveredButton, &HoveredComboBox](UWidget* Widget)
			{
				if (!IsValid(Widget) || !Widget->GetIsEnabled() || !Widget->IsHovered())
				{
					return;
				}

				if (UButton* Button = Cast<UButton>(Widget))
				{
					HoveredButton = Button;
					return;
				}

				if (UComboBoxString* ComboBox = Cast<UComboBoxString>(Widget))
				{
					HoveredComboBox = ComboBox;
					return;
				}
			});
	}

	if (HoveredButton)
	{
		HoveredButton->OnClicked.Broadcast();
		return true;
	}

	if (HoveredComboBox)
	{
		const int32 OptionCount = HoveredComboBox->GetOptionCount();
		if (OptionCount <= 0)
		{
			return false;
		}

		int32 SelectedIndex = HoveredComboBox->GetSelectedIndex();

		if (SelectedIndex == INDEX_NONE)
		{
			SelectedIndex = 0;
		}
		else
		{
			SelectedIndex = (SelectedIndex + 1) % OptionCount;
		}

		HoveredComboBox->SetSelectedIndex(SelectedIndex);
		return true;
	}

	return false;
}

bool AMortisPlayerControllerBase::AdjustHoveredSlider(float Direction)
{
	if (!bShowMouseCursor)
	{
		return false;
	}

	if (FMath::IsNearlyZero(Direction))
	{
		return false;
	}

	TArray<UUserWidget*> FoundWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(
		GetWorld(),
		FoundWidgets,
		UUserWidget::StaticClass(),
		false
	);

	USlider* HoveredSlider = nullptr;

	for (UUserWidget* UserWidget : FoundWidgets)
	{
		if (!IsValid(UserWidget) || !UserWidget->WidgetTree)
		{
			continue;
		}

		UserWidget->WidgetTree->ForEachWidget([&HoveredSlider](UWidget* Widget)
			{
				if (HoveredSlider)
				{
					return;
				}

				if (USlider* Slider = Cast<USlider>(Widget))
				{
					if (Slider->IsHovered() && Slider->GetIsEnabled())
					{
						HoveredSlider = Slider;
					}
				}
			});
	}

	if (!HoveredSlider)
	{
		return false;
	}

	const float CurrentValue = HoveredSlider->GetValue();

	const float Sign = Direction > 0.f ? 1.f : -1.f;
	const float NewValue = FMath::Clamp(
		CurrentValue + Sign * SliderStep,
		0.f,
		1.f
	);

	HoveredSlider->SetValue(NewValue);

	return true;
}

void AMortisPlayerControllerBase::ApplyMenuInputMode(UUserWidget* FocusWidget)
{
	ResetGamepadCursorInput();

	FInputModeGameAndUI InputMode;

	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);

	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void AMortisPlayerControllerBase::RestoreGameplayInputMode()
{
	ResetGamepadCursorInput();

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	bShowMouseCursor = false;
}