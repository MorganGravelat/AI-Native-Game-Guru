// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrantMoneyStatTypes.h"
#include "LevelComponent.generated.h"

class UStatsComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGrantMoneyLevelUp, int32, NewLevel, int32, PreviousLevel);

/**
 * Turns the owner's running Experience stat (UStatsComponent, see AXpOrbPickup) into a level and
 * progress toward the next one. Experience itself is never touched or reset here: it stays a
 * monotonically increasing total (also used for the Game Over XP readout); this component only
 * watches it change and derives level state from the deltas.
 *
 * Levels are 1-indexed, starting at 1. LevelThresholds[N - 1] is the XP needed to go from level N to
 * level N + 1; past the end of the array the last entry repeats, so leveling never silently stops.
 * Excess XP beyond a threshold is preserved (added to progress toward the next level, not discarded),
 * and a single stat change that crosses more than one threshold fires OnLevelUp once per level gained,
 * in order, so nothing is skipped.
 *
 * Not ticked: everything happens the moment UStatsComponent::OnStatChanged reports a change to Experience.
 */
UCLASS(ClassGroup = (Progression), meta = (BlueprintSpawnableComponent))
class GRANTMONEY_API ULevelComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULevelComponent();

	// XP required to go from level N to N + 1, indexed by N - 1 (so index 0 is level 1 -> 2). Past the
	// end of the array, the last entry is reused for every later level.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leveling")
	TArray<float> LevelThresholds = { 10.0f, 15.0f, 20.0f, 25.0f, 30.0f, 40.0f, 50.0f, 65.0f, 80.0f, 100.0f };

	UFUNCTION(BlueprintPure, Category = "Leveling")
	int32 GetLevel() const { return Level; }

	// XP earned toward the next level so far (never negative, never as much as GetXpToNextLevel).
	UFUNCTION(BlueprintPure, Category = "Leveling")
	float GetXpIntoLevel() const { return XpIntoLevel; }

	// XP required to go from the current level to the next.
	UFUNCTION(BlueprintPure, Category = "Leveling")
	float GetXpToNextLevel() const { return GetThreshold(Level); }

	// GetXpIntoLevel / GetXpToNextLevel, clamped to 0..1. For a progress bar.
	UFUNCTION(BlueprintPure, Category = "Leveling")
	float GetProgressFraction() const;

	// Fires once per level gained, in order, every time Experience increases enough to cross one or more
	// thresholds (so a single large pickup can fire this more than once in a row).
	UPROPERTY(BlueprintAssignable, Category = "Leveling")
	FOnGrantMoneyLevelUp OnLevelUp;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleStatChanged(EGrantMoneyStat Stat, float OldValue, float NewValue);

	// XP needed to go from ForLevel to ForLevel + 1. ForLevel below 1 is treated as 1.
	float GetThreshold(int32 ForLevel) const;

	UPROPERTY(Transient)
	TObjectPtr<UStatsComponent> Stats;

	int32 Level = 1;
	float XpIntoLevel = 0.0f;
};
