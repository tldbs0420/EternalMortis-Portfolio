// Fill out your copyright notice in the Description page of Project Settings.


#include "System/MortisGameInstance.h"

#include "System/MortisOptionsSettings.h"
#include "UObject/UObjectGlobals.h"

void UMortisGameInstance::Init()
{
	Super::Init();

	MortisOptionsSettings::ApplySavedMasterVolume(GetWorld());
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMapWithWorld);
}

void UMortisGameInstance::Shutdown()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	Super::Shutdown();
}

void UMortisGameInstance::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
	MortisOptionsSettings::ApplySavedMasterVolume(LoadedWorld);
}
