#pragma once

#include "CoreMinimal.h"
#include "MapArrays.h"

class AActor;
class USceneComponent;
class UStaticMesh;
class UMaterialInterface;

// Builds a 3D wall along every exposed edge of a TileGen map.
// A cell is playable only if Valid[X][Y] != 0 and Ground[X][Y] != 0; cells
// outside the map are never playable. One wall segment is created for each
// cardinal edge between a playable cell and a non-playable neighbour.
class GRANTMONEY_API FMapBoundaryGenerator
{
public:
    // Creates one HISM component (owned by Owner, attached to Parent) holding
    // all wall instances. Transforms are relative to Parent, matching the
    // ground tiles. BoundaryMesh is expected to be the 100x100x100 engine
    // cube. BoundaryMaterial may be null (the mesh's default material is used);
    // a custom material must enable "Used with Instanced Static Meshes".
    // If GeneratedTag is set, the component is added to ComponentTags and
    // marked transient (not saved, copied, or duplicated into PIE) so the
    // owner can find and destroy it before regenerating.
    // Returns the number of wall instances created (0 on invalid input).
    static int32 GenerateBoundary(
        AActor* Owner,
        USceneComponent* ParentComponent,
        const FGrantMoneyMapData& MapData,
        UStaticMesh* BoundaryMesh,
        UMaterialInterface* BoundaryMaterial,
        float TileSize,
        float WallHeight,
        float WallThickness,
        FName GeneratedTag = NAME_None
    );
};
