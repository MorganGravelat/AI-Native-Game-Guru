// Fill out your copyright notice in the Description page of Project Settings.

#include "LevelComponent.h"

#include "StatsComponent.h"

ULevelComponent::ULevelComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULevelComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const AActor* Owner = GetOwner())
	{
		Stats = Owner->FindComponentByClass<UStatsComponent>();
	}

	if (Stats)
	{
		Stats->OnStatChanged.AddUniqueDynamic(this, &ULevelComponent::HandleStatChanged);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("LevelComponent: %s has no StatsComponent, so it can never level up."), *GetNameSafe(GetOwner()));
	}
}

float ULevelComponent::GetThreshold(int32 ForLevel) const
{
	if (LevelThresholds.IsEmpty())
	{
		// Degenerate configuration: never level. Still safe (no divide-by-zero, no infinite loop).
		return TNumericLimits<float>::Max();
	}

	const int32 Index = FMath::Clamp(ForLevel - 1, 0, LevelThresholds.Num() - 1);
	return FMath::Max(UE_KINDA_SMALL_NUMBER, LevelThresholds[Index]);
}

float ULevelComponent::GetProgressFraction() const
{
	const float ToNext = GetXpToNextLevel();
	return ToNext > 0.0f ? FMath::Clamp(XpIntoLevel / ToNext, 0.0f, 1.0f) : 0.0f;
}

void ULevelComponent::HandleStatChanged(EGrantMoneyStat Stat, float OldValue, float NewValue)
{
	if (Stat != EGrantMoneyStat::Experience || NewValue <= OldValue)
	{
		return;
	}

	XpIntoLevel += (NewValue - OldValue);

	// A loop, not a single check: one big pickup can cross more than one threshold, and each level
	// gained fires its own event, in order, with no level skipped.
	while (XpIntoLevel >= GetThreshold(Level))
	{
		XpIntoLevel -= GetThreshold(Level);
		const int32 PreviousLevel = Level;
		++Level;

		UE_LOG(LogTemp, Log, TEXT("LevelComponent: %s reached level %d (%.1f XP into it)."), *GetNameSafe(GetOwner()), Level, XpIntoLevel);
		OnLevelUp.Broadcast(Level, PreviousLevel);
	}
}
