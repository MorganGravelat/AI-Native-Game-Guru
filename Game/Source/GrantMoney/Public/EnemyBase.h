// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyBase.generated.h"

/**
 * Reusable base for simple enemies that walk straight at the player.
 * It only knows how to find a target and move toward it. There is no pathfinding,
 * and no AI controller is needed. Health, attacks and death are added in later tickets.
 * Derive from this class and change the properties; override FindTarget or
 * GetMoveDirection to change who is chased or how.
 */
UCLASS()
class GRANTMONEY_API AEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyBase();

	virtual void Tick(float DeltaTime) override;

	// Walk speed in cm/s. Applied every frame, so it can be tuned while playing.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement", meta = (ClampMin = "0.0", Units = "cm/s"))
	float MoveSpeed = 200.0f;

	// The enemy stops walking when the distance on the ground plane between its centre
	// and the target's centre is at or below this. Both capsule radii count toward it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement", meta = (ClampMin = "0.0", Units = "cm"))
	float StopDistance = 100.0f;

	// Within this distance beyond StopDistance the enemy eases off, reaching zero speed at StopDistance.
	// Prevents stop-and-go stutter when the target moves away slowly. 0 turns this off.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement", meta = (ClampMin = "0.0", Units = "cm"))
	float SlowdownDistance = 50.0f;

	// The current target, or null if none has been found yet.
	AActor* GetTarget() const { return Target; }

protected:
	// Returns the actor to chase, or null if there is none. Defaults to the first player's pawn.
	virtual AActor* FindTarget() const;

	// Sets Target from FindTarget. Returns true if a valid target is held afterwards.
	bool AcquireTarget();

	// Flat (no vertical part) offset from this enemy to the target, or zero if there is no target.
	FVector GetFlatOffsetToTarget() const;

	// Flat (no vertical part) unit direction toward the target, or zero when already within StopDistance.
	virtual FVector GetMoveDirection() const;

	void MoveTowardTarget();

private:
	// Not saved or shown as an editable value; found again every run.
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Enemy|Debug")
	TObjectPtr<AActor> Target;

	// Stops the "no target" warning from repeating every frame.
	bool bLoggedMissingTarget = false;
};
