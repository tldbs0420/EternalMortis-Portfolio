// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MortisGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class ETERNALMORTIS_API UMortisGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

private:
	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);
};
