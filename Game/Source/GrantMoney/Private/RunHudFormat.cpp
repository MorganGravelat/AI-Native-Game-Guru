// Fill out your copyright notice in the Description page of Project Settings.

#include "RunHudFormat.h"

int32 FRunHudFormat::ComputeScore(float Experience, float SurvivalSeconds)
{
	const double Raw = FMath::Max(0.0f, Experience) * ScorePerXp + FMath::Max(0.0f, SurvivalSeconds) * ScorePerSurvivalSecond;
	return static_cast<int32>(FMath::Min(FMath::FloorToDouble(Raw), static_cast<double>(MAX_int32)));
}

FString FRunHudFormat::FormatScore(int32 Score)
{
	FString Digits = FString::FromInt(FMath::Abs(Score));
	FString Grouped;
	for (int32 Index = 0; Index < Digits.Len(); ++Index)
	{
		if (Index > 0 && (Digits.Len() - Index) % 3 == 0)
		{
			Grouped.AppendChar(TEXT(','));
		}
		Grouped.AppendChar(Digits[Index]);
	}

	return Score < 0 ? TEXT("-") + Grouped : Grouped;
}

FString FRunHudFormat::FormatHealth(float Current, float Max)
{
	return FString::Printf(TEXT("%d / %d"), FMath::CeilToInt32(FMath::Max(0.0f, Current)), FMath::RoundToInt32(FMath::Max(0.0f, Max)));
}

FString FRunHudFormat::FormatXp(float IntoLevel, float ToNextLevel)
{
	return FString::Printf(TEXT("%d / %d XP"), FMath::FloorToInt32(FMath::Max(0.0f, IntoLevel)), FMath::RoundToInt32(FMath::Max(0.0f, ToNextLevel)));
}

FString FRunHudFormat::FormatLevel(int32 Level)
{
	return FString::Printf(TEXT("Level %d"), FMath::Max(1, Level));
}

FString FRunHudFormat::FormatBuffRemaining(float Seconds)
{
	const float Safe = FMath::Max(0.0f, Seconds);
	if (Safe >= 10.0f)
	{
		return FString::Printf(TEXT("%ds"), FMath::CeilToInt32(Safe));
	}

	return FString::Printf(TEXT("%.1fs"), Safe);
}

FString FRunHudFormat::FormatBuffLine(const FText& DisplayName, float RemainingSeconds)
{
	return FString::Printf(TEXT("%s  %s"), *DisplayName.ToString(), *FormatBuffRemaining(RemainingSeconds));
}

FString FRunHudFormat::FormatUpgradeLine(const FText& DisplayName, int32 Stacks, int32 MaxStacks)
{
	FString Line = DisplayName.ToString();
	if (Stacks > 1)
	{
		Line += FString::Printf(TEXT(" x%d"), Stacks);
	}

	if (MaxStacks > 0 && Stacks >= MaxStacks && MaxStacks > 1)
	{
		Line += TEXT(" (max)");
	}

	return Line;
}

FString FRunHudFormat::FormatPhase(EGrantMoneyRunState State, bool bPaused)
{
	FString Label = ARunController::GetStateLabel(State);
	if (bPaused)
	{
		Label += TEXT(" (paused)");
	}

	return Label;
}

float FRunHudFormat::SafeFraction(float Value, float Max)
{
	return Max > 0.0f ? FMath::Clamp(Value / Max, 0.0f, 1.0f) : 0.0f;
}
