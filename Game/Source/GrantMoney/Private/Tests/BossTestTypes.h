// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BossSlimeEnemy.h"
#include "EnemySpawnPlacement.h"
#include "BossTestTypes.generated.h"

/**
 * Test helper only. A boss that chases a chosen actor (the test world has no player pawn) and exposes the
 * protected movement hooks the lunge overrides.
 */
UCLASS(Transient, NotBlueprintable, HideDropdown)
class ATestBossSlime : public ABossSlimeEnemy
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient)
	TObjectPtr<AActor> TestTarget;

	// Reported by GetTargetVelocity() instead of the target actor's real (usually zero, in tests) velocity.
	FVector TestTargetVelocity = FVector::ZeroVector;

	FVector ExposedMoveDirection() const { return GetMoveDirection(); }
	float ExposedDesiredSpeed() const { return GetDesiredMoveSpeed(); }

protected:
	virtual AActor* FindTarget() const override { return TestTarget; }
	virtual FVector GetTargetVelocity() const override { return TestTargetVelocity; }
};

namespace BossTest
{
	constexpr int32 N = FGrantMoneyMapData::MapSize;

	// A synthetic map whose playable cells are MinCell..MaxCell. Cell (X, Y) is centred on (X * 100, Y * 100),
	// so a 20 x 20 map from (0, 0) covers world X and Y from -50 to 1950. Keep this alive as long as Arena is used.
	struct FTestArena
	{
		uint16 Ground[N][N];
		uint8 Valid[N][N];
		FSpawnArena Arena;

		void Init(const FIntPoint& MinCell, const FIntPoint& MaxCell)
		{
			FMemory::Memzero(Ground, sizeof(Ground));
			FMemory::Memzero(Valid, sizeof(Valid));
			for (int32 X = MinCell.X; X <= MaxCell.X; ++X)
			{
				for (int32 Y = MinCell.Y; Y <= MaxCell.Y; ++Y)
				{
					Valid[X][Y] = 1;
					Ground[X][Y] = 1;
				}
			}

			Arena.Map.Ground = Ground;
			Arena.Map.Valid = Valid;
			Arena.Map.Width = N;
			Arena.Map.Height = N;
		}
	};
}
