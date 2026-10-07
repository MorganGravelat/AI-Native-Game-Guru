#include "MapArrays.h"

// TileGen generated files. Generated names must only be referenced in this file.

// Level 1
#include "Maps/Map1_1.h"
#include "Maps/Map1_1_TileAssets.h"
// Level 2
#include "Maps/Map1_2.h"
#include "Maps/Map1_2_TileAssets.h"

namespace
{
    struct FRegisteredMap
    {
        int32 Level;
        const uint16 (*Ground)[FGrantMoneyMapData::MapSize];
        const uint8 (*Valid)[FGrantMoneyMapData::MapSize];
        const TCHAR* const* TileAssets;
        int32 TileAssetCount;
    };

    // Add one entry per generated map export.
    const FRegisteredMap* FindMap(int32 Level)
    {
        static const FRegisteredMap RegisteredMaps[] =
        {
            { 1, Map1_1, Map1_1_Valid, Map1_1_TileAssets, Map1_1_TileAssetCount },
            { 2, Map1_2, Map1_2_Valid, Map1_2_TileAssets, Map1_2_TileAssetCount },
        };

        for (const FRegisteredMap& Entry : RegisteredMaps)
        {
            if (Entry.Level == Level)
            {
                return &Entry;
            }
        }

        return nullptr;
    }
}

bool FMapArrays::GetMap(int32 Level, FGrantMoneyMapData& OutMap)
{
    OutMap.Reset();

    const FRegisteredMap* Entry = FindMap(Level);

    if (!Entry || !Entry->Ground || !Entry->Valid)
    {
        return false;
    }

    OutMap.Ground = Entry->Ground;
    OutMap.Valid = Entry->Valid;
    OutMap.Width = FGrantMoneyMapData::MapSize;
    OutMap.Height = FGrantMoneyMapData::MapSize;

    return true;
}

const TCHAR* FMapArrays::GetTileAsset(int32 Level, int32 TileId)
{
    // Tile 0 is an empty cell.
    if (TileId <= 0)
    {
        return nullptr;
    }

    const FRegisteredMap* Entry = FindMap(Level);

    if (!Entry || !Entry->TileAssets || TileId >= Entry->TileAssetCount)
    {
        return nullptr;
    }

    const TCHAR* Path = Entry->TileAssets[TileId];

    if (!Path || Path[0] == TEXT('\0'))
    {
        return nullptr;
    }

    return Path;
}
