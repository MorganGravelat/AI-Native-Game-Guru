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

    // Which registered map (see MapArrays.cpp) to build. Previewed in the
    // editor viewport at edit time and rebuilt when gameplay starts.
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

#if WITH_EDITORONLY_DATA
    // Draws the map in the editor viewport while editing the level.
    // The preview is never saved; gameplay always builds a fresh copy.
    UPROPERTY(EditAnywhere, Category = "Map")
    bool bPreviewInEditor = true;
#endif

    // Clears and rebuilds the generated tiles and walls.
    // Shows up as a "Rebuild Map" button in the Details panel.
    UFUNCTION(CallInEditor, Category = "Map")
    void RebuildMap();

    virtual void OnConstruction(const FTransform& Transform) override;

    // ComponentTags entry on every generated component, so they can be found
    // and cleared again (also on a PIE copy of the actor).
    static const FName GeneratedComponentTag;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent
    ) override;

    virtual void PostEditUndo() override;
#endif

protected:
    virtual void BeginPlay() override;

private:

    // Root component that the generated tile components attach to.
    UPROPERTY()
    TObjectPtr<USceneComponent> SceneRoot;

    // A simple flat Unreal plane used for every map tile.
    UPROPERTY()
    TObjectPtr<UStaticMesh> TilePlaneMesh;

    // Built-in cube used for boundary wall segments.
    UPROPERTY()
    TObjectPtr<UStaticMesh> BoundaryCubeMesh;

    // Base material containing the "TileTexture" parameter.
    UPROPERTY()
    TObjectPtr<UMaterialInterface> TileBaseMaterial;

    // Builds the TileGen map selected by LevelToRender.
    // Expects a clean actor; RebuildMap clears first.
    void BuildMap();

    // Destroys every component that BuildMap created.
    void ClearGeneratedComponents();

#if WITH_EDITOR
    // True for editor (non-game) worlds where the preview should be drawn.
    bool IsEditorPreviewWorld() const;

    // Set by OnConstruction so PostEditChangeProperty does not rebuild twice.
    bool bConstructedDuringEdit = false;
#endif

    // Unreal uses centimeters.
    // The built-in plane is approximately 100 x 100 Unreal units.
    static constexpr float TileSize = 100.0f;
};