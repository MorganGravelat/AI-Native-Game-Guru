#pragma once

#include "CoreMinimal.h"

// Non-owning view of one TileGen map. The pointers reference the generated
// arrays directly (nothing is copied) and stay valid for the whole program.
// Both arrays are indexed [X][Y]: X = column, Y = row.
struct GRANTMONEY_API FGrantMoneyMapData
{
    static constexpr int32 MapSize = 64;

    // TileGen ground tile IDs (16-bit, 0 = empty).
    const uint16 (*Ground)[MapSize] = nullptr;

    // Playable-cell flags (8-bit, 0 = not part of the map).
    const uint8 (*Valid)[MapSize] = nullptr;

    int32 Width = 0;
    int32 Height = 0;

    bool IsValid() const
    {
        return Ground && Valid
            && Width > 0 && Width <= MapSize
            && Height > 0 && Height <= MapSize;
    }

    void Reset()
    {
        *this = FGrantMoneyMapData();
    }
};

// The only boundary between generated TileGen exports and game systems.
// To add a map, register it in MapArrays.cpp.
class GRANTMONEY_API FMapArrays
{
public:
    // Resolves a game-facing level number. On failure returns false and
    // leaves OutMap reset to an invalid state.
    static bool GetMap(int32 Level, FGrantMoneyMapData& OutMap);

    // Returns the Unreal asset path (under /Game/Tiles/) for a tile ID of the
    // given level, or nullptr for tile 0 (empty), unknown levels, out-of-range
    // IDs, or missing paths.
    static const TCHAR* GetTileAsset(int32 Level, int32 TileId);
};
