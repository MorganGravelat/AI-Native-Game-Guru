// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GrantMoneyStatTypes.generated.h"

/**
 * Every stat a UStatsComponent can hold. Add new entries at the end so values
 * saved in Blueprints and data assets keep pointing at the same stat.
 */
UENUM(BlueprintType)
enum class EGrantMoneyStat : uint8
{
	MaxHealth UMETA(DisplayName = "Max Health"),

	// Damage dealt by touching a target (UContactDamageComponent), per hit.
	ContactDamage UMETA(DisplayName = "Contact Damage"),

	// Seconds between an attacker's hits (lower is faster). E.g. a -0.25 multiplicative modifier attacks 25% more often.
	AttackInterval UMETA(DisplayName = "Attack Interval"),

	// Damage per hit dealt by an attack such as a projectile (UAttackHitComponent).
	Damage UMETA(DisplayName = "Damage"),

	// Extra targets one attack can pass through: 0 = stops at the first target, 2 = hits up to 3. Rounded down.
	Penetration UMETA(DisplayName = "Penetration"),
};

/**
 * How a modifier changes a stat. The final value is
 * (Base + sum of Additive) * (1 + sum of Multiplicative), never below zero.
 */
UENUM(BlueprintType)
enum class EGrantMoneyStatModifierOp : uint8
{
	// Adds a flat amount, e.g. +20 max health.
	Additive,

	// Adds a fraction of the stat, e.g. 0.1 = +10%, -0.25 = -25%. Fractions add up before being applied.
	Multiplicative,
};

/** Identifies one modifier so it can be removed later. A default-constructed handle is invalid. */
USTRUCT(BlueprintType)
struct GRANTMONEY_API FGrantMoneyStatModifierHandle
{
	GENERATED_BODY()

	bool IsValid() const { return Id != INDEX_NONE; }

	bool operator==(const FGrantMoneyStatModifierHandle& Other) const { return Id == Other.Id; }
	bool operator!=(const FGrantMoneyStatModifierHandle& Other) const { return Id != Other.Id; }

	UPROPERTY()
	int32 Id = INDEX_NONE;
};

/** One active change to a stat, owned by a UStatsComponent. */
USTRUCT(BlueprintType)
struct GRANTMONEY_API FGrantMoneyStatModifier
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stats")
	EGrantMoneyStat Stat = EGrantMoneyStat::MaxHealth;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stats")
	EGrantMoneyStatModifierOp Op = EGrantMoneyStatModifierOp::Additive;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stats")
	float Value = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stats")
	FGrantMoneyStatModifierHandle Handle;
};
