// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MagicOrbProjectile.generated.h"

class UAttackHitComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 * A magic orb: a small sphere that flies in a straight line, damages the first living
 * target it touches (plus one more per point of penetration), then disappears. It never homes, bounces or falls. It also
 * disappears when its lifetime runs out, so a missed orb does not fly forever.
 *
 * Hit rules and damage come from UAttackHitComponent (one hit per target, never the
 * shooter), and damage goes through UGameplayStatics::ApplyDamage, so the target only
 * needs a UHealthComponent. The orb damages whatever living target it actually touches,
 * not necessarily the one it was aimed at. It overlaps pawns only: walls and ground
 * do not stop it. Spawn it with Owner and Instigator set to the shooter; the damage
 * causer reported to the target (and to OnEnemyKilled) is the orb, whose Instigator is the shooter.
 *
 * With ExplosionRadius above 0 (see Launch), the orb has one explosion "charge": the instant it lands
 * its first direct hit (not when its hits are later exhausted by penetration), it also bursts right
 * there — UAttackHitComponent::TryAreaHit damages every other living target within ExplosionRadius once
 * (skipping anyone this orb already hit directly, and never using up or caring about penetration) — then
 * the charge is gone for good. If penetration lets the orb keep flying afterward, it keeps hitting more
 * enemies directly as normal, but never explodes again; only a total miss (no direct hit before its
 * lifetime runs out) can still spend the charge on expiry, at wherever the orb ends up. While the burst's
 * placeholder visual (grow and glow red, fading to black) plays, the orb's own movement and hit detection
 * are untouched unless it is also being removed (its hits are exhausted or its lifetime is over), in which
 * case removal waits for the visual to finish. Placeholder look (see ExplosionColor/ExplosionGlowMaterial):
 * swap for real VFX later without touching any of the above gameplay rules.
 */
UCLASS()
class GRANTMONEY_API AMagicOrbProjectile : public AActor
{
	GENERATED_BODY()

public:
	AMagicOrbProjectile();

	virtual void Tick(float DeltaSeconds) override;

	// Sets direction, speed, damage per hit, lifetime, penetration and explosion radius. Call between
	// SpawnActorDeferred and FinishSpawning, before the orb begins play. Direction does not need to be
	// normalised. Penetration is how many extra enemies the orb may pass through: 0 = used up by its
	// first hit, 2 = hits up to 3 enemies. Negative values count as 0. ExplosionRadius 0 (the default)
	// means no burst at all; above 0, an area burst happens once the orb is used up or expires (see class comment).
	void Launch(const FVector& Direction, float Speed, float Damage, float LifetimeSeconds, int32 Penetration = 0, float ExplosionRadius = 0.0f);

	UAttackHitComponent* GetAttackHit() const { return AttackHit; }

	float GetLifetimeSeconds() const { return LifetimeSeconds; }

	float GetExplosionRadius() const { return ExplosionRadius; }

	// True while the burst visual (grow + fade) is actively playing.
	bool IsExploding() const { return bExploding; }

	// True once this orb has spent its one explosion charge (via a direct hit or, failing that, on
	// expiry). Once true, it can never explode again, no matter how much penetration remains.
	bool HasExploded() const { return bHasExploded; }

	// Seconds the "grow then vanish" burst visual takes once the orb explodes.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orb|Explosion", meta = (ClampMin = "0.01", Units = "s"))
	float ExplosionVisualSeconds = 0.25f;

	// Emissive colour the burst glows at the moment it starts, fading to black (see Tick) as it grows
	// and vanishes. Placeholder look: replace with a real VFX later (see class comment on PlaceholderMesh).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orb|Explosion")
	FLinearColor ExplosionColor = FLinearColor(4.0f, 0.05f, 0.05f);

protected:
	virtual void BeginPlay() override;

	// Spends the orb's one explosion charge: runs the area damage pass and starts the grow/fade visual.
	// Safe to call when ExplosionRadius is not above 0 or the charge is already spent (does nothing).
	// Does not stop the orb's own movement/collision — see FinishIfOutOfHits for when it actually stops.
	void TriggerExplosion();

	// Stops the orb's movement and hit detection because it is done (its hits are exhausted or its
	// lifetime is over). If the burst visual from TriggerExplosion is still playing, destruction waits
	// for it to finish (see Tick); otherwise the orb is destroyed immediately.
	void FinishIfOutOfHits();

	// Overridden so a lifetime timeout can spend an unused explosion charge (if any) at wherever the orb
	// ends up, instead of just vanishing outright.
	virtual void LifeSpanExpired() override;

	// Solid sphere used only to detect pawns. Collision is query-only.
	UPROPERTY(VisibleAnywhere, Category = "Orb")
	TObjectPtr<USphereComponent> Collision;

	// Temporary visual (the engine sphere). Replace the mesh without touching gameplay.
	UPROPERTY(VisibleAnywhere, Category = "Placeholder")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

	UPROPERTY(VisibleAnywhere, Category = "Orb")
	TObjectPtr<UProjectileMovementComponent> Movement;

	// Once-per-target hit rule, damage and penetration (both set by Launch from the weapon).
	// With penetration 0 (the default) the orb is used up by its first hit.
	UPROPERTY(VisibleAnywhere, Category = "Orb")
	TObjectPtr<UAttackHitComponent> AttackHit;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleHitsExhausted(UAttackHitComponent* UsedAttackHit);

	// Seconds until the orb removes itself. Set by Launch.
	float LifetimeSeconds = 3.0f;

	// Set (deferred) by HandleHitsExhausted and consumed by HandleBeginOverlap right after, so the same
	// hit that exhausts the orb's budget still gets to spend an explosion charge first. See
	// HandleBeginOverlap for why the ordering matters: OnHitsExhausted fires synchronously inside TryHit,
	// before HandleBeginOverlap gets a chance to call TriggerExplosion for that same hit.
	bool bHitsExhausted = false;

	// Radius (cm) of the burst a direct hit (or, failing that, expiry) can trigger. 0 = no burst. Set by Launch.
	float ExplosionRadius = 0.0f;

	// True once the orb's one explosion charge has been spent (see TriggerExplosion). Prevents a second
	// burst from any later penetration hit.
	bool bHasExploded = false;

	// True while the grow/fade visual started by TriggerExplosion is actively playing.
	bool bExploding = false;

	// True if the orb should be destroyed as soon as the current burst visual finishes (set by
	// FinishIfOutOfHits when it is called while bExploding is still true).
	bool bDestroyWhenVisualFinishes = false;

	float ExplosionElapsed = 0.0f;
	FVector ExplosionStartScale = FVector::OneVector;
	FVector ExplosionTargetScale = FVector::OneVector;

	// Source material for the burst glow (the engine's emissive mesh material, same one XpOrbPickup and
	// StatBoostPickup already use for their glow). Tinted red and faded to black via ExplosionMID.
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ExplosionGlowMaterial;

	// Created the moment the orb explodes; its EmissiveColor is lerped from ExplosionColor to black over
	// ExplosionVisualSeconds in Tick, alongside the mesh's growing scale.
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ExplosionMID;
};
