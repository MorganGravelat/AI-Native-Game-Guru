#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "EnvironmentTypes.generated.h"

class AActor;
class UStaticMesh;

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

// One "what goes where" rule: which tiles may receive which asset.
// Example: Rule "Trees" = grass tile IDs + a tree mesh,
//          Rule "Rocks" = stone tile IDs + a rock mesh.
// The placement settings (tiles, density, spacing, seed...) are the same
// FEnvironmentSettings the placer already uses; they are NOT changed here.
USTRUCT(BlueprintType)
struct GRANTMONEY_API FEnvironmentAssetRule
{
    GENERATED_BODY()

    // Name shown in the Details panel list.
    UPROPERTY(EditAnywhere, Category = "Rule")
    FString Label = TEXT("New Rule");

    UPROPERTY(EditAnywhere, Category = "Rule")
    bool bEnabled = true;

    // Drag a Static Mesh from the Content Browser here (recommended: rendered
    // as cheap instances).
    UPROPERTY(EditAnywhere, Category = "Asset")
    TObjectPtr<UStaticMesh> Mesh;

    // Optional alternative: a Blueprint "prefab" Actor. If set, it is used
    // INSTEAD of Mesh and one Actor is spawned per placement (slower).
    UPROPERTY(EditAnywhere, Category = "Asset")
    TSubclassOf<AActor> ActorClass;

    // Mesh only: scale the mesh uniformly so its widest side fills
    // FootprintCells * TileSize * FitFill. Turn off to use the mesh's own size.
    UPROPERTY(EditAnywhere, Category = "Asset")
    bool bFitToFootprint = true;

    UPROPERTY(EditAnywhere, Category = "Asset", meta = (ClampMin = "0.1", ClampMax = "1.0", EditCondition = "bFitToFootprint"))
    float FitFill = 0.8f;

    // Extra uniform scale multiplier applied on top.
    UPROPERTY(EditAnywhere, Category = "Asset", meta = (ClampMin = "0.01"))
    float Scale = 1.0f;

    // Each placement gets a random scale multiplier between X (min) and Y (max).
    UPROPERTY(EditAnywhere, Category = "Asset")
    FVector2D ScaleVariation = FVector2D(1.0, 1.0);

    UPROPERTY(EditAnywhere, Category = "Asset")
    bool bRandomYaw = true;

    // Lift above the ground plane (world units).
    UPROPERTY(EditAnywhere, Category = "Asset")
    float ZOffset = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Asset")
    bool bCollision = false;

    // Tiles, density, seed, footprint, spacing, clearance, connectivity.
    UPROPERTY(EditAnywhere, Category = "Placement")
    FEnvironmentSettings Settings;
};
