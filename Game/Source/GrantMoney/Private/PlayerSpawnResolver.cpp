#include "PlayerSpawnResolver.h"

#include "Algo/StableSort.h"

TArray<FIntPoint> FPlayerSpawnResolver::FindSafeStartCells(const FGrantMoneyMapData& MapData)
{
    TArray<FIntPoint> SafeCells;

    if (!MapData.IsValid())
    {
        return SafeCells;
    }

    auto IsPlayable = [&MapData](int32 X, int32 Y) -> bool
    {
        return X >= 0 && X < MapData.Width
            && Y >= 0 && Y < MapData.Height
            && MapData.Valid[X][Y] != 0
            && MapData.Ground[X][Y] != 0;
    };

    // The centre of the playable area is the average of every playable cell.
    FVector2D PlayableSum = FVector2D::ZeroVector;
    int32 PlayableCount = 0;

    for (int32 X = 0; X < MapData.Width; ++X)
    {
        for (int32 Y = 0; Y < MapData.Height; ++Y)
        {
            if (!IsPlayable(X, Y))
            {
                continue;
            }

            PlayableSum += FVector2D(X, Y);
            ++PlayableCount;

            bool bNeighboursPlayable = true;

            for (int32 DX = -1; DX <= 1 && bNeighboursPlayable; ++DX)
            {
                for (int32 DY = -1; DY <= 1; ++DY)
                {
                    if (!IsPlayable(X + DX, Y + DY))
                    {
                        bNeighboursPlayable = false;
                        break;
                    }
                }
            }

            if (bNeighboursPlayable)
            {
                SafeCells.Emplace(X, Y);
            }
        }
    }

    if (SafeCells.IsEmpty())
    {
        return SafeCells;
    }

    const FVector2D Centre = PlayableSum / PlayableCount;

    // Cells were collected in grid order, so a stable sort keeps that order for ties.
    Algo::StableSortBy(SafeCells, [&Centre](const FIntPoint& Cell)
    {
        return FVector2D::DistSquared(FVector2D(Cell.X, Cell.Y), Centre);
    });

    return SafeCells;
}
