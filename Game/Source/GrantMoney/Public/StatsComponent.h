// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrantMoneyStatTypes.h"
#include "StatsComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGrantMoneyStatChanged, EGrantMoneyStat, Stat, float, OldValue, float, NewValue);

/**
 * Holds an actor's stats (max health and, later, damage, speed and so on).
 * Each stat has a base value set in the editor plus any number of modifiers
 * (buffs, debuffs, permanent upgrades). Other components read the final value
 * with GetStat and listen to OnStatChanged instead of storing their own copy.
 * Not ticked; values are recalculated only when asked for.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class GRANTMONEY_API UStatsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStatsComponent();

	// Final value: (Base + sum of Additive) * (1 + sum of Multiplicative), never below zero.
	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetStat(EGrantMoneyStat Stat) const;

	// Value before modifiers. A stat with no base value is 0.
	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetBaseStat(EGrantMoneyStat Stat) const;

	// True if Stat has a base value. Lets callers fall back to their own default
	// instead of treating a missing stat as 0.
	UFUNCTION(BlueprintPure, Category = "Stats")
	bool HasStat(EGrantMoneyStat Stat) const { return BaseStats.Contains(Stat); }

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void SetBaseStat(EGrantMoneyStat Stat, float Value);

	// Returns a handle to pass to RemoveModifier. Keep it if the modifier is temporary;
	// a permanent upgrade can ignore it.
	UFUNCTION(BlueprintCallable, Category = "Stats")
	FGrantMoneyStatModifierHandle AddModifier(EGrantMoneyStat Stat, EGrantMoneyStatModifierOp Op, float Value);

	// Returns false if the handle is invalid or was already removed.
	UFUNCTION(BlueprintCallable, Category = "Stats")
	bool RemoveModifier(FGrantMoneyStatModifierHandle Handle);

	const TArray<FGrantMoneyStatModifier>& GetModifiers() const { return Modifiers; }

	// Fires only when a stat's final value actually changes.
	UPROPERTY(BlueprintAssignable, Category = "Stats")
	FOnGrantMoneyStatChanged OnStatChanged;

protected:
	// Starting values. Owners set defaults in their constructor; designers can override per Blueprint or instance.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	TMap<EGrantMoneyStat, float> BaseStats;

private:
	// Runs Change, then broadcasts OnStatChanged if Stat's final value is different afterwards.
	void ChangeStat(EGrantMoneyStat Stat, TFunctionRef<void()> Change);

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Stats")
	TArray<FGrantMoneyStatModifier> Modifiers;

	int32 NextModifierId = 0;
};
