// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnemyEmergenceComponent.h"
#include "RunController.h"
#include "UObject/Object.h"
#include "RunTestListener.generated.h"

/**
 * Test helper only. Records the run events it receives, because dynamic delegates can bind
 * only to UFUNCTIONs, not to lambdas inside a test.
 */
UCLASS(Transient, NotBlueprintable, HideDropdown)
class URunTestListener : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void OnStateChanged(EGrantMoneyRunState OldState, EGrantMoneyRunState NewState)
	{
		Transitions.Emplace(OldState, NewState);
		BossPhaseEntries += NewState == EGrantMoneyRunState::BossPhase ? 1 : 0;
	}

	UFUNCTION()
	void OnTimeChanged(float ElapsedSeconds, float RemainingSeconds)
	{
		++TimeChangedCount;
		LastRemainingSeconds = RemainingSeconds;
	}

	UFUNCTION()
	void OnBossPhaseStarted()
	{
		++BossPhaseStartedCount;
	}

	UFUNCTION()
	void OnEmergenceFinished(UEnemyEmergenceComponent* Emergence)
	{
		++EmergenceFinishedCount;
	}

	TArray<TPair<EGrantMoneyRunState, EGrantMoneyRunState>> Transitions;
	int32 BossPhaseEntries = 0;
	int32 BossPhaseStartedCount = 0;
	int32 EmergenceFinishedCount = 0;
	int32 TimeChangedCount = 0;
	float LastRemainingSeconds = -1.0f;
};
