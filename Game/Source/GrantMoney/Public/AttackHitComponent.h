// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttackHitComponent.generated.h"

class UAttackHitComponent;
class UStatsComponent;

/** What happened when an attack tried to hit an actor. */
UENUM(BlueprintType)
enum class EGrantMoneyHitResult : uint8
{
	// Damage was sent and the target was recorded.
	Hit,

	// This attack already hit the target; no damage. Does not use up penetration.
	AlreadyHit,

	// Not something this attack damages (itself, its owner or instigator, no living health). Does not use up penetration.
	NotATarget,

	// The attack has used all of its hits (1 + Penetration).
	Exhausted,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGrantMoneyAttackTargetHit, AActor*, Target, float, Damage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGrantMoneyAttackHitsExhausted, UAttackHitComponent*, AttackHit);

/**
 * Hit rules for one attack (a projectile, a sword swing, an area pulse):
 *  - each target is damaged at most once, however many frames or components overlap it;
 *  - the attack can hit 1 + Penetration targets, then fires OnHitsExhausted;
 *  - it never hits itself, its owner (e.g. the weapon) or its instigator (e.g. the player);
 *  - only actors with a living UHealthComponent count, so walls, props and dying enemies
 *    do not use up penetration.
 *
 * Damage and Penetration are the owner's Damage and Penetration stats (UStatsComponent),
 * or FallbackDamage / FallbackPenetration when the owner has no such stat. Damage is sent
 * through UGameplayStatics::ApplyDamage with the owner as the damage causer.
 *
 * Hooking it to a projectile (SCRUM-32):
 *  1. Add the component in the projectile's constructor.
 *  2. Spawn the projectile with Owner = the weapon and Instigator = the player pawn, and set its
 *     damage/penetration (copy the weapon's final values into FallbackDamage/FallbackPenetration,
 *     or give the projectile a UStatsComponent).
 *  3. In the projectile's overlap handler, call TryHit(OtherActor).
 *  4. Bind OnHitsExhausted to destroy (or return to pool) the projectile.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class GRANTMONEY_API UAttackHitComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAttackHitComponent();

	// Damages Target if the rules above allow it. Safe to call on every overlap.
	UFUNCTION(BlueprintCallable, Category = "Combat")
	EGrantMoneyHitResult TryHit(AActor* Target);

	// Damages Target once, the same as TryHit, but ignores IsExhausted(): penetration limits how many
	// targets a flight-through hit reaches, not how many an area effect (e.g. an explosion) can reach,
	// and it never counts toward that limit either, so it can never rob a penetrating hit of a target
	// still ahead on its path. Still never hits the same target twice (TryHit and TryAreaHit share
	// HitActors for that), never hits an invalid target, and still only ever damages each target once in
	// total between the two. Returns true if damage was applied.
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool TryAreaHit(AActor* Target);

	// True if TryHit(Target) would hit right now.
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool CanHit(const AActor* Target) const;

	// True if Target is something this attack damages at all (ignores whether it was already hit).
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsValidTarget(const AActor* Target) const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool HasHit(const AActor* Target) const;

	// Damage per hit: the Damage stat, or FallbackDamage.
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetDamage() const;

	// Extra targets after the first: the Penetration stat rounded down, or FallbackPenetration. Never negative.
	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetPenetration() const;

	// Hits left before the attack is exhausted. Counts only TryHit successes (direct, piercing hits); an
	// area hit from TryAreaHit never uses this up, so an explosion catching an enemy still ahead on a
	// penetrating attack's path cannot rob it of a direct hit on that same enemy.
	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetRemainingHits() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsExhausted() const { return GetRemainingHits() <= 0; }

	// Every target hit so far, by either TryHit or TryAreaHit (both count here: this is about double-
	// damage prevention, not the penetration budget — see GetRemainingHits for that).
	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetHitCount() const { return HitActors.Num(); }

	// Forgets every target hit so far (and resets the penetration budget), e.g. for a pooled projectile
	// or one that returns and may hit again.
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ResetHits();

	// Fires after each successful hit (for hit VFX/sound).
	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnGrantMoneyAttackTargetHit OnTargetHit;

	// Fires once, after the hit that uses up the last allowed hit. Bind it to destroy the projectile.
	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnGrantMoneyAttackHitsExhausted OnHitsExhausted;

	// Damage per hit when the owner has no Damage stat.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.0"))
	float FallbackDamage = 10.0f;

	// Extra targets when the owner has no Penetration stat. 0 = stops at the first target.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0"))
	int32 FallbackPenetration = 0;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UStatsComponent> Stats;

	// Targets already hit by this attack (by TryHit or TryAreaHit). Weak so a destroyed target never
	// keeps anything alive. Used only to prevent double-damage; see DirectHitCount for the penetration budget.
	TSet<TWeakObjectPtr<const AActor>> HitActors;

	// How many TryHit successes this attack has landed. Separate from HitActors.Num() so an area hit
	// (TryAreaHit) never counts toward penetration, even if it happens to catch a target still ahead on
	// a piercing attack's path.
	int32 DirectHitCount = 0;
};
