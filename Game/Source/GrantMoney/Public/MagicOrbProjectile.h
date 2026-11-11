// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MagicOrbProjectile.generated.h"

class UAttackHitComponent;
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
 */
UCLASS()
class GRANTMONEY_API AMagicOrbProjectile : public AActor
{
	GENERATED_BODY()

public:
	AMagicOrbProjectile();

	// Sets direction, speed, damage per hit, lifetime and penetration. Call between SpawnActorDeferred
	// and FinishSpawning, before the orb begins play. Direction does not need to be normalised.
	// Penetration is how many extra enemies the orb may pass through: 0 = used up by its first hit,
	// 2 = hits up to 3 enemies. Negative values count as 0.
	void Launch(const FVector& Direction, float Speed, float Damage, float LifetimeSeconds, int32 Penetration = 0);

	UAttackHitComponent* GetAttackHit() const { return AttackHit; }

	float GetLifetimeSeconds() const { return LifetimeSeconds; }

protected:
	virtual void BeginPlay() override;

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
};
