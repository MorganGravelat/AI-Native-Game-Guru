// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapArrays.h"

class ATileMapRenderer;

/**
 * The playable part of the world as the spawner sees it: one TileGen map plus where it sits in the
 * world. Pure data and math (no world queries), so it works on any registered map and can be tested
 * with synthetic maps.
 *
 * A cell is playable only if Valid[X][Y] != 0 and Ground[X][Y] != 0, the same rule as
 * FMapBoundaryGenerator and FPlayerSpawnResolver. Cell (X, Y) is centred on local position
 * (X * CellSize, Y * CellSize), so it covers half a cell either side of that point.
 */
struct GRANTMONEY_API FSpawnArena
{
	FGrantMoneyMapData Map;

	// Map-local to world, normally the TileMapRenderer's actor transform.
	FTransform MapToWorld = FTransform::Identity;

	// Width of one cell in Unreal units.
	float CellSize = 100.0f;

	bool IsValid() const { return Map.IsValid() && CellSize > UE_KINDA_SMALL_NUMBER; }

	// World height of the tile surface (local Z = 0).
	double GetGroundZ() const { return MapToWorld.GetLocation().Z; }

	bool IsPlayableCell(const FIntPoint& Cell) const;

	// The cell containing a world position (height is ignored). False if the arena is invalid.
	// The cell can lie outside the map's bounds; IsPlayableCell then returns false for it.
	bool WorldToCell(const FVector& WorldLocation, FIntPoint& OutCell) const;

	// True if a position is on a playable cell.
	bool IsLocationPlayable(const FVector& WorldLocation) const;

	// True if a circle of Radius around Centre (ground plane) lies on playable cells. Tests the centre
	// and eight points on the rim, so a circle that sticks out past a map edge is rejected.
	bool IsFootprintPlayable(const FVector& Centre, float Radius) const;

	// Builds the arena for a renderer's selected level. False if the level is not a valid registered map.
	static bool FromRenderer(const ATileMapRenderer* Renderer, FSpawnArena& OutArena);
};

class GRANTMONEY_API FEnemySpawnPlacement
{
public:
	// A ground-plane offset in the ring MinRadius..MaxRadius around the origin, spread evenly over the
	// ring's area. AngleAlpha and RadiusAlpha are random numbers in 0..1. MaxRadius below MinRadius is
	// treated as equal to it, and negative radii as 0.
	static FVector2D SampleRingOffset(float MinRadius, float MaxRadius, float AngleAlpha, float RadiusAlpha);

	// True if Candidate is between MinRadius and MaxRadius (inclusive) from Centre on the ground plane.
	static bool IsInRing(const FVector& Centre, const FVector& Candidate, float MinRadius, float MaxRadius);
};
