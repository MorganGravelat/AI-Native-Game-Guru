# OUTPUT A — Milestone Change Report
## Procedural Map Boundary Generator (GMS-97)

## Milestone Summary

**Before:** `ATileMapRenderer` drew the ground of the selected TileGen map (Level 1 produces 2,819 tile instances) and nothing else. Nothing marked where the playable area ended and nothing could stop a character from leaving it.

**After:** a small helper, `FMapBoundaryGenerator`, builds a 3D wall along every exposed edge of the selected map. It reads the same `FGrantMoneyMapData` the renderer already uses, so every level registered in `MapArrays` automatically gets walls that match its own shape, including irregular outlines and internal holes.

**Why it matters architecturally:**

- The walls are derived from data. There are no hardcoded coordinates and no per-level code.
- The renderer still does not know any generated map name. It hands the same `MapData` object to the generator.
- The wall builder is a separate class, not another level Actor, so it can be reused or replaced with polished art later.
- Walls use blocking collision so a future third-person character can be kept inside the arena.

A review of the first implementation also produced one performance fix. Ground tiles and walls were added to their HISM components one at a time, which rebuilt the instance tree on every insert in UE 5.8. Both now use one batched `AddInstances` call per component.

## Files Created

| File | Purpose |
|------|---------|
| `Game/Source/GrantMoney/Public/MapBoundaryGenerator.h` | Declares `FMapBoundaryGenerator::GenerateBoundary(...)`, which returns the number of wall instances created. |
| `Game/Source/GrantMoney/Private/MapBoundaryGenerator.cpp` | Playable-cell test, exposed-edge detection, wall transforms, HISM component creation, collision and logging. |
| `docs/course/Module1-BuildingAWorld/Sprint3/BorderCreationPrompt.md` | The scoped Copilot prompt that defined this milestone (written by the engineer). |
| `docs/course/Module1-BuildingAWorld/Sprint3/MapBoundaryGuide.md` | Plain-language end-user guide: what was added and how to use it. |
| `docs/course/Module1-BuildingAWorld/Sprint3/MapBoundary_ChangeReport.md` | This report. |
| `docs/course/Module1-BuildingAWorld/Sprint3/MapBoundary_DeveloperGuide.md` | Reproducible implementation guide. |
| `docs/course/Module1-BuildingAWorld/Sprint3/MapBoundary_Script.md` | Instructor recording script. |

## Files Modified

| File | What Changed | Why |
|------|--------------|-----|
| `Game/Source/GrantMoney/Public/TileMapRenderer.h` | Added `bGenerateBoundaries` (true), `WallHeight` (300), `WallThickness` (20) and `BoundaryMaterial` as `UPROPERTY` fields under the **Boundary** category (height, thickness and material grey out when the toggle is off). Added a private `BoundaryCubeMesh` pointer. | Let designers control the walls in the Details panel with no code changes. |
| `Game/Source/GrantMoney/Private/TileMapRenderer.cpp` | Includes `MapBoundaryGenerator.h`. Loads `/Engine/BasicShapes/Cube.Cube` in the constructor. At the end of `BuildMap()`, after the tile-count log, calls `GenerateBoundary` with the same `MapData`. Ground instances are now collected per tile ID and added with one `AddInstances` call per group. | Integrate the generator after the map was validated; remove the per-tile HISM tree rebuild. |
| `.gitignore` | Added ignore rules for `.vscode/`, `*.code-workspace` and `**/.ignore`. | Keep local editor files out of the repository. (Included in the branch's commit `7f62734`.) |

## Files Not Changed

| System | Status |
|--------|--------|
| TileGen application and exported arrays (`Private/Maps/*`) | Untouched. |
| `MapArrays.h` / `MapArrays.cpp` | Untouched. Its API and the `[X][Y]` indexing were preserved. |
| `M_Tile`, `TileTexture` parameter, ground texture loading | Untouched. The walls do not use `M_Tile`. |
| Ground tile collision | Untouched (still `NoCollision`). |
| Camera configuration and `PlayGame.umap` | Untouched. |
| Imported textures, packaging settings | Untouched. |

Nothing was staged or committed by the AI. Git history shows the work landed in three commits (`5faafa9`, `7f62734`, `b799c83`) and was merged by PR #7 (`44fb914`).

## Git Staging Recommendation

The code is already merged, so these commands are for the documentation files. They are also the record of what belongs to this milestone if someone reproduces it. Run `git status` first. The current working tree also contains **unrelated** camera-milestone files (`GrantMoneyGameMode`, `MapCameraPawn`, `MapCameraPlayerController`, `Content/Input/`, `DefaultEngine.ini`, `PlayGame1.umap` and the `ThirdPersonCamera_*` documents). Do **not** stage those with this milestone, and do not stage `Binaries/`, `Intermediate/`, `Saved/` or `DerivedDataCache/`.

```bash
# Source (already merged via PR #7; listed for reference)
git add Game/Source/GrantMoney/Public/MapBoundaryGenerator.h
git add Game/Source/GrantMoney/Private/MapBoundaryGenerator.cpp
git add Game/Source/GrantMoney/Public/TileMapRenderer.h
git add Game/Source/GrantMoney/Private/TileMapRenderer.cpp

# Documentation for this milestone
git add docs/course/Module1-BuildingAWorld/Sprint3/BorderCreationPrompt.md
git add docs/course/Module1-BuildingAWorld/Sprint3/MapBoundaryGuide.md
git add docs/course/Module1-BuildingAWorld/Sprint3/MapBoundary_ChangeReport.md
git add docs/course/Module1-BuildingAWorld/Sprint3/MapBoundary_DeveloperGuide.md
git add docs/course/Module1-BuildingAWorld/Sprint3/MapBoundary_Script.md

git status
git diff --cached --stat
git diff --cached
```

Suggested commit for the new documentation:

```text
GMS-97: add change report, developer guide and recording script for map boundary generator
```

Original milestone branch and PR title:

```text
GMS-97-map-boundary-generator
GMS-97: Add procedural map boundary walls
```

If you are currently on another branch, create the documentation commit on a branch dedicated to it (for example `GMS-97-boundary-docs`) so it is not mixed with the camera work.

## Verification Status

**Performed (evidence exists):**

- `Build.bat GrantMoneyEditor Win64 Development` succeeded after the generator was added (editor closed, no errors).
- The same build succeeded again after the `AddInstances` batching fix.
- The engine source for UE 5.8 was read to confirm that `UHierarchicalInstancedStaticMeshComponent::AddInstance` calls `BuildTreeIfOutdated` after each insert unless auto-rebuild is disabled.

**Pending. No test result is recorded for any of these, so do not treat them as passed:**

- Level 1 still logs `Created 2819 tiles.` and shows correct textures with no grey checkerboard.
- Walls appear around the outline of Level 1 and Level 2 with no gaps at corners.
- No walls appear between adjacent playable cells.
- `BlockAll` collision works (`show collision`, or a future character).
- Unchecking **Generate Boundaries** restores the wall-free map.
- `Level To Render = 999` logs an error and builds nothing, without a crash.
- Wall height and thickness changes take effect.
- A custom `BoundaryMaterial` with **Used with Instanced Static Meshes** displays correctly.
- Measured load-time improvement from batching (not measured at all).
- Packaged-build behavior.
