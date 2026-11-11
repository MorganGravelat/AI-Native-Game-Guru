// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PickupBase.h"
#include "EnemyPickupDrop.generated.h"

/**
 * One extra pickup an enemy may drop when it dies, with its own chance. Every entry is rolled
 * independently of the others and of the guaranteed XP orb, so adding a rare power-up never
 * changes how often XP drops. See AEnemyBase::BonusPickupDrops.
 */
USTRUCT(BlueprintType)
struct GRANTMONEY_API FEnemyPickupDrop
{
	GENERATED_BODY()

	// What to spawn. An entry with no class never drops.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop")
	TSubclassOf<APickupBase> PickupClass;

	// Chance per death, from 0 (never) to 1 (always). 0.05 = 5%.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DropChance = 1.0f;

	// True if this entry drops for a random Roll in [0, 1]. 0 never drops and 1 always drops,
	// whatever the roll (FMath::FRand can return exactly 0 or 1).
	bool ShouldDrop(float Roll) const
	{
		if (!PickupClass || DropChance <= 0.0f)
		{
			return false;
		}

		return DropChance >= 1.0f || Roll < DropChance;
	}
};
