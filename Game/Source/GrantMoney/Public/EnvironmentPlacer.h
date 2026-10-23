#pragma once

#include "CoreMinimal.h"
#include "EnvironmentTypes.h"

struct FGrantMoneyMapData;

// One placed block, in grid cells. (CellX, CellY) is the min corner of the
// footprint; the block covers [CellX, CellX+FootprintCells) x [CellY, CellY+FootprintCells).
// Indexing follows the TileGen convention: X = column (outer), Y = row (inner).
struct FEnvironmentPlacement
{
    int32 CellX = 0;
    int32 CellY = 0;
    int32 FootprintCells = 1;
};

// Pure, deterministic placement logic. No UObjects, no spawning, so it can be
// unit-tested with synthetic maps.
class GRANTMONEY_API FEnvironmentPlacer
{
public:
    // Fills OutPlacements. Returns false only if the map view is invalid.
    // Density 0 returns true with an empty list.
    static bool Populate(
        const FGrantMoneyMapData& Map,
        const FEnvironmentSettings& Settings,
        TArray<FEnvironmentPlacement>& OutPlacements);
};
