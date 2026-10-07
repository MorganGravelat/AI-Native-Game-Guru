# Procedural Map Boundary Generator — Developer Implementation Guide

> **Status:** The code compiles (Development Editor | Win64). Editor behavior tests are listed in section 9 as pending. Nothing here claims they passed.

## 1. Objective

At the end you will have a reusable C++ helper that reads the playable-cell data of a generated map and builds a 3D wall along its exposed edges, with these properties:

- one HISM component holds every wall (no per-wall Actors);
- walls follow irregular shapes and enclose internal holes;
- walls have blocking collision, ready for a character;
- height, thickness, material and an on/off switch are editable in the Unreal Details panel;
- it works for every map registered behind your map interface, with no per-level code.

GrantMoney is the worked example. The method applies to any game that has a grid of "playable" cells and wants walls from data.

## 2. Why This System Exists

**Problem:** the renderer drew the ground only. Nothing showed where the arena ends, and nothing could contain a character.

**Alternatives considered:**

| Option | Verdict |
|--------|---------|
| Hand-place wall meshes in the level | Rejected. It breaks whenever the map changes and doesn't scale to more maps. |
| Hardcode wall coordinates for Level 1 | Rejected. It only automates one case. |
| One Actor per wall | Rejected. Hundreds of Actors are heavy. HISM draws one mesh many times. |
| Flat visible-only border (first Jira idea) | Deferred. The engineer chose real 3D walls with collision. |
| Dedicated wall Actor placed in the level | Rejected. A helper class called by the existing renderer avoids a second Actor and a second level-selection setting. |
| Polished corner and wall art | Deferred. Version 1 uses the engine Cube as a placeholder. |

**Chosen design:** derive walls from the same map data the renderer already validated, and keep the logic in a small class that does one thing.

## 3. Prerequisites

- Unreal Engine 5.8 and an existing C++ project (GrantMoney module).
- Visual Studio 2022 with the C++ game development workload.
- A working map interface that returns a map data struct. Here: `FMapArrays::GetMap(int32 Level, FGrantMoneyMapData&)`.
- A renderer Actor that already builds the ground (`ATileMapRenderer`) with a `SceneRoot` and a validated `MapData`.
- The map data, in this project:
  - `Ground[X][Y]`, 16-bit tile IDs, 0 = empty;
  - `Valid[X][Y]`, 8-bit, 0 = not part of the map;
  - `Width`, `Height` (64 x 64);
  - arrays indexed `[X][Y]`, where X is the column.
- Engine meshes `/Engine/BasicShapes/Cube.Cube` (about 100 x 100 x 100 units).
- A working baseline: Level 1 shows 2,819 tile instances and correct textures.

If your game has no map interface yet, build that first (see the Sprint2 MapArrays guide). This milestone depends on it.

## 4. Architecture Overview

```text
TileGen export (generated arrays)
        ↓
FMapArrays (stable data interface, the only file that knows generated names)
        ↓
FGrantMoneyMapData (Ground / Valid / Width / Height, no copies)
        ↓
ATileMapRenderer::BuildMap()
   ├── builds the ground (HISM per tile ID)
   └── FMapBoundaryGenerator::GenerateBoundary(...)
              ↓
        one HISM "BoundaryWalls" component (Cube mesh, BlockAll)
              ↓
        visible, solid walls
```

**Responsibilities:**

| Piece | Owns | Does not own |
|-------|------|--------------|
| `FMapArrays` | level number → arrays | rendering, walls |
| `ATileMapRenderer` | the Actor, settings, ground, calling the generator | wall geometry rules |
| `FMapBoundaryGenerator` | which cells are playable, where walls go, wall transforms, wall component | level selection, ground, materials for tiles |

**How walls are placed (version 1):**

1. A cell is playable only if it is inside the map **and** `Valid != 0` **and** `Ground != 0`. Outside the grid is never playable. This one definition is used everywhere.
2. For every playable cell, check four neighbors (X-1, X+1, Y-1, Y+1). Each non-playable neighbor gets exactly one wall piece on the shared edge. Diagonals are ignored.
3. Local position: `(X * TileSize + DX * TileSize/2, Y * TileSize + DY * TileSize/2, WallHeight/2)`. This is the same local space as the ground, so walls follow the Actor if it moves.
4. Scale (cube is about 100 units): `(WallThickness/100, (TileSize + WallThickness)/100, WallHeight/100)`. The extra length overlaps pieces at corners.
5. Edges that face X use no rotation (the wall's long axis is Y). Edges that face Y use yaw 90 (long axis is X).
6. The component uses the `BlockAll` profile with `QueryAndPhysics` collision.

## 5. AI-Assisted Planning

### AI Prompt

Use a prompt like this (the real prompt for this milestone is `BorderCreationPrompt.md`). Replace names with your own.

```text
You are working on an Unreal Engine 5.8 C++ project.

FIRST inspect the repository. Read:
- the map interface header and source (the type that returns map data),
- the Actor that renders the ground (header and source).
Do not assume file or symbol names. Do not edit generated map data.
Preserve the existing map interface and its [X][Y] indexing.

GOAL
Create a small helper class (not a new level Actor) that builds a wall
along every exposed edge of the selected map.

RULES
- A cell is playable only if it is inside the map, Valid != 0 and Ground != 0.
  Outside the map is not playable.
- For each playable cell, check four neighbours. Create one wall segment per
  non-playable neighbour. No diagonal walls and none between playable cells.
- Use the engine Cube mesh and ONE hierarchical instanced mesh component.
  Attach it to the renderer's root and use the renderer as owner.
- Use local transforms so the walls match the ground.
- Wall bottom at Z = 0; thickness perpendicular to the edge; length along it.
- Blocking collision (BlockAll).
- Settings must be UPROPERTY fields: enable toggle, height, thickness,
  optional material, in a "Boundary" category.
- Do not change the ground material, ground collision, camera, or generated files.
- Handle missing mesh, invalid map data and invalid sizes safely.
- Log a single summary line. No per-cell logging.

SCOPE EXCLUSIONS
No runtime map switching, PCG, decoration, player character, or corner meshes.

VERIFY
Tell me what you compiled and what you did NOT test. Do not claim editor
tests passed.
```

### Expected AI Output

- A new header and source file for the helper with one static function that returns the wall count.
- Small edits to the renderer header (properties) and source (load the cube, call the generator after the map is validated).
- A summary of files changed and an honest statement of build and test status.

### Human Review

Check these assumptions before accepting the output:

- The generator receives the **same** validated map data object. It does not look up the level itself.
- Arrays are still indexed `[X][Y]`, not transposed.
- Playable means `Valid` **and** `Ground`, and the helper does not modify the arrays.
- The wall component's owner is the Actor and its parent is the renderer's root.
- No generated map names appear outside the map interface file.
- The ground material, collision, textures and camera were not touched.
- Cube scale math matches the engine cube size (about 100) and wall bottoms sit at Z = 0.
- Null mesh, null parent and bad sizes return 0 instead of crashing.

## 6. Step-by-Step Implementation

1. **Protect the baseline.** *(Git Bash)* Update `main`, branch (for example `GMS-97-map-boundary-generator`), and `git status`. WHY: you need a clean point to compare against. EXPECT: no unrelated changes.
2. **Record the baseline result.** *(Unreal Editor)* Open `PlayGame`, press Play, and open Window → Output Log. Note `TileMapRenderer finished. Created 2819 tiles.` WHY: it is the number the change must not alter.
3. **Inspect the existing architecture.** *(VS Code with Copilot)* Use Prompt A (section 7). Confirm how the renderer obtains `FGrantMoneyMapData` and which side owns `SceneRoot`. WHY: the new code must reuse the existing path instead of making another one.
4. **Define "playable" and the edge rule yourself.** Write the rules from section 4 into the prompt. WHY: this is the contract. If you let AI choose, it may treat only `Valid` or only `Ground` as playable, or add diagonal walls.
5. **Ask AI to create the helper.** Use Prompt B. It should create `Public/MapBoundaryGenerator.h` and `Private/MapBoundaryGenerator.cpp`. The Build.cs file needs no change in this project because the module already depends on Engine.
6. **Review the generated helper.** Check the list in "Human Review". Key snippet (simplified, from the real file):

   ```cpp
   auto IsPlayable = [&MapData](int32 X, int32 Y) -> bool
   {
       return X >= 0 && X < MapData.Width
           && Y >= 0 && Y < MapData.Height
           && MapData.Valid[X][Y] != 0
           && MapData.Ground[X][Y] != 0;
   };
   ```

   ```cpp
   Walls->SetCollisionProfileName(TEXT("BlockAll"));
   Walls->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
   Walls->RegisterComponent();
   Walls->AddInstances(WallTransforms, false); // one batch, not one call per wall
   ```

7. **Integrate with the renderer.** *(VS Code)* In `TileMapRenderer.h` add the four `UPROPERTY` fields in the `Boundary` category plus a private cube mesh pointer. In `TileMapRenderer.cpp` load `/Engine/BasicShapes/Cube.Cube` in the constructor, and at the end of `BuildMap()` call:

   ```cpp
   if (bGenerateBoundaries)
   {
       FMapBoundaryGenerator::GenerateBoundary(
           this, SceneRoot, MapData, BoundaryCubeMesh,
           BoundaryMaterial, TileSize, WallHeight, WallThickness);
   }
   ```

   WHY at the end: the map was already validated, and a wall problem must not stop the ground from rendering.
8. **Close Unreal Editor and build.** *(Windows terminal or Visual Studio)* Close the editor first (Live Coding blocks builds, and new `UPROPERTY` fields need a full build). Then:

   ```text
   "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" GrantMoneyEditor Win64 Development -Project="<path>\Game\GrantMoney.uproject" -WaitMutex
   ```

   EXPECT: `Result: Succeeded`. Alternatively, build in Visual Studio with Development Editor | Win64.
9. **Test in the editor.** Work through section 9.
10. **Review third-party feedback critically.** Someone supplied four review claims about the renderer. Evaluate each against the engine source, not just the claim (see section 8, Mistake 1). Fix only what is confirmed.
11. **Apply the confirmed fix.** In `TileMapRenderer.cpp`, collect transforms in a `TMap<int32, TArray<FTransform>>` and call `AddInstances` once per tile group after the loop. In the generator, build one `TArray<FTransform>` and make one `AddInstances` call.
12. **Rebuild and re-test.** The numbers and look must be the same as step 2.
13. **Stage only milestone files.** See the Source-Control Checkpoint.

## 7. AI Prompts by Stage

### Prompt A — Inspect Existing Architecture

```text
Inspect these files and report only facts: <map interface header/source>,
<renderer header/source>. Tell me (1) how the renderer gets map data,
(2) the array types and indexing, (3) which component owns the tiles,
(4) the constant used for tile spacing. Do not edit anything.
```

### Prompt B — Implement the Core System

```text
Create <HelperName>.h/.cpp with one static function that builds walls on the
exposed edges of a map. Use my playable definition and edge rule (pasted
below). One instanced mesh component, local transforms, blocking collision,
return the wall count. Do not touch the ground code or generated files.
<paste rules>
```

### Prompt C — Add Validation

```text
Review <HelperName>.cpp. List every way it could crash or misbehave with:
null owner/parent/mesh, invalid map data, zero or negative sizes, an empty map,
a level that does not exist. Add the smallest checks needed and log one clear
error for each. Do not add per-cell logging.
```

### Prompt D — Debug the Result

```text
Here is the Output Log and a description of what I see: <paste>. Expected:
<expected>. List the 3 most likely causes in order, and for each tell me
the single check that would confirm or rule it out. Do not change code yet.
```

### Prompt E — Refactor After Verification

```text
Here is a review claim: "<claim>". Check it against the engine source for
UE 5.8 (quote the function you read) and tell me whether it is true for this
code. If true, propose the smallest fix. If false, explain why and change
nothing.
```

## 8. Common Mistakes

### Mistake 1 — Adding instances one at a time (observed in review)

**Symptom:** none visible in a small test. The review flagged that large maps could load slowly. No timing was measured.

**Likely cause:** the first version called `AddInstance` for every tile and every wall. In UE 5.8, `UHierarchicalInstancedStaticMeshComponent::AddInstance` calls `BuildTreeIfOutdated` after each insert unless `bAutoRebuildTreeOnInstanceChanges` is off, so the spatial tree can be rebuilt many times.

**Fix:** collect transforms first, then call `AddInstances` once per component. (The review also suggested `BatchUpdateInstancesTransforms`. That function updates existing instances, so it is the wrong tool here.)

**Lesson:** verify a suggested fix against the engine source. Part of a plausible claim can be right and part wrong.

### Mistake 2 — Building while the editor is open (observed in the earlier map-selection milestone)

**Symptom:** `Unable to build while Live Coding is active`, result `OtherCompilationError`.

**Likely cause:** the editor was running.

**Fix:** close the editor and build again. This milestone's builds ran with the editor closed.

**Lesson:** adding `UPROPERTY` fields or new files needs a full build, not Live Coding.

### Mistake 3 — Wrong or missing wall material usage flag (known project risk, not observed in this milestone)

**Symptom:** the walls show Unreal's grey checkerboard default material.

**Likely cause:** the project already hit `Material missing usage flag InstancedStaticMeshes` with the tile material. A custom wall material has the same requirement.

**Fix:** open the material and enable **Used with Instanced Static Meshes**.

**Lesson:** any material used on a HISM component needs that flag.

### Mistake 4 — Gaps at corners (anticipated design issue, not a recorded bug)

**Symptom:** small notches where two wall pieces meet at a convex corner.

**Likely cause:** pieces that are exactly one tile long only touch at their ends.

**Fix:** the implementation makes each piece `TileSize + WallThickness` long so neighbors overlap. This is a deliberate deviation from "exactly TileSize" in the original prompt, and it was disclosed when delivered.

**Lesson:** when AI deviates from the spec for a good reason, write the deviation down.

### Review claims that were not bugs

- **Dynamic materials could be garbage collected:** not true here. Each material is referenced by its component, which is owned by the Actor.
- **Soft pointers for texture loading:** a valid idea in general, but `LoadObject` runs once per tile type before gameplay, and the project has three tile textures. Revisit it if the tile count grows or runtime level switching arrives.
- **X/Y axis alignment:** with the current top-down camera, world X points up the screen, so the layout may look rotated compared with TileGen's preview. The camera is moving to third person, and the grid-to-world mapping matches Unreal's axes (X forward, Y right). No change was made.

## 9. Verification

| Test | Action | Expected Result | Status |
|------|--------|-----------------|--------|
| Compile | Build Development Editor | `Result: Succeeded` | **Done** (twice: after the generator, and after batching) |
| Level 1 regression | Level To Render = 1, Play, check log | `Created 2819 tiles`, correct textures, no grey checkerboard | Pending |
| Walls appear | Simulate (Alt+S) and fly around, or view from an angle | Continuous walls along the outer edge of the valid area | Pending |
| No interior walls | Look at adjacent playable cells | No walls between them | Pending |
| Internal holes | Use a map with an empty region inside | Walls surround the hole | Pending |
| Corners | Inspect convex and concave corners | No visible gaps | Pending |
| Wall log | Check Output Log | `MapBoundaryGenerator: Created N wall segments.` | Pending |
| Collision | Console: `show collision` (character test needs a future Pawn) | Solid walls visible | Pending |
| Toggle off | Uncheck Generate Boundaries, Play | No walls; ground unchanged | Pending |
| Settings | Change Wall Height / Thickness, Play | Walls resize | Pending |
| Level 2 | Level To Render = 2, Play | Walls match Level 2's shape | Pending |
| Invalid level | Level To Render = 999, Play | Error in log, no tiles, no walls, no crash | Pending |
| Custom material | Assign a material with the instancing flag | Material shows on walls | Pending |
| Batching effect | Compare load times before/after | Not measured | Pending |
| Packaged build | Package and run | Same as editor | Not planned |

Only the compile row has evidence. Mark the others only after you have observed them.

## 10. Known Limitations

- Walls are plain engine cubes with a default material.
- Corners use overlap only, with no special pieces.
- Walls are centered on the tile edge, so half the thickness (10 units by default) extends into the playable cell.
- A cell counts as playable if `Valid` and `Ground` are nonzero. The ground renderer additionally skips tile IDs with no usable texture path, so such a cell would get walls but no ground tile.
- The map and walls are built once in `BeginPlay`. The wall component has a fixed name, so calling the build twice on the same Actor is not supported (runtime map switching is future work).
- Wall collision is untested with a real character because none exists yet.
- The generated `Deco` and `Collision` arrays are not used.
- Maps must fit the 64 x 64 `FGrantMoneyMapData`.
- No navigation or navmesh integration.

## 11. Extension Ideas

The pattern is "generated playable mask → edge detection → instanced geometry":

- **Dungeon rooms:** walls on room outlines; add door gaps by marking those cells playable.
- **Racing tracks:** curbs or barriers along the track edge.
- **RTS territory:** border markers where one owner's cells meet another's.
- **Arena games:** the same walls as invisible blockers with a visible rim.
- **Survival biomes:** cliffs or water edges where biome type changes (compare neighbor type instead of playable/non-playable).
- **Better art:** replace the cube with modular wall and corner meshes by passing the mesh in (already an argument).

## 12. Source-Control Checkpoint

- **Jira:** GMS-97 (map boundary). Placeholder for yours: `GMS-XX`.
- **Branch suggestion:** `GMS-97-map-boundary-generator`.
- **Changed files:** `MapBoundaryGenerator.h/.cpp` (new); `TileMapRenderer.h/.cpp` (modified); `.gitignore` (editor-local ignores).
- **Staging:**

  ```bash
  git add Game/Source/GrantMoney/Public/MapBoundaryGenerator.h
  git add Game/Source/GrantMoney/Private/MapBoundaryGenerator.cpp
  git add Game/Source/GrantMoney/Public/TileMapRenderer.h
  git add Game/Source/GrantMoney/Private/TileMapRenderer.cpp
  git status
  git diff --cached --stat
  git diff --cached
  ```

- **Commit message:** `GMS-97: add procedural map boundary generator`
- **PR title:** `GMS-97: Add procedural map boundary walls and batch HISM instance creation`
- **Checklist before merging:**
  - [ ] Build succeeds (Development Editor | Win64).
  - [ ] Level 1 still creates 2,819 tiles.
  - [ ] Walls visible and blocking.
  - [ ] Toggle off restores the old map.
  - [ ] Level 999 logs an error and doesn't crash.
  - [ ] No generated map files, `Binaries/`, `Intermediate/`, `Saved/` or unrelated assets staged.
