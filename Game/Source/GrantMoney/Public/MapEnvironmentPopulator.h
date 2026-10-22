#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnvironmentTypes.h"
#include "MapEnvironmentPopulator.generated.h"

class USceneComponent;
class UHierarchicalInstancedStaticMeshComponent;
struct FGrantMoneyMapData;

// Thin spawner: for each rule, asks FEnvironmentPlacer where assets go (using
// that rule's settings), then renders the rule's asset at those cells.
// Place one in PlayGame at (0,0,0), next to TileMapRenderer.
UCLASS()
class GRANTMONEY_API AMapEnvironmentPopulator : public AActor
{
    GENERATED_BODY()

public:
    AMapEnvironmentPopulator();

    // Must match the TileMapRenderer's Level To Render.
    UPROPERTY(EditAnywhere, Category = "Map", meta = (ClampMin = "1"))
    int32 LevelToRender = 1;

    // World units per cell. MUST match TileMapRenderer::TileSize (100),
    // which is private there, so it is duplicated here.
    UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "1.0"))
    float TileSize = 100.0f;

    // Add one rule per asset type (trees on grass, rocks on stone, ...).
    UPROPERTY(EditAnywhere, Category = "Environment", meta = (TitleProperty = "Label"))
    TArray<FEnvironmentAssetRule> Rules;

    // Removes everything previously generated, then rebuilds from all rules.
    void Populate();

    // Removes generated instances and spawned actors.
    void ClearPopulated();

protected:
    virtual void BeginPlay() override;

private:
    // Returns how many assets were placed for the rule.
    int32 PopulateRule(int32 RuleIndex, const FGrantMoneyMapData& MapData);

    UPROPERTY()
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> RuleComponents;

    UPROPERTY(Transient)
    TArray<TObjectPtr<AActor>> SpawnedActors;
};
