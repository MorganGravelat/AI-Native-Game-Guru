// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "StarterWeaponComponent.generated.h"

class AMagicOrbProjectile;

/**
 * The player's starter weapon: fires a magic orb at the nearest living enemy in range,
 * automatically, once per FireInterval. The player never presses a button or turns toward
 * the target; the owner's and the camera's facing play no part in choosing it.
 *
 * Each interval it asks SelectTarget for a target. If there is none in range it skips that
 * shot and asks again next interval. Targeting lives in SelectTarget and firing in FireAt, so
 * a future weapon can derive from this class and override either one with its own rule.
 *
 * The orb is aimed at the target's collision center at the moment of firing, then flies in a
 * straight line. All tuning values below are read when a shot is fired, so they can be changed
 * while playing.
 *
 * Damage, FireInterval and Penetration are the weapon's base values. If the owner has a
 * UStatsComponent, its Damage, AttackInterval and Penetration modifiers (for example permanent
 * stat boost pickups) are applied on top of them; see the GetEffective* functions. If the owner's
 * stats hold a base value for one of those stats, that base replaces the weapon's own value,
 * the same rule UAttackHitComponent and UContactDamageComponent use. ProjectileCount and
 * ExplosionRadius (SCRUM-36 upgrades) follow the exact same rule.
 *
 * With ProjectileCount above 1, TryFire fires that many orbs in the same cycle, one each at up to that
 * many distinct living enemies in range (nearest first); with fewer enemies in range than orbs, the
 * extra orbs re-target the same enemies rather than being skipped. With ExplosionRadius above 0, every
 * orb also bursts once it is used up or expires (see AMagicOrbProjectile).
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class GRANTMONEY_API UStarterWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStarterWeaponComponent();

	// Nearest living enemy within TargetRange of the owner, or null if there is none.
	// Distance is measured on the ground plane, from the owner's center to the enemy's center.
	// Enemies that are dead (even if still in the world) or still emerging from the ground are never chosen.
	virtual AActor* SelectTarget() const;

	// Up to Count distinct living enemies within TargetRange, nearest first (same rule as SelectTarget).
	// Fewer than Count if fewer are in range; empty if none are. Count below 1 is treated as 1.
	virtual TArray<AActor*> SelectTargets(int32 Count) const;

	// Spawns one orb at the owner, aimed at Target's collision center, with the weapon's damage,
	// speed and lifetime. Returns the orb, or null if it could not be fired.
	virtual AMagicOrbProjectile* FireAt(AActor* Target);

	// One firing interval: selects up to GetEffectiveProjectileCount() targets and fires one orb at
	// each (reusing targets if there are fewer enemies in range than orbs), or skips the whole cycle if
	// nothing is in range at all. Returns true if at least one orb was fired.
	bool TryFire();

	// Damage per orb after the owner's Damage stat modifiers.
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetEffectiveDamage() const;

	// Seconds between shots after the owner's AttackInterval stat modifiers. Never below 0.05 s.
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetEffectiveFireInterval() const;

	// Extra enemies each orb passes through after the owner's Penetration stat modifiers, rounded down.
	// 0 = an orb stops at the first enemy, 2 = it can hit up to 3. Never negative.
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetEffectivePenetration() const;

	// Orbs fired per cycle after the owner's ProjectileCount stat modifiers, rounded down. Never below 1.
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetEffectiveProjectileCount() const;

	// Burst radius (cm) each orb leaves behind after the owner's ExplosionRadius stat modifiers.
	// 0 (the default, no upgrade taken) means no burst at all. Never negative.
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetEffectiveExplosionRadius() const;

	// Damage per orb, applied through the existing health system.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.0"))
	float Damage = 10.0f;

	// Seconds between shots (and between target checks when nothing is in range).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.05", Units = "s"))
	float FireInterval = 1.0f;

	// Extra enemies each orb can pass through before it is used up. 0 = only the first enemy is hit.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0"))
	int32 Penetration = 0;

	// Orbs fired per cycle. 1 (the default) is today's single-orb behavior.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "1"))
	int32 ProjectileCount = 1;

	// Burst radius (cm) each orb leaves behind when it is used up or expires. 0 (the default) means no burst.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.0", Units = "cm"))
	float ExplosionRadius = 0.0f;

	// Largest ground-plane distance, owner center to enemy center, at which an enemy can be targeted.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.0", Units = "cm"))
	float TargetRange = 800.0f;

	// Orb speed in cm/s.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "1.0", Units = "cm/s"))
	float ProjectileSpeed = 1000.0f;

	// Seconds before an orb that hit nothing removes itself. Also limits its reach to Speed x Lifetime.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.05", Units = "s"))
	float ProjectileLifetime = 2.0f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// The orb to spawn. Defaults to AMagicOrbProjectile.
	UPROPERTY(EditAnywhere, Category = "Weapon")
	TSubclassOf<AMagicOrbProjectile> ProjectileClass;

private:
	void HandleFireTimer();

	// Starts the countdown to the next shot, using the current effective fire interval.
	void ScheduleNextShot();

	FTimerHandle FireTimerHandle;
};
