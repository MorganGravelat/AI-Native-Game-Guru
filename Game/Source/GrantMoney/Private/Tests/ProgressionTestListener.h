// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProgressionTestListener.generated.h"

/**
 * Test helper only. Records the leveling and upgrade-choice events it receives, because dynamic
 * delegates can bind only to UFUNCTIONs, not to lambdas inside a test.
 */
UCLASS(Transient, NotBlueprintable, HideDropdown)
class UProgressionTestListener : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void OnLevelUp(int32 NewLevel, int32 PreviousLevel)
	{
		LevelUps.Emplace(NewLevel, PreviousLevel);
	}

	UFUNCTION()
	void OnChoiceReady()
	{
		++ChoiceReadyCount;
	}

	UFUNCTION()
	void OnChoicesResolved()
	{
		++ChoicesResolvedCount;
	}

	// (NewLevel, PreviousLevel), in the order OnLevelUp fired.
	UFUNCTION()
	void OnUpgradeChosen()
	{
		++UpgradeChosenCount;
	}

	UFUNCTION()
	void OnBuffsChanged()
	{
		++BuffsChangedCount;
	}

	TArray<TPair<int32, int32>> LevelUps;

	int32 BuffsChangedCount = 0;

	int32 UpgradeChosenCount = 0;

	int32 ChoiceReadyCount = 0;
	int32 ChoicesResolvedCount = 0;
};
