#include "MapEnvironmentPopulator.h"

#include "EnvironmentPlacer.h"
#include "MapArrays.h"

#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AMapEnvironmentPopulator::AMapEnvironmentPopulator()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    BlockInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("BlockInstances"));
    BlockInstances->SetupAttachment(SceneRoot);
    BlockInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Engine cube: 100 x 100 x 100 units, pivot at its center.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(
        TEXT("/Engine/BasicShapes/Cube.Cube"));

    if (CubeFinder.Succeeded())
    {
        BlockInstances->SetStaticMesh(CubeFinder.Object);
    }
}

void AMapEnvironmentPopulator::BeginPlay()
{
    Super::BeginPlay();
    Populate();
}

void AMapEnvironmentPopulator::Populate()
{
    BlockInstances->ClearInstances();

    if (!BlockInstances->GetStaticMesh())
    {
        UE_LOG(LogTemp, Error, TEXT("MapEnvironmentPopulator: cube mesh failed to load."));
        return;
    }

    if (BlockMaterial)
    {
        BlockInstances->SetMaterial(0, BlockMaterial);
    }

    FGrantMoneyMapData MapData;

    if (!FMapArrays::GetMap(LevelToRender, MapData))
    {
        UE_LOG(LogTemp, Error,
            TEXT("MapEnvironmentPopulator: level %d is not registered in MapArrays."), LevelToRender);
        return;
    }

    // Print this level's tile table so Placeable Tile Ids can be chosen.
    for (int32 TileId = 1; TileId < 64; ++TileId)
    {
        if (const TCHAR* Path = FMapArrays::GetTileAsset(LevelToRender, TileId))
        {
            UE_LOG(LogTemp, Warning, TEXT("MapEnvironmentPopulator: Level %d Tile ID %d = %s [%s]"),
                LevelToRender, TileId, Path,
                Settings.PlaceableTileIds.Contains(TileId) ? TEXT("placeable") : TEXT("blocked"));
        }
    }

    if (Settings.PlaceableTileIds.Num() == 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("MapEnvironmentPopulator: Placeable Tile Ids is empty, so nothing will be placed."));
    }

    TArray<FEnvironmentPlacement> Placements;

    if (!FEnvironmentPlacer::Populate(MapData, Settings, Placements))
    {
        UE_LOG(LogTemp, Error, TEXT("MapEnvironmentPopulator: map data was invalid."));
        return;
    }

    TArray<FTransform> Transforms;
    Transforms.Reserve(Placements.Num());

    for (const FEnvironmentPlacement& P : Placements)
    {
        // TileMapRenderer puts cell (X,Y)'s CENTER at (X*TileSize, Y*TileSize),
        // so a multi-cell footprint's center is offset by (Footprint-1)/2 cells.
        const float CenterX = (P.CellX + (P.FootprintCells - 1) * 0.5f) * TileSize;
        const float CenterY = (P.CellY + (P.FootprintCells - 1) * 0.5f) * TileSize;

        // The cube mesh is 100 units, so scale = desired size / 100.
        const float XYScale = (P.FootprintCells * TileSize * FootprintFill) / 100.0f;
        const float ZScale = BlockHeight / 100.0f;

        Transforms.Emplace(
            FRotator::ZeroRotator,
            FVector(CenterX, CenterY, BlockHeight * 0.5f + ZOffset),
            FVector(XYScale, XYScale, ZScale));
    }

    if (Transforms.Num() > 0)
    {
        BlockInstances->AddInstances(Transforms, false);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("MapEnvironmentPopulator finished. Placed %d blocks (Level %d, Seed %d, Density %.2f)."),
        Transforms.Num(), LevelToRender, Settings.Seed, Settings.Density);
}
