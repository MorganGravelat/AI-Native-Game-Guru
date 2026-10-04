#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TileMapRenderer.generated.h"

class USceneComponent;
class UStaticMesh;
class UMaterialInterface;

UCLASS()
class GRANTMONEY_API ATileMapRenderer : public AActor
{
    GENERATED_BODY()

public:
    ATileMapRenderer();

    // Which registered map (see MapArrays.cpp) to build when gameplay starts.
    UPROPERTY(
        EditAnywhere,
        Category = "Map",
        meta = (ClampMin = "1")
    )
    int32 LevelToRender = 1;

    // Builds a 3D wall along the exposed edges of the playable area.
    UPROPERTY(EditAnywhere, Category = "Boundary")
    bool bGenerateBoundaries = true;

    // Wall height in Unreal units.
    UPROPERTY(
        EditAnywhere,
        Category = "Boundary",
        meta = (ClampMin = "1.0", EditCondition = "bGenerateBoundaries")
    )
    float WallHeight = 300.0f;

    // Wall thickness in Unreal units.
    UPROPERTY(
        EditAnywhere,
        Category = "Boundary",
        meta = (ClampMin = "1.0", EditCondition = "bGenerateBoundaries")
    )
    float WallThickness = 20.0f;

    // Optional wall material. If empty, the cube's default material is used.
    // A custom material must enable "Used with Instanced Static Meshes".
    UPROPERTY(
        EditAnywhere,
        Category = "Boundary",
        meta = (EditCondition = "bGenerateBoundaries")
    )
    TObjectPtr<UMaterialInterface> BoundaryMaterial;

protected:
    virtual void BeginPlay() override;

private:

    // Root component that the generated tile components attach to.
    UPROPERTY()
    USceneComponent* SceneRoot;

    // A simple flat Unreal plane used for every map tile.
    UPROPERTY()
    UStaticMesh* TilePlaneMesh;

    // Built-in cube used for boundary wall segments.
    UPROPERTY()
    UStaticMesh* BoundaryCubeMesh;

    // Base material containing the "TileTexture" parameter.
    UPROPERTY()
    UMaterialInterface* TileBaseMaterial;

    // Builds the TileGen map selected by LevelToRender.
    void BuildMap();

    // Unreal uses centimeters.
    // The built-in plane is approximately 100 x 100 Unreal units.
    static constexpr float TileSize = 100.0f;
};