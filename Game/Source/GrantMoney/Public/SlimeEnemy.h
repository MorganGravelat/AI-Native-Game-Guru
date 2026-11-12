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

protected:
	// The placeholder rises out of the ground when the slime emerges. To use a real mesh later, swap the
	// mesh on PlaceholderMesh, or override this to return the new mesh component; nothing else changes.
	virtual USceneComponent* GetEmergenceVisual() const override;

	// Temporary stand-in until the slime mesh exists. Visual only: the capsule owns collision.
	// Replace the mesh or hide this component without touching gameplay.
	UPROPERTY(VisibleAnywhere, Category = "Placeholder")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;
};
