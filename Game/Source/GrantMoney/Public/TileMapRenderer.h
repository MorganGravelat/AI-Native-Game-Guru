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

protected:
    virtual void BeginPlay() override;

private:

    // Root component that the generated tile components attach to.
    UPROPERTY()
    USceneComponent* SceneRoot;

    // A simple flat Unreal plane used for every map tile.
    UPROPERTY()
    UStaticMesh* TilePlaneMesh;

    // Base material containing the "TileTexture" parameter.
    UPROPERTY()
    UMaterialInterface* TileBaseMaterial;

    // Builds the 64x64 TileGen map.
    void BuildMap();

    // Unreal uses centimeters.
    // The built-in plane is approximately 100 x 100 Unreal units.
    static constexpr float TileSize = 100.0f;
};