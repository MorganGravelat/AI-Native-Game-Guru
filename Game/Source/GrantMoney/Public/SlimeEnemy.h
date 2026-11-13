// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnemyBase.h"
#include "SlimeEnemy.generated.h"

class UStaticMeshComponent;

/**
 * The first enemy type. Targeting, pursuit, speed and stopping distance all come
 * from AEnemyBase, as do health, contact damage and death; this class only supplies
 * slime defaults (including its rare stat boost drops) and a temporary visual. When spawned by an
 * AEnemySpawner the visual rises out of the ground first (see AEnemyBase::BeginEmergence). Climbing is added in a later ticket.
 */
UCLASS()
class GRANTMONEY_API ASlimeEnemy : public AEnemyBase
{
	GENERATED_BODY()

public:
	ASlimeEnemy();

	// Code default for each permanent stat boost a slime can drop (1% per boost type, rolled
	// separately). Designers change it per entry in BonusPickupDrops on a Blueprint or placed slime.
	static constexpr float DefaultStatBoostDropChance = 0.01f;

	virtual void Tick(float DeltaTime) override;

	// While walking, the body gently stretches tall and squashes flat. Volume is roughly kept: when it gets
	// taller it also gets slightly narrower. It eases out when the slime stops. Visual only.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime|Squish")
	bool bSquishWhileMoving = true;

	// How far the height swings either way, as a fraction of the normal height (0.1 = 10%).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime|Squish", meta = (ClampMin = "0.0", ClampMax = "0.5", EditCondition = "bSquishWhileMoving"))
	float SquishAmount = 0.1f;

	// Full up-and-down cycles per second while walking. Lower is slower.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime|Squish", meta = (ClampMin = "0.0", Units = "Hz", EditCondition = "bSquishWhileMoving"))
	float SquishFrequency = 1.5f;

	// How quickly the squish fades in and out when the slime starts and stops walking.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime|Squish", meta = (ClampMin = "0.0", EditCondition = "bSquishWhileMoving"))
	float SquishBlendSpeed = 4.0f;

	// The squish pose for a base mesh pose. Phase is in radians and Amplitude (0 to 1) scales the swing.
	// The bottom of the mesh stays where it was, so the slime never sinks into or floats above the ground.
	static void ComputeSquishPose(const FVector& BaseScale, const FVector& BaseLocation, float BottomLocalZ,
		float Phase, float Amount, float Amplitude, FVector& OutScale, FVector& OutLocation);

	// The same pose for a given swing: the change in height as a fraction (+0.2 = 20% taller and a little
	// thinner, -0.2 = 20% flatter and a little wider). Bottom held, as above.
	static void ComputeSwingPose(const FVector& BaseScale, const FVector& BaseLocation, float BottomLocalZ,
		float Swing, FVector& OutScale, FVector& OutLocation);

protected:
	virtual void BeginPlay() override;
	virtual void HandleDeath(AActor* Killer) override;

	// Lets a subclass take over the body shape for a while (the boss does this for its lunge). Return true
	// and set OutSwing (see ComputeSwingPose) to hold that shape instead of the walking squish; the body eases
	// into it and back out again when this returns false. Normal slimes never override.
	virtual bool GetSquishOverride(float& OutSwing) const { return false; }

	// The placeholder rises out of the ground when the slime emerges. To use a real mesh later, swap the
	// mesh on PlaceholderMesh, or override this to return the new mesh component; nothing else changes.
	virtual USceneComponent* GetEmergenceVisual() const override;

	// Temporary stand-in until the slime mesh exists. Visual only: the capsule owns collision.
	// Replace the mesh or hide this component without touching gameplay.
	UPROPERTY(VisibleAnywhere, Category = "Placeholder")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

private:
	void UpdateSquish(float DeltaTime);

	// The resting pose of PlaceholderMesh, read at BeginPlay, that the squish is applied on top of.
	FVector BaseMeshScale = FVector::OneVector;
	FVector BaseMeshLocation = FVector::ZeroVector;
	float BaseMeshBottomLocalZ = 0.0f;

	// Radians. Starts at a random value so a crowd of slimes does not pulse in unison.
	float SquishPhase = 0.0f;

	// 0 = standing still, 1 = full walking squish.
	float SquishAlpha = 0.0f;

	// How much of the pose comes from GetSquishOverride (0 to 1), and the eased swing it asked for.
	float OverrideWeight = 0.0f;
	float OverrideSwing = 0.0f;

	bool bMeshAtRest = true;
};
