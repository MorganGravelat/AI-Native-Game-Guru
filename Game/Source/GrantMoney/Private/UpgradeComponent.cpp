// Fill out your copyright notice in the Description page of Project Settings.

#include "UpgradeComponent.h"

#include "StatsComponent.h"

namespace
{
	// Starter orb upgrades required by SCRUM-36. Magnitudes are placeholders, tuned to be clearly
	// visible rather than balanced; every value is editable per instance in the Details panel.
	FGrantMoneyUpgradeDefinition MakeDamageUpgrade()
	{
		FGrantMoneyUpgradeDefinition Def;
		Def.Id = TEXT("Damage");
		Def.DisplayName = NSLOCTEXT("GrantMoneyUpgrades", "Damage", "Damage");
		Def.Description = NSLOCTEXT("GrantMoneyUpgrades", "DamageDesc", "+3 damage per magic orb.");
		Def.Stat = EGrantMoneyStat::Damage;
		Def.Op = EGrantMoneyStatModifierOp::Additive;
		Def.Magnitude = 3.0f;
		Def.MaxStacks = 5;
		return Def;
	}

	FGrantMoneyUpgradeDefinition MakeAttackSpeedUpgrade()
	{
		FGrantMoneyUpgradeDefinition Def;
		Def.Id = TEXT("AttackSpeed");
		Def.DisplayName = NSLOCTEXT("GrantMoneyUpgrades", "AttackSpeed", "Attack Speed");
		Def.Description = NSLOCTEXT("GrantMoneyUpgrades", "AttackSpeedDesc", "Fires 8% more often.");
		Def.Stat = EGrantMoneyStat::AttackInterval;
		Def.Op = EGrantMoneyStatModifierOp::Multiplicative;
		Def.Magnitude = -0.08f;
		Def.MaxStacks = 5;
		return Def;
	}

	FGrantMoneyUpgradeDefinition MakePenetrationUpgrade()
	{
		FGrantMoneyUpgradeDefinition Def;
		Def.Id = TEXT("Penetration");
		Def.DisplayName = NSLOCTEXT("GrantMoneyUpgrades", "Penetration", "Penetration");
		Def.Description = NSLOCTEXT("GrantMoneyUpgrades", "PenetrationDesc", "Each orb hits one more enemy.");
		Def.Stat = EGrantMoneyStat::Penetration;
		Def.Op = EGrantMoneyStatModifierOp::Additive;
		Def.Magnitude = 1.0f;
		Def.MaxStacks = 3;
		return Def;
	}

	FGrantMoneyUpgradeDefinition MakeProjectileCountUpgrade()
	{
		FGrantMoneyUpgradeDefinition Def;
		Def.Id = TEXT("ProjectileCount");
		Def.DisplayName = NSLOCTEXT("GrantMoneyUpgrades", "ProjectileCount", "Projectile Count");
		Def.Description = NSLOCTEXT("GrantMoneyUpgrades", "ProjectileCountDesc", "Fires one more orb per cycle.");
		Def.Stat = EGrantMoneyStat::ProjectileCount;
		Def.Op = EGrantMoneyStatModifierOp::Additive;
		Def.Magnitude = 1.0f;
		Def.MaxStacks = 3;
		return Def;
	}

	FGrantMoneyUpgradeDefinition MakeExplosionUpgrade()
	{
		FGrantMoneyUpgradeDefinition Def;
		Def.Id = TEXT("ExplosionRadius");
		Def.DisplayName = NSLOCTEXT("GrantMoneyUpgrades", "Explosion", "AOE Explosion");
		Def.Description = NSLOCTEXT("GrantMoneyUpgrades", "ExplosionDesc", "Orbs burst, widening to hit more enemies.");
		Def.Stat = EGrantMoneyStat::ExplosionRadius;
		Def.Op = EGrantMoneyStatModifierOp::Additive;
		Def.Magnitude = 60.0f;
		Def.MaxStacks = 4;
		return Def;
	}

	FGrantMoneyUpgradeDefinition MakeMoveSpeedUpgrade()
	{
		FGrantMoneyUpgradeDefinition Def;
		Def.Id = TEXT("MoveSpeed");
		Def.DisplayName = NSLOCTEXT("GrantMoneyUpgrades", "MoveSpeed", "Move Speed");
		Def.Description = NSLOCTEXT("GrantMoneyUpgrades", "MoveSpeedDesc", "+5% move speed.");
		Def.Stat = EGrantMoneyStat::MoveSpeed;
		Def.Op = EGrantMoneyStatModifierOp::Multiplicative;
		Def.Magnitude = 0.05f;
		Def.MaxStacks = 5;
		return Def;
	}
}

UUpgradeComponent::UUpgradeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// AOE Explosion (MakeExplosionUpgrade) is deliberately left out of the offered pool for now — the
	// underlying AMagicOrbProjectile/UAttackHitComponent functionality is fully implemented and tested;
	// add it back to this list whenever it is ready to be offered again. Move Speed stands in its place.
	AvailableUpgrades = {
		MakeDamageUpgrade(),
		MakeAttackSpeedUpgrade(),
		MakePenetrationUpgrade(),
		MakeProjectileCountUpgrade(),
		MakeMoveSpeedUpgrade(),
	};
}

void UUpgradeComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const AActor* Owner = GetOwner())
	{
		Stats = Owner->FindComponentByClass<UStatsComponent>();
	}

	if (!Stats)
	{
		UE_LOG(LogTemp, Warning, TEXT("UpgradeComponent: %s has no StatsComponent, so upgrades cannot be applied."), *GetNameSafe(GetOwner()));
	}

	RandomStream.GenerateNewSeed();
}

void UUpgradeComponent::SetRandomSeed(int32 Seed)
{
	RandomStream.Initialize(Seed);
}

int32 UUpgradeComponent::GetStackCount(FName Id) const
{
	const int32* Found = StackCounts.Find(Id);
	return Found ? *Found : 0;
}

void UUpgradeComponent::NotifyLevelGained()
{
	++PendingChoices;

	if (!bChoiceActive)
	{
		PresentNextChoice();
	}
}

TArray<FGrantMoneyUpgradeDefinition> UUpgradeComponent::RollOptions() const
{
	TArray<FGrantMoneyUpgradeDefinition> Eligible;
	for (const FGrantMoneyUpgradeDefinition& Def : AvailableUpgrades)
	{
		if (Def.MaxStacks <= 0 || GetStackCount(Def.Id) < Def.MaxStacks)
		{
			Eligible.Add(Def);
		}
	}

	// Partial Fisher-Yates: shuffles just the first Count slots, which is all a "pick N distinct of M" needs.
	const int32 Count = FMath::Min(FMath::Max(1, OptionsPerChoice), Eligible.Num());
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const int32 SwapIndex = Index + RandomStream.RandRange(0, Eligible.Num() - 1 - Index);
		Eligible.Swap(Index, SwapIndex);
	}

	if (Eligible.Num() > Count)
	{
		Eligible.RemoveAt(Count, Eligible.Num() - Count);
	}

	return Eligible;
}

void UUpgradeComponent::PresentNextChoice()
{
	while (PendingChoices > 0)
	{
		TArray<FGrantMoneyUpgradeDefinition> Options = RollOptions();
		if (Options.IsEmpty())
		{
			// Every upgrade is already at its stack limit: nothing to offer. Consume this pending
			// choice silently and try the next one, rather than getting stuck waiting on it forever.
			--PendingChoices;
			continue;
		}

		CurrentOptions = MoveTemp(Options);
		bChoiceActive = true;
		UE_LOG(LogTemp, Log, TEXT("UpgradeComponent: %s presenting a choice of %d (pending: %d)."), *GetNameSafe(GetOwner()), CurrentOptions.Num(), PendingChoices);
		OnChoiceReady.Broadcast();
		return;
	}

	bChoiceActive = false;
	CurrentOptions.Reset();
	OnChoicesResolved.Broadcast();
}

bool UUpgradeComponent::ChooseUpgrade(int32 Index)
{
	if (!bChoiceActive || !CurrentOptions.IsValidIndex(Index))
	{
		return false;
	}

	ApplyUpgrade(CurrentOptions[Index]);

	--PendingChoices;
	bChoiceActive = false;
	CurrentOptions.Reset();

	// Still paused (or about to be): PresentNextChoice either rolls the next queued choice (OnChoiceReady
	// fires again, same pause) or, if the queue is now empty, resolves (OnChoicesResolved fires once).
	PresentNextChoice();
	return true;
}

void UUpgradeComponent::ApplyUpgrade(const FGrantMoneyUpgradeDefinition& Upgrade)
{
	StackCounts.FindOrAdd(Upgrade.Id)++;

	if (Stats)
	{
		Stats->AddModifier(Upgrade.Stat, Upgrade.Op, Upgrade.Magnitude);
	}

	UE_LOG(LogTemp, Log, TEXT("UpgradeComponent: %s picked %s (stack %d/%d)."),
		*GetNameSafe(GetOwner()), *Upgrade.Id.ToString(), GetStackCount(Upgrade.Id), Upgrade.MaxStacks);
}
