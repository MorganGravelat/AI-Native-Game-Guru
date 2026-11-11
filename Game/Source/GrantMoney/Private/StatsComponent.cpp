// Fill out your copyright notice in the Description page of Project Settings.

#include "StatsComponent.h"

UStatsComponent::UStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

float UStatsComponent::GetBaseStat(EGrantMoneyStat Stat) const
{
	const float* Base = BaseStats.Find(Stat);
	return Base ? *Base : 0.0f;
}

float UStatsComponent::GetStat(EGrantMoneyStat Stat) const
{
	return GetStatFromBase(Stat, GetBaseStat(Stat));
}

float UStatsComponent::GetStatFromBase(EGrantMoneyStat Stat, float BaseValue) const
{
	float Additive = 0.0f;
	float Multiplicative = 0.0f;

	for (const FGrantMoneyStatModifier& Modifier : Modifiers)
	{
		if (Modifier.Stat != Stat)
		{
			continue;
		}

		if (Modifier.Op == EGrantMoneyStatModifierOp::Additive)
		{
			Additive += Modifier.Value;
		}
		else
		{
			Multiplicative += Modifier.Value;
		}
	}

	return FMath::Max(0.0f, (BaseValue + Additive) * (1.0f + Multiplicative));
}

void UStatsComponent::SetBaseStat(EGrantMoneyStat Stat, float Value)
{
	ChangeStat(Stat, [this, Stat, Value]() { BaseStats.Add(Stat, Value); });
}

FGrantMoneyStatModifierHandle UStatsComponent::AddModifier(EGrantMoneyStat Stat, EGrantMoneyStatModifierOp Op, float Value)
{
	FGrantMoneyStatModifier Modifier;
	Modifier.Stat = Stat;
	Modifier.Op = Op;
	Modifier.Value = Value;
	Modifier.Handle.Id = NextModifierId++;

	ChangeStat(Stat, [this, &Modifier]() { Modifiers.Add(Modifier); });

	return Modifier.Handle;
}

bool UStatsComponent::RemoveModifier(FGrantMoneyStatModifierHandle Handle)
{
	if (!Handle.IsValid())
	{
		return false;
	}

	const int32 Index = Modifiers.IndexOfByPredicate([Handle](const FGrantMoneyStatModifier& Modifier) { return Modifier.Handle == Handle; });
	if (Index == INDEX_NONE)
	{
		return false;
	}

	ChangeStat(Modifiers[Index].Stat, [this, Index]() { Modifiers.RemoveAt(Index); });
	return true;
}

void UStatsComponent::ChangeStat(EGrantMoneyStat Stat, TFunctionRef<void()> Change)
{
	const float OldValue = GetStat(Stat);
	Change();
	const float NewValue = GetStat(Stat);

	if (!FMath::IsNearlyEqual(OldValue, NewValue))
	{
		OnStatChanged.Broadcast(Stat, OldValue, NewValue);
	}
}
