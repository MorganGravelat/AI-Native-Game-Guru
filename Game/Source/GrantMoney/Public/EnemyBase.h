// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnemyPickupDrop.h"
#include "GameFramework/Character.h"
#include "EnemyBase.generated.h"

class AXpOrbPickup;
class UContactDamageComponent;
class UEnemyEmergenceComponent;
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
 * UCombatEventsSubsystem::OnEnemyKilled, drops its pickups, then is destroyed after DeathCleanupDelay.
 * Override HandleDeath for per-type death effects.
 * Derive from this class and change the properties; override FindTarget or
 * GetMoveDirection to change who is chased or how.
 *
 * A spawner can have the enemy rise out of the ground first (BeginEmergence). While emerging the enemy
 * stands still, does no contact damage and is skipped by automatic weapon targeting (CanBeTargeted).
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

	// Where the enemy died (world transform), captured at the start of death handling, before it
	// stops and is removed. A drop system reads it from UCombatEventsSubsystem::OnEnemyKilled to
	// spawn items at this spot. Identity until the enemy dies.
	const FTransform& GetDeathTransform() const { return DeathTransform; }

	// Starts the emergence presentation (see UEnemyEmergenceComponent) on GetEmergenceVisual().
	// Until it finishes the enemy does not chase, does not hurt anyone, and cannot be targeted by weapons.
	// A spawner calls this right after spawning; enemies that are never told to emerge behave as before.
	// Does nothing if the enemy is dead or already emerging.
	void BeginEmergence();

	// True from BeginEmergence until the animation finishes (or the enemy dies).
	UFUNCTION(BlueprintPure, Category = "Enemy|Emergence")
	bool IsEmerging() const { return bIsEmerging; }

	// True if automatic weapons may choose this enemy: it is alive, not being removed and not emerging.
	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	bool CanBeTargeted() const;

	UEnemyEmergenceComponent* GetEmergence() const { return Emergence; }

	// Seconds a dead enemy stays in the level (stopped, no collision) before it is destroyed,
	// e.g. for a death animation. 0 destroys it straight away.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat", meta = (ClampMin = "0.0", Units = "s"))
	float DeathCleanupDelay = 0.0f;

	// When true, one XpOrbClass orb is dropped at the death location, every time. Off by default;
	// enemy types turn it on in their constructor (slimes do).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Drops")
	bool bDropsXpOrb = false;

	// The guaranteed orb. Its XpValue says how much XP it gives. Defaults to AXpOrbPickup.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Drops")
	TSubclassOf<AXpOrbPickup> XpOrbClass;

	// Extra pickups with their own drop chances (for example a rare power-up). Each entry is rolled
	// on its own and never affects the guaranteed XP orb above.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Drops")
	TArray<FEnemyPickupDrop> BonusPickupDrops;
	UStatsComponent* GetStats() const { return Stats; }
	UHealthComponent* GetHealth() const { return Health; }
	UContactDamageComponent* GetContactDamage() const { return ContactDamage; }

protected:
	virtual void BeginPlay() override;

	// Runs once when health reaches zero. Stops the enemy, drops its pickups, reports the kill, and schedules destruction.
	// Override to add per-type death effects; call Super to keep the cleanup and the kill event.
	virtual void HandleDeath(AActor* Killer);

	// Spawns the guaranteed XP orb (if bDropsXpOrb) and each bonus drop that passes its chance roll,
	// all at the death location. Called once from HandleDeath, which health runs only once per enemy.
	virtual void SpawnPickupDrops();

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

	// The component that rises out of the ground during emergence, or null for no visual (emergence then
	// takes no time). Enemy types return their mesh here; a replacement mesh only needs to be returned from here.
	virtual USceneComponent* GetEmergenceVisual() const { return nullptr; }

	// Timing and starting shape of the emergence. Edit Duration and FlattenedScale on it.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Emergence")
	TObjectPtr<UEnemyEmergenceComponent> Emergence;

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

	// Bound to Emergence->OnFinished. Turns pursuit and contact damage on.
	UFUNCTION()
	void HandleEmergenceFinished(UEnemyEmergenceComponent* FinishedEmergence);

	bool bIsEmerging = false;

	// Not saved or shown as an editable value; found again every run.
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Enemy|Debug")
	TObjectPtr<AActor> Target;

	// Stops the "no target" warning from repeating every frame.
	bool bLoggedMissingTarget = false;

	// Set once, when the enemy dies.
	FTransform DeathTransform;
};
