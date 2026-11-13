// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RunController.h"

/**
 * Pure helpers behind the gameplay HUD: the score formula and the text the HUD shows. No widgets and no
 * world, so every rule here is covered by headless tests while the layout itself is checked in the Editor.
 */
struct GRANTMONEY_API FRunHudFormat
{
	// Score is a mix of XP earned and time survived.
	static constexpr float ScorePerXp = 10.0f;
	static constexpr float ScorePerSurvivalSecond = 5.0f;

	// Never negative; fractions are dropped.
	static int32 ComputeScore(float Experience, float SurvivalSeconds);

	// With thousands separators: 12345 -> "12,345".
	static FString FormatScore(int32 Score);

	// "80 / 100". Current health rounds up so a living player never reads 0.
	static FString FormatHealth(float Current, float Max);

	// "12 / 25 XP".
	static FString FormatXp(float IntoLevel, float ToNextLevel);

	// "Level 3".
	static FString FormatLevel(int32 Level);

	// "12s" from 10 s up (rounded up, so it reads 1s until it really is over), "4.3s" below that.
	static FString FormatBuffRemaining(float Seconds);

	// "Speed Surge  12s".
	static FString FormatBuffLine(const FText& DisplayName, float RemainingSeconds);

	// "Damage" for a first pick, "Damage x3" for more, and "(max)" appended once a stack limit above 1 is
	// reached (MaxStacks 0 = unlimited; a one-pick upgrade is not marked, it is trivially at its limit).
	static FString FormatUpgradeLine(const FText& DisplayName, int32 Stacks, int32 MaxStacks);

	// The headline for the phase line: the state label, plus "(paused)" while the game is paused.
	static FString FormatPhase(EGrantMoneyRunState State, bool bPaused);

	// Percent as 0..1, safe against a zero or negative maximum.
	static float SafeFraction(float Value, float Max);
};
