// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GrantMoneyStatTypes.h"
#include "UpgradeTypes.generated.h"

/**
 * One reusable upgrade a level-up choice can offer: display text, the stat it changes, how it changes
 * it, and how many times it can be picked in one run. Applying one is nothing more than
 * UStatsComponent::AddModifier(Stat, Op, Magnitude), the same call SCRUM-53's stat boost pickups use,
 * so upgrades picked here and boosts picked up in the field combine through the same formula.
 *
 * Plain data, no runtime state: how many of each the player has already picked lives on
 * UUpgradeComponent, keyed by Id, so these definitions stay reusable templates.
 */
USTRUCT(BlueprintType)
struct GRANTMONEY_API FGrantMoneyUpgradeDefinition
{
	GENERATED_BODY()

	// Identifies this upgrade for stack tracking and eligibility. Must be unique within one
	// UUpgradeComponent's AvailableUpgrades list.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	FName Id;

	// Shown on the choice button. Placeholder text is fine.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	FText DisplayName;

	// Shown under the name. Placeholder text is fine.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	FText Description;

	// The stat this upgrade changes when picked.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	EGrantMoneyStat Stat = EGrantMoneyStat::Damage;

	// Additive (a flat amount) or Multiplicative (a fraction of the stat); same rule as every other modifier.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	EGrantMoneyStatModifierOp Op = EGrantMoneyStatModifierOp::Additive;

	// Amount added per pick (e.g. +3 flat, or -0.08 for an 8% multiplicative reduction).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	float Magnitude = 0.0f;

	// How many times this upgrade can be picked in one run. Once a stack reaches this, it is no longer
	// offered. 0 means unlimited.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade", meta = (ClampMin = "0"))
	int32 MaxStacks = 1;
};
