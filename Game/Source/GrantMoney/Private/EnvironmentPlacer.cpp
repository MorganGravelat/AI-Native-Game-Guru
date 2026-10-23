#include "EnvironmentPlacer.h"

#include "MapArrays.h"
#include "Math/RandomStream.h"

namespace
{
    // Flat index for [X][Y] data: X is the outer (column) index.
    FORCEINLINE int32 Idx(int32 X, int32 Y, int32 Height)
    {
        return X * Height + Y;
    }

    // Counts 4-connected components of cells where Walk != 0.
    int32 CountComponents(
        const TArray<uint8>& Walk,
        int32 Width,
        int32 Height,
        TArray<uint8>& Visited,
        TArray<int32>& Stack)
    {
        const int32 Total = Width * Height;
        Visited.Init(0, Total);
        Stack.Reset();

        int32 Components = 0;

        for (int32 Start = 0; Start < Total; ++Start)
        {
            if (!Walk[Start] || Visited[Start])
            {
                continue;
            }

            ++Components;
            Visited[Start] = 1;
            Stack.Add(Start);

            while (Stack.Num() > 0)
            {
                const int32 Cur = Stack.Pop(EAllowShrinking::No);
                const int32 X = Cur / Height;
                const int32 Y = Cur % Height;

                const int32 NX[4] = { X + 1, X - 1, X, X };
                const int32 NY[4] = { Y, Y, Y + 1, Y - 1 };

                for (int32 i = 0; i < 4; ++i)
                {
                    if (NX[i] < 0 || NY[i] < 0 || NX[i] >= Width || NY[i] >= Height)
                    {
                        continue;
                    }

                    const int32 N = Idx(NX[i], NY[i], Height);

                    if (Walk[N] && !Visited[N])
                    {
                        Visited[N] = 1;
                        Stack.Add(N);
                    }
                }
            }
        }

        return Components;
    }
}

bool FEnvironmentPlacer::Populate(
    const FGrantMoneyMapData& Map,
    const FEnvironmentSettings& Settings,
    TArray<FEnvironmentPlacement>& OutPlacements)
{
    OutPlacements.Reset();

    if (!Map.IsValid())
    {
        return false;
    }

    const int32 W = Map.Width;
    const int32 H = Map.Height;
    const int32 F = FMath::Clamp(Settings.FootprintCells, 1, 16);
    const int32 Pad = FMath::Max(0, Settings.BorderPaddingCells);
    const int32 Gap = FMath::Max(0, Settings.MinSpacingCells);
    const int32 Clr = FMath::Max(0, Settings.PathClearanceCells);
    const float Density = FMath::Clamp(Settings.Density, 0.0f, 1.0f);

    if (Density <= 0.0f)
    {
        return true;
    }

    // 1) Base walkable cells: part of the map AND has a ground tile.
    TArray<uint8> Base;
    Base.Init(0, W * H);

    for (int32 X = 0; X < W; ++X)
    {
        for (int32 Y = 0; Y < H; ++Y)
        {
            Base[Idx(X, Y, H)] = (Map.Valid[X][Y] != 0 && Map.Ground[X][Y] != 0) ? 1 : 0;
        }
    }

    // 2) Placeable cells: walkable AND the ground tile is on the allow-list.
    const TSet<int32> Allowed(Settings.PlaceableTileIds);

    TArray<uint8> Place;
    Place.Init(0, W * H);

    for (int32 X = 0; X < W; ++X)
    {
        for (int32 Y = 0; Y < H; ++Y)
        {
            const int32 I = Idx(X, Y, H);
            Place[I] = (Base[I] && Allowed.Contains(static_cast<int32>(Map.Ground[X][Y]))) ? 1 : 0;
        }
    }

    // 3) Usable mask. A cell is usable if:
    //    - it is placeable,
    //    - every cell within Pad (Chebyshev) is walkable (wall/edge padding;
    //      out-of-bounds counts as a wall), and
    //    - no walkable-but-NOT-placeable cell (road/path) lies within Clr.
    TArray<uint8> Usable;
    Usable.Init(0, W * H);

    const int32 Reach = FMath::Max(Pad, Clr);

    for (int32 X = 0; X < W; ++X)
    {
        for (int32 Y = 0; Y < H; ++Y)
        {
            bool bOk = Place[Idx(X, Y, H)] != 0;

            for (int32 DX = -Reach; DX <= Reach && bOk; ++DX)
            {
                for (int32 DY = -Reach; DY <= Reach; ++DY)
                {
                    const int32 NX = X + DX;
                    const int32 NY = Y + DY;
                    const bool bInBounds = NX >= 0 && NY >= 0 && NX < W && NY < H;
                    const bool bWalkable = bInBounds && Base[Idx(NX, NY, H)] != 0;

                    const bool bInPad = FMath::Abs(DX) <= Pad && FMath::Abs(DY) <= Pad;
                    const bool bInClr = FMath::Abs(DX) <= Clr && FMath::Abs(DY) <= Clr;

                    if (bInPad && !bWalkable)
                    {
                        bOk = false;
                        break;
                    }

                    if (bInClr && bWalkable && !Place[Idx(NX, NY, H)])
                    {
                        bOk = false;
                        break;
                    }
                }
            }

            Usable[Idx(X, Y, H)] = bOk ? 1 : 0;
        }
    }

    // 4) Candidate anchors: footprint fully inside the usable mask.
    //    Built in fixed X-then-Y order so the pre-shuffle list is deterministic.
    TArray<int32> Candidates;

    for (int32 X = 0; X + F <= W; ++X)
    {
        for (int32 Y = 0; Y + F <= H; ++Y)
        {
            bool bOk = true;

            for (int32 FX = 0; FX < F && bOk; ++FX)
            {
                for (int32 FY = 0; FY < F; ++FY)
                {
                    if (!Usable[Idx(X + FX, Y + FY, H)])
                    {
                        bOk = false;
                        break;
                    }
                }
            }

            if (bOk)
            {
                Candidates.Add(Idx(X, Y, H));
            }
        }
    }

    // 5) Seeded Fisher-Yates shuffle.
    FRandomStream Rng(Settings.Seed);

    for (int32 i = Candidates.Num() - 1; i > 0; --i)
    {
        const int32 J = Rng.RandRange(0, i);
        Candidates.Swap(i, J);
    }

    // 6) Greedy pass. Accepting in shuffled order means any prefix of the
    //    result is itself a valid layout, which is how Density is applied.
    TArray<uint8> Occupied;
    Occupied.Init(0, W * H);

    TArray<uint8> Walk = Base;
    TArray<uint8> Visited;
    TArray<int32> Stack;

    int32 Baseline = Settings.bPreserveConnectivity
        ? CountComponents(Walk, W, H, Visited, Stack)
        : 0;

    TArray<FEnvironmentPlacement> Accepted;

    for (const int32 Candidate : Candidates)
    {
        const int32 X = Candidate / H;
        const int32 Y = Candidate % H;

        // Spacing: the footprint grown by Gap must not touch an occupied cell.
        const int32 X0 = FMath::Max(0, X - Gap);
        const int32 Y0 = FMath::Max(0, Y - Gap);
        const int32 X1 = FMath::Min(W - 1, X + F - 1 + Gap);
        const int32 Y1 = FMath::Min(H - 1, Y + F - 1 + Gap);

        bool bBlocked = false;

        for (int32 CX = X0; CX <= X1 && !bBlocked; ++CX)
        {
            for (int32 CY = Y0; CY <= Y1; ++CY)
            {
                if (Occupied[Idx(CX, CY, H)])
                {
                    bBlocked = true;
                    break;
                }
            }
        }

        if (bBlocked)
        {
            continue;
        }

        // Critical path check: tentatively block the footprint and recount.
        if (Settings.bPreserveConnectivity)
        {
            for (int32 FX = 0; FX < F; ++FX)
            {
                for (int32 FY = 0; FY < F; ++FY)
                {
                    Walk[Idx(X + FX, Y + FY, H)] = 0;
                }
            }

            const int32 After = CountComponents(Walk, W, H, Visited, Stack);

            if (After > Baseline)
            {
                // Would split the walkable area: undo.
                for (int32 FX = 0; FX < F; ++FX)
                {
                    for (int32 FY = 0; FY < F; ++FY)
                    {
                        Walk[Idx(X + FX, Y + FY, H)] = 1;
                    }
                }

                continue;
            }

            Baseline = After;
        }

        for (int32 FX = 0; FX < F; ++FX)
        {
            for (int32 FY = 0; FY < F; ++FY)
            {
                Occupied[Idx(X + FX, Y + FY, H)] = 1;
            }
        }

        FEnvironmentPlacement Placement;
        Placement.CellX = X;
        Placement.CellY = Y;
        Placement.FootprintCells = F;
        Accepted.Add(Placement);
    }

    // 7) Density = fraction of the achievable capacity (ceil so any
    //    density > 0 yields at least one block when capacity exists).
    const int32 Target = FMath::Min(
        Accepted.Num(),
        FMath::CeilToInt(Density * static_cast<float>(Accepted.Num())));

    OutPlacements.Append(Accepted.GetData(), Target);
    return true;
}
