// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnemyBase.h"
#include "GrantMoneyStatTypes.h"
#include "UObject/Object.h"
#include "CombatTestListener.generated.h"

class AActor;
class UAttackHitComponent;
class UHealthComponent;

/**
 * Test helper only. Records the stat, health, death, kill and attack-hit events it receives, because
 * dynamic delegates can bind only to UFUNCTIONs, not to lambdas inside a test.
 */
UCLASS(Transient, NotBlueprintable, HideDropdown)
class UCombatTestListener : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void OnStatChanged(EGrantMoneyStat Stat, float OldValue, float NewValue)
	{
		++StatChangedCount;
		LastStatOldValue = OldValue;
		LastStatNewValue = NewValue;
	}

	UFUNCTION()
	void OnHealthChanged(UHealthComponent* HealthComponent, float NewHealth, float Delta)
	{
		++HealthChangedCount;
		LastHealth = NewHealth;
		LastDelta = Delta;
	}

	UFUNCTION()
	void OnDeath(UHealthComponent* HealthComponent, AActor* DamageCauser)
	{
		++DeathCount;
		LastDamageCauser = DamageCauser;
	}

	UFUNCTION()
	void OnEnemyKilled(AEnemyBase* Enemy, AActor* Killer)
	{
		++EnemyKilledCount;
		LastKilledEnemy = Enemy;
		LastKiller = Killer;
		bKilledEnemyWasAliveWhenReported = IsValid(Enemy) && !Enemy->IsActorBeingDestroyed();
	}

	UFUNCTION()
	void OnAttackTargetHit(AActor* Target, float Damage)
	{
		++AttackHitCount;
		LastAttackDamage = Damage;
	}

	UFUNCTION()
	void OnAttackHitsExhausted(UAttackHitComponent* AttackHit)
	{
		++AttackExhaustedCount;
	}

	int32 AttackHitCount = 0;
	float LastAttackDamage = 0.0f;
	int32 AttackExhaustedCount = 0;

	int32 EnemyKilledCount = 0;
	// The enemy may be destroyed right after the event; compare with LastKilledEnemy.Get(true).
	TWeakObjectPtr<AEnemyBase> LastKilledEnemy;
	bool bKilledEnemyWasAliveWhenReported = false;
	TWeakObjectPtr<AActor> LastKiller;

	int32 StatChangedCount = 0;
	float LastStatOldValue = 0.0f;
	float LastStatNewValue = 0.0f;

	int32 HealthChangedCount = 0;
	float LastHealth = 0.0f;
	float LastDelta = 0.0f;

	int32 DeathCount = 0;
	TWeakObjectPtr<AActor> LastDamageCauser;
};
