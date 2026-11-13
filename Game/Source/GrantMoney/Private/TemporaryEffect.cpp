// Fill out your copyright notice in the Description page of Project Settings.

#include "TemporaryEffect.h"

#include "StatsComponent.h"

bool UTemporaryEffect::Apply(AActor* Target)
{
	if (bApplied || !IsValid(Target))
	{
		return false;
	}

	if (!ApplyEffect(Target))
	{
		return false;
	}

	bApplied = true;
	AppliedTarget = Target;
	return true;
}

void UTemporaryEffect::Remove()
{
	if (!bApplied)
	{
		return;
	}

	// Cleared first, so a RemoveEffect that triggers another Remove cannot undo the effect twice.
	bApplied = false;
	AActor* Target = AppliedTarget.Get();
	AppliedTarget.Reset();
	RemoveEffect(Target);
}

UStatModifierEffect::UStatModifierEffect()
{
	DisplayName = NSLOCTEXT("GrantMoney", "StatModifierEffectName", "Stat Boost");
}

bool UStatModifierEffect::ApplyEffect_Implementation(AActor* Target)
{
	UStatsComponent* Stats = Target ? Target->FindComponentByClass<UStatsComponent>() : nullptr;
	if (!Stats)
	{
		UE_LOG(LogTemp, Warning, TEXT("StatModifierEffect: %s has no Stats component, so %s was not applied."), *GetNameSafe(Target), *DisplayName.ToString());
		return false;
	}

	if (FMath::IsNearlyZero(Magnitude))
	{
		return false;
	}

	ModifierHandle = Stats->AddModifier(Stat, ModifierOp, Magnitude);
	ModifiedStats = Stats;
	return true;
}

void UStatModifierEffect::RemoveEffect_Implementation(AActor* Target)
{
	// The stats component may outlive or predate the target pointer; the handle is what matters.
	if (UStatsComponent* Stats = ModifiedStats.Get())
	{
		Stats->RemoveModifier(ModifierHandle);
	}

	ModifiedStats.Reset();
	ModifierHandle = FGrantMoneyStatModifierHandle();
}
