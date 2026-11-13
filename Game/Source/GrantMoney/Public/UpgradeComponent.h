// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UpgradeTypes.h"
#include "UpgradeComponent.generated.h"

class UStatsComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGrantMoneyUpgradeChoiceReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGrantMoneyUpgradeChoicesResolved);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGrantMoneyUpgradeChosen);

/** One upgrade the player has taken this run, for a "what I picked" readout. */
USTRUCT(BlueprintType)
struct GRANTMONEY_API FGrantMoneyPickedUpgrade
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
	FText DisplayName;

	// Times picked so far.
	UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
	int32 Stacks = 0;

	// 0 = unlimited.
	UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
	int32 MaxStacks = 0;
};

/**
 * Queues level-ups into upgrade choices and applies the one the player picks. Reusable: the five
 * starter orb upgrades (SCRUM-36) are set up as code defaults in the constructor, editable per entry
 * (or replaceable entirely) in the Details panel, the same way ASlimeEnemy's BonusPickupDrops works.
 *
 * NotifyLevelGained (bound by the owner to ULevelComponent::OnLevelUp) queues one choice. If nothing
 * is currently being chosen, a choice is rolled immediately: up to OptionsPerChoice distinct upgrades
 * that have not yet reached their MaxStacks, picked at random. OnChoiceReady fires for the UI to show;
 * ChooseUpgrade(Index) applies the chosen one (through the same UStatsComponent::AddModifier every
 * other stat change goes through, so it combines correctly with SCRUM-53 stat boosts), then either
 * rolls the next queued choice (OnChoiceReady fires again) or, once the queue is empty,
 * OnChoicesResolved fires once so the UI can hide and gameplay can resume. This is how "queue multiple
 * earned selections safely and resume only after the final choice" is satisfied: nothing here assumes
 * only one level is ever gained before the player responds.
 *
 * If every upgrade is already at its stack limit when a choice is rolled, that choice is silently
 * consumed (nothing to offer) and the next queued one (if any) is tried instead, so the queue can never
 * get stuck waiting for a choice that has no eligible options.
 *
 * Owns no pause/UI logic itself: whatever shows the choice (a widget, via the owning controller) is
 * responsible for pausing the game and showing/hiding it, exactly like the existing pause menu and
 * Game Over screen do, so the same mechanism also suspends combat, the run timer and any
 * timer-driven temporary effect while a choice is open.
 */
UCLASS(ClassGroup = (Progression), meta = (BlueprintSpawnableComponent))
class GRANTMONEY_API UUpgradeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUpgradeComponent();

	// The upgrades this component can ever offer. Code defaults set five starter orb upgrades (Damage,
	// Attack Speed, Penetration, Projectile Count, Move Speed); edit any entry, or add/remove entries, in
	// the Details panel. AOE Explosion is implemented (AMagicOrbProjectile/UAttackHitComponent) but
	// deliberately left out of this default list for now; add it back here to offer it again.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrades")
	TArray<FGrantMoneyUpgradeDefinition> AvailableUpgrades;

	// How many distinct eligible upgrades to offer per choice (fewer if fewer are eligible).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrades", meta = (ClampMin = "1"))
	int32 OptionsPerChoice = 3;

	// Queues one more choice (called when ULevelComponent::OnLevelUp fires). If no choice is currently
	// active, rolls and presents one immediately; otherwise it waits in the queue.
	void NotifyLevelGained();

	// Applies CurrentOptions[Index] (through UStatsComponent::AddModifier) and consumes one queued
	// choice. Returns false (does nothing) if there is no active choice or Index is out of range.
	UFUNCTION(BlueprintCallable, Category = "Upgrades")
	bool ChooseUpgrade(int32 Index);

	// The options the current choice is offering, in the order to display them. Empty when IsChoiceActive() is false.
	UFUNCTION(BlueprintPure, Category = "Upgrades")
	const TArray<FGrantMoneyUpgradeDefinition>& GetCurrentOptions() const { return CurrentOptions; }

	UFUNCTION(BlueprintPure, Category = "Upgrades")
	bool IsChoiceActive() const { return bChoiceActive; }

	// Choices still waiting (including the one currently being presented, if any).
	UFUNCTION(BlueprintPure, Category = "Upgrades")
	int32 GetPendingChoiceCount() const { return PendingChoices; }

	// How many times Id has already been picked this run.
	UFUNCTION(BlueprintPure, Category = "Upgrades")
	int32 GetStackCount(FName Id) const;

	// Every upgrade picked this run, in the order it was first picked, with its current stack count.
	UFUNCTION(BlueprintPure, Category = "Upgrades")
	TArray<FGrantMoneyPickedUpgrade> GetPickedUpgrades() const;

	// Fires after each pick has been applied, before the next queued choice (if any) is rolled. For HUD readouts.
	UPROPERTY(BlueprintAssignable, Category = "Upgrades")
	FOnGrantMoneyUpgradeChosen OnUpgradeChosen;

	// Fires every time a choice becomes ready to show (including a second, third, ... choice from the
	// same queue, once the previous one was answered). Read GetCurrentOptions() when this fires.
	UPROPERTY(BlueprintAssignable, Category = "Upgrades")
	FOnGrantMoneyUpgradeChoiceReady OnChoiceReady;

	// Fires once, only when the queue becomes empty (every pending choice has been answered or had
	// nothing eligible to offer). Safe to resume gameplay / hide the choice UI here.
	UPROPERTY(BlueprintAssignable, Category = "Upgrades")
	FOnGrantMoneyUpgradeChoicesResolved OnChoicesResolved;

	// Makes RollOptions repeatable; for tests.
	void SetRandomSeed(int32 Seed);

	// Up to OptionsPerChoice distinct upgrades from AvailableUpgrades that have not reached MaxStacks,
	// chosen at random. Can return fewer than OptionsPerChoice (if fewer are eligible) or none at all
	// (if every upgrade is maxed). Public (not just used by PresentNextChoice) so tests can check the
	// distribution directly without driving the whole level-up flow.
	UFUNCTION(BlueprintPure, Category = "Upgrades")
	virtual TArray<FGrantMoneyUpgradeDefinition> RollOptions() const;

protected:
	virtual void BeginPlay() override;

private:
	// Rolls and presents the next queued choice, skipping (and consuming) any with nothing eligible to
	// offer, until either a real choice is presented or the queue is empty.
	void PresentNextChoice();

	void ApplyUpgrade(const FGrantMoneyUpgradeDefinition& Upgrade);

	UPROPERTY(Transient)
	TObjectPtr<UStatsComponent> Stats;

	TMap<FName, int32> StackCounts;

	// Ids in the order they were first picked.
	TArray<FName> PickOrder;

	UPROPERTY(Transient)
	TArray<FGrantMoneyUpgradeDefinition> CurrentOptions;

	int32 PendingChoices = 0;
	bool bChoiceActive = false;

	FRandomStream RandomStream;
};
