#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/CapsuleComponent.h"
#include "EnemyBase.h"
#include "EnemySpawnPlacement.h"
#include "EnemySpawner.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MapArrays.h"
#include "PlayerSpawnResolver.h"
#include "TileMapRenderer.h"
#include "Tests/AutomationCommon.h"

// These tests build the real level-1 map with the real renderer (tiles, ground slabs and walls) in an
// empty test world. They cover IsMapReady, which the run uses to wait for the map, and the spawner
// working on the real generated map and its walls.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTileMapRendererReadyTest,
    "GrantMoney.Map.TileMapRenderer.IsMapReady",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTileMapRendererReadyTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    ATileMapRenderer* Renderer = World->SpawnActor<ATileMapRenderer>();
    if (!TestNotNull(TEXT("Renderer spawned"), Renderer))
    {
        return false;
    }

    TestFalse(TEXT("Not ready before play begins: nothing is built in a game world yet"), Renderer->IsMapReady());

    if (!WorldWrapper.BeginPlayInTestWorld())
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TestTrue(TEXT("Ready once BeginPlay has built the map"), Renderer->IsMapReady());
    TestEqual(TEXT("Cell size matches the grid spacing"), ATileMapRenderer::GetCellSize(), 100.0f);

    FSpawnArena Arena;
    TestTrue(TEXT("An arena can be built from the renderer"), FSpawnArena::FromRenderer(Renderer, Arena));
    TestTrue(TEXT("The arena is valid"), Arena.IsValid());

    // The ground slabs exist: a trace straight down onto a safe cell hits them at the tile height.
    const TArray<FIntPoint> SafeCells = FPlayerSpawnResolver::FindSafeStartCells(Arena.Map);
    if (!TestFalse(TEXT("The map has safe cells"), SafeCells.IsEmpty()))
    {
        return false;
    }

    const FVector Cell = Renderer->GetCellWorldLocation(SafeCells[0]);
    FHitResult Hit;
    const bool bHit = World->LineTraceSingleByChannel(Hit, Cell + FVector(0.0, 0.0, 400.0), Cell - FVector(0.0, 0.0, 400.0), ECC_Visibility);
    TestTrue(TEXT("The ground blocks a downward trace"), bHit);
    TestEqual(TEXT("At the tile height"), static_cast<float>(Hit.ImpactPoint.Z), static_cast<float>(Arena.GetGroundZ()), 1.0f);

    // Rebuilding clears and rebuilds, and ends ready again.
    Renderer->RebuildMap();
    TestTrue(TEXT("Still ready after a rebuild"), Renderer->IsMapReady());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSpawnerOnRealMapTest,
    "GrantMoney.Map.TileMapRenderer.SpawnerOnRealMapEdge",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpawnerOnRealMapTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game) || !WorldWrapper.BeginPlayInTestWorld())
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    ATileMapRenderer* Renderer = World->SpawnActor<ATileMapRenderer>();
    FSpawnArena Arena;
    if (!TestNotNull(TEXT("Renderer spawned"), Renderer) || !TestTrue(TEXT("Renderer built the map"), Renderer->IsMapReady()) || !FSpawnArena::FromRenderer(Renderer, Arena))
    {
        return false;
    }

    // Stand the player on the playable cell closest to the map's left edge, so much of the ring is off the map.
    FIntPoint EdgeCell(Arena.Map.Width, 0);
    for (int32 X = 0; X < Arena.Map.Width && EdgeCell.X == Arena.Map.Width; ++X)
    {
        for (int32 Y = 0; Y < Arena.Map.Height; ++Y)
        {
            if (Arena.IsPlayableCell(FIntPoint(X, Y)))
            {
                EdgeCell = FIntPoint(X, Y);
                break;
            }
        }
    }

    if (!TestTrue(TEXT("Found a playable edge cell"), EdgeCell.X < Arena.Map.Width))
    {
        return false;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ACharacter* Player = World->SpawnActor<ACharacter>(ACharacter::StaticClass(), FTransform(Renderer->GetCellWorldLocation(EdgeCell) + FVector(0.0, 0.0, 100.0)), Params);
    AEnemySpawner* Spawner = World->SpawnActor<AEnemySpawner>();
    if (!TestNotNull(TEXT("Player spawned"), Player) || !TestNotNull(TEXT("Spawner spawned"), Spawner))
    {
        return false;
    }

    // No arena override: the spawner finds the renderer and its map by itself.
    Player->GetCharacterMovement()->DisableMovement();
    Spawner->bRequireActiveRun = false;
    Spawner->SetCentreActor(Player);
    Spawner->SetRandomSeed(99);
    Spawner->MinSpawnDistance = 200.0f;
    Spawner->MaxSpawnDistance = 900.0f;
    Spawner->BatchSizeAtStart = Spawner->BatchSizeAtEnd = 20;
    Spawner->MaxActiveEnemiesAtStart = Spawner->MaxActiveEnemiesAtEnd = 100;

    int32 Spawned = 0;
    for (int32 Batch = 0; Batch < 3; ++Batch)
    {
        Spawned += Spawner->SpawnBatch().Spawned;
    }

    TestTrue(TEXT("Enemies spawn on the real map"), Spawned > 0);

    for (TActorIterator<AEnemyBase> It(World); It; ++It)
    {
        float Radius = 0.0f;
        float HalfHeight = 0.0f;
        It->GetCapsuleComponent()->GetScaledCapsuleSize(Radius, HalfHeight);
        if (!Arena.IsFootprintPlayable(It->GetActorLocation(), Radius))
        {
            AddError(FString::Printf(TEXT("%s spawned at %s, off the playable cells"), *It->GetName(), *It->GetActorLocation().ToString()));
        }

        // Standing on the ground slab, not inside it or on a wall.
        TestEqual(TEXT("Standing on the tile height"), static_cast<float>(It->GetActorLocation().Z - HalfHeight), static_cast<float>(Arena.GetGroundZ()) + Spawner->GroundClearance, 1.0f);
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
