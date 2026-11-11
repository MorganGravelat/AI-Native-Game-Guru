// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupBase.generated.h"

class UPrimitiveComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 * Shared base for everything the player can pick up (XP orbs now, power-ups later).
 *
 * This class owns the whole pickup life cycle, so a pickup type only says what it does:
 *  1. A UPickupComponent on the collector finds the pickup inside its radius and calls BeginAttraction.
 *  2. The pickup locks onto that collector and flies to its current position every frame, speeding
 *     up from BaseSpeed to MaxSpeed. It keeps chasing even if the collector leaves the radius.
 *  3. On arrival (within CollectionDistance, or when a fast step would overshoot) TryCollect runs
 *     OnCollected exactly once and then removes the pickup.
 *
 * To add a pickup type, derive from this class and override OnCollected. Example, a temporary buff:
 *
 *   void ASpeedBoostPickup::OnCollected(AActor* Collector)
 *   {
 *       if (UStatsComponent* Stats = Collector->FindComponentByClass<UStatsComponent>())
 *       {
 *           const FGrantMoneyStatModifierHandle Handle = Stats->AddModifier(EGrantMoneyStat::MaxHealth, EGrantMoneyStatModifierOp::Multiplicative, 0.5f);
 *           // Remove the modifier after Duration with a timer owned by the collector (this actor is about to be destroyed).
 *       }
 *   }
 *
 * Neither UPickupComponent nor the player needs to change. To make an enemy drop it, add an entry
 * to AEnemyBase::BonusPickupDrops. Swap the look by replacing PickupMesh's mesh or material.
 */
UCLASS(Abstract)
class GRANTMONEY_API APickupBase : public AActor
{
	GENERATED_BODY()

public:
	APickupBase();

	virtual void Tick(float DeltaSeconds) override;

	// Locks onto Collector and starts flying toward it. Returns true if this call started the pursuit;
	// false if the pickup is already collected or already chasing a live collector. Safe to call every frame.
	bool BeginAttraction(AActor* Collector);

	// Applies the pickup's effect to Collector through OnCollected, then removes the pickup. Runs at most
	// once per pickup: later calls (for example from a second overlap callback) return false and do nothing.
	bool TryCollect(AActor* Collector);

	bool IsAttracted() const { return LockedCollector.IsValid(); }
	bool IsCollected() const { return bCollected; }
	AActor* GetCollector() const { return LockedCollector.Get(); }
	float GetCurrentSpeed() const { return CurrentSpeed; }

	// Speed (cm/s) the pickup starts at when it locks onto a collector.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Movement", meta = (ClampMin = "0.0", Units = "cm/s"))
	float BaseSpeed = 400.0f;

	// How fast the speed grows while chasing (cm/s each second).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Movement", meta = (ClampMin = "0.0", Units = "cm/s^2"))
	float Acceleration = 1500.0f;

	// Top speed (cm/s). Keep it above the collector's top movement speed (a character's default is 600)
	// or a running collector can outrun the pickup for good.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Movement", meta = (ClampMin = "0.0", Units = "cm/s"))
	float MaxSpeed = 1200.0f;

	// Distance (cm) from the collector's centre at which the pickup counts as arrived.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Movement", meta = (ClampMin = "0.0", Units = "cm"))
	float CollectionDistance = 40.0f;

protected:
	// What happens when the pickup reaches the collector. Called once, just before the pickup is removed.
	virtual void OnCollected(AActor* Collector) {}

	// Query-only collision that UPickupComponent's radius scan finds. Not used for movement.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<USphereComponent> Collision;

	// Visual only. Replace the mesh or material (or hide it) without touching gameplay.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UStaticMeshComponent> PickupMesh;

private:
	// Stops chasing, e.g. because the collector was destroyed. The pickup waits for another collector.
	void StopAttraction();

	// Not saved; set when a collector locks on.
	TWeakObjectPtr<AActor> LockedCollector;

	float CurrentSpeed = 0.0f;

	// Set the moment collection starts, so a second collection attempt cannot apply the effect again.
	bool bCollected = false;
};
