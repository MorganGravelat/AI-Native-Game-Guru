#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "EnvironmentPlacer.h"
#include "MapArrays.h"

namespace EnvTest
{
    constexpr int32 N = 64;

    // Synthetic map: valid square 6..57 with a wall slab at x=28..31, y=6..40.
    struct FTestMap
    {
        uint16 Ground[N][N];
        uint8 Valid[N][N];
        FGrantMoneyMapData View;

        FTestMap()
        {
            FMemory::Memzero(Ground, sizeof(Ground));
            FMemory::Memzero(Valid, sizeof(Valid));

            for (int32 X = 6; X <= 57; ++X)
            {
                for (int32 Y = 6; Y <= 57; ++Y)
                {
                    const bool bWall = (X >= 28 && X <= 31 && Y >= 6 && Y <= 40);
                    Valid[X][Y] = bWall ? 0 : 1;
                    Ground[X][Y] = bWall ? 0 : ((Y >= 44 && Y <= 46) ? 2 : 1);
                }
            }

            View.Ground = Ground;
            View.Valid = Valid;
            View.Width = N;
            View.Height = N;
        }
    };

    bool IsBaseCell(const FTestMap& M, int32 X, int32 Y)
    {
        return X >= 0 && Y >= 0 && X < N && Y < N && M.Valid[X][Y] != 0 && M.Ground[X][Y] != 0;
    }

    // Returns an empty string when every placement is legal.
    FString CheckLegal(
        const FTestMap& M,
        const FEnvironmentSettings& S,
        const TArray<FEnvironmentPlacement>& Placements)
    {
        TArray<uint8> Occ;
        Occ.Init(0, N * N);

        for (const FEnvironmentPlacement& P : Placements)
        {
            const int32 F = P.FootprintCells;

            // Bounds + valid cells + padding ring.
            for (int32 X = P.CellX - S.BorderPaddingCells; X < P.CellX + F + S.BorderPaddingCells; ++X)
            {
                for (int32 Y = P.CellY - S.BorderPaddingCells; Y < P.CellY + F + S.BorderPaddingCells; ++Y)
                {
                    if (!IsBaseCell(M, X, Y))
                    {
                        return FString::Printf(TEXT("Block at (%d,%d) touches wall/void/edge at (%d,%d)"),
                            P.CellX, P.CellY, X, Y);
                    }
                }
            }

            // Footprint must sit on placeable tiles only.
            for (int32 X = P.CellX; X < P.CellX + F; ++X)
            {
                for (int32 Y = P.CellY; Y < P.CellY + F; ++Y)
                {
                    if (!S.PlaceableTileIds.Contains(static_cast<int32>(M.Ground[X][Y])))
                    {
                        return FString::Printf(TEXT("Block at (%d,%d) sits on unplaceable tile at (%d,%d)"),
                            P.CellX, P.CellY, X, Y);
                    }
                }
            }

            // Clearance: no walkable-but-unplaceable tile within PathClearanceCells.
            for (int32 X = P.CellX - S.PathClearanceCells; X < P.CellX + F + S.PathClearanceCells; ++X)
            {
                for (int32 Y = P.CellY - S.PathClearanceCells; Y < P.CellY + F + S.PathClearanceCells; ++Y)
                {
                    if (IsBaseCell(M, X, Y) && !S.PlaceableTileIds.Contains(static_cast<int32>(M.Ground[X][Y])))
                    {
                        return FString::Printf(TEXT("Block at (%d,%d) too close to path tile at (%d,%d)"),
                            P.CellX, P.CellY, X, Y);
                    }
                }
            }

            // Overlap + spacing against earlier blocks.
            for (int32 X = P.CellX - S.MinSpacingCells; X < P.CellX + F + S.MinSpacingCells; ++X)
            {
                for (int32 Y = P.CellY - S.MinSpacingCells; Y < P.CellY + F + S.MinSpacingCells; ++Y)
                {
                    if (X >= 0 && Y >= 0 && X < N && Y < N && Occ[X * N + Y])
                    {
                        return FString::Printf(TEXT("Block at (%d,%d) too close to another"), P.CellX, P.CellY);
                    }
                }
            }

            for (int32 X = P.CellX; X < P.CellX + F; ++X)
            {
                for (int32 Y = P.CellY; Y < P.CellY + F; ++Y)
                {
                    Occ[X * N + Y] = 1;
                }
            }
        }

        return FString();
    }

    bool SameLayout(const TArray<FEnvironmentPlacement>& A, const TArray<FEnvironmentPlacement>& B)
    {
        if (A.Num() != B.Num())
        {
            return false;
        }

        for (int32 i = 0; i < A.Num(); ++i)
        {
            if (A[i].CellX != B[i].CellX || A[i].CellY != B[i].CellY)
            {
                return false;
            }
        }

        return true;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnvironmentPlacerLegalTest,
    "GrantMoney.Environment.Placer.BoundsPaddingSpacing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnvironmentPlacerLegalTest::RunTest(const FString& Parameters)
{
    EnvTest::FTestMap Map;

    for (const int32 Footprint : { 1, 2, 3 })
    {
        FEnvironmentSettings S;
        S.Seed = 7;
        S.Density = 1.0f;
        S.FootprintCells = Footprint;
        S.BorderPaddingCells = 1;
        S.MinSpacingCells = 1;

        TArray<FEnvironmentPlacement> Placements;
        TestTrue(TEXT("Populate succeeds"), FEnvironmentPlacer::Populate(Map.View, S, Placements));
        TestTrue(TEXT("Some blocks placed"), Placements.Num() > 0);

        const FString Problem = EnvTest::CheckLegal(Map, S, Placements);
        TestTrue(FString::Printf(TEXT("Footprint %d legal: %s"), Footprint, *Problem), Problem.IsEmpty());
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnvironmentPlacerDeterminismTest,
    "GrantMoney.Environment.Placer.SeedAndDensity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnvironmentPlacerDeterminismTest::RunTest(const FString& Parameters)
{
    EnvTest::FTestMap Map;

    FEnvironmentSettings S;
    S.Seed = 99;
    S.Density = 1.0f;

    TArray<FEnvironmentPlacement> A, B, C;
    FEnvironmentPlacer::Populate(Map.View, S, A);
    FEnvironmentPlacer::Populate(Map.View, S, B);
    TestTrue(TEXT("Same seed gives identical layout"), EnvTest::SameLayout(A, B));

    S.Seed = 100;
    FEnvironmentPlacer::Populate(Map.View, S, C);
    TestFalse(TEXT("Different seed gives a different layout"), EnvTest::SameLayout(A, C));

    // Density 0 -> nothing.
    S.Seed = 99;
    S.Density = 0.0f;
    TArray<FEnvironmentPlacement> None;
    FEnvironmentPlacer::Populate(Map.View, S, None);
    TestEqual(TEXT("Density 0 places nothing"), None.Num(), 0);

    // Density 0.5 is the first half of the density 1 layout (prefix property).
    S.Density = 0.5f;
    TArray<FEnvironmentPlacement> Half;
    FEnvironmentPlacer::Populate(Map.View, S, Half);
    TestTrue(TEXT("Half density places fewer blocks"), Half.Num() > 0 && Half.Num() < A.Num());

    bool bPrefix = true;
    for (int32 i = 0; i < Half.Num(); ++i)
    {
        bPrefix &= (Half[i].CellX == A[i].CellX && Half[i].CellY == A[i].CellY);
    }
    TestTrue(TEXT("Raising density only adds blocks"), bPrefix);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnvironmentPlacerTileFilterTest,
    "GrantMoney.Environment.Placer.TileAllowList",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnvironmentPlacerTileFilterTest::RunTest(const FString& Parameters)
{
    EnvTest::FTestMap Map; // tile 1 = grass, tile 2 = road at y=44..46

    FEnvironmentSettings S;
    S.Seed = 3;
    S.Density = 1.0f;
    S.PlaceableTileIds = { 1 };
    S.PathClearanceCells = 1;

    TArray<FEnvironmentPlacement> Placements;
    FEnvironmentPlacer::Populate(Map.View, S, Placements);
    TestTrue(TEXT("Grass-only: blocks placed"), Placements.Num() > 0);

    for (const FEnvironmentPlacement& P : Placements)
    {
        const bool bAboveRoad = P.CellY + P.FootprintCells + S.PathClearanceCells <= 44;
        const bool bBelowRoad = P.CellY - S.PathClearanceCells >= 47;
        TestTrue(TEXT("No block on or hugging the road"), bAboveRoad || bBelowRoad);
    }

    // Allow only the road tile: every block must sit on road cells.
    S.PlaceableTileIds = { 2 };
    TArray<FEnvironmentPlacement> OnRoad;
    FEnvironmentPlacer::Populate(Map.View, S, OnRoad);
    TestTrue(TEXT("Road-only: blocks placed"), OnRoad.Num() > 0);

    for (const FEnvironmentPlacement& P : OnRoad)
    {
        TestEqual(TEXT("Road-only block is on tile 2"), static_cast<int32>(Map.Ground[P.CellX][P.CellY]), 2);
    }

    // Empty allow-list places nothing.
    S.PlaceableTileIds.Reset();
    TArray<FEnvironmentPlacement> None;
    FEnvironmentPlacer::Populate(Map.View, S, None);
    TestEqual(TEXT("Empty allow-list places nothing"), None.Num(), 0);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
