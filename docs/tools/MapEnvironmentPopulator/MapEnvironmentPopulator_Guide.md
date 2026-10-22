# MapEnvironmentPopulator: Team Guide

Populates a TileGen map with your own imported assets (trees on grass, rocks on stone, and so on). Placement is **deterministic**: the same inputs always produce the same layout, so anyone on the team can reproduce a result.

**Status:** first iteration. Assets appear only while the game is running.

## How it works

You add **Rules**. Each rule says "put *this asset* on *these tiles*, with *these* density and spacing settings." For each rule the tool:

1. Reads the level's ground and valid-cell data through `FMapArrays::GetMap`.
2. Keeps only cells whose ground tile is in the rule's **Placeable Tile Ids**.
3. Removes cells too close to walls, the map edge, or non-placeable walkable tiles (roads, other terrain).
4. Shuffles the remaining cells with the rule's seed and accepts assets greedily, enforcing spacing.
5. Optionally rejects any asset that would split the walkable area.
6. Keeps the first `ceil(Density x capacity)` placements and draws your asset there.

Raising density only **adds** assets. Existing ones don't move.

## Files

| File | Role |
|---|---|
| `EnvironmentTypes.h` | `FEnvironmentSettings` (placement settings) and `FEnvironmentAssetRule` (one rule) |
| `EnvironmentPlacer.h/.cpp` | Placement logic. Plain C++, unit-tested |
| `MapEnvironmentPopulator.h/.cpp` | Actor that runs each rule and renders its asset |
| `Tests/EnvironmentPlacerTests.cpp` | Automation tests for the placement logic |

Headers live in `Game/Source/GrantMoney/Public/` and `.cpp` files in `Private/`, next to `MapArrays`.

## Setup

1. Complete the team onboarding guide first (clean clone, LFS, successful build, `PlayGame` renders the map).
2. Work on a Jira ticket branch. Close Unreal and Rider before switching branches.
3. Do a **full build** of `GrantMoneyEditor | Win64 | Development`. New `UCLASS` and `USTRUCT` types don't work with Live Coding.
4. Open `PlayGame` and drag **MapEnvironmentPopulator** into the level (Place Actors, or the Content Drawer with C++ classes shown).
5. Set its location to `(0, 0, 0)`, the same as `TileMapRenderer`.
6. Set **Level To Render** to the same value as the renderer.

## Step 1: Import your assets

1. In the Content Browser, create a folder for props, for example `Content/Props/`.
2. Drag your FBX (or other supported mesh file) into the folder, or use **Import**. Accept the defaults, and make sure the mesh and its materials/textures import together.
3. Double-click each imported mesh to open the Static Mesh editor and check that it:
   - looks correct and has its materials,
   - faces the right way up (Z up),
   - has collision, if you plan to turn on a rule's **Collision**.
4. Save the assets.

The pivot point doesn't matter. The tool centers each mesh on its cell and sits it on the ground using the mesh's bounds.

Imported `.uasset` files are binary and stored with Git LFS. Commit them on your ticket branch with the rest of your work (see "Committing your work").

## Step 2: Find your tile IDs

Tile IDs are numbers assigned by each TileGen export. They are **not** the same across maps: the same texture can be ID 1 in `Map1_1` and ID 2 in `Map1_2`.

1. Press Play once with the populator in the level.
2. Open the Output Log and find lines like:
   ```
   MapEnvironmentPopulator: Level 1 Tile ID 1 = /Game/Tiles/tile_20260930_174923_435 [used by: no rule]
   ```
3. Open those textures in `Content/Tiles` and note what each ID looks like (grass, stone, road, border).
4. Use these IDs in step 4. Once rules exist, the same lines show which rules use each tile.

## Step 3: Add and configure rules

1. Select the `MapEnvironmentPopulator` actor in the World Outliner.
2. In Details, open **Environment > Rules** and click **+** to add a rule.
3. Fill in the rule (the fields are listed below).
4. Add one rule per asset type, for example a `Trees` rule on grass and a `Rocks` rule on stone.
5. Press Play and check the log for `rule 'Trees' placed N assets`.

Dragging an asset works on the **Mesh** field: drag a Static Mesh from the Content Browser onto it, or use its picker. The **Actor Class** field accepts a Blueprint class the same way.

## Step 4: Choose tiles for each rule

Inside the rule, open **Placement > Settings** and edit **Placeable Tile Ids**.

- For trees on grass, list the grass tile ID(s).
- For rocks on stone, list the stone tile ID(s).
- Any tile **not** listed is off limits: roads, paths, borders and any new tile type you add later.
- An empty list places nothing.
- Don't list the same tile in two rules. Rules don't know about each other, so their assets could overlap. The log warns if you do.

## Actor fields

| Field | Meaning | Default |
|---|---|---|
| Level To Render | Which registered map to populate. Must match the `TileMapRenderer` | 1 |
| Tile Size | World units per cell. Must match `TileMapRenderer` (its value is private, so it is duplicated here) | 100 |
| Rules | The list of rules. Each rule is one asset type | empty |

## Rule fields

### Rule

| Field | Meaning | Default |
|---|---|---|
| Label | Name shown in the list and in log messages | `New Rule` |
| Enabled | Untick to skip the rule without deleting it | on |

### Asset

| Field | Meaning | Default |
|---|---|---|
| Mesh | The Static Mesh to place. Recommended: it is drawn with cheap instancing | none |
| Actor Class | Optional Blueprint "prefab". If set, it is used **instead of** Mesh and one Actor is spawned per placement. Heavier than a mesh, so use it only when the prop needs several components | none |
| Fit To Footprint | Scales the mesh uniformly so its widest horizontal side fills the footprint (see "Sizing"). Mesh only | on |
| Fit Fill | Fraction of the footprint the mesh fills when Fit To Footprint is on (0.1 to 1). Smaller leaves visible gaps | 0.8 |
| Scale | Extra uniform scale multiplier | 1 |
| Scale Variation | X = minimum, Y = maximum. Each asset gets a random multiplier in this range. `(1, 1)` means no variation | (1, 1) |
| Random Yaw | Rotates each asset by a random angle around the vertical axis | on |
| Z Offset | Lifts the asset above the ground, in world units | 0 |
| Collision | Turns on collision (BlockAll) for the instances. The mesh needs collision set up in its asset | off |

A rule with neither Mesh nor Actor Class is skipped, and the log tells you.

### Placement > Settings

These control **where** assets go. The placement logic itself is documented in "How it works".

| Field | Meaning | Default |
|---|---|---|
| Seed | Random seed. Also seeds the yaw and scale variation | 12345 |
| Density | 0 = no assets, 1 = as many as fit. Applied as a fraction of what the space can hold | 0.5 |
| Footprint Cells | Size of each asset's footprint, as a square of N x N cells (1 to 16). It also sets the fit scale and how much room each asset needs | 1 |
| Border Padding Cells | Clearance kept from walls, void and the map edge | 1 |
| Min Spacing Cells | Empty cells required between two assets **of the same rule** | 1 |
| Preserve Connectivity | Rejects any asset that would split the walkable area into more pieces | on |
| Placeable Tile Ids | Ground tile IDs this rule may place on. Empty = nothing. IDs are per map | `{1}` |
| Path Clearance Cells | Clearance kept from walkable tiles that are **not** in the list (roads, other terrain). 0 lets assets touch the edge | 1 |

## Sizing and placement

**Sizing.** With **Fit To Footprint** on, the scale is:

```
scale = (Footprint Cells x Tile Size x Fit Fill) / (mesh's widest horizontal size)
        x Scale x random Scale Variation
```

For example, with Footprint 1, Tile Size 100 and Fit Fill 0.8, the widest side of the mesh becomes 80 units. The mesh scales uniformly, so a tall tree stays in proportion. With Fit To Footprint off, the size is the mesh's own size times Scale and the random variation.

**Position.** Each mesh's bounds center goes on the center of its footprint, and its bottom sits on the ground plus **Z Offset**. Cell `(X, Y)` is centered at `(X x Tile Size, Y x Tile Size)`, the same as the tile renderer.

**Quick tuning guide**

| Goal | Change |
|---|---|
| More or fewer assets | Density |
| Bigger assets that need more room | Footprint Cells (and Fit Fill) |
| Same room, slightly larger or smaller look | Scale, or Fit Fill |
| Natural variety | Scale Variation, Random Yaw |
| Assets further from walls | Border Padding Cells |
| Assets further from each other | Min Spacing Cells |
| Assets further from roads or other tiles | Path Clearance Cells |
| A different arrangement | Seed |
| Asset floating or sunk | Z Offset |

**Example values (tune for your map).** Trees on grass: Placeable Tile Ids = grass ID, Density 0.3, Footprint 2, Border Padding 1, Min Spacing 2, Path Clearance 1. Rocks on stone: Placeable Tile Ids = stone ID, Density 0.2, Footprint 1, Path Clearance 0. Thin stone bands can't fit rocks with a clearance of 1, because the clearance rule rejects cells next to non-listed tiles.

## Replicating a layout

A layout is reproduced only if all of these match:

- the same map export and **Level To Render**
- every rule's **Settings**, including **Seed** and **Placeable Tile Ids**
- each rule's mesh, **Fit To Footprint**, **Fit Fill**, **Scale**, **Scale Variation**, **Random Yaw** and **Z Offset**
- the same version of the placer code

Assets are rebuilt every time you press Play and are not saved in the level, so record the rule values in your task doc and PR. A table copied from the Details panel is enough.

## Committing your work

- Adding rules to the actor modifies `PlayGame.umap`, a binary file that merges badly. Tell the team before committing it, and don't edit the level in parallel with someone else.
- Commit the imported meshes under `Game/Content/` (they go through LFS), and stage only what you intend. See steps 28 and 29 of the onboarding guide.
- Put your rule settings and screenshots in the task document.

## Tests

Run from **Tools > Session Frontend > Automation**, filtered by `GrantMoney.Environment`. They cover bounds and padding, spacing and overlap, seed determinism, density, and the tile allow-list. They test the placement logic only, not the rendering of meshes.

## Troubleshooting

| Symptom | Check |
|---|---|
| Nothing appears | The log says why. Check that the rule is **Enabled**, has a Mesh or Actor Class, has **Placeable Tile Ids**, and has Density above 0 |
| `no Mesh or Actor Class` in the log | Drag an asset onto the rule's Mesh field |
| `placed 0 assets` | The tile IDs don't match this level's tile table, or Path Clearance and Padding leave no room (common on thin tile bands) |
| Assets on roads | The road tile ID is in Placeable Tile Ids. Remove it |
| Rocks never appear on stone | Set Path Clearance Cells to 0, or check the stone ID |
| Assets too big or too small | Adjust Fit Fill and Scale, or turn Fit To Footprint off. Spacing is still governed by Footprint Cells |
| Assets float or sink | Adjust Z Offset. Check that the mesh isn't far from its own origin |
| Two rules' assets overlap | They share a tile ID. Give each rule its own tiles |
| `level N is not registered in MapArrays` | Register the level in `MapArrays.cpp`, or fix Level To Render |
| Assets misaligned with tiles | Tile Size must match `TileMapRenderer` (100), and the actor must sit at `(0, 0, 0)` |
| Old settings gone after updating | The older single-block settings were replaced by Rules. Re-enter them as a rule |
| Different layout on a teammate's machine | Compare every value in "Replicating a layout" and confirm both are on the same commit |

## Known limitations

- Only ground and valid cells are used. The exported `Collision` and `Deco` layers are not read.
- Rotation is yaw only, and scaling is uniform only.
- Connectivity checking only catches an asset that splits the walkable area. Rules don't see each other's assets, so a rule's check ignores other rules.
- A route that matters for gameplay but isn't encoded in the tiles isn't protected.
- Greedy packing is a good fill, not a mathematically maximal one.
- `Tile Size` is duplicated from the private value in `TileMapRenderer`.
- Fit To Footprint, Fit Fill, and the center/ground alignment apply to **Mesh** only. Actor Class assets use only Scale, Scale Variation, Random Yaw and Z Offset.
