#pragma once

#include "CoreMinimal.h"
#include "MapArrays.h"

// Picks player start cells from a TileGen map. Works on any registered map, so
// new maps need no spawn changes.
// A cell is playable only if Valid[X][Y] != 0 and Ground[X][Y] != 0 (the same
// rule as FMapBoundaryGenerator). A cell is a safe start when it and all eight
// neighbours are playable, which keeps the player at least one tile away from
// every boundary wall.
class GRANTMONEY_API FPlayerSpawnResolver
{
public:
    // Returns every safe start cell, best first: closest to the centre of the
    // playable area, ties broken by grid order so the result is deterministic.
    // Returns an empty array for invalid data or a map with no safe cell.
    static TArray<FIntPoint> FindSafeStartCells(const FGrantMoneyMapData& MapData);
};
