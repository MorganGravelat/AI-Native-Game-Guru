# MapEnvironmentPopulator: Team Guide

Places proof-of-concept blocks (rocks, foliage, props later) on a TileGen map. Placement is **deterministic**: the same inputs always produce the same layout, so anyone on the team can reproduce a result.

**Status:** first iteration. Blocks are engine cubes, and there is one prop category.

## What it does

1. Reads the level's ground and valid-cell arrays through `FMapArrays::GetMap`.
2. Keeps only cells whose ground tile is on the **allow-list** (for example grass).
3. Removes cells too close to walls, the map edge, or non-placeable walkable tiles (roads).
4. Shuffles the remaining anchor cells with the seed and accepts blocks greedily, enforcing spacing with an occupancy grid.
5. Optionally rejects any block that would split the walkable area.
6. Keeps the first `ceil(Density x capacity)` blocks and draws them as instanced cubes.

Raising density only **adds** blocks. Existing blocks don't move.

## Files

| File | Role |
|---|---|
| `EnvironmentTypes.h` | `FEnvironmentSettings` (all tunable settings) |
| `EnvironmentPlacer.h/.cpp` | Placement logic. Plain C++, no spawning, unit-testable |
| `MapEnvironmentPopulator.h/.cpp` | Actor that calls the placer and renders the blocks |
| `Tests/EnvironmentPlacerTests.cpp` | Automation tests |

Headers live in `Game/Source/GrantMoney/Public/` and `.cpp` files in `Private/`, next to `MapArrays`.

## Setup

1. Complete the team onboarding guide first (clean clone, LFS, successful build, `PlayGame` renders the map).
2. Work on a Jira ticket branch. Close Unreal and Rider before switching branches.
3. Do a **full build** of `GrantMoneyEditor | Win64 | Development`. New `UCLASS` and `USTRUCT` types don't work with Live Coding.

## Using it in the editor

1. Open `PlayGame`.
2. Drag **MapEnvironmentPopulator** into the level from Place Actors (or the Content Drawer with C++ classes shown).
3. Set its location to `(0, 0, 0)`, the same as `TileMapRenderer`.
4. Set **Level To Render** to the same value as the renderer.
5. Press Play once and read the Output Log. It prints this level's tile table:
   ```
   MapEnvironmentPopulator: Level 1 Tile ID 1 = /Game/Tiles/tile_... [placeable]
   ```
6. Open the listed textures in `Content/Tiles` and note which ID is grass and which is road.
7. Stop Play. In the actor's Details panel, set **Environment > Tiles > Placeable Tile Ids** to the grass ID(s).
8. Press Play. The log should end with `Placed N blocks (Level L, Seed S, Density D)`.

Blocks are built at runtime in `BeginPlay`, like the tile map. They won't appear in the editor viewport until you press Play, and they are not saved in the level.

## Settings

| Setting | Meaning | Default |
|---|---|---|
| Seed | Random seed for the layout | 12345 |
| Density | 0 = no blocks, 1 = as many as fit | 0.5 |
| Footprint Cells | Block size, square, in cells | 1 |
| Border Padding Cells | Clearance from walls, void and map edge | 1 |
| Min Spacing Cells | Empty cells required between blocks | 1 |
| Preserve Connectivity | Reject blocks that split the walkable area | on |
| Placeable Tile Ids | Ground tile IDs blocks may sit on. Empty means nothing is placed | `{1}` |
| Path Clearance Cells | Clearance from non-placeable walkable tiles (roads) | 1 |

Visual-only settings on the actor: **Block Material**, **Tile Size**, **Block Height**, **Footprint Fill**, **Z Offset**.

## Replicating a layout

A layout is reproduced only if **all** of these match:

- the same map export and **Level To Render**
- the same **Seed**
- the same **Density, Footprint Cells, Border Padding Cells, Min Spacing Cells, Path Clearance Cells, Preserve Connectivity**
- the same **Placeable Tile Ids**
- the same version of the placer code

Because the blocks aren't saved, record the settings in your task doc and PR description. A table copied from the Details panel is enough. Changing any one value changes the layout. Density is the exception: with everything else fixed, it only adds or removes blocks from the end of the sequence.

## Tile IDs are per map

Each TileGen export assigns its own tile IDs. The same texture can be ID 1 in `Map1_1` and ID 2 in `Map1_2`. Always check the logged tile table for the level you are using, and use one populator actor per level.

## Tests

Run from **Tools > Session Frontend > Automation**, filtered by `GrantMoney.Environment`.

They cover bounds and padding, spacing and overlap, seed determinism, density behavior, and the tile allow-list (roads stay clear).

## Troubleshooting

| Symptom | Check |
|---|---|
| No blocks | Placeable Tile Ids is empty or has the wrong IDs. Compare it with the logged tile table. Also check that Density is above 0. |
| Blocks on roads | The road tile ID is in the allow-list. Remove it. |
| `level N is not registered in MapArrays` | Register the level in `MapArrays.cpp`, or fix Level To Render. |
| Blocks don't line up with tiles | Tile Size must match `TileMapRenderer` (100), and the actor must sit at `(0,0,0)`. |
| Build errors after pulling | Close Unreal and do a full rebuild. |
| Different layout on a teammate's machine | Compare every value in the replication list above, and confirm both are on the same commit. |

## Known limitations

- Only ground and valid cells are used. The exported `Collision` and `Deco` layers are not yet read.
- The connectivity check only catches blocks that split the walkable area. A route that matters for gameplay but isn't encoded in the tiles isn't protected.
- Greedy packing is a good fill, not a mathematically maximal one.
- `TileSize` is duplicated from the private value in `TileMapRenderer`.
- Placing the actor in `PlayGame` modifies `PlayGame.umap`, a binary file that merges badly. Tell the team before committing it.
