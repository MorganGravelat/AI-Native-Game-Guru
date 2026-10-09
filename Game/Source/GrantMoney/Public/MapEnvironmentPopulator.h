#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnvironmentTypes.h"
#include "MapEnvironmentPopulator.generated.h"

class USceneComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;

// Thin spawner: asks FEnvironmentPlacer where blocks go, then renders them as
// instanced cubes. Place one in PlayGame at (0,0,0), next to TileMapRenderer.
UCLASS()
class GRANTMONEY_API AMapEnvironmentPopulator : public AActor
{
    GENERATED_BODY()

public:
    AMapEnvironmentPopulator();

    // Must match the TileMapRenderer's Level To Render.
    UPROPERTY(EditAnywhere, Category = "Map", meta = (ClampMin = "1"))
    int32 LevelToRender = 1;

    UPROPERTY(EditAnywhere, Category = "Environment")
    FEnvironmentSettings Settings;

    // Optional. Leave empty to use the engine's default material.
    UPROPERTY(EditAnywhere, Category = "Environment|Visual")
    TObjectPtr<UMaterialInterface> BlockMaterial;

    // World units per cell. MUST match TileMapRenderer::TileSize (100),
    // which is private there, so it is duplicated here.
    UPROPERTY(EditAnywhere, Category = "Environment|Visual", meta = (ClampMin = "1.0"))
    float TileSize = 100.0f;

    // World height of each block.
    UPROPERTY(EditAnywhere, Category = "Environment|Visual", meta = (ClampMin = "1.0"))
    float BlockHeight = 100.0f;

    // Fraction of the footprint the cube fills (leaves a visible gap).
    UPROPERTY(EditAnywhere, Category = "Environment|Visual", meta = (ClampMin = "0.1", ClampMax = "1.0"))
    float FootprintFill = 0.8f;

    // Small lift so cube bottoms never z-fight the ground planes.
    UPROPERTY(EditAnywhere, Category = "Environment|Visual")
    float ZOffset = 1.0f;

    // Clears and rebuilds all blocks from the current settings.
    void Populate();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY()
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BlockInstances;
};
