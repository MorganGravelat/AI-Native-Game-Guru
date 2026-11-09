// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CombatEventsSubsystem.generated.h"

class AEnemyBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGrantMoneyEnemyKilled, AEnemyBase*, Enemy, AActor*, Killer);

/**
 * One place per world to hear about combat events, so systems such as XP, score,
 * kill counters, or global sound/VFX do not have to bind to every enemy as it spawns.
 *
 * Get it with GetWorld()->GetSubsystem<UCombatEventsSubsystem>() and bind to the events.
 * Enemies report themselves; listeners never need to know the enemy's type.
 */
UCLASS()
class GRANTMONEY_API UCombatEventsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Called by AEnemyBase when it dies. Broadcasts OnEnemyKilled.
	void NotifyEnemyKilled(AEnemyBase* Enemy, AActor* Killer);

	// Fires once per enemy death, before the enemy is destroyed, so the enemy and its
	// location and stats can still be read. Killer is the damage causer (for example
	// a projectile) and may be null.
	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnGrantMoneyEnemyKilled OnEnemyKilled;
};
