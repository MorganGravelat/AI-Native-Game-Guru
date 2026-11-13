// Fill out your copyright notice in the Description page of Project Settings.

#include "TemporaryBuffComponent.h"

#include "StatsComponent.h"

UTemporaryBuffComponent::UTemporaryBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

UStatsComponent* UTemporaryBuffComponent::ResolveStats()
{
	if (!Stats && GetOwner())
	{
		Stats = GetOwner()->FindComponentByClass<UStatsComponent>();
	}

	return Stats;
}

void UTemporaryBuffComponent::SetStatsComponent(UStatsComponent* InStats)
{
	Stats = InStats;
}

bool UTemporaryBuffComponent::HasBuff(FName Id) const
{
	return ActiveBuffs.ContainsByPredicate([Id](const FGrantMoneyActiveBuff& Buff) { return Buff.Id == Id; });
}

bool UTemporaryBuffComponent::AddBuff(FName Id, const FText& DisplayName, EGrantMoneyStat Stat, EGrantMoneyStatModifierOp Op, float Magnitude, float DurationSeconds)
{
	UStatsComponent* TargetStats = ResolveStats();
	if (!TargetStats || Id.IsNone() || DurationSeconds <= 0.0f)
	{
		return false;
	}

	// A refresh replaces the old modifier, so a changed magnitude takes effect and nothing stacks.
	const int32 ExistingIndex = ActiveBuffs.IndexOfByPredicate([Id](const FGrantMoneyActiveBuff& Buff) { return Buff.Id == Id; });
	if (ExistingIndex != INDEX_NONE)
	{
		RemoveModifierFor(ActiveBuffs[ExistingIndex]);
		ActiveBuffs.RemoveAt(ExistingIndex);
	}

	FGrantMoneyActiveBuff Buff;
	Buff.Id = Id;
	Buff.DisplayName = DisplayName;
	Buff.Stat = Stat;
	Buff.Op = Op;
	Buff.Magnitude = Magnitude;
	Buff.DurationSeconds = DurationSeconds;
	Buff.RemainingSeconds = DurationSeconds;
	Buff.Handle = TargetStats->AddModifier(Stat, Op, Magnitude);
	ActiveBuffs.Add(Buff);

	UpdateTickEnabled();
	OnBuffsChanged.Broadcast();
	return true;
}

bool UTemporaryBuffComponent::RemoveBuff(FName Id)
{
	const int32 Index = ActiveBuffs.IndexOfByPredicate([Id](const FGrantMoneyActiveBuff& Buff) { return Buff.Id == Id; });
	if (Index == INDEX_NONE)
	{
		return false;
	}

	RemoveModifierFor(ActiveBuffs[Index]);
	ActiveBuffs.RemoveAt(Index);

	UpdateTickEnabled();
	OnBuffsChanged.Broadcast();
	return true;
}

void UTemporaryBuffComponent::ClearBuffs()
{
	if (ActiveBuffs.IsEmpty())
	{
		return;
	}

	for (const FGrantMoneyActiveBuff& Buff : ActiveBuffs)
	{
		RemoveModifierFor(Buff);
	}

	ActiveBuffs.Reset();
	UpdateTickEnabled();
	OnBuffsChanged.Broadcast();
}

void UTemporaryBuffComponent::AdvanceBuffs(float DeltaSeconds)
{
	if (ActiveBuffs.IsEmpty() || DeltaSeconds <= 0.0f)
	{
		return;
	}

	bool bAnyExpired = false;
	for (int32 Index = ActiveBuffs.Num() - 1; Index >= 0; --Index)
	{
		FGrantMoneyActiveBuff& Buff = ActiveBuffs[Index];
		Buff.RemainingSeconds -= DeltaSeconds;

		if (Buff.RemainingSeconds <= 0.0f)
		{
			RemoveModifierFor(Buff);
			ActiveBuffs.RemoveAt(Index);
			bAnyExpired = true;
		}
	}

	if (bAnyExpired)
	{
		UpdateTickEnabled();
		OnBuffsChanged.Broadcast();
	}
}

void UTemporaryBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AdvanceBuffs(DeltaTime);
}

void UTemporaryBuffComponent::RemoveModifierFor(const FGrantMoneyActiveBuff& Buff)
{
	if (Stats)
	{
		Stats->RemoveModifier(Buff.Handle);
	}
}

void UTemporaryBuffComponent::UpdateTickEnabled()
{
	SetComponentTickEnabled(!ActiveBuffs.IsEmpty());
}
