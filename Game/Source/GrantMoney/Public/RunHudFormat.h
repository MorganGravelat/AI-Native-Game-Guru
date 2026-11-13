// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GrantMoneyStatTypes.h"
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

	// "Speed Surge  12s" for a timed buff; just "Swiftness" (no time at all) when bPermanent is set, since
	// a permanent totem buff never counts down.
	static FString FormatBuffLine(const FText& DisplayName, float RemainingSeconds, bool bPermanent = false);

	// "Damage" for a first pick, "Damage x3" for more, and "(max)" appended once a stack limit above 1 is
	// reached (MaxStacks 0 = unlimited; a one-pick upgrade is not marked, it is trivially at its limit).
	static FString FormatUpgradeLine(const FText& DisplayName, int32 Stacks, int32 MaxStacks);

	// The headline for the phase line: the state label, plus "(paused)" while the game is paused.
	static FString FormatPhase(EGrantMoneyRunState State, bool bPaused);

	// Percent as 0..1, safe against a zero or negative maximum.
	static float SafeFraction(float Value, float Max);

	// The player-facing name for a stat. AttackInterval reads as "Attack Speed": a buff lowers the
	// interval (shorter = faster attacks), and showing "Attack Interval" with a negative number would
	// read backwards to a player, where every other stat's sign already matches "more is better".
	static FString FormatStatName(EGrantMoneyStat Stat);

	// "+50%" or "-20%" for a Multiplicative modifier, "+3" or "-1.5" for an Additive one. AttackInterval's
	// sign is flipped first (see FormatStatName) so a positive number always reads as a buff.
	static FString FormatStatMagnitude(EGrantMoneyStat Stat, EGrantMoneyStatModifierOp Op, float Magnitude);

	// What a gained buff's toast says: "Frenzy  Attack Speed +30%". Falls back to just DisplayName (or
	// "Buff gained" if that is also empty) when the effect is not a stat change.
	static FString FormatBuffToast(const FText& DisplayName, bool bHasStat, EGrantMoneyStat Stat, EGrantMoneyStatModifierOp Op, float Magnitude);
};
