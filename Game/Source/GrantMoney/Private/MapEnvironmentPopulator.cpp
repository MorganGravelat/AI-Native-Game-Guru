#include "MapEnvironmentPopulator.h"

#include "EnvironmentPlacer.h"
#include "MapArrays.h"

#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Math/RandomStream.h"

AMapEnvironmentPopulator::AMapEnvironmentPopulator()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;
}

void AMapEnvironmentPopulator::BeginPlay()
{
    Super::BeginPlay();
    Populate();
}

void AMapEnvironmentPopulator::ClearPopulated()
{
    for (const TObjectPtr<UHierarchicalInstancedStaticMeshComponent>& Component : RuleComponents)
    {
        if (Component)
        {
            Component->DestroyComponent();
        }
    }

    RuleComponents.Reset();

    for (const TObjectPtr<AActor>& SpawnedActor : SpawnedActors)
    {
        if (IsValid(SpawnedActor))
        {
            SpawnedActor->Destroy();
        }
    }

    SpawnedActors.Reset();
}

void AMapEnvironmentPopulator::Populate()
{
    ClearPopulated();

    FGrantMoneyMapData MapData;

    if (!FMapArrays::GetMap(LevelToRender, MapData))
    {
        UE_LOG(LogTemp, Error,
            TEXT("MapEnvironmentPopulator: level %d is not registered in MapArrays."), LevelToRender);
        return;
    }

    // Print this level's tile table, and which rules use each tile, so
    // Placeable Tile Ids can be chosen.
    for (int32 TileId = 1; TileId < 64; ++TileId)
    {
        const TCHAR* Path = FMapArrays::GetTileAsset(LevelToRender, TileId);

        if (!Path)
        {
            continue;
        }

        FString Users;
        int32 UserCount = 0;

        for (int32 i = 0; i < Rules.Num(); ++i)
        {
            if (Rules[i].bEnabled && Rules[i].Settings.PlaceableTileIds.Contains(TileId))
            {
                Users += (UserCount > 0 ? TEXT(", ") : TEXT("")) + Rules[i].Label;
                ++UserCount;
            }
        }

        UE_LOG(LogTemp, Warning, TEXT("MapEnvironmentPopulator: Level %d Tile ID %d = %s [used by: %s]"),
            LevelToRender, TileId, Path, UserCount > 0 ? *Users : TEXT("no rule"));

        if (UserCount > 1)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("MapEnvironmentPopulator: Tile ID %d is used by several rules (%s). Their assets may overlap."),
                TileId, *Users);
        }
    }

    if (Rules.Num() == 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("MapEnvironmentPopulator: no Rules configured. Add a rule in the Details panel."));
        return;
    }

    int32 Total = 0;

    for (int32 i = 0; i < Rules.Num(); ++i)
    {
        Total += PopulateRule(i, MapData);
    }

    UE_LOG(LogTemp, Warning, TEXT("MapEnvironmentPopulator finished. Placed %d assets across %d rules (Level %d)."),
        Total, Rules.Num(), LevelToRender);
}

int32 AMapEnvironmentPopulator::PopulateRule(int32 RuleIndex, const FGrantMoneyMapData& MapData)
{
    const FEnvironmentAssetRule& Rule = Rules[RuleIndex];

    if (!Rule.bEnabled)
    {
        return 0;
    }

    UStaticMesh* Mesh = Rule.Mesh;
    const bool bUseActor = (Rule.ActorClass != nullptr);

    if (!bUseActor && !Mesh)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("MapEnvironmentPopulator: rule '%s' has no Mesh or Actor Class. Skipped."), *Rule.Label);
        return 0;
    }

    if (Rule.Settings.PlaceableTileIds.Num() == 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("MapEnvironmentPopulator: rule '%s' has no Placeable Tile Ids, so nothing will be placed."),
            *Rule.Label);
        return 0;
    }

    // Placement logic is unchanged: where assets may go is decided entirely
    // by FEnvironmentPlacer using this rule's settings.
    TArray<FEnvironmentPlacement> Placements;

    if (!FEnvironmentPlacer::Populate(MapData, Rule.Settings, Placements))
    {
        UE_LOG(LogTemp, Error, TEXT("MapEnvironmentPopulator: rule '%s': map data was invalid."), *Rule.Label);
        return 0;
    }

    // Visual-only randomness (yaw, scale) uses its own stream so it can never
    // influence placement. Both values are always drawn, in a fixed order, so
    // toggling bRandomYaw does not change the scale sequence.
    FRandomStream Visual(static_cast<int32>(static_cast<uint32>(Rule.Settings.Seed) ^ 0x9E3779B9u));

    const float ScaleLo = FMath::Min(Rule.ScaleVariation.X, Rule.ScaleVariation.Y);
    const float ScaleHi = FMath::Max(Rule.ScaleVariation.X, Rule.ScaleVariation.Y);

    // Mesh bounds are used to center the mesh on its cell and sit it on the
    // ground no matter where its pivot is.
    FBoxSphereBounds Bounds(ForceInit);

    if (Mesh)
    {
        Bounds = Mesh->GetBounds();
    }

    TArray<FTransform> Transforms;
    Transforms.Reserve(Placements.Num());

    for (const FEnvironmentPlacement& P : Placements)
    {
        const float YawRoll = Visual.FRand();
        const float ScaleRoll = Visual.FRand();

        const float Yaw = Rule.bRandomYaw ? YawRoll * 360.0f : 0.0f;
        float Uniform = Rule.Scale * FMath::Lerp(ScaleLo, ScaleHi, ScaleRoll);

        // TileMapRenderer puts cell (X,Y)'s CENTER at (X*TileSize, Y*TileSize),
        // so a multi-cell footprint's center is offset by (Footprint-1)/2 cells.
        const float CenterX = (P.CellX + (P.FootprintCells - 1) * 0.5f) * TileSize;
        const float CenterY = (P.CellY + (P.FootprintCells - 1) * 0.5f) * TileSize;

        const FRotator Rotation(0.0f, Yaw, 0.0f);
        FVector Location(CenterX, CenterY, Rule.ZOffset);

        if (!bUseActor && Mesh)
        {
            if (Rule.bFitToFootprint)
            {
                const float MeshSize = FMath::Max(Bounds.BoxExtent.X, Bounds.BoxExtent.Y) * 2.0f;

                if (MeshSize > KINDA_SMALL_NUMBER)
                {
                    Uniform *= (P.FootprintCells * TileSize * Rule.FitFill) / MeshSize;
                }
            }

            // Move the bounds center onto the cell center and the bounds
            // bottom onto the ground.
            const FVector Offset(-Bounds.Origin.X, -Bounds.Origin.Y, -(Bounds.Origin.Z - Bounds.BoxExtent.Z));
            Location += Rotation.RotateVector(Offset * Uniform);
        }

        Transforms.Emplace(Rotation, Location, FVector(Uniform));
    }

    if (Transforms.Num() == 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("MapEnvironmentPopulator: rule '%s' placed 0 assets (Seed %d, Density %.2f). Check its Placeable Tile Ids."),
            *Rule.Label, Rule.Settings.Seed, Rule.Settings.Density);
        return 0;
    }

    if (bUseActor)
    {
        UWorld* World = GetWorld();

        if (!World)
        {
            return 0;
        }

        FActorSpawnParameters Params;
        Params.Owner = this;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        for (const FTransform& Local : Transforms)
        {
            const FTransform WorldTransform = Local * GetActorTransform();

            if (AActor* Spawned = World->SpawnActor<AActor>(Rule.ActorClass, WorldTransform, Params))
            {
                SpawnedActors.Add(Spawned);
            }
        }
    }
    else
    {
        UHierarchicalInstancedStaticMeshComponent* Component =
            NewObject<UHierarchicalInstancedStaticMeshComponent>(this);

        AddInstanceComponent(Component);
        Component->SetupAttachment(SceneRoot);
        Component->SetStaticMesh(Mesh);

        if (Rule.bCollision)
        {
            Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Component->SetCollisionProfileName(TEXT("BlockAll"));
        }
        else
        {
            Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }

        Component->RegisterComponent();
        Component->AddInstances(Transforms, false);

        RuleComponents.Add(Component);
    }

    UE_LOG(LogTemp, Warning, TEXT("MapEnvironmentPopulator: rule '%s' placed %d assets (Seed %d, Density %.2f)."),
        *Rule.Label, Transforms.Num(), Rule.Settings.Seed, Rule.Settings.Density);

    return Transforms.Num();
}
