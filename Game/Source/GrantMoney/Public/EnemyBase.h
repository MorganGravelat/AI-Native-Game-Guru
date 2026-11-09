// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyBase.generated.h"

class UContactDamageComponent;
class UHealthComponent;
class UStatsComponent;

/**
 * Reusable base for simple enemies that walk straight at the player.
 * It finds a target, moves toward it, and damages it on contact through a
 * UContactDamageComponent (damage = the ContactDamage stat). There is no pathfinding,
 * and no AI controller is needed.
 *
 * Enemies have health (max health = the MaxHealth stat) and take damage from
 * UGameplayStatics::ApplyDamage. On death the enemy stops, reports itself to
 * UCombatEventsSubsystem::OnEnemyKilled, then is destroyed after DeathCleanupDelay.
 * Override HandleDeath for per-type death effects.
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

	// Seconds a dead enemy stays in the level (stopped, no collision) before it is destroyed,
	// e.g. for a death animation. 0 destroys it straight away.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat", meta = (ClampMin = "0.0", Units = "s"))
	float DeathCleanupDelay = 0.0f;

	UStatsComponent* GetStats() const { return Stats; }
	UHealthComponent* GetHealth() const { return Health; }
	UContactDamageComponent* GetContactDamage() const { return ContactDamage; }

protected:
	virtual void BeginPlay() override;

	// Runs once when health reaches zero. Stops the enemy, reports the kill, and schedules destruction.
	// Override to add per-type death effects; call Super to keep the cleanup and the kill event.
	virtual void HandleDeath(AActor* Killer);

	// Current health. Max health is the MaxHealth stat.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	TObjectPtr<UHealthComponent> Health;

	// Enemy stats: MaxHealth, ContactDamage (how hard a touch hurts) and AttackInterval (seconds
	// between hits, default 1). Override per enemy type; buffs and difficulty scaling add modifiers.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	TObjectPtr<UStatsComponent> Stats;

	// Damages the target while touching it, using the stats above. Reach is set on the component.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	TObjectPtr<UContactDamageComponent> ContactDamage;

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
	// Bound to Health->OnDeath; forwards to HandleDeath.
	UFUNCTION()
	void HandleHealthDepleted(UHealthComponent* DepletedHealth, AActor* DamageCauser);

	// Not saved or shown as an editable value; found again every run.
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Enemy|Debug")
	TObjectPtr<AActor> Target;

	// Stops the "no target" warning from repeating every frame.
	bool bLoggedMissingTarget = false;
};
