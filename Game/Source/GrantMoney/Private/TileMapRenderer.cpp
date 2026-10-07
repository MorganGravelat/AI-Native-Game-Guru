#include "TileMapRenderer.h"

#include "MapArrays.h"
#include "MapBoundaryGenerator.h"

// Unreal components/assets
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ATileMapRenderer::ATileMapRenderer()
{
    // Our tile map does not need to execute code every frame.
    PrimaryActorTick.bCanEverTick = false;

    // Create a root component for this Actor.
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    // Load Unreal's built-in 100x100 plane mesh.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshFinder(
        TEXT("/Engine/BasicShapes/Plane.Plane")
    );

    if (PlaneMeshFinder.Succeeded())
    {
        TilePlaneMesh = PlaneMeshFinder.Object;
    }

    // Load Unreal's built-in 100x100x100 cube mesh for boundary walls.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(
        TEXT("/Engine/BasicShapes/Cube.Cube")
    );

    if (CubeMeshFinder.Succeeded())
    {
        BoundaryCubeMesh = CubeMeshFinder.Object;
    }

    // Load the tile material we created in Content/Materials.
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(
        TEXT("/Game/Materials/M_Tile.M_Tile")
    );

    if (MaterialFinder.Succeeded())
    {
        TileBaseMaterial = MaterialFinder.Object;
    }
}

void ATileMapRenderer::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("TileMapRenderer starting..."));

    BuildMap();
}

void ATileMapRenderer::BuildMap()
{
    if (!TilePlaneMesh)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("TileMapRenderer: Could not load the plane mesh.")
        );

        return;
    }

    if (!TileBaseMaterial)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("TileMapRenderer: Could not load M_Tile.")
        );

        return;
    }

    // Ask MapArrays for the selected level.
    FGrantMoneyMapData MapData;

    if (!FMapArrays::GetMap(LevelToRender, MapData))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("TileMapRenderer: Failed to load level %d. It is not registered in MapArrays."),
            LevelToRender
        );

        return;
    }

    if (!MapData.IsValid())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("TileMapRenderer: MapArrays returned invalid data for level %d (Width=%d Height=%d)."),
            LevelToRender,
            MapData.Width,
            MapData.Height
        );

        return;
    }

    // One instanced-mesh component will be created for each tile type.
    TMap<int32, UHierarchicalInstancedStaticMeshComponent*> TileGroups;

    // Instance transforms are collected per tile type and added in one batch,
    // so each HISM component builds its spatial tree once instead of per tile.
    TMap<int32, TArray<FTransform>> PendingTransforms;

    // Tile IDs with no usable asset path; logged once per ID, not per cell.
    TSet<int32> InvalidTileIds;

    int32 TilesCreated = 0;

    // The generated data uses [x][y].
    for (int32 X = 0; X < MapData.Width; ++X)
    {
        for (int32 Y = 0; Y < MapData.Height; ++Y)
        {
            // Skip cells that TileGen says are not part of the map.
            if (MapData.Valid[X][Y] == 0)
            {
                continue;
            }

            const int32 TileId = MapData.Ground[X][Y];

            // Tile ID 0 means no ground tile.
            if (TileId == 0)
            {
                continue;
            }

            if (InvalidTileIds.Contains(TileId))
            {
                continue;
            }

            UHierarchicalInstancedStaticMeshComponent* TileGroup = nullptr;

            // Have we already created a group for this tile type?
            if (UHierarchicalInstancedStaticMeshComponent** ExistingGroup =
                TileGroups.Find(TileId))
            {
                TileGroup = *ExistingGroup;
            }
            else
            {
                // Find the texture path TileGen generated for this level.
                const TCHAR* TexturePath =
                    FMapArrays::GetTileAsset(LevelToRender, TileId);

                if (!TexturePath)
                {
                    UE_LOG(
                        LogTemp,
                        Warning,
                        TEXT("Level %d: no asset for Tile ID %d (first seen at X=%d Y=%d). Skipping all tiles with this ID."),
                        LevelToRender,
                        TileId,
                        X,
                        Y
                    );

                    InvalidTileIds.Add(TileId);
                    continue;
                }

                // First time we have encountered this Tile ID.
                // Create an instanced mesh component for it.
                const FName ComponentName(
                    *FString::Printf(TEXT("TileGroup_%d"), TileId)
                );

                TileGroup =
                    NewObject<UHierarchicalInstancedStaticMeshComponent>(
                        this,
                        ComponentName
                    );

                AddInstanceComponent(TileGroup);

                TileGroup->SetupAttachment(SceneRoot);
                TileGroup->SetStaticMesh(TilePlaneMesh);

                // Ground tiles do not need collision yet.
                TileGroup->SetCollisionEnabled(ECollisionEnabled::NoCollision);

                TileGroup->RegisterComponent();

                UTexture2D* TileTexture =
                    LoadObject<UTexture2D>(
                        nullptr,
                        TexturePath
                    );

                if (TileTexture)
                {
                    UMaterialInstanceDynamic* DynamicMaterial =
                        UMaterialInstanceDynamic::Create(
                            TileBaseMaterial,
                            this
                        );

                    if (DynamicMaterial)
                    {
                        DynamicMaterial->SetTextureParameterValue(
                            TEXT("TileTexture"),
                            TileTexture
                        );

                        TileGroup->SetMaterial(
                            0,
                            DynamicMaterial
                        );
                    }
                }
                else
                {
                    UE_LOG(
                        LogTemp,
                        Warning,
                        TEXT("Could not load texture for Tile ID %d: %s"),
                        TileId,
                        TexturePath
                    );
                }

                TileGroups.Add(TileId, TileGroup);
            }

            // Convert TileGen's grid position into an Unreal world position.
            const FVector TileLocation(
                X * TileSize,
                Y * TileSize,
                0.0f
            );

            const FTransform TileTransform(
                FRotator::ZeroRotator,
                TileLocation,
                FVector::OneVector
            );

            PendingTransforms.FindOrAdd(TileId).Add(TileTransform);

            ++TilesCreated;
        }
    }

    for (const TPair<int32, TArray<FTransform>>& Pending : PendingTransforms)
    {
        if (UHierarchicalInstancedStaticMeshComponent* const* Group =
            TileGroups.Find(Pending.Key))
        {
            (*Group)->AddInstances(Pending.Value, false);
        }
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("TileMapRenderer finished. Created %d tiles."),
        TilesCreated
    );

    if (bGenerateBoundaries)
    {
        FMapBoundaryGenerator::GenerateBoundary(
            this,
            SceneRoot,
            MapData,
            BoundaryCubeMesh,
            BoundaryMaterial,
            TileSize,
            WallHeight,
            WallThickness
        );
    }
}