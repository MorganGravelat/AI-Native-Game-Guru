#pragma once

#include "CoreMinimal.h"
#include "EnvironmentTypes.generated.h"

// Tunable settings for the environment placer. Editable in the Details panel
// of AMapEnvironmentPopulator.
USTRUCT(BlueprintType)
struct GRANTMONEY_API FEnvironmentSettings
{
    GENERATED_BODY()

    // Same seed + same settings + same map = identical placements.
    UPROPERTY(EditAnywhere, Category = "Environment")
    int32 Seed = 12345;

    // 0 = no blocks, 1 = as many blocks as the usable space can fit.
    UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Density = 0.5f;

    // Block footprint in cells (square, FootprintCells x FootprintCells).
    UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "1", ClampMax = "16"))
    int32 FootprintCells = 1;

    // Cells of clearance kept between a block and any wall/void/map edge.
    UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0"))
    int32 BorderPaddingCells = 1;

    // Empty cells required between two blocks.
    UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0"))
    int32 MinSpacingCells = 1;

    // Reject any block that would split the walkable area into more pieces.
    UPROPERTY(EditAnywhere, Category = "Environment")
    bool bPreserveConnectivity = true;

    // Ground tile IDs that blocks MAY be placed on (e.g. grass). Any tile not
    // listed (roads, stone paths, borders) is off limits. IDs are specific to
    // one TileGen export: check the Output Log tile table for your level.
    // Empty list = nothing is placeable.
    UPROPERTY(EditAnywhere, Category = "Environment|Tiles")
    TArray<int32> PlaceableTileIds = { 1 };

    // Cells of clearance kept between a block and any NON-placeable walkable
    // tile (so blocks do not hug roads). 0 = may touch the road edge.
    UPROPERTY(EditAnywhere, Category = "Environment|Tiles", meta = (ClampMin = "0"))
    int32 PathClearanceCells = 1;
};
