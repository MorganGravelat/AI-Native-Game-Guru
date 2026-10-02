#include "TileMapRenderer.h"

// TileGen generated files
#include "Maps/Map1_1.h"
#include "Maps/Map1_1_TileAssets.h"

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

    // One instanced-mesh component will be created for each tile type.
    TMap<int32, UHierarchicalInstancedStaticMeshComponent*> TileGroups;

    int32 TilesCreated = 0;

    // TileGen maps are 64 x 64.
    // The generated data uses [x][y].
    for (int32 X = 0; X < 64; ++X)
    {
        for (int32 Y = 0; Y < 64; ++Y)
        {
            // Skip cells that TileGen says are not part of the map.
            if (Map1_1_Valid[X][Y] == 0)
            {
                continue;
            }

            const int32 TileId = Map1_1[X][Y];

            // Tile ID 0 means no ground tile.
            if (TileId == 0)
            {
                continue;
            }

            // Make sure the Tile ID exists in the generated lookup table.
            if (TileId < 0 || TileId >= Map1_1_TileAssetCount)
            {
                UE_LOG(
                    LogTemp,
                    Warning,
                    TEXT("Invalid Tile ID %d at X=%d Y=%d"),
                    TileId,
                    X,
                    Y
                );

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

                // Find the texture path TileGen generated.
                const TCHAR* TexturePath = Map1_1_TileAssets[TileId];

                if (TexturePath && TexturePath[0] != '\0')
                {
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

            TileGroup->AddInstance(TileTransform);

            ++TilesCreated;
        }
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("TileMapRenderer finished. Created %d tiles."),
        TilesCreated
    );
}