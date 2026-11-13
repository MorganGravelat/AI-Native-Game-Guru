// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrantMoneyStatTypes.h"
#include "TemporaryBuffComponent.generated.h"

class UStatsComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGrantMoneyBuffsChanged);

/** One timed stat change that is currently active. */
USTRUCT(BlueprintType)
struct GRANTMONEY_API FGrantMoneyActiveBuff
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Buff")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "Buff")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Buff")
	EGrantMoneyStat Stat = EGrantMoneyStat::MaxHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Buff")
	EGrantMoneyStatModifierOp Op = EGrantMoneyStatModifierOp::Additive;

	UPROPERTY(BlueprintReadOnly, Category = "Buff")
	float Magnitude = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Buff")
	float DurationSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Buff")
	float RemainingSeconds = 0.0f;

	UPROPERTY()
	FGrantMoneyStatModifierHandle Handle;
};

/**
 * Temporary buffs: a UStatsComponent modifier that removes itself after a duration. The buff is applied
 * through the same UStatsComponent::AddModifier as every permanent upgrade, so everything that already
 * reads stats reacts on its own, and removing it on expiry restores the exact previous value.
 *
 * Time is counted by this component's tick, so pausing the game (pause menu, level-up choice, result
 * screen) and time dilation affect a buff the way they affect everything else. The tick is enabled only
 * while a buff is active. Adding a buff whose Id is already active refreshes it (new magnitude, full
 * duration) instead of stacking, so a repeated pickup cannot make a buff permanent or unbounded.
 *
 * The HUD reads GetActiveBuffs() for the name and remaining time of each buff and listens to
 * OnBuffsChanged for the set changing (added, refreshed, expired, removed).
 */
UCLASS(ClassGroup = (Progression), meta = (BlueprintSpawnableComponent))
class GRANTMONEY_API UTemporaryBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTemporaryBuffComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Applies the buff, or refreshes it if Id is already active. Returns false and changes nothing if
	// there is no stats component to apply it to, Id is empty or DurationSeconds is not positive.
	UFUNCTION(BlueprintCallable, Category = "Buffs")
	bool AddBuff(FName Id, const FText& DisplayName, EGrantMoneyStat Stat, EGrantMoneyStatModifierOp Op, float Magnitude, float DurationSeconds);

	// Ends the buff early. Returns false if it is not active.
	UFUNCTION(BlueprintCallable, Category = "Buffs")
	bool RemoveBuff(FName Id);

	// Ends every buff.
	UFUNCTION(BlueprintCallable, Category = "Buffs")
	void ClearBuffs();

	// Moves every buff's clock forward and removes the ones that ran out. TickComponent calls this;
	// tests and tools can call it directly.
	void AdvanceBuffs(float DeltaSeconds);

	// Uses this stats component instead of the owner's. For tests and tools.
	void SetStatsComponent(UStatsComponent* InStats);

	UFUNCTION(BlueprintPure, Category = "Buffs")
	bool HasBuff(FName Id) const;

	// Active buffs, oldest first.
	const TArray<FGrantMoneyActiveBuff>& GetActiveBuffs() const { return ActiveBuffs; }

	// Fires when a buff is added, refreshed, expires or is removed. Not every tick: read the remaining
	// time from GetActiveBuffs() while it is showing.
	UPROPERTY(BlueprintAssignable, Category = "Buffs")
	FOnGrantMoneyBuffsChanged OnBuffsChanged;

private:
	UStatsComponent* ResolveStats();

	// Removes the buff's modifier. Does not touch ActiveBuffs.
	void RemoveModifierFor(const FGrantMoneyActiveBuff& Buff);

	void UpdateTickEnabled();

	UPROPERTY(Transient)
	TObjectPtr<UStatsComponent> Stats;

	UPROPERTY(Transient)
	TArray<FGrantMoneyActiveBuff> ActiveBuffs;
};
