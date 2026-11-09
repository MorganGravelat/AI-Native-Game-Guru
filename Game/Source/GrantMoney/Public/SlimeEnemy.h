// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnemyBase.h"
#include "SlimeEnemy.generated.h"

class UStaticMeshComponent;

/**
 * The first enemy type. Targeting, pursuit, speed and stopping distance all come
 * from AEnemyBase, as do health, contact damage and death; this class only supplies
 * slime defaults and a temporary visual. Emergence and climbing are added in later tickets.
 */
UCLASS()
class GRANTMONEY_API ASlimeEnemy : public AEnemyBase
{
	GENERATED_BODY()

public:
	ASlimeEnemy();

protected:
	// Temporary stand-in until the slime mesh exists. Visual only: the capsule owns collision.
	// Replace the mesh or hide this component without touching gameplay.
	UPROPERTY(VisibleAnywhere, Category = "Placeholder")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;
};
