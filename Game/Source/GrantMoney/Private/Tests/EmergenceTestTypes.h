// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnemyBase.h"
#include "EmergenceTestTypes.generated.h"

/**
 * Test helper only. An enemy that chases a chosen actor, because the test world has no player
 * pawn for AEnemyBase::FindTarget to find.
 */
UCLASS(NotPlaceable, Transient)
class AChaseTestEnemy : public AEnemyBase
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<AActor> ChaseTarget;

protected:
	virtual AActor* FindTarget() const override { return ChaseTarget.Get(); }

	// Any scene component will do; the character's (empty) mesh stands in so emergence takes time.
	virtual USceneComponent* GetEmergenceVisual() const override { return GetMesh(); }
};
