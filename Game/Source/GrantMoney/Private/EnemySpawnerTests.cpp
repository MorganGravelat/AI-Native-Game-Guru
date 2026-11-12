#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnemyBase.h"
#include "EnemyEmergenceComponent.h"
#include "EnemySpawner.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "HealthComponent.h"
#include "RunController.h"
#include "SlimeEnemy.h"
#include "Tests/AutomationCommon.h"

namespace SpawnerTest
{
    constexpr int32 N = FGrantMoneyMapData::MapSize;

    // A game world with play started, a synthetic map (playable cells MinCell..MaxCell), a ground slab
    // under it (a plain blocking box, standing in for the renderer's slabs), a frozen stand-in player and
    // a spawner that uses them. The spawner ignores the run state and uses a fixed random seed.
    // Nothing ticks unless a test steps the world, so enemies stay where they spawn.
    struct FScene
    {
        FTestWorldWrapper WorldWrapper;
        uint16 Ground[N][N];
        uint8 Valid[N][N];
        FSpawnArena Arena;
        ACharacter* Player = nullptr;
        AEnemySpawner* Spawner = nullptr;

        // GroundMargin makes the slab larger than the playable area, so only the map data keeps enemies off it.
        bool Create(FAutomationTestBase& Test, const FIntPoint& MinCell, const FIntPoint& MaxCell, const FVector& PlayerLocation, float GroundMargin = 0.0f)
        {
            if (!WorldWrapper.CreateTestWorld(EWorldType::Game) || !WorldWrapper.BeginPlayInTestWorld())
            {
                WorldWrapper.ForwardErrorMessages(&Test);
                return false;
            }

            FMemory::Memzero(Ground, sizeof(Ground));
            FMemory::Memzero(Valid, sizeof(Valid));
            for (int32 X = MinCell.X; X <= MaxCell.X; ++X)
            {
                for (int32 Y = MinCell.Y; Y <= MaxCell.Y; ++Y)
                {
                    Valid[X][Y] = 1;
                    Ground[X][Y] = 1;
                }
            }

            Arena.Map.Ground = Ground;
            Arena.Map.Valid = Valid;
            Arena.Map.Width = N;
            Arena.Map.Height = N;

            const FVector Low((MinCell.X - 0.5) * 100.0, (MinCell.Y - 0.5) * 100.0, 0.0);
            const FVector High((MaxCell.X + 0.5) * 100.0, (MaxCell.Y + 0.5) * 100.0, 0.0);
            AddBox(Low - FVector(GroundMargin, GroundMargin, 0.0), High + FVector(GroundMargin, GroundMargin, 0.0), -20.0, 0.0, true);

            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            Player = World()->SpawnActor<ACharacter>(ACharacter::StaticClass(), FTransform(PlayerLocation), Params);
            Spawner = World()->SpawnActor<AEnemySpawner>();
            if (!Test.TestNotNull(TEXT("Player spawned"), Player) || !Test.TestNotNull(TEXT("Spawner spawned"), Spawner))
            {
                return false;
            }

            Player->GetCharacterMovement()->DisableMovement();
            Spawner->bRequireActiveRun = false;
            Spawner->SetArenaOverride(Arena);
            Spawner->SetCentreActor(Player);
            Spawner->SetRandomSeed(2026);
            return true;
        }

        UWorld* World() const { return WorldWrapper.GetTestWorld(); }

        // A static box from corner Low to corner High (XY) spanning heights BottomZ..TopZ.
        // BlocksTraces: blocks everything (like the ground slabs). Otherwise it blocks only pawns, so the
        // ground trace passes through it and only the capsule check can reject a position.
        AActor* AddBox(const FVector& Low, const FVector& High, double BottomZ, double TopZ, bool bBlocksTraces)
        {
            AActor* Actor = World()->SpawnActor<AActor>();
            UBoxComponent* Box = NewObject<UBoxComponent>(Actor, TEXT("Box"));
            Box->SetBoxExtent(FVector((High.X - Low.X) * 0.5, (High.Y - Low.Y) * 0.5, (TopZ - BottomZ) * 0.5));
            Actor->SetRootComponent(Box);
            if (bBlocksTraces)
            {
                Box->SetCollisionProfileName(TEXT("BlockAll"));
            }
            else
            {
                Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                Box->SetCollisionObjectType(ECC_WorldStatic);
                Box->SetCollisionResponseToAllChannels(ECR_Ignore);
                Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
            }
            Box->RegisterComponent();
            Box->SetWorldLocation(FVector((Low.X + High.X) * 0.5, (Low.Y + High.Y) * 0.5, (BottomZ + TopZ) * 0.5));
            return Actor;
        }

        TArray<AEnemyBase*> Enemies() const
        {
            TArray<AEnemyBase*> Result;
            for (TActorIterator<AEnemyBase> It(World()); It; ++It)
            {
                if (!It->IsActorBeingDestroyed())
                {
                    Result.Add(*It);
                }
            }
            return Result;
        }

        void SetPaused(bool bPaused)
        {
            World()->GetWorldSettings()->SetPauserPlayerState(bPaused ? World()->SpawnActor<APlayerState>() : nullptr);
        }
    };

    // The 20 x 20 cell map used by most tests, with the player in the middle.
    const FIntPoint MapMin(0, 0);
    const FIntPoint MapMax(19, 19);
    const FVector MapCentre(1000.0, 1000.0, 100.0);

    float GroundDistance(const AActor* A, const AActor* B)
    {
        return static_cast<float>(FVector::Dist2D(A->GetActorLocation(), B->GetActorLocation()));
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemySpawnerDefaultsTest,
    "GrantMoney.Run.EnemySpawner.Defaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemySpawnerDefaultsTest::RunTest(const FString& Parameters)
{
    const AEnemySpawner* Defaults = GetDefault<AEnemySpawner>();
    TestTrue(TEXT("Spawns slimes by default"), Defaults->EnemyClass == ASlimeEnemy::StaticClass());
    TestTrue(TEXT("Minimum distance is below the maximum"), Defaults->MinSpawnDistance < Defaults->MaxSpawnDistance);
    TestTrue(TEXT("Spawning starts slower than it ends"), Defaults->SpawnIntervalAtStart > Defaults->SpawnIntervalAtEnd);
    TestTrue(TEXT("Batches start smaller than they end"), Defaults->BatchSizeAtStart < Defaults->BatchSizeAtEnd);
    TestTrue(TEXT("The cap starts lower than it ends"), Defaults->MaxActiveEnemiesAtStart < Defaults->MaxActiveEnemiesAtEnd);
    TestTrue(TEXT("Both ends of the ramp are at least one enemy"), Defaults->BatchSizeAtStart >= 1 && Defaults->BatchSizeAtEnd >= 1);
    TestTrue(TEXT("A positive ramp time"), Defaults->DifficultyRampSeconds > 0.0f);
    TestTrue(TEXT("Waits for the run by default"), Defaults->bRequireActiveRun);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemySpawnerDifficultyRampTest,
    "GrantMoney.Run.EnemySpawner.DifficultyRampsOverTime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemySpawnerDifficultyRampTest::RunTest(const FString& Parameters)
{
    SpawnerTest::FScene Scene;
    if (!Scene.Create(*this, SpawnerTest::MapMin, SpawnerTest::MapMax, SpawnerTest::MapCentre))
    {
        return false;
    }

    AEnemySpawner* Spawner = Scene.Spawner;
    Spawner->SpawnIntervalAtStart = 4.0f;
    Spawner->SpawnIntervalAtEnd = 1.0f;
    Spawner->BatchSizeAtStart = 1;
    Spawner->BatchSizeAtEnd = 5;
    Spawner->MaxActiveEnemiesAtStart = 4;
    Spawner->MaxActiveEnemiesAtEnd = 20;
    Spawner->DifficultyRampSeconds = 100.0f;
    Spawner->bRequireActiveRun = false;

    // This test checks the ramp's raw Lerp math, so it pins the exponent to a straight line (the
    // SCRUM-37 default eases the ramp instead — see FEnemySpawnerDifficultyEasesByDefaultTest).
    Spawner->DifficultyCurveExponent = 1.0f;

    // With no run controller and no override, survival time is 0: the slow, small, low-cap start.
    TestEqual(TEXT("No run, no override: difficulty is 0"), Spawner->GetDifficultyAlpha(), 0.0f);
    TestEqual(TEXT("Slow interval at the start"), Spawner->GetCurrentSpawnInterval(), 4.0f);
    TestEqual(TEXT("Small batch at the start"), Spawner->GetCurrentBatchSize(), 1);
    TestEqual(TEXT("Low cap at the start"), Spawner->GetCurrentMaxActiveEnemies(), 4);

    // Halfway through the ramp: halfway between the two ends.
    Spawner->SetElapsedSecondsOverride(50.0f);
    TestEqual(TEXT("Halfway difficulty"), Spawner->GetDifficultyAlpha(), 0.5f, 0.001f);
    TestEqual(TEXT("Halfway interval"), Spawner->GetCurrentSpawnInterval(), 2.5f, 0.001f);
    TestEqual(TEXT("Halfway batch, rounded"), Spawner->GetCurrentBatchSize(), 3);
    TestEqual(TEXT("Halfway cap, rounded"), Spawner->GetCurrentMaxActiveEnemies(), 12);

    // Past the ramp: stays at the "AtEnd" values, never overshoots.
    Spawner->SetElapsedSecondsOverride(500.0f);
    TestEqual(TEXT("Difficulty caps at 1"), Spawner->GetDifficultyAlpha(), 1.0f);
    TestEqual(TEXT("Fast interval at the end"), Spawner->GetCurrentSpawnInterval(), 1.0f);
    TestEqual(TEXT("Large batch at the end"), Spawner->GetCurrentBatchSize(), 5);
    TestEqual(TEXT("High cap at the end"), Spawner->GetCurrentMaxActiveEnemies(), 20);

    // Spawning itself speeds up: the one due batch is small early and large late, even over the same time.
    Spawner->SetElapsedSecondsOverride(0.0f);
    Spawner->MinSpawnDistance = 100.0f;
    Spawner->MaxSpawnDistance = 800.0f;
    Spawner->AdvanceSpawning(4.0f);
    const int32 EarlyCount = Scene.Enemies().Num();
    TestEqual(TEXT("One small batch lands once the (long) early interval is reached"), EarlyCount, 1);

    Spawner->SetElapsedSecondsOverride(500.0f);
    Spawner->AdvanceSpawning(4.0f);
    const int32 LateCount = Scene.Enemies().Num() - EarlyCount;
    TestTrue(FString::Printf(TEXT("The same 4 s produces a bigger batch late (%d) than early (%d)"), LateCount, EarlyCount), LateCount > EarlyCount);

    // Going back to following the run controller.
    Spawner->SetElapsedSecondsOverride(-1.0f);
    TestEqual(TEXT("No run controller, override cleared: back to difficulty 0"), Spawner->GetDifficultyAlpha(), 0.0f);

    ARunController* Run = Scene.World()->SpawnActor<ARunController>();
    Run->StartDelaySeconds = 0.0f;
    Run->RunDurationSeconds = 100.0f;
    Spawner->SetRunController(Run);
    Run->StartRun();
    Run->AdvanceRun(50.0f);
    TestEqual(TEXT("Difficulty follows the run controller's elapsed time"), Spawner->GetDifficultyAlpha(), 0.5f, 0.001f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemySpawnerDifficultyEasesByDefaultTest,
    "GrantMoney.Run.EnemySpawner.DifficultyEasesInByDefault",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemySpawnerDifficultyEasesByDefaultTest::RunTest(const FString& Parameters)
{
    // SCRUM-37: with no DifficultyCurve asset, the default exponent must noticeably ease the ramp in,
    // not rise in a straight line, so a run begins gently and saves its busiest pressure for the end.
    SpawnerTest::FScene Scene;
    if (!Scene.Create(*this, SpawnerTest::MapMin, SpawnerTest::MapMax, SpawnerTest::MapCentre))
    {
        return false;
    }

    AEnemySpawner* Spawner = Scene.Spawner;
    Spawner->bRequireActiveRun = false;
    Spawner->DifficultyRampSeconds = 100.0f;
    TestTrue(TEXT("Eased by default (exponent above 1)"), Spawner->DifficultyCurveExponent > 1.0f);

    Spawner->SetElapsedSecondsOverride(25.0f); // a quarter of the way through the ramp
    TestTrue(TEXT("Noticeably gentler than linear a quarter of the way in"), Spawner->GetDifficultyAlpha() < 0.25f);

    Spawner->SetElapsedSecondsOverride(50.0f); // halfway through the ramp
    TestTrue(TEXT("Still gentler than linear at the halfway point"), Spawner->GetDifficultyAlpha() < 0.5f);

    Spawner->SetElapsedSecondsOverride(100.0f); // the full ramp
    TestEqual(TEXT("Still reaches exactly 1 at the end, whatever the exponent"), Spawner->GetDifficultyAlpha(), 1.0f);

    // An exponent of 1 is a plain straight line (the pre-SCRUM-37 behaviour), still available if wanted.
    Spawner->DifficultyCurveExponent = 1.0f;
    Spawner->SetElapsedSecondsOverride(50.0f);
    TestEqual(TEXT("Exponent 1 is exactly linear"), Spawner->GetDifficultyAlpha(), 0.5f, 0.001f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemySpawnerRingAndCapTest,
    "GrantMoney.Run.EnemySpawner.RingAndCap",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemySpawnerRingAndCapTest::RunTest(const FString& Parameters)
{
    SpawnerTest::FScene Scene;
    if (!Scene.Create(*this, SpawnerTest::MapMin, SpawnerTest::MapMax, SpawnerTest::MapCentre))
    {
        return false;
    }

    AEnemySpawner* Spawner = Scene.Spawner;
    Spawner->MinSpawnDistance = 300.0f;
    Spawner->MaxSpawnDistance = 700.0f;
    Spawner->BatchSizeAtStart = Spawner->BatchSizeAtEnd = 3;
    Spawner->MaxActiveEnemiesAtStart = Spawner->MaxActiveEnemiesAtEnd = 7;

    FEnemySpawnBatchResult First = Spawner->SpawnBatch();
    TestEqual(TEXT("First batch spawns 3"), First.Spawned, 3);
    TestEqual(TEXT("Nothing skipped"), First.SkippedNoPosition + First.SkippedByCap, 0);
    TestEqual(TEXT("Second batch spawns 3"), Spawner->SpawnBatch().Spawned, 3);
    TestEqual(TEXT("Counts 6 active"), Spawner->GetActiveEnemyCount(), 6);

    const FEnemySpawnBatchResult Third = Spawner->SpawnBatch();
    TestEqual(TEXT("Third batch is cut to the one free slot"), Third.Spawned, 1);
    TestEqual(TEXT("Two were skipped by the cap"), Third.SkippedByCap, 2);
    TestEqual(TEXT("At the cap"), Spawner->GetActiveEnemyCount(), 7);

    const FEnemySpawnBatchResult Fourth = Spawner->SpawnBatch();
    TestEqual(TEXT("A full spawner spawns nothing"), Fourth.Spawned, 0);
    TestEqual(TEXT("All skipped by the cap"), Fourth.SkippedByCap, 3);
    TestEqual(TEXT("Never above the cap"), Scene.Enemies().Num(), 7);

    // Every enemy is inside the ring, on the ground, facing the player's side of the map.
    for (const AEnemyBase* Enemy : Scene.Enemies())
    {
        const float Distance = SpawnerTest::GroundDistance(Enemy, Scene.Player);
        TestTrue(*FString::Printf(TEXT("%s is %.0f cm away, inside 300..700"), *Enemy->GetName(), Distance), Distance >= 299.0f && Distance <= 701.0f);
        TestTrue(TEXT("Standing on the ground, capsule bottom just above it"),
            FMath::IsNearlyEqual(static_cast<float>(Enemy->GetActorLocation().Z), Enemy->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + Spawner->GroundClearance, 0.5f));
    }

    // Bad distances are repaired: a maximum below the minimum means exactly the minimum.
    Spawner->MaxActiveEnemiesAtStart = Spawner->MaxActiveEnemiesAtEnd = 100;
    Spawner->MinSpawnDistance = 600.0f;
    Spawner->MaxSpawnDistance = 100.0f;
    Spawner->BatchSizeAtStart = Spawner->BatchSizeAtEnd = 4;
    TArray<AEnemyBase*> Before = Scene.Enemies();
    TestEqual(TEXT("Batch with inverted distances still spawns"), Spawner->SpawnBatch().Spawned, 4);
    for (const AEnemyBase* Enemy : Scene.Enemies())
    {
        if (!Before.Contains(Enemy))
        {
            TestEqual(TEXT("Inverted distances: exactly the minimum"), SpawnerTest::GroundDistance(Enemy, Scene.Player), 600.0f, 1.0f);
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemySpawnerReplacementTest,
    "GrantMoney.Run.EnemySpawner.ReplacesDeadEnemies",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemySpawnerReplacementTest::RunTest(const FString& Parameters)
{
    SpawnerTest::FScene Scene;
    if (!Scene.Create(*this, SpawnerTest::MapMin, SpawnerTest::MapMax, SpawnerTest::MapCentre))
    {
        return false;
    }

    AEnemySpawner* Spawner = Scene.Spawner;
    Spawner->MinSpawnDistance = 300.0f;
    Spawner->MaxSpawnDistance = 700.0f;
    Spawner->BatchSizeAtStart = Spawner->BatchSizeAtEnd = 5;
    Spawner->MaxActiveEnemiesAtStart = Spawner->MaxActiveEnemiesAtEnd = 5;

    TestEqual(TEXT("Filled to the cap"), Spawner->SpawnBatch().Spawned, 5);
    TestEqual(TEXT("No room"), Spawner->SpawnBatch().Spawned, 0);

    TArray<AEnemyBase*> Enemies = Scene.Enemies();
    if (!TestEqual(TEXT("Five enemies"), Enemies.Num(), 5))
    {
        return false;
    }

    // One dies and is removed at once (the slime default); one dies but stays in the world for a while.
    Enemies[0]->GetHealth()->ApplyDamage(1000.0f);
    Enemies[1]->DeathCleanupDelay = 5.0f;
    Enemies[1]->GetHealth()->ApplyDamage(1000.0f);
    TestFalse(TEXT("The second corpse is still in the world"), Enemies[1]->IsActorBeingDestroyed());
    TestEqual(TEXT("Both slots are free as soon as the enemies are dead"), Spawner->GetActiveEnemyCount(), 3);

    // The next batch replaces exactly the missing two; the third attempt hits the cap.
    const FEnemySpawnBatchResult Refill = Spawner->SpawnBatch();
    TestEqual(TEXT("Two replacements"), Refill.Spawned, 2);
    TestEqual(TEXT("The rest skipped by the cap"), Refill.SkippedByCap, 3);
    TestEqual(TEXT("Back to the cap"), Spawner->GetActiveEnemyCount(), 5);

    // An enemy removed without dying frees its slot too.
    Enemies[2]->Destroy();
    TestEqual(TEXT("A destroyed enemy frees its slot"), Spawner->GetActiveEnemyCount(), 4);
    TestEqual(TEXT("And is replaced"), Spawner->SpawnBatch().Spawned, 1);
    TestEqual(TEXT("At the cap again"), Spawner->GetActiveEnemyCount(), 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemySpawnerNoPositionTest,
    "GrantMoney.Run.EnemySpawner.SkipsWhenNoValidPosition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemySpawnerNoPositionTest::RunTest(const FString& Parameters)
{
    {
        // The whole map is 3 x 3 cells, so nothing 900 cm from the player can be on it.
        SpawnerTest::FScene Scene;
        if (!Scene.Create(*this, FIntPoint(0, 0), FIntPoint(2, 2), FVector(100.0, 100.0, 100.0)))
        {
            return false;
        }

        AEnemySpawner* Spawner = Scene.Spawner;
        Spawner->MinSpawnDistance = 900.0f;
        Spawner->MaxSpawnDistance = 1500.0f;
        Spawner->BatchSizeAtStart = Spawner->BatchSizeAtEnd = 3;
        Spawner->MaxPlacementAttempts = 5;

        const FEnemySpawnBatchResult Result = Spawner->SpawnBatch();
        TestEqual(TEXT("Nothing spawned"), Result.Spawned, 0);
        TestEqual(TEXT("Every enemy skipped for lack of a position"), Result.SkippedNoPosition, 3);
        TestEqual(TEXT("Retries are bounded: 5 attempts for each of 3 enemies"), Spawner->GetPlacementAttemptCount(), 15);
        TestEqual(TEXT("No enemies in the world"), Scene.Enemies().Num(), 0);
        TestEqual(TEXT("No active enemies"), Spawner->GetActiveEnemyCount(), 0);

        // Skipped spawns do not use up the cap, and a later valid situation spawns normally.
        Spawner->MinSpawnDistance = 50.0f;
        Spawner->MaxSpawnDistance = 150.0f;
        Spawner->MaxPlacementAttempts = 30;
        TestTrue(TEXT("Spawns once a valid ring exists"), Spawner->SpawnBatch().Spawned > 0);
    }

    {
        // No player: nothing to spawn around, and no crash.
        SpawnerTest::FScene Lonely;
        if (!Lonely.Create(*this, SpawnerTest::MapMin, SpawnerTest::MapMax, SpawnerTest::MapCentre))
        {
            return false;
        }
        Lonely.Player->Destroy();
        TestEqual(TEXT("No player, no spawns"), Lonely.Spawner->SpawnBatch().Spawned, 0);
    }

    {
        // No arena: also nothing.
        SpawnerTest::FScene NoMap;
        if (!NoMap.Create(*this, SpawnerTest::MapMin, SpawnerTest::MapMax, SpawnerTest::MapCentre))
        {
            return false;
        }
        NoMap.Spawner->SetArenaOverride(FSpawnArena());
        TestEqual(TEXT("No map, no spawns"), NoMap.Spawner->SpawnBatch().Spawned, 0);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemySpawnerMapEdgeTest,
    "GrantMoney.Run.EnemySpawner.StaysOnPlayableCellsAtMapEdges",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemySpawnerMapEdgeTest::RunTest(const FString& Parameters)
{
    // Players standing in two corners and at an edge. The ground slab extends 2000 cm past the playable
    // cells, so only the map data stops an enemy from spawning off the map.
    const FVector PlayerLocations[] = { FVector(100.0, 100.0, 100.0), FVector(1900.0, 1900.0, 100.0), FVector(0.0, 1000.0, 100.0) };

    for (const FVector& PlayerLocation : PlayerLocations)
    {
        SpawnerTest::FScene Scene;
        if (!Scene.Create(*this, SpawnerTest::MapMin, SpawnerTest::MapMax, PlayerLocation, 2000.0f))
        {
            return false;
        }

        AEnemySpawner* Spawner = Scene.Spawner;
        Spawner->MinSpawnDistance = 150.0f;
        Spawner->MaxSpawnDistance = 600.0f;
        Spawner->BatchSizeAtStart = Spawner->BatchSizeAtEnd = 20;
        Spawner->MaxActiveEnemiesAtStart = Spawner->MaxActiveEnemiesAtEnd = 200;

        int32 Spawned = 0;
        for (int32 Batch = 0; Batch < 3; ++Batch)
        {
            Spawned += Spawner->SpawnBatch().Spawned;
        }

        TestTrue(FString::Printf(TEXT("Enemies spawn with the player at %s"), *PlayerLocation.ToString()), Spawned > 20);

        for (const AEnemyBase* Enemy : Scene.Enemies())
        {
            float Radius = 0.0f;
            float HalfHeight = 0.0f;
            Enemy->GetCapsuleComponent()->GetScaledCapsuleSize(Radius, HalfHeight);
            if (!Scene.Arena.IsFootprintPlayable(Enemy->GetActorLocation(), Radius))
            {
                AddError(FString::Printf(TEXT("%s spawned at %s, whose footprint is not on playable cells"), *Enemy->GetName(), *Enemy->GetActorLocation().ToString()));
            }
        }
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemySpawnerObstacleTest,
    "GrantMoney.Run.EnemySpawner.RejectsBlockedAndRaisedGround",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemySpawnerObstacleTest::RunTest(const FString& Parameters)
{
    auto Configure = [](AEnemySpawner* Spawner)
    {
        Spawner->MinSpawnDistance = 300.0f;
        Spawner->MaxSpawnDistance = 700.0f;
        Spawner->BatchSizeAtStart = Spawner->BatchSizeAtEnd = 20;
        Spawner->MaxActiveEnemiesAtStart = Spawner->MaxActiveEnemiesAtEnd = 100;
    };

    // 1. A pawn-blocking prop over the east half (X >= 1000). The ground trace passes through it, so
    //    only the capsule clearance check can keep enemies out of it.
    {
        SpawnerTest::FScene Scene;
        if (!Scene.Create(*this, SpawnerTest::MapMin, SpawnerTest::MapMax, SpawnerTest::MapCentre))
        {
            return false;
        }

        Scene.AddBox(FVector(1000.0, -50.0, 0.0), FVector(1950.0, 1950.0, 0.0), 0.0, 200.0, false);
        Configure(Scene.Spawner);
        TestTrue(TEXT("Still spawns on the free half"), Scene.Spawner->SpawnBatch().Spawned > 5);

        for (const AEnemyBase* Enemy : Scene.Enemies())
        {
            TestTrue(*FString::Printf(TEXT("Enemy at X=%.0f is clear of the prop"), Enemy->GetActorLocation().X), Enemy->GetActorLocation().X < 1000.0 - 30.0);
        }
    }

    // 2. A raised floor over the east half, 60 cm above the tile height. The trace finds it, and it is too far from the tile height.
    {
        SpawnerTest::FScene Scene;
        if (!Scene.Create(*this, SpawnerTest::MapMin, SpawnerTest::MapMax, SpawnerTest::MapCentre))
        {
            return false;
        }

        Scene.AddBox(FVector(1000.0, -50.0, 0.0), FVector(1950.0, 1950.0, 0.0), -100.0, 60.0, true);
        Configure(Scene.Spawner);
        TestTrue(TEXT("Still spawns on the normal half"), Scene.Spawner->SpawnBatch().Spawned > 5);

        for (const AEnemyBase* Enemy : Scene.Enemies())
        {
            TestTrue(*FString::Printf(TEXT("Enemy at X=%.0f is not on the raised floor"), Enemy->GetActorLocation().X), Enemy->GetActorLocation().X < 1000.0);
        }
    }

    // 3. No ground under the map at all: nothing can spawn.
    {
        SpawnerTest::FScene Scene;
        if (!Scene.Create(*this, SpawnerTest::MapMin, SpawnerTest::MapMax, SpawnerTest::MapCentre))
        {
            return false;
        }

        TArray<AActor*> Boxes;
        for (TActorIterator<AActor> It(Scene.World()); It; ++It)
        {
            if (It->FindComponentByClass<UBoxComponent>())
            {
                Boxes.Add(*It);
            }
        }

        for (AActor* Box : Boxes)
        {
            Box->Destroy();
        }

        Configure(Scene.Spawner);
        TestEqual(TEXT("No ground, no spawns"), Scene.Spawner->SpawnBatch().Spawned, 0);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemySpawnerRunGatingTest,
    "GrantMoney.Run.EnemySpawner.FollowsRunStateAndPause",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemySpawnerRunGatingTest::RunTest(const FString& Parameters)
{
    SpawnerTest::FScene Scene;
    if (!Scene.Create(*this, SpawnerTest::MapMin, SpawnerTest::MapMax, SpawnerTest::MapCentre))
    {
        return false;
    }

    AEnemySpawner* Spawner = Scene.Spawner;
    Spawner->MinSpawnDistance = 300.0f;
    Spawner->MaxSpawnDistance = 700.0f;
    Spawner->SpawnIntervalAtStart = Spawner->SpawnIntervalAtEnd = 2.0f;
    Spawner->BatchSizeAtStart = Spawner->BatchSizeAtEnd = 3;
    Spawner->MaxActiveEnemiesAtStart = Spawner->MaxActiveEnemiesAtEnd = 100;
    Spawner->bRequireActiveRun = true;

    // No run controller: idle.
    Spawner->AdvanceSpawning(10.0f);
    TestEqual(TEXT("Nothing spawns without a run controller"), Scene.Enemies().Num(), 0);

    ARunController* Run = Scene.World()->SpawnActor<ARunController>();
    Run->RunDurationSeconds = 10.0f;
    Run->StartDelaySeconds = 0.0f;
    Spawner->SetRunController(Run);

    Spawner->AdvanceSpawning(10.0f);
    TestEqual(TEXT("Nothing spawns while the run is Starting"), Scene.Enemies().Num(), 0);

    Run->StartRun();
    Spawner->AdvanceSpawning(1.9f);
    TestEqual(TEXT("Nothing before the first interval is up"), Scene.Enemies().Num(), 0);
    Spawner->AdvanceSpawning(0.2f);
    TestEqual(TEXT("The first batch spawns when the interval is up"), Scene.Enemies().Num(), 3);
    Spawner->AdvanceSpawning(1.0f);
    TestEqual(TEXT("Nothing between batches"), Scene.Enemies().Num(), 3);
    Spawner->AdvanceSpawning(1.1f);
    TestEqual(TEXT("The second batch"), Scene.Enemies().Num(), 6);

    // A long frame produces one batch, not a burst of catch-up batches.
    Spawner->AdvanceSpawning(30.0f);
    TestEqual(TEXT("One batch for a long frame"), Scene.Enemies().Num(), 9);

    // Pausing the game freezes spawning, even if the spawner is ticked anyway.
    Scene.SetPaused(true);
    Spawner->Tick(30.0f);
    Spawner->Tick(30.0f);
    TestEqual(TEXT("Nothing spawns while paused"), Scene.Enemies().Num(), 9);
    Scene.SetPaused(false);
    Spawner->Tick(2.1f);
    TestEqual(TEXT("Spawning resumes after the pause"), Scene.Enemies().Num(), 12);

    // BossPhase: normal spawning stops.
    Run->AdvanceRun(100.0f);
    TestTrue(TEXT("BossPhase"), Run->GetState() == EGrantMoneyRunState::BossPhase);
    Spawner->AdvanceSpawning(100.0f);
    Spawner->AdvanceSpawning(100.0f);
    TestEqual(TEXT("No normal spawning in BossPhase"), Scene.Enemies().Num(), 12);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemySpawnerEmergenceTest,
    "GrantMoney.Run.EnemySpawner.SlimeEmerges",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemySpawnerEmergenceTest::RunTest(const FString& Parameters)
{
    SpawnerTest::FScene Scene;
    if (!Scene.Create(*this, SpawnerTest::MapMin, SpawnerTest::MapMax, SpawnerTest::MapCentre))
    {
        return false;
    }

    AEnemySpawner* Spawner = Scene.Spawner;
    Spawner->MinSpawnDistance = 300.0f;
    Spawner->MaxSpawnDistance = 700.0f;

    AEnemyBase* Enemy = Spawner->TrySpawnEnemy();
    if (!TestNotNull(TEXT("A slime spawned"), Enemy) || !TestTrue(TEXT("It is a slime"), Enemy->IsA<ASlimeEnemy>()))
    {
        return false;
    }

    UStaticMeshComponent* Mesh = Enemy->FindComponentByClass<UStaticMeshComponent>();
    if (!TestNotNull(TEXT("The slime has its placeholder mesh"), Mesh))
    {
        return false;
    }

    const float HalfHeight = Enemy->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    auto MeshBottomAboveGround = [&]() -> float
    {
        Mesh->UpdateBounds();
        return static_cast<float>(Mesh->Bounds.Origin.Z - Mesh->Bounds.BoxExtent.Z);
    };

    // Just spawned: flat, near the ground, and inactive.
    TestTrue(TEXT("Emerging"), Enemy->IsEmerging());
    TestFalse(TEXT("Not targetable while emerging"), Enemy->CanBeTargeted());
    TestTrue(TEXT("The collision capsule is already in place"), Enemy->GetCapsuleComponent()->IsCollisionEnabled());
    TestTrue(TEXT("The visual starts flattened"), Mesh->GetRelativeScale3D().Z < 0.1);
    TestEqual(TEXT("Mesh bottom is at the ground when flat"), MeshBottomAboveGround(), 2.0f, 1.0f);

    // Partway through: still emerging, still on the ground, taller.
    const double FlatHeight = Mesh->GetRelativeScale3D().Z;
    UEnemyEmergenceComponent* Emergence = Enemy->GetEmergence();
    Emergence->AdvanceEmergence(Emergence->Duration * 0.5f);
    TestTrue(TEXT("Still emerging halfway"), Enemy->IsEmerging());
    TestFalse(TEXT("Still not targetable halfway"), Enemy->CanBeTargeted());
    TestTrue(TEXT("Taller halfway"), Mesh->GetRelativeScale3D().Z > FlatHeight);
    TestEqual(TEXT("Mesh bottom stays at the ground"), MeshBottomAboveGround(), 2.0f, 1.0f);

    // Done: normal shape, and the enemy is live.
    Emergence->AdvanceEmergence(Emergence->Duration);
    TestFalse(TEXT("No longer emerging"), Enemy->IsEmerging());
    TestTrue(TEXT("Targetable after emerging"), Enemy->CanBeTargeted());
    TestTrue(TEXT("Normal scale"), Mesh->GetRelativeScale3D().Equals(FVector(0.6), 0.001));
    TestEqual(TEXT("Mesh bottom at the ground at the end"), MeshBottomAboveGround(), 2.0f, 1.0f);
    TestTrue(TEXT("Capsule centre is one half-height above the ground"), FMath::IsNearlyEqual(static_cast<float>(Enemy->GetActorLocation().Z), HalfHeight + Spawner->GroundClearance, 0.5f));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
