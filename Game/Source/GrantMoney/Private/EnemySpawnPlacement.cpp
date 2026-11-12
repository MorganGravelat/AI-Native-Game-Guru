// Fill out your copyright notice in the Description page of Project Settings.

#include "EnemySpawnPlacement.h"

#include "TileMapRenderer.h"

bool FSpawnArena::IsPlayableCell(const FIntPoint& Cell) const
{
	return Map.IsValid()
		&& Cell.X >= 0 && Cell.X < Map.Width
		&& Cell.Y >= 0 && Cell.Y < Map.Height
		&& Map.Valid[Cell.X][Cell.Y] != 0
		&& Map.Ground[Cell.X][Cell.Y] != 0;
}

bool FSpawnArena::WorldToCell(const FVector& WorldLocation, FIntPoint& OutCell) const
{
	if (!IsValid())
	{
		return false;
	}

	const FVector Local = MapToWorld.InverseTransformPosition(WorldLocation);
	OutCell = FIntPoint(FMath::RoundToInt32(Local.X / CellSize), FMath::RoundToInt32(Local.Y / CellSize));
	return true;
}

bool FSpawnArena::IsLocationPlayable(const FVector& WorldLocation) const
{
	FIntPoint Cell;
	return WorldToCell(WorldLocation, Cell) && IsPlayableCell(Cell);
}

bool FSpawnArena::IsFootprintPlayable(const FVector& Centre, float Radius) const
{
	if (!IsLocationPlayable(Centre))
	{
		return false;
	}

	constexpr int32 RimPoints = 8;
	for (int32 Index = 0; Index < RimPoints; ++Index)
	{
		const double Angle = 2.0 * UE_DOUBLE_PI * Index / RimPoints;
		const FVector RimPoint = Centre + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0) * FMath::Max(0.0f, Radius);
		if (!IsLocationPlayable(RimPoint))
		{
			return false;
		}
	}

	return true;
}

bool FSpawnArena::FromRenderer(const ATileMapRenderer* Renderer, FSpawnArena& OutArena)
{
	OutArena = FSpawnArena();

	if (!Renderer || !FMapArrays::GetMap(Renderer->LevelToRender, OutArena.Map) || !OutArena.Map.IsValid())
	{
		return false;
	}

	OutArena.MapToWorld = Renderer->GetActorTransform();
	OutArena.CellSize = ATileMapRenderer::GetCellSize();
	return true;
}

FVector2D FEnemySpawnPlacement::SampleRingOffset(float MinRadius, float MaxRadius, float AngleAlpha, float RadiusAlpha)
{
	const double Min = FMath::Max(0.0f, MinRadius);
	const double Max = FMath::Max(Min, static_cast<double>(MaxRadius));

	// Interpolating the squared radius spreads points evenly over the ring's area instead of crowding the middle.
	const double Radius = FMath::Sqrt(FMath::Lerp(Min * Min, Max * Max, FMath::Clamp(static_cast<double>(RadiusAlpha), 0.0, 1.0)));
	const double Angle = 2.0 * UE_DOUBLE_PI * FMath::Clamp(static_cast<double>(AngleAlpha), 0.0, 1.0);
	return FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
}

bool FEnemySpawnPlacement::IsInRing(const FVector& Centre, const FVector& Candidate, float MinRadius, float MaxRadius)
{
	const double Distance = FVector::Dist2D(Centre, Candidate);
	const double Min = FMath::Max(0.0f, MinRadius);
	const double Max = FMath::Max(Min, static_cast<double>(MaxRadius));
	return Distance >= Min - UE_KINDA_SMALL_NUMBER && Distance <= Max + UE_KINDA_SMALL_NUMBER;
}
