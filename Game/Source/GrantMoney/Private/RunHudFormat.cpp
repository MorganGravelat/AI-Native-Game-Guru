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

FString FRunHudFormat::FormatBuffLine(const FText& DisplayName, float RemainingSeconds, bool bPermanent)
{
	if (bPermanent)
	{
		return DisplayName.ToString();
	}

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

FString FRunHudFormat::FormatStatName(EGrantMoneyStat Stat)
{
	switch (Stat)
	{
	case EGrantMoneyStat::MaxHealth:      return TEXT("Max Health");
	case EGrantMoneyStat::ContactDamage:  return TEXT("Contact Damage");
	// A buff lowers this value (shorter interval = faster attacks), so it is shown under the name
	// players actually experience the change as: FormatStatMagnitude flips its sign to match.
	case EGrantMoneyStat::AttackInterval: return TEXT("Attack Speed");
	case EGrantMoneyStat::Damage:         return TEXT("Damage");
	case EGrantMoneyStat::Penetration:    return TEXT("Penetration");
	case EGrantMoneyStat::Experience:     return TEXT("Experience");
	case EGrantMoneyStat::ProjectileCount:return TEXT("Projectile Count");
	case EGrantMoneyStat::ExplosionRadius:return TEXT("Explosion Radius");
	case EGrantMoneyStat::MoveSpeed:      return TEXT("Move Speed");
	}

	return TEXT("Stat");
}

FString FRunHudFormat::FormatStatMagnitude(EGrantMoneyStat Stat, EGrantMoneyStatModifierOp Op, float Magnitude)
{
	// AttackInterval is the one stat where lower is better; flipping its sign here is what makes
	// FormatStatName("Attack Speed") read correctly as "+30%" for a buff instead of "-30%".
	const float Displayed = Stat == EGrantMoneyStat::AttackInterval ? -Magnitude : Magnitude;
	const TCHAR* Sign = Displayed >= 0.0f ? TEXT("+") : TEXT("-");
	const float AbsValue = FMath::Abs(Displayed);

	if (Op == EGrantMoneyStatModifierOp::Multiplicative)
	{
		return FString::Printf(TEXT("%s%.0f%%"), Sign, AbsValue * 100.0f);
	}

	const bool bWhole = FMath::IsNearlyEqual(AbsValue, FMath::RoundToFloat(AbsValue));
	return bWhole ? FString::Printf(TEXT("%s%.0f"), Sign, AbsValue) : FString::Printf(TEXT("%s%.1f"), Sign, AbsValue);
}

FString FRunHudFormat::FormatBuffToast(const FText& DisplayName, bool bHasStat, EGrantMoneyStat Stat, EGrantMoneyStatModifierOp Op, float Magnitude)
{
	const FString Name = DisplayName.IsEmpty() ? TEXT("Buff gained") : DisplayName.ToString();
	if (!bHasStat)
	{
		return Name;
	}

	return FString::Printf(TEXT("%s  %s %s"), *Name, *FormatStatName(Stat), *FormatStatMagnitude(Stat, Op, Magnitude));
}
