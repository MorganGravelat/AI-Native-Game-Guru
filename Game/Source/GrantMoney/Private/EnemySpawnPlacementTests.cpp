#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "EnemySpawnPlacement.h"
#include "MapArrays.h"
#include "PlayerSpawnResolver.h"

namespace SpawnPlacementTest
{
    constexpr int32 N = FGrantMoneyMapData::MapSize;

    // Synthetic map: playable cells X 0..9, Y 0..9, except a hole at (5, 5) where Valid is 0 and
    // a hole at (6, 5) where Ground is 0.
    struct FTestArena
    {
        uint16 Ground[N][N];
        uint8 Valid[N][N];
        FSpawnArena Arena;

        explicit FTestArena(const FTransform& MapToWorld = FTransform::Identity)
        {
            FMemory::Memzero(Ground, sizeof(Ground));
            FMemory::Memzero(Valid, sizeof(Valid));

            for (int32 X = 0; X <= 9; ++X)
            {
                for (int32 Y = 0; Y <= 9; ++Y)
                {
                    Valid[X][Y] = 1;
                    Ground[X][Y] = 1;
                }
            }

            Valid[5][5] = 0;
            Ground[6][5] = 0;

            Arena.Map.Ground = Ground;
            Arena.Map.Valid = Valid;
            Arena.Map.Width = N;
            Arena.Map.Height = N;
            Arena.MapToWorld = MapToWorld;
            Arena.CellSize = 100.0f;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSpawnArenaCellsTest,
    "GrantMoney.Run.SpawnPlacement.ArenaCells",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpawnArenaCellsTest::RunTest(const FString& Parameters)
{
    SpawnPlacementTest::FTestArena Test;
    const FSpawnArena& Arena = Test.Arena;

    TestTrue(TEXT("Arena is valid"), Arena.IsValid());
    TestFalse(TEXT("An empty arena is not valid"), FSpawnArena().IsValid());

    TestTrue(TEXT("Corner cell is playable"), Arena.IsPlayableCell(FIntPoint(0, 0)));
    TestTrue(TEXT("Far corner is playable"), Arena.IsPlayableCell(FIntPoint(9, 9)));
    TestFalse(TEXT("Past the playable area"), Arena.IsPlayableCell(FIntPoint(10, 0)));
    TestFalse(TEXT("Negative cell"), Arena.IsPlayableCell(FIntPoint(-1, 3)));
    TestFalse(TEXT("Outside the array"), Arena.IsPlayableCell(FIntPoint(SpawnPlacementTest::N, 0)));
    TestFalse(TEXT("Valid = 0 is not playable"), Arena.IsPlayableCell(FIntPoint(5, 5)));
    TestFalse(TEXT("Ground = 0 is not playable"), Arena.IsPlayableCell(FIntPoint(6, 5)));

    // Cell (3, 4) is centred on (300, 400) and covers half a cell either side.
    FIntPoint Cell;
    TestTrue(TEXT("World to cell"), Arena.WorldToCell(FVector(300.0, 400.0, 999.0), Cell));
    TestEqual(TEXT("Cell X"), Cell.X, 3);
    TestEqual(TEXT("Cell Y"), Cell.Y, 4);
    Arena.WorldToCell(FVector(349.0, 351.0, 0.0), Cell);
    TestEqual(TEXT("Cell X near its edge"), Cell.X, 3);
    TestEqual(TEXT("Cell Y near its edge"), Cell.Y, 4);
    Arena.WorldToCell(FVector(-40.0, 0.0, 0.0), Cell);
    TestEqual(TEXT("Just outside the left edge"), Cell.X, 0);
    Arena.WorldToCell(FVector(-60.0, 0.0, 0.0), Cell);
    TestEqual(TEXT("Past the left edge"), Cell.X, -1);

    TestTrue(TEXT("Location on a playable cell"), Arena.IsLocationPlayable(FVector(120.0, 80.0, 0.0)));
    TestFalse(TEXT("Location in the Valid = 0 hole"), Arena.IsLocationPlayable(FVector(500.0, 500.0, 0.0)));
    TestFalse(TEXT("Location beyond the map"), Arena.IsLocationPlayable(FVector(5000.0, 0.0, 0.0)));

    // A moved map: cells follow the actor's transform.
    SpawnPlacementTest::FTestArena Moved(FTransform(FRotator::ZeroRotator, FVector(1000.0, 2000.0, 50.0)));
    TestTrue(TEXT("Moved: cell (0, 0) is at the actor"), Moved.Arena.IsLocationPlayable(FVector(1000.0, 2000.0, 0.0)));
    TestFalse(TEXT("Moved: the old origin is no longer playable"), Moved.Arena.IsLocationPlayable(FVector(0.0, 0.0, 0.0)));
    TestEqual(TEXT("Ground height follows the actor"), static_cast<float>(Moved.Arena.GetGroundZ()), 50.0f);

    // A rotated map: 90 degrees of yaw sends local +X to world +Y.
    SpawnPlacementTest::FTestArena Rotated(FTransform(FRotator(0.0, 90.0, 0.0), FVector::ZeroVector));
    Rotated.Arena.WorldToCell(FVector(0.0, 300.0, 0.0), Cell);
    TestEqual(TEXT("Rotated: world +Y is local X = 3"), Cell.X, 3);
    TestEqual(TEXT("Rotated: local Y = 0"), Cell.Y, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSpawnArenaFootprintTest,
    "GrantMoney.Run.SpawnPlacement.FootprintAtEdges",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpawnArenaFootprintTest::RunTest(const FString& Parameters)
{
    SpawnPlacementTest::FTestArena Test;
    const FSpawnArena& Arena = Test.Arena;

    TestTrue(TEXT("Well inside"), Arena.IsFootprintPlayable(FVector(200.0, 200.0, 0.0), 35.0f));

    // The left edge of the map is at X = -50.
    TestTrue(TEXT("Rim just inside the left edge"), Arena.IsFootprintPlayable(FVector(-10.0, 200.0, 0.0), 35.0f));
    TestFalse(TEXT("Rim sticks out past the left edge"), Arena.IsFootprintPlayable(FVector(-30.0, 200.0, 0.0), 35.0f));
    TestFalse(TEXT("Centre outside"), Arena.IsFootprintPlayable(FVector(-80.0, 200.0, 0.0), 1.0f));

    // The far edge is at X = 950.
    TestTrue(TEXT("Rim just inside the far edge"), Arena.IsFootprintPlayable(FVector(910.0, 200.0, 0.0), 35.0f));
    TestFalse(TEXT("Rim sticks out past the far edge"), Arena.IsFootprintPlayable(FVector(930.0, 200.0, 0.0), 35.0f));

    // Corner: both edges matter.
    TestFalse(TEXT("Rim sticks out of a corner"), Arena.IsFootprintPlayable(FVector(-20.0, -20.0, 0.0), 40.0f));

    // A hole next to the circle counts even though the centre is playable.
    TestTrue(TEXT("Small circle beside the hole"), Arena.IsFootprintPlayable(FVector(420.0, 500.0, 0.0), 20.0f));
    TestFalse(TEXT("Circle touching the Valid = 0 hole"), Arena.IsFootprintPlayable(FVector(440.0, 500.0, 0.0), 40.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSpawnArenaRealMapTest,
    "GrantMoney.Run.SpawnPlacement.RealMap",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpawnArenaRealMapTest::RunTest(const FString& Parameters)
{
    // Level 1 is the first registered map. This uses the real generated data, no world needed.
    FSpawnArena Arena;
    if (!TestTrue(TEXT("Level 1 is registered"), FMapArrays::GetMap(1, Arena.Map)))
    {
        return false;
    }

    TestTrue(TEXT("Arena built from a real map is valid"), Arena.IsValid());

    // The player's safe start cells have every neighbour playable, so a small circle on them fits.
    const TArray<FIntPoint> SafeCells = FPlayerSpawnResolver::FindSafeStartCells(Arena.Map);
    if (!TestFalse(TEXT("The map has safe start cells"), SafeCells.IsEmpty()))
    {
        return false;
    }

    for (const FIntPoint& Cell : SafeCells)
    {
        const FVector Centre(Cell.X * Arena.CellSize, Cell.Y * Arena.CellSize, 0.0);
        if (!Arena.IsFootprintPlayable(Centre, 35.0f))
        {
            AddError(FString::Printf(TEXT("Safe cell (%d, %d) is not playable for a 35 cm footprint"), Cell.X, Cell.Y));
            break;
        }
    }

    // Walk every playable cell: those at the map edge must reject a footprint that reaches past the cell,
    // and every cell must at least accept a point footprint at its centre.
    int32 PlayableCells = 0;
    int32 EdgeCellsRejected = 0;
    for (int32 X = 0; X < Arena.Map.Width; ++X)
    {
        for (int32 Y = 0; Y < Arena.Map.Height; ++Y)
        {
            if (!Arena.IsPlayableCell(FIntPoint(X, Y)))
            {
                continue;
            }

            ++PlayableCells;
            const FVector Centre(X * Arena.CellSize, Y * Arena.CellSize, 0.0);
            if (!Arena.IsFootprintPlayable(Centre, 1.0f))
            {
                AddError(FString::Printf(TEXT("Playable cell (%d, %d) rejects its own centre"), X, Y));
                return false;
            }

            // A footprint reaching 2 cells out cannot fit in a cell at the edge of the playable area.
            if (!Arena.IsFootprintPlayable(Centre, 2.0f * Arena.CellSize))
            {
                ++EdgeCellsRejected;
            }
        }
    }

    TestTrue(TEXT("The map has playable cells"), PlayableCells > 0);
    TestTrue(TEXT("Cells near the map edge reject a wide footprint"), EdgeCellsRejected > 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSpawnRingSamplingTest,
    "GrantMoney.Run.SpawnPlacement.RingSampling",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpawnRingSamplingTest::RunTest(const FString& Parameters)
{
    FRandomStream Random(12345);
    const float Min = 400.0f;
    const float Max = 900.0f;

    double SumSquares = 0.0;
    constexpr int32 Samples = 4000;
    for (int32 Index = 0; Index < Samples; ++Index)
    {
        const FVector2D Offset = FEnemySpawnPlacement::SampleRingOffset(Min, Max, Random.FRand(), Random.FRand());
        const double Distance = Offset.Size();
        if (Distance < Min - 0.01 || Distance > Max + 0.01)
        {
            AddError(FString::Printf(TEXT("Sample %d is %.2f from the centre, outside %.0f..%.0f"), Index, Distance, Min, Max));
            return false;
        }

        SumSquares += Distance * Distance;
    }

    // Even over the area: the mean squared distance is the average of the two squared radii.
    const double ExpectedMeanSquare = (Min * Min + Max * Max) * 0.5;
    TestTrue(TEXT("Samples are spread over the area, not crowded at one radius"), FMath::Abs(SumSquares / Samples - ExpectedMeanSquare) < ExpectedMeanSquare * 0.05);

    // Both ends of the random ranges give the ring's edges.
    TestEqual(TEXT("Radius alpha 0 is the inner radius"), static_cast<float>(FEnemySpawnPlacement::SampleRingOffset(Min, Max, 0.0f, 0.0f).Size()), Min, 0.01f);
    TestEqual(TEXT("Radius alpha 1 is the outer radius"), static_cast<float>(FEnemySpawnPlacement::SampleRingOffset(Min, Max, 0.3f, 1.0f).Size()), Max, 0.01f);

    // Bad input is repaired, never NaN.
    const FVector2D Swapped = FEnemySpawnPlacement::SampleRingOffset(800.0f, 300.0f, 0.2f, 0.9f);
    TestEqual(TEXT("A maximum below the minimum gives the minimum"), static_cast<float>(Swapped.Size()), 800.0f, 0.01f);
    const FVector2D Negative = FEnemySpawnPlacement::SampleRingOffset(-50.0f, -10.0f, 0.2f, 0.9f);
    TestEqual(TEXT("Negative radii give zero"), static_cast<float>(Negative.Size()), 0.0f, 0.01f);

    TestTrue(TEXT("IsInRing inside"), FEnemySpawnPlacement::IsInRing(FVector(100.0, 100.0, 5.0), FVector(100.0, 600.0, -80.0), 400.0f, 900.0f));
    TestFalse(TEXT("IsInRing too close"), FEnemySpawnPlacement::IsInRing(FVector::ZeroVector, FVector(300.0, 0.0, 0.0), 400.0f, 900.0f));
    TestFalse(TEXT("IsInRing too far"), FEnemySpawnPlacement::IsInRing(FVector::ZeroVector, FVector(0.0, 1000.0, 0.0), 400.0f, 900.0f));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
