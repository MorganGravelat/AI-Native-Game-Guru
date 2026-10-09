# VIDEO SCRIPT — Procedural Map Boundary Generator
## Module 1 — Building a World

> **Script status:** Written from the work that actually happened. Compile results are real. Editor test results are marked **record on camera** because no editor test result was captured. Report only what you see when you record.

# Opening

### SAY

Welcome back to Grant Money. At this point our generated map renders in Unreal. A tile-map renderer asks a small map interface for the selected level, and builds the ground with instanced meshes.

But the ground just stops. Nothing shows the player where the arena ends, and nothing can keep a character inside it.

In this part we will automatically build walls from the map data. The exact game, map size, and art style do not matter. The reusable idea is this: take a data mask that says where play is allowed, find its edges, and turn those edges into world geometry.

We will let AI do the repetitive C++ work. We will make the design decisions, and we will check what it did.

# Acceptance Criteria

### SAY

At the end of this part:

- a boundary helper class exists and compiles;
- the renderer calls it after the map is validated;
- the Details panel has a Boundary section with an on/off toggle, wall height, wall thickness and an optional material;
- walls appear only on exposed edges of the playable area, including around holes inside it;
- the walls have blocking collision;
- the ground still produces the same number of tiles as before;
- turning the toggle off gives the old map back;
- an invalid level does not crash the game.

I will tell you honestly which of these I have verified on camera, and which are still pending.

# Engineering Principle

### SAY

Here is the principle for this part:

**Derive world geometry from data, behind a stable interface.**

If I place walls by hand, they are wrong the moment the map changes. If I hardcode Level 1's coordinates, I have automated exactly one case. If the wall builder reads the same validated map object the renderer uses, every future map gets correct walls for free.

# Prerequisites

### SAY

Before starting, make sure the project builds, `PlayGame` opens, and the map renders with its textures. You should know your baseline: for this project, Level 1 logs 2,819 tiles.

We also need a map interface that hands us the arrays. Here that is `FMapArrays`. The wall builder will not know any generated file names. If your project does not have this boundary yet, build that first.

## Project-Specific Example

This GrantMoney implementation uses TileGen, 64 x 64 maps, a `Ground` array of 16-bit tile IDs, a `Valid` array of 8-bit flags, and indexing `[X][Y]`.

## Reusable Engineering Method

The broader pattern is: generated data → stable interface → Unreal runtime system → verified output. Your generator, map size and class names can all be different.

---

# Step 1 — Protect the Working Baseline

### SAY

Before changing anything, I want a clean branch and a number I can compare against.

### SHOW

Git Bash, then the Unreal Output Log.

### DO

```bash
git checkout main
git pull
git lfs pull
git status
git checkout -b GMS-97-map-boundary-generator
```

Replace the ticket number with yours. Open `PlayGame`, press Play, and filter the Output Log for `TileMapRenderer`.

### AI PROMPT

No AI is needed for this step.

### VERIFY

You are on the new branch, the working tree is clean, and the log shows `Created 2819 tiles.` for Level 1. Write that number down.

---

# Step 2 — Make AI Inspect Before It Writes

### SAY

The first prompt to AI contains no instruction to write code. I want facts: how the renderer gets its data, the array types, and how tiles are positioned.

### SHOW

VS Code with Copilot chat open, and `MapArrays.h` and `TileMapRenderer.cpp` visible.

### DO

Paste the prompt below.

### AI PROMPT

```text
Inspect these files and report only facts: MapArrays.h, MapArrays.cpp,
TileMapRenderer.h, TileMapRenderer.cpp. Tell me:
1. how the renderer gets map data,
2. the array types and indexing convention,
3. which component the tile components attach to,
4. the constant used for tile spacing.
Do not edit anything.
```

### VERIFY

Check the answer against the files. You should see `FGrantMoneyMapData` with `Ground` (16-bit) and `Valid` (8-bit) pointers, `[X][Y]` indexing, attachment to `SceneRoot`, and `TileSize = 100`. If the answer invents a name, correct it before continuing.

---

# Step 3 — Write the Contract Yourself

### SAY

Now the part AI should not decide for us. What is a playable cell? Where does a wall go? Which direction is it facing?

Here are my decisions.

A cell is playable only if it is inside the map, `Valid` is not zero, and `Ground` is not zero. Anything outside the map is not playable.

For each playable cell I check four neighbors. For every neighbor that is not playable I create exactly one wall on the shared edge. No diagonals. Nothing between two playable cells.

Walls are 3D cubes for now. They sit on the ground, use blocking collision, and use the same local coordinates as the ground so they move with the Actor.

### SHOW

A small diagram on screen:

```text
. . . . .
. P P P .      P = playable cell
. P . P .      . = not playable
. P P P .      wall on every P/. edge, including the hole
. . . . .
```

### DO

Write the rules into a text file or the prompt for Step 4.

### AI PROMPT

No AI is needed for this step.

### VERIFY

Make sure someone else could implement the rules from your text alone.

---

# Step 4 — Ask AI for the Bounded Implementation

### SAY

Now we ask for the code. The prompt is long because it is a contract: what to preserve, what to build, what to leave alone, and how to report.

### SHOW

The prompt document in the editor, then the Copilot chat.

### DO

Paste the prompt (this is the milestone's actual prompt in condensed form):

### AI PROMPT

```text
Inspect the existing source first. Do not assume names. Preserve the map
interface and [X][Y] indexing. Do not edit generated map data.

Create Public/MapBoundaryGenerator.h and Private/MapBoundaryGenerator.cpp: a
small helper class, not a new level Actor, with a function equivalent to
GenerateBoundary(Owner, ParentComponent, MapData, BoundaryMesh,
BoundaryMaterial, TileSize, WallHeight, WallThickness) that returns the
number of walls created.

Playable = inside the map AND Valid != 0 AND Ground != 0. Outside the map is
not playable. For each playable cell, create one wall per non-playable
cardinal neighbour. No diagonal walls, none between playable cells.

Use the engine Cube mesh and ONE hierarchical instanced mesh component, owned
by the renderer and attached to its SceneRoot, with local transforms matching
the ground. Wall bottom at Z=0, centre at WallHeight/2. Scale from the 100-unit
cube. X-facing edges run along Y; Y-facing edges use yaw 90. Use BlockAll collision.

In the renderer add UPROPERTY fields in a "Boundary" category:
bGenerateBoundaries=true, WallHeight=300, WallThickness=20, optional
BoundaryMaterial. Call the generator once at the end of BuildMap, after the
map is validated, using the same MapData. Do not change M_Tile, ground
collision, textures, or the camera. Handle missing meshes, invalid data and
invalid sizes safely. Log one summary line.

Do not implement runtime switching, PCG, decoration, a player character or
corner meshes. Report what you compiled and what you did not test.
```

### VERIFY

AI should report two new files and edits in `TileMapRenderer.h/.cpp` only. If it touched `MapArrays`, generated map files or the camera, stop and ask why.

---

# Step 5 — Review What AI Wrote

### SAY

Before building, we read the output. I am looking for five things.

First, the playable test. It should check bounds first, then `Valid`, then `Ground`.

Second, the neighbor loop. Four neighbors, one wall each.

Third, the transform. Wall center at `WallHeight / 2`, so the bottom sits at zero. Scale is thickness over 100, length over 100, height over 100, because the engine cube is about 100 units.

Fourth, ownership. The wall component's owner is the Actor, and it attaches to `SceneRoot`.

Fifth, safety. A null mesh, a null parent or a bad size must return zero, not crash.

### SHOW

`MapBoundaryGenerator.cpp`, scrolling to the `IsPlayable` lambda and the wall transform code.

### DO

Highlight these lines:

```cpp
auto IsPlayable = [&MapData](int32 X, int32 Y) -> bool
{
    return X >= 0 && X < MapData.Width
        && Y >= 0 && Y < MapData.Height
        && MapData.Valid[X][Y] != 0
        && MapData.Ground[X][Y] != 0;
};
```

### AI PROMPT

Optional validation prompt:

```text
Review MapBoundaryGenerator.cpp. List every way it could crash or misbehave
with null owner/parent/mesh, invalid map data, zero or negative sizes, an
empty map, or a level that does not exist. Propose only the smallest checks
needed. Do not add per-cell logging.
```

### VERIFY

Look for one disclosed deviation: each wall is `TileSize + WallThickness` long, not exactly `TileSize`. The AI did that so perpendicular pieces overlap at corners. It is acceptable for version 1, but it is a deviation from what we wrote, so note it.

---

# Engineering Discussion

### SAY

Here is an engineering discussion.

It would be tempting to hand-place walls around the map, or to ask AI for coordinates for Level 1. Both work once.

The data-driven approach works for every map. The generator receives the same validated `MapData` the renderer uses. If you register a Level 2 in the map interface, the walls for Level 2 appear without touching the wall code.

We also chose a helper class instead of a second Actor. A second Actor would need its own level-selection setting, and the two could easily disagree about which map to draw.

---

# Step 6 — Close the Editor and Build

### SAY

We added new files and new `UPROPERTY` fields. That means a full build with the editor closed. If the editor is open, Live Coding blocks the build.

### SHOW

A terminal or Visual Studio build output.

### DO

Close Unreal Editor, then:

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" GrantMoneyEditor Win64 Development -Project="<path>\Game\GrantMoney.uproject" -WaitMutex
```

### AI PROMPT

If the build fails, use this:

```text
Here is the compiler output: <paste>. Explain each error in plain words, name
the line, and give the smallest fix. Do not refactor anything else.
```

### VERIFY

For this project, the build succeeded after the generator was added: `Result: Succeeded`. Record your own result on camera.

---

# Step 7 — Test in the Editor

### SAY

Compiling proves the code is valid, not that it behaves correctly. Now we test, starting with the regression.

### SHOW

Unreal Editor, `PlayGame`, the Details panel for `TileMapRenderer`, and the Output Log.

### DO

1. Reopen the project. Select `TileMapRenderer`.
2. In Details, find the **Boundary** section: Generate Boundaries, Wall Height, Wall Thickness, Boundary Material.
3. Press Play. Check the log for `Created 2819 tiles.` and `MapBoundaryGenerator: Created N wall segments.`
4. Press Alt+S (Simulate) and fly around to look at the walls from the side.
5. Type `show collision` in the console.
6. Uncheck Generate Boundaries and Play again.
7. Set Level To Render to 2 and Play. Then 999 and Play. Restore it to 1.

### AI PROMPT

No AI is needed. If something is wrong, use this:

```text
Here is the Output Log and what I see: <paste>. Expected: <expected>. Give me
the 3 most likely causes in order and, for each, the single check that would
confirm or rule it out. Do not change code yet.
```

### VERIFY

**Record on camera.** The checks are: tile count unchanged, walls only on exposed edges, no gaps at corners, collision shows, toggle off removes walls, level 2 walls follow level 2, level 999 logs an error and does not crash. Only say "it works" for what you actually saw.

---

# Step 8 — Evaluate an Outside Review

### SAY

After the first version, a colleague gave us four review claims about the renderer. This is a good exercise: we do not accept or reject them by feeling.

### SHOW

The review text beside the renderer source.

### DO

For each claim, ask AI to check the engine source.

### AI PROMPT

```text
Claim: "Calling AddInstance for each tile triggers internal bounds updates;
use AddInstances or BatchUpdateInstancesTransforms."
Check this against the UE 5.8 engine source. Quote the functions you read.
Tell me which part is true for my code, and propose the smallest fix.
If a claim is false, say why and change nothing.
```

### VERIFY

The engine source showed `UHierarchicalInstancedStaticMeshComponent::AddInstance` calls `BuildTreeIfOutdated` after each insert unless auto-rebuild is turned off. So the first claim is true. `BatchUpdateInstancesTransforms` edits existing instances, so `AddInstances` is the right fix.

The other three: dynamic materials are referenced by their component, so they are not garbage collected; soft pointers for textures are not worth it for three textures loaded once; and the axis question depends on camera choice, which is moving to third person, so we leave the mapping alone.

---

# Step 9 — Apply the Fix and Re-Test

### SAY

We fix only what we confirmed. The ground loop now collects transforms in a map from tile ID to an array of transforms, then calls `AddInstances` once per tile group. The wall generator does the same for its single component.

### SHOW

The diff for `TileMapRenderer.cpp` and `MapBoundaryGenerator.cpp`.

### DO

Apply the change, close the editor, rebuild, reopen, and repeat Step 7.

### AI PROMPT

```text
Change only the instance creation: collect FTransforms per tile ID and call
AddInstances once per HISM component after the loop. Do not change the
transforms, tile counts, materials or logging.
```

### VERIFY

For this project the build succeeded again. On camera, confirm the tile count and the look are identical to Step 7. We did not measure load time, so do not claim it got faster.

---

# Common Mistakes

### SAY

Here are some common mistakes.

First, building while the editor is open. Live Coding blocks the build and gives an error. Close the editor.

Second, trusting a review claim without checking it. One of the four claims was a real performance problem, one suggested the wrong function, and two were not bugs.

Third, a custom wall material without **Used with Instanced Static Meshes**. You get a grey checkerboard.

Fourth, wall pieces that are exactly one tile long. Corners can show small notches. We overlap the pieces on purpose.

Fifth, treating "playable" differently in different places in code. We defined it once and use it everywhere.

---

# Verification Checkpoint

### SAY

Now let's verify our result.

### SHOW / VERIFY

Go through this list, ticking only what you saw:

```text
[ ] Build succeeded (Development Editor | Win64)
[ ] Output Log: TileMapRenderer finished. Created 2819 tiles.
[ ] Output Log: MapBoundaryGenerator: Created N wall segments.
[ ] Ground textures correct, no grey checkerboard
[ ] Walls only on exposed edges, none between playable cells
[ ] Walls around internal holes (use a map that has one)
[ ] No visible corner gaps
[ ] show collision displays the walls
[ ] Wall height / thickness changes take effect
[ ] Generate Boundaries off = no walls
[ ] Level 2 walls match Level 2
[ ] Level 999: error in log, no crash
[ ] Camera unchanged
```

Not yet testable: collision against a character (none exists), packaged-build behavior, and measured load-time improvement.

---

# AI Review

### SAY

Let's review what AI actually did here.

AI inspected the code, wrote the helper and the renderer integration, and compiled the project. It also confirmed a review claim by reading engine source, and then applied a batching fix. It disclosed one deviation: walls overlap at corners.

The human decisions were the scope, the playable-cell definition, the 3D walls with blocking collision, the cube as a placeholder, the choice of a helper class over another Actor, and what to do about the review claims.

What failed: nothing failed to compile in this milestone. The first draft had a performance pattern worth fixing.

How it was verified: by compiling twice and reading the engine source. Everything else is verified only when you run the editor tests.

---

# Engineering Pause

### SAY

Here is an engineering pause.

Consider this question: **if our wall builder only works for Level 1, did we build a reusable system, or just automate one hardcoded case?**

We built a reusable system only if a new map gets correct walls without touching the wall code. That is why the generator takes the same map data object as the renderer, and why the first thing we should test after Level 1 is Level 2.

---

# Completion Criteria

### SAY

This milestone is complete when:

- the helper and renderer changes compile;
- Level 1 still creates the same number of ground tiles;
- walls appear on exposed edges only;
- the walls block, as shown by collision;
- the toggle, height and thickness work from the Details panel;
- Level 2 and an invalid level behave correctly;
- batching has been applied and rechecked;
- only the milestone files are staged.

If you have not run the editor tests yet, the milestone is code-complete but not verified.

# Next Milestone

### SAY

Next, a third-person camera and character will use these walls. We will test collision with a real character and replace the placeholder cubes with modular wall art.

We will not do that in this part.

---

# Suggested Screenshots

| Filename | What to capture |
|----------|-----------------|
| `boundary_01_baseline_log.png` | Output Log showing `Created 2819 tiles.` before the change |
| `boundary_02_copilot_inspect.png` | Copilot inspection answer |
| `boundary_03_details_boundary_section.png` | Details panel with the Boundary category |
| `boundary_04_build_succeeded.png` | Terminal showing `Result: Succeeded` |
| `boundary_05_walls_simulate_angle.png` | Walls seen from an angle in Simulate mode |
| `boundary_06_show_collision.png` | `show collision` outline of the walls |
| `boundary_07_wall_log.png` | Output Log with `Created N wall segments.` |
| `boundary_08_toggle_off.png` | Map with Generate Boundaries unchecked |
| `boundary_09_level2_walls.png` | Level 2 with its own boundary |
| `boundary_10_invalid_level_error.png` | Output Log error for Level 999 |
| `boundary_11_review_claim_check.png` | AI checking the `AddInstance` claim against engine source |
