#include "MapBoundaryGenerator.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"

namespace
{
    // The engine cube is approximately 100 units on each side.
    constexpr float EngineCubeSize = 100.0f;
}

int32 FMapBoundaryGenerator::GenerateBoundary(
    AActor* Owner,
    USceneComponent* ParentComponent,
    const FGrantMoneyMapData& MapData,
    UStaticMesh* BoundaryMesh,
    UMaterialInterface* BoundaryMaterial,
    float TileSize,
    float WallHeight,
    float WallThickness)
{
    if (!Owner || !ParentComponent || !BoundaryMesh)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("MapBoundaryGenerator: Missing owner, parent component or boundary mesh.")
        );

        return 0;
    }

    if (!MapData.IsValid())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("MapBoundaryGenerator: Map data is invalid.")
        );

        return 0;
    }

    if (TileSize <= 0.0f || WallHeight <= 0.0f || WallThickness <= 0.0f)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("MapBoundaryGenerator: Invalid sizes (TileSize=%f WallHeight=%f WallThickness=%f)."),
            TileSize,
            WallHeight,
            WallThickness
        );

        return 0;
    }

    // Arrays are indexed [X][Y]. Anything outside the map is not playable.
    auto IsPlayable = [&MapData](int32 X, int32 Y) -> bool
    {
        return X >= 0 && X < MapData.Width
            && Y >= 0 && Y < MapData.Height
            && MapData.Valid[X][Y] != 0
            && MapData.Ground[X][Y] != 0;
    };

    // Wall length is TileSize plus the thickness so perpendicular walls
    // overlap at corners instead of leaving a notch.
    const float WallLength = TileSize + WallThickness;

    const FVector WallScale(
        WallThickness / EngineCubeSize,
        WallLength / EngineCubeSize,
        WallHeight / EngineCubeSize
    );

    // The cube is centered on its origin, so lift it by half its height to
    // put the bottom at Z = 0.
    const float WallCenterZ = WallHeight * 0.5f;

    // Walls facing X run along Y (no rotation); walls facing Y are yawed 90.
    struct FEdge
    {
        int32 DX;
        int32 DY;
    };

    static const FEdge Edges[] =
    {
        { -1, 0 },
        {  1, 0 },
        { 0, -1 },
        { 0,  1 },
    };

    TArray<FTransform> WallTransforms;

    for (int32 X = 0; X < MapData.Width; ++X)
    {
        for (int32 Y = 0; Y < MapData.Height; ++Y)
        {
            if (!IsPlayable(X, Y))
            {
                continue;
            }

            for (const FEdge& Edge : Edges)
            {
                if (IsPlayable(X + Edge.DX, Y + Edge.DY))
                {
                    continue;
                }

                // Same local coordinates as the ground: cell center is
                // (X * TileSize, Y * TileSize); the wall sits on the edge.
                const FVector Location(
                    X * TileSize + Edge.DX * TileSize * 0.5f,
                    Y * TileSize + Edge.DY * TileSize * 0.5f,
                    WallCenterZ
                );

                const FRotator Rotation(
                    0.0f,
                    Edge.DY != 0 ? 90.0f : 0.0f,
                    0.0f
                );

                WallTransforms.Emplace(Rotation, Location, WallScale);
            }
        }
    }

    UHierarchicalInstancedStaticMeshComponent* Walls =
        NewObject<UHierarchicalInstancedStaticMeshComponent>(
            Owner,
            TEXT("BoundaryWalls")
        );

    if (!Walls)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("MapBoundaryGenerator: Could not create the wall component.")
        );

        return 0;
    }

    Owner->AddInstanceComponent(Walls);

    Walls->SetupAttachment(ParentComponent);
    Walls->SetStaticMesh(BoundaryMesh);

    if (BoundaryMaterial)
    {
        Walls->SetMaterial(0, BoundaryMaterial);
    }

    Walls->SetCollisionProfileName(TEXT("BlockAll"));
    Walls->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    Walls->RegisterComponent();

    for (const FTransform& WallTransform : WallTransforms)
    {
        Walls->AddInstance(WallTransform);
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("MapBoundaryGenerator: Created %d wall segments."),
        WallTransforms.Num()
    );

    return WallTransforms.Num();
}
