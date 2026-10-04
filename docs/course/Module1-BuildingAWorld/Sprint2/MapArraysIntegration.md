# MapArrays Integration

## What Was Built

`ATileMapRenderer` no longer knows which TileGen export it is drawing. A small
`FMapArrays` boundary resolves a game-facing level number to generated map data
and tile asset paths:

```
PlayGame -> ATileMapRenderer (LevelToRender)
         -> FMapArrays::GetMap / GetTileAsset
         -> generated TileGen data (only referenced in MapArrays.cpp)
         -> HISM tile instances
```

The level is chosen in the Unreal Details panel: **Map > Level To Render**
(default `1`).

## Why We Built It

The renderer previously included `Maps/Map1_1.h` and `Maps/Map1_1_TileAssets.h`
and read `Map1_1`, `Map1_1_Valid`, `Map1_1_TileAssets` and
`Map1_1_TileAssetCount` directly, so every new export meant editing the renderer.

## Files Changed

| File | Responsibility |
|------|----------------|
| `Game/Source/GrantMoney/Public/MapArrays.h` (new) | `FGrantMoneyMapData` (non-owning pointers to the ground and validity arrays plus dimensions) and `FMapArrays` (`GetMap`, `GetTileAsset`). |
| `Game/Source/GrantMoney/Private/MapArrays.cpp` (new) | Registration table mapping level numbers to generated symbols. The only file that includes generated map headers. |
| `Game/Source/GrantMoney/Public/TileMapRenderer.h` | Adds `UPROPERTY(EditAnywhere, Category="Map", meta=(ClampMin="1")) int32 LevelToRender = 1;`. |
| `Game/Source/GrantMoney/Private/TileMapRenderer.cpp` | Uses `FMapArrays`; validates map data; resolves texture paths via `GetTileAsset`; logs unusable tile IDs once per ID. |

Generated files under `Private/Maps/` were not modified.

## Implementation Steps

1. Inspected the export: individual-map format (`Map1_1[64][64]` as `unsigned short`,
   `Map1_1_Valid[64][64]` as `unsigned char`, `Map1_1_TileAssets[]` and
   `Map1_1_TileAssetCount` in `Map1_1_TileAssets.h`). Arrays are indexed `[X][Y]`.
   No `MapArrays` or map-set directory existed.
2. Added `MapArrays.h/.cpp`. `Ground` is `const uint16 (*)[64]` and `Valid` is
   `const uint8 (*)[64]`, so IDs above 255 are not truncated and nothing is copied.
   `GetMap` resets the output on failure. `GetTileAsset` returns `nullptr` for
   tile 0, unknown levels, out-of-range IDs and empty paths.
3. Refactored `BuildMap()`: fetch the map, check `IsValid()`, loop over
   `Width`/`Height`, and keep the existing HISM grouping, plane mesh, 100-unit
   spacing, `M_Tile` dynamic material instances (`TileTexture`), disabled collision
   and the final tile-count log.
4. To add a map, include its two generated headers in `MapArrays.cpp` and add one
   line to the `RegisteredMaps` table, e.g.
   `{ 2, Map1_2, Map1_2_Valid, Map1_2_TileAssets, Map1_2_TileAssetCount }`.
   No renderer change is needed.

## Detailed Change Log

### `Public/MapArrays.h` (new)

- `struct GRANTMONEY_API FGrantMoneyMapData`
  - `static constexpr int32 MapSize = 64;`
  - `const uint16 (*Ground)[MapSize]`: pointer to the generated ground array (`[X][Y]`, 16-bit IDs, 0 = empty).
  - `const uint8 (*Valid)[MapSize]`: pointer to the generated validity array (`[X][Y]`, 0 = not part of the map).
  - `int32 Width`, `int32 Height`: default `0` (invalid) until `GetMap` fills them.
  - `IsValid()`: true only if both pointers are non-null and Width/Height are within `1..MapSize`.
  - `Reset()`: returns the struct to the invalid default state.
- `class GRANTMONEY_API FMapArrays`
  - `static bool GetMap(int32 Level, FGrantMoneyMapData& OutMap)`
  - `static const TCHAR* GetTileAsset(int32 Level, int32 TileId)`

### `Private/MapArrays.cpp` (new)

- Includes `Maps/Map1_1.h` and `Maps/Map1_1_TileAssets.h`. This is the only file allowed to name generated symbols.
- An anonymous-namespace `FRegisteredMap` struct holds: `Level`, `Ground`, `Valid`, `TileAssets`, `TileAssetCount`.
- `FindMap(Level)` searches a function-local static table (`RegisteredMaps`) that currently has one entry: `{ 1, Map1_1, Map1_1_Valid, Map1_1_TileAssets, Map1_1_TileAssetCount }`. The table is function-local to avoid static-initialization-order problems.
- `GetMap` always calls `OutMap.Reset()` first, then fills pointers and sets Width/Height to 64 if the level is registered. Returns `false` (leaving the struct invalid) otherwise.
- `GetTileAsset` returns `nullptr` when `TileId <= 0`, the level is unregistered, `TileId >= TileAssetCount`, or the path is null/empty. Otherwise it returns the path from the generated table (e.g. `/Game/Tiles/tile_...`).

### `Public/TileMapRenderer.h`

- Added public property:
  ```cpp
  UPROPERTY(EditAnywhere, Category = "Map", meta = (ClampMin = "1"))
  int32 LevelToRender = 1;
  ```
- Updated the `BuildMap()` comment (no longer "the 64x64 map"). Everything else is unchanged.

### `Private/TileMapRenderer.cpp`

| Before | After |
|--------|-------|
| `#include "Maps/Map1_1.h"` and `"Maps/Map1_1_TileAssets.h"` | `#include "MapArrays.h"` |
| Loops hardcoded to `0..63` | Loops over `MapData.Width` / `MapData.Height` |
| `Map1_1_Valid[X][Y]` | `MapData.Valid[X][Y]` |
| `Map1_1[X][Y]` | `MapData.Ground[X][Y]` |
| `TileId >= Map1_1_TileAssetCount` check, warning per cell | `FMapArrays::GetTileAsset(LevelToRender, TileId)`; if `nullptr`, warn once per tile ID (tracked in a `TSet<int32> InvalidTileIds`) and skip those cells |
| `Map1_1_TileAssets[TileId]` read after the component was created | Path resolved **before** creating the HISM component, so an unusable ID never creates an empty component |
| No map-level checks | `GetMap` failure logs `Failed to load level N...` and returns; `!MapData.IsValid()` logs the level and dimensions and returns |

Unchanged on purpose: HISM component per tile ID, `TileGroup_<id>` naming, built-in Plane mesh, `TileSize = 100`, `X * TileSize, Y * TileSize` positioning, no collision, `M_Tile` dynamic material with `TileTexture`, texture-load-failure warning, and the final `Created %d tiles` log.

### Not changed

Generated files in `Private/Maps/`, `GrantMoney.Build.cs`, the `PlayGame` level, camera, materials, textures, `.gitignore`/`.gitattributes`.

## How to Use the New Structure

### Pick a level in the editor

1. Open `PlayGame` and select the `TileMapRenderer` actor.
2. In Details, under **Map**, set **Level To Render** (default `1`).
3. Press Play. The renderer builds that level in `BeginPlay()`.

An unregistered number logs an error and builds nothing.

### Add a new TileGen map (e.g. Level 2)

1. Copy the generated files into `Game/Source/GrantMoney/Private/Maps/` (e.g. `Map1_2.h`, `Map1_2.cpp`, `Map1_2_TileAssets.h`). Do not edit their contents.
2. Import the tile textures into `/Game/Tiles/` as TileGen's import script describes.
3. In `MapArrays.cpp`, add the includes:
   ```cpp
   #include "Maps/Map1_2.h"
   #include "Maps/Map1_2_TileAssets.h"
   ```
4. Add one row to `RegisteredMaps`:
   ```cpp
   { 2, Map1_2, Map1_2_Valid, Map1_2_TileAssets, Map1_2_TileAssetCount },
   ```
5. Build, then set **Level To Render** to `2`. `TileMapRenderer` needs no edits.

If two exports define the same tile asset array name, give them distinct symbol names (TileGen already prefixes with the map name). Maps must be 64×64 unless `MapSize` and the array types are updated together.

### Use the API from other code

```cpp
#include "MapArrays.h"

FGrantMoneyMapData Map;
if (FMapArrays::GetMap(1, Map))
{
    const uint16 Tile = Map.Ground[X][Y];   // X = column, Y = row
    const uint8 Playable = Map.Valid[X][Y];
    const TCHAR* Path = FMapArrays::GetTileAsset(1, Tile); // nullptr if empty/invalid
}
```

The pointers reference the generated arrays directly, so never write through them or copy the whole map unnecessarily.

## AI Use

Copilot inspected the repository, wrote the MapArrays interface, refactored the
renderer and drafted this document from the task prompt. Human decisions: the
task scope, the level-number scheme, and reviewing/building the result in the editor.

Verification status: **no build or in-editor test has been run by Copilot.** An
automated build was attempted (`Build.bat GrantMoneyEditor Win64 Development`) but
UnrealBuildTool refused because the Unreal Editor was running (Live Coding active).
Tests A, B and C below are pending.

## Verification

Close the editor and build **Development Editor | Win64** (the new `UPROPERTY`
and new files require a full build, not Live Coding), reopen `PlayGame`, then:

- **Test A (Level 1):** select `TileMapRenderer`, confirm Level To Render = 1, Play.
  Expect log `TileMapRenderer finished. Created 2819 tiles.`, correct textures, no grey
  checkerboard, unchanged orientation/camera.
- **Test B (unsupported level):** set Level To Render = 999, Play. Expect
  `TileMapRenderer: Failed to load level 999...` in the Output Log, no tiles, no crash.
  Restore to 1.
- **Test C (Level 2):** only possible once a second TileGen export exists and is
  registered. Not yet verified.

## Known Limitations

- Only Level 1 is registered; multi-map behavior is unverified until a second export exists.
- The map is built once in `BeginPlay()`. Runtime switching (`LoadLevel`) is future
  work: it would need to destroy the existing `TileGroup_*` components and clear
  references before rebuilding.
- If a tile texture fails to load, its group keeps the mesh's default material (existing behavior); the failure is logged once per tile ID.
- The `Deco` and `Collision` arrays are exported but not exposed through `MapArrays` yet.
