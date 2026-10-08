# Third-Person / Free-Roaming Map Camera — Editor Preparation and Implementation Guide

> **Status:** Preparation and implementation of the free-roaming perspective camera are complete for the current prototype (no player character yet).
>
> **Verified (evidence exists):** the C++ project builds (Unreal Build Tool, `GrantMoneyEditor Win64 Development`); the startup chain `BP_GrantMoneyGameMode → MapCameraPlayerController → MapCameraPawn` was confirmed by reading the saved assets and by a headless launch of `PlayGame1`; the developer played the camera in the editor and reported that movement, mouse look, zoom, walking-height placement and map-edge limits work and feel good.
>
> **Not verified:** the follow-target feature (no player pawn exists to follow yet), gamepad input, packaged builds, camera/player collision, and numeric tuning of mouse sensitivity beyond "feels good" in the editor.
>
> Sections 1–7 are the preparation (written before the code). **Section 8 records what was actually built, tuned and corrected.** Sections 9–14 contain the prompts, common mistakes, verification table, limitations and source-control notes updated with real results.

## 1. Objective

This guide prepares an Unreal C++ project for an AI-assisted camera implementation.

At the end of this preparation stage, the project should have:

- Enhanced Input assets for camera movement, rotation, look pitch, and zoom;
- an Input Mapping Context that maps WASD and mouse controls;
- C++ class skeletons for a camera Pawn, PlayerController, and GameMode;
- a predictable PlayerStart location;
- a defined camera style and tuning targets;
- defined movement boundaries derived from the current generated world;
- the existing temporary Camera Actor retained until the new camera is proven;
- no unnecessary collision or player-character systems added early.

This guide prepares the project **before Copilot writes the behavior**, and then records the implementation that followed (Section 8 onward).

After the implementation stage the project also has:

- a `AMapCameraPawn` rig (`SceneRoot → SpringArm → Camera`, perspective) driven by the existing Enhanced Input assets;
- `AMapCameraPlayerController` registering `IMC_MapCamera` for the local player;
- `BP_GrantMoneyGameMode`, a data-only Blueprint child of the C++ GameMode, that selects the Pawn and Controller;
- walking-height placement above the ground plane;
- X/Y movement limits derived from the playable cells of the map the renderer is showing;
- an optional follow target so a future player pawn can drive the camera.

## 2. Why This System Exists

The current world renderer creates the generated TileGen map at runtime. The camera should be a separate system.

A tempting shortcut would be to place another Camera Actor into the level and manually adjust it. That is useful for debugging, but it is not a reusable gameplay camera architecture.

The reusable architecture is:

```text
Enhanced Input Assets
        ↓
PlayerController
        ↓
Camera Pawn
        ↓
Spring Arm + Camera Component
        ↓
Generated World
```

The TileGen renderer stays responsible for world presentation. The camera stays responsible for view, navigation, zoom, and movement limits.

This separation matters because the camera will eventually be replaced or adapted when the project moves from free-map navigation to a player-follow relationship.

## 3. Prerequisites

Before beginning, confirm:

- the Unreal C++ project opens successfully;
- `PlayGame1` is the gameplay level (`Content/Maps/PlayGame1`; see the note in Step 1 about the `PlayGame` redirector);
- the project compiles;
- the dynamic TileGen map renderer works;
- the map is generated from registered map data;
- the tile material works;
- the current map uses the existing 64×64 TileGen layout;
- current tile spacing is 100 Unreal units per tile;
- the temporary orthographic Camera Actor still works;
- you are on a feature branch rather than developing directly on `main`.

### Recommended branch setup

**Where:** Git Bash

```bash
git checkout main
git pull
git lfs pull
git status
git checkout -b GMS-XX-map-camera-controller
```

Replace `XX` with the real Jira ticket.

## 4. Architecture Overview

### GrantMoney example

```text
PlayGame1
   |
   +-- TileMapRenderer
   |      ↓
   |   Generated World
   |
   +-- PlayerStart
          ↓
   GrantMoneyGameMode
          ↓
   MapCameraPlayerController
          ↓
   MapCameraPawn
          ↓
   SpringArm
          ↓
   CameraComponent
```

### Reusable engineering method

The same pattern can be used in another game:

```text
Input Assets
    ↓
Input Owner / Controller
    ↓
Camera Pawn or Character
    ↓
Camera Rig
    ↓
World Constraints
```

The class names do not matter. The responsibility boundaries do.

## 5. AI-Assisted Planning

### AI Prompt

Use this **after** completing the editor setup in Section 6:

```text
Inspect the existing Unreal C++ project before modifying anything.

Goal:
Implement a perspective free-roaming map camera using the editor assets and
C++ class skeletons that already exist.

Current editor assets:
- IA_CameraMove (Axis2D)
- IA_CameraTurn (Axis1D)
- IA_CameraLookUp (Axis1D)
- IA_CameraZoom (Axis1D)
- IMC_MapCamera

Current C++ class skeletons:
- AMapCameraPawn
- AMapCameraPlayerController
- AGrantMoneyGameMode

Requirements:
1. Preserve TileMapRenderer and MapArrays unchanged.
2. Keep gameplay camera/input in C++; no Blueprint gameplay.
3. AMapCameraPawn should use a SceneRoot, SpringArmComponent, and CameraComponent.
4. Perspective camera.
5. WASD movement should move across the map plane.
6. Mouse X rotates yaw.
7. Mouse Y adjusts pitch.
8. Clamp pitch to a sensible range so the camera cannot flip.
9. Mouse wheel changes spring-arm TargetArmLength for zoom.
10. Clamp zoom to a safe range.
11. Use frame-rate-independent movement.
12. Clamp camera Pawn X/Y movement to configurable world bounds.
13. Initial GrantMoney test bounds are approximately X/Y 0..6300 because
    the current 64x64 map uses 100 Unreal units per tile.
14. Expose useful tuning values with EditAnywhere properties:
    move speed, turn sensitivity, pitch sensitivity, minimum/maximum pitch,
    minimum/maximum zoom, and movement bounds.
15. Do not enable ground collision or change TileMapRenderer collision.
16. The existing temporary CameraActor should remain untouched by source code;
    it will be disabled manually only after the new Pawn works.
17. AMapCameraPlayerController should add IMC_MapCamera to the Enhanced Input
    local-player subsystem and bind/possess correctly.
18. AGrantMoneyGameMode should use AMapCameraPawn and
    AMapCameraPlayerController as defaults.
19. Inspect actual project paths/names first. Do not invent asset paths or
    class names that differ from the repository.
20. Report every changed file and do not claim compile/runtime tests succeeded
    unless they were actually executed.

Before editing, give me:
- the existing architecture you found;
- the exact files you will modify;
- your implementation plan;
- any assumptions that require human confirmation.

Then implement the smallest working version.
```

### Expected AI Output

The AI should:

- inspect the actual project first;
- identify the input assets and C++ classes;
- add camera components and exposed tuning properties;
- bind Enhanced Input;
- implement movement, yaw, pitch, zoom, and bounds;
- configure GameMode defaults;
- avoid modifying the map renderer;
- provide a change report and manual test plan.

### Human Review

Before accepting the AI output, verify:

- it did not modify TileGen arrays;
- it did not add Blueprint gameplay;
- it did not enable tile collision as a shortcut;
- it uses the existing input assets rather than inventing differently named ones;
- it uses frame-time (`DeltaSeconds`) for movement;
- pitch and zoom are clamped;
- movement bounds are configurable;
- it does not delete the old Camera Actor in code;
- it does not hardcode a future player-character relationship.

## 6. Step-by-Step Editor Preparation

### Step 1 — Open the correct level

**Where:** Unreal Editor

1. Open `Game/GrantMoney.uproject`.
2. Open `Content/Maps/PlayGame1`.

> **Correction found during inspection:** the original plan said `PlayGame`. In this repository `Content/Maps/PlayGame.umap` is only a redirector to `PlayGame2`, a World Partition template map that does not contain the renderer. The level that actually contains `TileMapRenderer`, the `PlayerStart` and the temporary `CameraActor`, and that is the project default map, is `PlayGame1`. Always confirm the level by looking at what it contains, not at its name.

3. Confirm `TileMapRenderer` is in the Outliner.
4. Confirm the generated world still appears when you press Play.
5. Stop Play before continuing.

**Why:** The camera should be built on top of a known working world.

**Expected:** The current map renderer remains unchanged.

---

### Step 2 — Create the Input folder

**Where:** Unreal Editor → Content Drawer

1. Press `Ctrl + Space`.
2. Right-click under `Content`.
3. Create a folder named:

```text
Input
```

Result:

```text
Content/Input/
```

**Why:** Input configuration is an Unreal asset concern and should be organized separately from source code.

---

### Step 3 — Verify Enhanced Input

**Where:** Unreal Editor

1. Open `Edit → Plugins`.
2. Search for:

```text
Enhanced Input
```

3. Confirm it is enabled.
4. If you enable it now, restart Unreal when requested.

**Expected:** Enhanced Input is available for Input Actions and Mapping Contexts.

---

### Step 4 — Create the Input Actions

**Where:** `Content/Input`

Create four Input Action assets:

```text
IA_CameraMove
IA_CameraTurn
IA_CameraLookUp
IA_CameraZoom
```

Set Value Type:

| Asset | Value Type |
|---|---|
| `IA_CameraMove` | Axis2D |
| `IA_CameraTurn` | Axis1D |
| `IA_CameraLookUp` | Axis1D |
| `IA_CameraZoom` | Axis1D |

**Why:** The AI implementation should bind to stable assets that already exist.

---

### Step 5 — Create the Input Mapping Context

**Where:** `Content/Input`

1. Right-click.
2. Create:

```text
Input Mapping Context
```

3. Name it:

```text
IMC_MapCamera
```

Add mappings:

```text
W → IA_CameraMove
S → IA_CameraMove
A → IA_CameraMove
D → IA_CameraMove

Mouse X → IA_CameraTurn
Mouse Y → IA_CameraLookUp
Mouse Wheel Axis → IA_CameraZoom
```

For the `Axis2D` movement action, configure WASD so the desired conceptual values are:

```text
W → (0, +1)
S → (0, -1)
A → (-1, 0)
D → (+1, 0)
```

Use Enhanced Input modifiers as needed:

```text
W → Swizzle Input Axis Values to Y
S → Swizzle to Y + Negate
A → Negate
D → no special modifier
```

**Expected:** WASD provides a normal two-dimensional movement vector.

---

### Step 6 — Create `MapCameraPawn`

**Where:** Unreal Editor

1. Open `Tools → New C++ Class`.
2. Select parent:

```text
Pawn
```

3. Name:

```text
MapCameraPawn
```

4. Create the class.
5. Let Unreal generate the `.h` and `.cpp`.
6. Do not implement the full behavior yet.

The intended component hierarchy for Copilot is:

```text
AMapCameraPawn
    SceneRoot
        ↓
    SpringArm
        ↓
    CameraComponent
```

**Why:** Spring Arm gives the later implementation a simple way to control distance and camera collision behavior.

---

### Step 7 — Create `MapCameraPlayerController`

**Where:** Unreal Editor

1. `Tools → New C++ Class`.
2. Choose:

```text
PlayerController
```

3. Name:

```text
MapCameraPlayerController
```

**Why:** Keep input-context setup and possession separate from camera movement mechanics.

---

### Step 8 — Create `GrantMoneyGameMode`

**Where:** Unreal Editor

1. `Tools → New C++ Class`.
2. Choose:

```text
GameModeBase
```

3. Name:

```text
GrantMoneyGameMode
```

The later AI implementation should make this GameMode use:

```text
Default Pawn Class → MapCameraPawn
Player Controller Class → MapCameraPlayerController
```

**Why:** This creates a clean startup path rather than manually possessing a camera placed in the level.

**What was actually done in GrantMoney:** the C++ `AGrantMoneyGameMode` was left empty. The developer created a Blueprint class `Content/Blueprints/BP_GrantMoneyGameMode` whose parent is `GrantMoneyGameMode`, and set **Default Pawn Class** and **Player Controller Class** on that Blueprint. This is a data-only configuration asset; all gameplay behavior remains in C++.

---

### Step 9 — Full compile after class creation

**Where:** Visual Studio

Because multiple C++ classes were added:

1. Save everything.
2. Close Unreal Editor.
3. Open the solution in Visual Studio.
4. Use:

```text
Development Editor | x64
```

5. Choose:

```text
Build → Build Solution
```

or:

```text
Ctrl + Shift + B
```

6. Reopen Unreal only after the build succeeds.

**Expected:** Unreal recognizes all three classes.

---

### Step 10 — Select the GameMode

**Where:** Unreal Editor

For a level-specific test:

1. Open `PlayGame1`.
2. Open `World Settings`.
3. Find:

```text
GameMode Override
```

4. Choose `GrantMoneyGameMode` once the class is compiled.

For a project-wide default:

1. `Edit → Project Settings`.
2. Open `Maps & Modes`.
3. Set:

```text
Default GameMode = GrantMoneyGameMode
```

Use one intentional approach and document it.

> **Warning — a level override beats the project default.** `PlayGame1` had been saved with **GameMode Override = the C++ `GrantMoneyGameMode`** before the Blueprint existed. Changing only the project default would have had no effect in that level, so the game would still have spawned the engine's default Pawn and PlayerController. The override in World Settings must be changed to `BP_GrantMoneyGameMode` (or cleared to `None`) and the level saved.
>
> **Final GrantMoney configuration (read from the repository):**
>
> - `Game/Config/DefaultEngine.ini`: `GlobalDefaultGameMode` and `GlobalDefaultServerGameMode` = `/Game/Blueprints/BP_GrantMoneyGameMode.BP_GrantMoneyGameMode_C`
> - `PlayGame1` World Settings: GameMode Override = `BP_GrantMoneyGameMode_C`
> - Runtime evidence: the log line `LogLoad: Game class is 'BP_GrantMoneyGameMode_C'`

---

### Step 11 — Add a PlayerStart

**Where:** Unreal Editor → `PlayGame1`

Add a `PlayerStart`.

For the current GrantMoney 64×64 world with 100-unit spacing, use an initial location around:

```text
X = 3150
Y = 3150
Z = 600
```

This places the Pawn approximately over the map center.

The exact Z value is a starting point, not a permanent rule.

> `PlayGame1` already contained `PlayerStart_0` at (3150, 3150, 600). Z=600 proved to be far too high for the final camera; the final Pawn ignores the PlayerStart's Z and locks itself to walking height (Section 8.4).

---

## 7. Camera Design Decisions Before Coding

This section should be completed **before** the Copilot prompt is sent.

### 7.1 Initial Camera Feel

For the current Megabonk-inspired direction, begin with:

```text
Projection:
Perspective

Pawn altitude:
approximately 400–800 Unreal units above the ground

Spring arm:
approximately 800–1200 units

Initial pitch:
approximately -35° to -50°

Movement:
WASD across the X/Y plane

Turn:
Mouse X

Look:
Mouse Y with pitch clamp

Zoom:
Mouse wheel changes SpringArm TargetArmLength

Camera collision:
Spring-arm camera collision enabled

Move speed:
approximately 1200–2000 Unreal units/second
```

These are **starting tuning values**, not hard requirements.

> **Outcome:** The first implementation used a high strategy-style view (arm 1500, pitch −55°, Pawn Z from the PlayerStart). The developer reported that it "mostly floats well above the level", because the target experience is a camera that behaves as if attached to a player walking around the level. The values were retuned (Section 8.5). The values listed in the block above were the **planning targets**; the final defaults are in Section 8.5. The planned "spring-arm camera collision enabled" was **not** used: `bDoCollisionTest` is false because ground/wall collision is deliberately out of scope until the player pawn exists (see 7.3).

The developer should expose the important values to the Unreal Details panel so the feel can be adjusted without recompiling.

### Why define this now?

If the prompt only says:

> Make a third-person camera.

AI must invent the desired movement model.

A better engineering prompt provides the intended user experience first, then asks AI to implement it.

---

### 7.2 Define World / Camera Boundaries

The current renderer uses a 64×64 map and places each tile at:

```text
X * 100
Y * 100
```

Therefore the current map occupies approximately:

```text
Min X = 0
Max X = 6300

Min Y = 0
Max Y = 6300
```

For the first camera milestone, tell Copilot to expose movement bounds and begin with those values.

You may later add a margin:

```text
200–400 units
```

depending on how much of the outside edge the camera should be allowed to reveal.

### General principle

Do not permanently design the camera around a magic `6300`.

The reusable design is:

```text
World data
    ↓
World bounds
    ↓
Camera clamp
```

For the first milestone, a configurable property is enough. A future version can derive the bounds from the selected map data.

> **Outcome:** Both steps happened. The first implementation exposed `MovementBoundsMin/Max` with defaults (0,0)–(6300,6300). A follow-up then derived the bounds from the playable cells of the selected map (Section 8.6). For Level 2 the derived rectangle is X 400..5900, Y 400..5900, and the camera now stops close to the visible border instead of at the empty edge of the 64×64 grid.

---

### 7.3 Do Not Add Ground Collision Just for the Camera

The current TileMapRenderer uses flat HISM tile planes with collision disabled.

Do not enable tile collision merely because a perspective camera now exists.

A free-roaming camera Pawn can move above the map without a walkable floor.

Ground collision becomes a different requirement when the actual character is introduced.

At that later stage, choose deliberately between:

- collision-enabled terrain;
- separate simplified collision geometry;
- another world representation appropriate for the character.

**Lesson:** Do not solve the next milestone early by changing a system that is already working.

> **Observed consequence:** `MapBoundaryGenerator` builds the border walls with the `BlockAll` collision profile, but the camera Pawn has no collision component and moves with `SetActorLocation`, so it passes through the walls. The developer noticed that "the outer border does not stop the camera". This is expected for a free camera and was left as is. The X/Y clamp is what keeps the camera near the border. Real wall collision starts to matter when the player pawn is added.

---

### 7.4 Keep the Old Camera Until the New Pawn Works

The current orthographic Camera Actor is a known-good debugging fallback.

Do not delete it before proving the new camera.

Recommended transition:

```text
1. Keep old CameraActor.
2. Implement and compile MapCameraPawn.
3. Verify GameMode creates/possesses the new Pawn.
4. Verify WASD, yaw, pitch, zoom and bounds.
5. Verify the gameplay viewport uses the new camera.
6. Only then set **Auto Activate for Player** to **Disabled** on the old
   CameraActor, or remove it from PlayGame1.
```

> **Correction:** an earlier AI answer called this property "Auto Receive Input". That name belongs to Pawns. On a `CameraActor` the property is **Auto Activate for Player**, found in the Details panel under **Auto Player Activation** (search the Details panel for "Auto Activate"). The developer could not find "Auto Receive Input", which is how the wrong name was caught. In `PlayGame1` the saved value had been `Player0`; after the developer set it to Disabled the property is no longer serialized in the map. `CameraActor_0` was kept in the level as a fallback.

If two cameras both attempt to auto-activate for Player 0, camera debugging becomes ambiguous.

**Lesson:** Preserve the known-good observation path until the replacement is verified.

## 8. Implemented Architecture and Final Tuning

> Everything in this section describes the code and assets as they exist in the repository after the implementation prompts in Section 9 were run. File paths are relative to `Game/`.

### 8.1 Startup and possession chain

```text
Config/DefaultEngine.ini
  GlobalDefaultGameMode = BP_GrantMoneyGameMode_C
        ↓   (PlayGame1 World Settings → GameMode Override = BP_GrantMoneyGameMode_C)
Content/Blueprints/BP_GrantMoneyGameMode      (Blueprint child of AGrantMoneyGameMode)
  DefaultPawnClass      = MapCameraPawn
  PlayerControllerClass = MapCameraPlayerController
        ↓
AMapCameraPlayerController   adds IMC_MapCamera, hides the cursor, game-only input
        ↓ possesses
AMapCameraPawn               spawned at PlayerStart_0
  SceneRoot → SpringArm → Camera (perspective)
        ↓
Player 0 views through the Pawn's Camera
(CameraActor_0 remains in the level with Auto Activate for Player = Disabled)
```

### 8.2 Class responsibilities

| Class | File(s) | Responsibility |
|---|---|---|
| `AGrantMoneyGameMode` | `Source/GrantMoney/Public/GrantMoneyGameMode.h`, `Private/GrantMoneyGameMode.cpp` | Empty C++ base. Configured by the Blueprint child, not edited by the camera work. |
| `AMapCameraPlayerController` | `Public/MapCameraPlayerController.h`, `Private/MapCameraPlayerController.cpp` | Adds `IMC_MapCamera` to the local player's Enhanced Input subsystem in `SetupInputComponent`; hides the cursor and sets game-only input in `BeginPlay`. Contains no camera math. |
| `AMapCameraPawn` | `Public/MapCameraPawn.h`, `Private/MapCameraPawn.cpp` | Owns the camera rig, binds the four Input Actions, moves, rotates, zooms, clamps to bounds, locks to walking height, optionally follows a target. |

### 8.3 Input contract found during inspection

The assets were read from disk (names and value types) before any code was written.

| Asset | Value type | Mapping(s) in `IMC_MapCamera` |
|---|---|---|
| `/Game/Input/IA_CameraMove` | Axis2D | W (Swizzle), S (Swizzle + Negate), A (Negate), D (none) |
| `/Game/Input/IA_CameraTurn` | Axis1D | Mouse X |
| `/Game/Input/IA_CameraLookUp` | Axis1D | Mouse Y |
| `/Game/Input/IA_CameraZoom` | Axis1D | Mouse Scroll Up (+1), Mouse Scroll Down (Negate → −1) |

Two things differed slightly from the plan and were harmless:

- The zoom is mapped with the separate `MouseScrollUp` / `MouseScrollDown` keys rather than the single **Mouse Wheel Axis** key named in Step 5. The Pawn binds only to `IA_CameraZoom`, so the key choice is irrelevant to the code.
- The WASD values come out as designed: W = (0,+1), S = (0,−1), A = (−1,0), D = (+1,0).

The default asset references are set in the C++ constructors with `ConstructorHelpers::FObjectFinder` using the paths above. **A human must verify these paths if any asset is renamed or moved**; a missing asset produces a log error and no input, not a crash.

### 8.4 Behavior as implemented

| Behavior | Implementation |
|---|---|
| Move (`IA_CameraMove`) | `Triggered` stores the 2D value, `Completed`/`Canceled` clear it. `Tick` moves the Pawn on the ground plane relative to the Pawn's yaw by `MoveSpeed × DeltaTime`. Input length is capped at 1 so diagonals are not faster. Frame-rate independent. |
| Turn (`IA_CameraTurn`) | Adds `value × TurnSensitivity` degrees of yaw to the Pawn. Mouse values are per-frame deltas, so they are intentionally **not** multiplied by `DeltaTime`. |
| Look (`IA_CameraLookUp`) | Changes the spring arm's pitch by `value × PitchSensitivity`, clamped to `MinPitch..MaxPitch`. `bInvertPitch` flips the direction. |
| Zoom (`IA_CameraZoom`) | Wheel up reduces a desired arm length by `ZoomStep`, clamped to `MinArmLength..MaxArmLength`. The real arm length eases toward it with `FInterpTo` (`ZoomInterpSpeed`; 0 snaps). |
| Height | With `bLockHeightToGround`, the Pawn's Z is `GroundZ + HeightAboveGround` at start and after every move. |
| Bounds | `ConstrainLocation` clamps X/Y to `MovementBoundsMin/Max` when `bClampToBounds` is on. |
| Follow | If `FollowTarget` is valid, the Pawn tracks `Target location + FollowOffset`; WASD and bounds are skipped; yaw, pitch and zoom still work. |
| Collision | The spring arm's `bDoCollisionTest` is **false**. No collision was added anywhere. |

### 8.5 Tunable properties and tuning history

All properties are `EditAnywhere` on `AMapCameraPawn`, so they can be adjusted in the Details panel of the Pawn class defaults.

| Property | First implementation | Final default | Notes |
|---|---:|---:|---|
| `InitialArmLength` | 1500 | **450** | Distance from pivot to camera. |
| `MinArmLength` / `MaxArmLength` | 500 / 6000 | **200 / 1500** | Zoom bounds. |
| `ZoomStep` | 250 | **100** | Arm change per wheel notch. |
| `ZoomInterpSpeed` | 10 | 10 | 0 snaps instantly. |
| `InitialPitch` | −55 | **−25** | Negative looks down. |
| `MinPitch` / `MaxPitch` | −85 / −15 | **−85 / −5** | Pitch clamp. |
| `TurnSensitivity` / `PitchSensitivity` | 1.0 / 1.0 | 1.0 / 1.0 | Degrees per mouse unit. |
| `bInvertPitch` | false | false | |
| `MoveSpeed` | 1500 | **600** | Units per second. |
| `bLockHeightToGround`, `GroundZ`, `HeightAboveGround` | n/a | true, 0, **100** | New in the walking-height pass. |
| `bClampToBounds` | true | true | |
| `bDeriveBoundsFromMap` | n/a | **true** | New. |
| `MapTileSize`, `MapBoundsPadding` | n/a | 100, 0 | New. |
| `MovementBoundsMin` / `Max` | (0,0)–(6300,6300) | same (fallback) | Overwritten at start when `bDeriveBoundsFromMap` succeeds. |
| `FollowTarget`, `FollowOffset` | n/a | none, (0,0,50) | New. |
| `FieldOfView` | 90 | 90 | |
| Camera / rotation lag | off | off | Optional smoothing. |

**Why the walking-height pass was needed.** The first implementation inherited the Pawn's Z from `PlayerStart_0` (Z = 600), then placed the camera `1500 × sin(55°) ≈ 1230` units higher, so the view sat near Z ≈ 1800 over a ground plane at Z = 0. After the pass, the camera starts about `100 + 450 × sin(25°) ≈ 290` units above the ground, which reads as a camera attached to a walking character. These are tuning starting points; adjust them in the editor.

### 8.6 World bounds: from magic numbers to map data

1. **First implementation.** Manual `MovementBoundsMin/Max` of (0,0)–(6300,6300), the extent of a 64×64 grid at 100 units per tile.
2. **Derived bounds.** At `BeginPlay` the Pawn finds the `ATileMapRenderer` in the world, reads its public `LevelToRender`, calls `FMapArrays::GetMap(...)` and takes the minimum and maximum X/Y over cells where `Valid[X][Y] != 0` and `Ground[X][Y] != 0`. This is the same definition of "playable" used by `FMapBoundaryGenerator`. Bounds are the cell **centers** times `MapTileSize`, plus or minus `MapBoundsPadding`.
3. **Result for Level 2 (from the log):** `MapCameraPawn: Level 2 bounds X 400..5900 Y 400..5900`.
4. **Fallback.** If no renderer is found, the level is not registered, or there are no playable cells, a warning is logged and the manual values stay in effect.
5. **Open hook.** `SetMovementBounds(Min, Max)` is public, so another system can set the bounds without the camera knowing about maps.

Neither `TileMapRenderer` nor `MapArrays` was modified. The Pawn reads `LevelToRender` (already a public property) and the existing `FMapArrays::GetMap` function.

**Limitation:** the playable area is irregular (about 2,900 of the 4,096 cells are tiles), but the camera limit is the bounding rectangle. The camera can still reach a corner of the rectangle that is not playable. Fixing that needs a per-cell test or collision, which belongs to the player-pawn milestone.

### 8.7 Follow target

`SetFollowTarget(AActor*)` (also editable as `FollowTarget`) lets a later player pawn drive the camera without rewriting it. The intended future use is `Camera->SetFollowTarget(this)` from the player pawn or the GameMode. The free-roam mode stays available as a debug camera by passing `nullptr`. **This code path has compiled but has not been exercised**, because no follow-able actor exists yet.

### 8.8 Corrections made during the session

| # | Issue | How it was found | Correction |
|---|---|---|---|
| 1 | Guide named the level `PlayGame`; it is a redirector to a different map. | AI inspection of the maps. | Guide and checks now use `PlayGame1`. |
| 2 | An AI answer called the old camera's property "Auto Receive Input". | Developer could not find it in the editor. | Correct name is **Auto Activate for Player**; guide updated. |
| 3 | `PlayGame1` World Settings still overrode the GameMode with the C++ class, defeating the new Blueprint default. | AI read the saved map. | Developer set the override to `BP_GrantMoneyGameMode`. |
| 4 | First-run camera floated far above the map. | Developer observation. | Walking-height lock and retuned defaults. |
| 5 | Header changes could not be compiled while the editor was open. | AI checked for a running editor before building. | Developer closed the editor; the build then succeeded. |
| 6 | Camera passes through the border walls. | Developer observation. | Accepted for now; collision belongs with the player pawn. |

### 8.9 What AI did, what the engineer decided, and what remains unverified

| Area | AI proposed / did | Engineer decided | Tested by | Still unverified |
|---|---|---|---|---|
| Architecture | Inspected the project, reported real paths and mismatches. | Responsibilities, scope exclusions, Blueprint GameMode. | Asset and map inspection. | — |
| Camera rig | Components, perspective, tuning properties. | Collision off, no input yet in that step. | UBT build. | — |
| Input | Enhanced Input binding in Pawn; mapping context in Controller. | Existing assets only; C++-only gameplay. | UBT build; developer play-test. | Gamepad; asset paths after renames. |
| Possession | Verified the chain from saved data and a headless run. | Level GameMode override change; disable old camera auto-activation. | Log line, `obj list` output, developer play-test. | Packaged build. |
| Height / feel | Proposed a walking-height lock and new defaults. | Wanted a walking-player feel. | Developer play-test ("looks great"). | Fine tuning of every value. |
| Bounds / follow | Derived bounds from map data; added follow target. | Chose both; deferred collision. | Log shows derived bounds; developer reports the camera stops near the border. | Follow target; non-rectangular edges. |

### 8.10 Build and run evidence

Commands used (adjust paths to your Unreal install):

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' `
    GrantMoneyEditor Win64 Development `
    -Project="C:\Dev\AI-Native-Game-Guru\Game\GrantMoney.uproject" -WaitMutex
```

Result of each build run during this work: `Result: Succeeded`.

Headless possession check (editor must be closed; no renderer is created):

```powershell
UnrealEditor.exe GrantMoney.uproject /Game/Maps/PlayGame1 -game -nullrhi -nosound -unattended `
    -ExecCmds="obj list class=MapCameraPawn,obj list class=MapCameraPlayerController,obj list class=BP_GrantMoneyGameMode_C,obj list class=CameraActor,obj list class=DefaultPawn,obj list class=PlayerStart,quit"
```

Observed: `Game class is 'BP_GrantMoneyGameMode_C'`; exactly one `MapCameraPawn_0`, one `MapCameraPlayerController_0`, one `BP_GrantMoneyGameMode_C_0`, one `CameraActor_0`, one `PlayerStart_0`, and zero `DefaultPawn`. A later run logged the derived bounds `X 400..5900 Y 400..5900`.

**Important:** close the editor before building after changing a header of a reflected class. Live Coding cannot hot-reload those changes.

### 8.11 Suggested screenshots (none captured yet)

| Filename | Show |
|---|---|
| `camera_01_input_assets.png` | `Content/Input` with the four Input Actions and `IMC_MapCamera`. |
| `camera_02_imc_mappings.png` | `IMC_MapCamera` mappings with the Swizzle/Negate modifiers. |
| `camera_03_world_settings_gamemode.png` | `PlayGame1` World Settings with GameMode Override = `BP_GrantMoneyGameMode`. |
| `camera_04_bp_gamemode_classes.png` | `BP_GrantMoneyGameMode` showing the Pawn and Controller classes. |
| `camera_05_old_camera_auto_activate.png` | `CameraActor_0` with Auto Activate for Player = Disabled. |
| `camera_06_first_run_too_high.png` | First implementation: camera floating high above the map. |
| `camera_07_walking_height.png` | After the walking-height pass. |
| `camera_08_pawn_details.png` | The Pawn class defaults showing the Camera categories. |
| `camera_09_bounds_log.png` | Output Log line with the derived bounds. |
| `camera_10_border_edge.png` | Camera stopped near the map border. |

## 9. AI Prompts by Stage

> **How these prompts were actually used.** Prompts A, B, C+D (sent together) and E were run in order; F and G are the follow-up prompts that produced the walking-height pass and the derived bounds / follow target. The AI stopped at the end of A to report findings before any code was written. Prompt E as sent ended after `MapCameraPawn →`; it was interpreted as "→ CameraComponent → view target". The complete wording is shown below and should be used instead.

### Prompt A — Inspect the Existing Project

```text
Inspect the current Unreal C++ project before editing. Find the existing
MapCameraPawn, MapCameraPlayerController, GrantMoneyGameMode, the Enhanced
Input assets, the current temporary CameraActor, PlayerStart, TileMapRenderer,
and MapArrays. Report the actual paths and current class contents. Do not
modify anything yet. Identify any mismatch between the editor assets and
the intended input names.
```

### Prompt B — Implement the Camera Components

```text
Implement only the AMapCameraPawn camera rig. Add a SceneRoot,
USpringArmComponent and UCameraComponent. Use a perspective camera. Expose
initial arm length, pitch, zoom bounds and camera-tuning properties. Do not
bind input yet and do not modify TileMapRenderer, MapArrays, collision, or
the level. Report changed files.
```

### Prompt C — Implement Enhanced Input

```text
Using the existing IA_CameraMove, IA_CameraTurn, IA_CameraLookUp,
IA_CameraZoom and IMC_MapCamera assets, implement the input binding through
the existing PlayerController/Pawn architecture. Preserve C++-only gameplay.
Do not invent new Input Action names. Movement must be frame-rate independent.
Clamp pitch and zoom. Report any asset path that must be verified by a human.
```

### Prompt D — Add World Bounds

```text
Add configurable X/Y camera movement bounds to MapCameraPawn. For the first
GrantMoney test, use editor-exposed defaults around 0..6300 for X and Y,
because the current map is 64x64 with 100-unit spacing. Keep the values
editable and explain how the architecture could later derive them from map
data. Do not modify the renderer to implement this.
```

### Prompt E — Debug Camera Ownership

```text
The new camera code compiles, but verify the startup/possession chain:
GrantMoneyGameMode → MapCameraPlayerController → MapCameraPawn →
CameraComponent. Explain how to diagnose a case where the old CameraActor is
still active or Player 0 is viewing the wrong camera. Do not delete or modify
assets automatically. Give manual Unreal checks first.
```

### Prompt F — Walking-Height Camera

```text
The camera currently floats far above the level. Put it just above the ground
so it behaves as if attached to a player walking around the level. Lock the
Pawn's height to a configurable ground Z plus a configurable height, retune the
default arm length, pitch, zoom range and move speed for a walking-player view,
and keep every value editable in the Details panel. Do not add collision and do
not modify TileMapRenderer, MapArrays or any map. Tell me anything I must do in
the editor before building.
```

### Prompt G — Map-Derived Bounds and Follow Target

```text
Derive the camera's X/Y movement bounds from the playable cells of the level
the TileMapRenderer is showing. Use the same definition of playable as the
boundary generator (valid cell with a ground tile). Keep the manual bounds as a
fallback and add a padding option. Do not modify TileMapRenderer or MapArrays;
read only what is already public. Also add an optional follow target
(SetFollowTarget plus an editable property) so a future player pawn can drive
the camera, while free roaming stays available. Do not implement the player
pawn. Report changed files and any behavior that was compiled but not run.
```

## 10. Common Mistakes

### Symptom
The project still uses the old orthographic camera.

### Likely Cause
The old Camera Actor is still auto-activated for Player 0, or the new Pawn is not possessed.

### Fix
Verify GameMode, Default Pawn Class, PlayerController Class, possession, and old Camera Actor activation.

### Lesson
Camera behavior is not only a transform problem; ownership and view-target selection matter.

---

### Symptom
W/S move sideways or A/D move forward/backward.

### Likely Cause
`IA_CameraMove` Axis2D mappings were configured incorrectly.

### Fix
Check Swizzle/Negate modifiers and confirm the expected `(X,Y)` values.

### Lesson
Verify input contracts before blaming movement code.

---

### Symptom
The camera rotates completely upside down.

### Likely Cause
Pitch was not clamped.

### Fix
Clamp pitch to the selected safe range.

### Lesson
Expose and constrain player-controlled camera degrees of freedom.

---

### Symptom
The camera leaves the map and shows empty space.

### Likely Cause
No world-boundary clamp or incorrect movement limits.

### Fix
Start with configurable X/Y bounds derived from the current map size.

### Lesson
The camera is part of world presentation and must respect world extent.

---

### Symptom
The Blueprint GameMode is set as the project default, but the level still spawns the default Pawn and PlayerController.

### Likely Cause
The level's World Settings **GameMode Override** still names an older GameMode. A level override beats the project default.

### Fix
Open the level's World Settings and set the override to the intended GameMode (or `None`), then save the level. Confirm with the log line `Game class is '...'`.

### Lesson
Check the narrowest scope first: level override, then project default.

---

### Symptom
You cannot find "Auto Receive Input" on the old camera Actor.

### Likely Cause
That is the Pawn property name. A `CameraActor` calls it **Auto Activate for Player** (category **Auto Player Activation**).

### Fix
Search the Details panel for "Auto Activate" and set it to Disabled.

### Lesson
When a described editor field does not exist, verify the name before assuming the asset is wrong.

---

### Symptom
After possession the camera hovers far above the map.

### Likely Cause
The Pawn inherits the `PlayerStart` height and the spring arm adds more height (long arm, steep pitch).

### Fix
Lock the Pawn to ground height plus a small offset and reduce the arm length and pitch.

### Lesson
A camera that "works" can still miss the intended experience; tune against the target feel, not just against "it moves".

---

### Symptom
The camera passes through the generated border walls.

### Likely Cause
The walls use `BlockAll`, but the camera Pawn has no collision component and moves with `SetActorLocation`.

### Fix
Expected for a free camera. Use the X/Y bounds for now and add collision with the player pawn.

### Lesson
Collision is a property of the mover, not only of the obstacle.

---

### Symptom
A rebuild fails or is skipped after editing a header while Unreal Editor is open.

### Likely Cause
Live Coding cannot hot-reload layout changes to reflected classes.

### Fix
Close the editor, build the `GrantMoneyEditor Win64 Development` target, then reopen the editor.

### Lesson
Use a full build whenever a `UCLASS` or `UPROPERTY` declaration changes.

---

### Symptom
Documents or checks refer to `PlayGame`, but nothing in that level matches.

### Likely Cause
`PlayGame.umap` is a redirector to a different map (`PlayGame2`).

### Fix
Use the level that actually contains `TileMapRenderer`, here `PlayGame1`.

### Lesson
Confirm what a level contains before basing instructions on its name.

## 11. Verification

"Developer play-test" means the developer ran the game in the editor and reported the result; no numeric measurements or recordings were captured.

| Test | Action | Expected Result | Status |
|---|---|---|---|
| Input assets exist | Read `Content/Input` | 4 Input Actions + 1 Mapping Context with the expected value types and mappings | **Verified** by reading the assets (all five present; value types and key mappings as in 8.3) |
| C++ builds | `Build.bat GrantMoneyEditor Win64 Development` | No compile errors | **Passed** after every code change in this work |
| GameMode applies | Launch `PlayGame1` | `BP_GrantMoneyGameMode_C` is the game class | **Verified** by log line and saved World Settings |
| Pawn / Controller spawn | Headless launch + `obj list` | One `MapCameraPawn`, one `MapCameraPlayerController`, no `DefaultPawn` | **Verified** |
| WASD movement | Use WASD in PIE | Camera moves over the ground plane | **Developer play-test: works** |
| Mouse yaw | Move mouse X | Yaw changes | **Developer play-test: works** |
| Mouse pitch | Move mouse Y | Pitch changes inside the clamp | **Developer play-test: works** |
| Zoom | Mouse wheel | Arm length changes inside min/max | **Developer play-test: works** |
| Camera ownership | Press Play | Viewport uses the Pawn's camera | **Developer play-test: works** |
| Old camera transition | Disable Auto Activate for Player | No camera conflict | **Done**; `CameraActor_0` kept in the level |
| Walking height | Press Play after the height pass | Camera just above the tiles | **Developer play-test: "looks great"** (exact heights not measured) |
| Derived bounds | Launch and read the log | Bounds match the playable cells | **Verified** in the log: X 400..5900, Y 400..5900 |
| Edge stop | Walk to the border | Camera stops near the border | **Developer play-test: "stops relatively close to the border"** |
| Walls block camera | Walk into a wall | — | **Does not block** (expected; no collision on the Pawn) |
| Follow target | Set `FollowTarget` | Camera tracks the actor | **Not run** (compiled only) |
| Gamepad | — | — | **Not run** (no gamepad mappings exist) |
| Packaged build | — | — | **Not run** |

## 12. Known Limitations

The camera milestone intentionally does **not** implement:

- the final player character;
- a tested camera-follow-character path (the follow target exists but has not been used);
- walkable ground collision, and camera/Pawn collision with the border walls (the walls exist but only block objects that collide);
- per-cell playable-area limits (the camera is limited to the bounding rectangle of the playable cells);
- environment population;
- live runtime map switching (bounds are derived once at `BeginPlay`);
- multiplayer camera logic;
- final camera polish and per-value tuning (all values are starting points);
- controller/gamepad support;
- packaged-build verification;
- automated tests.

## 13. Extension Ideas

The same architecture can later support:

- character-follow cameras (set `FollowTarget` from the player pawn or the GameMode);
- strategy-game free cameras;
- RTS edge scrolling;
- dungeon spectator cameras;
- replay/spectator cameras;
- tactical camera zoom;
- bounds that follow the active map when maps change at runtime;
- using the exported per-map `Collision` array, once it is exposed through `MapArrays`, for tile-based blocking.

Do not implement these until the current milestone is verified in the editor.

## 14. Source-Control Checkpoint

Current branch in the repository: `GMS-76-third-person-camera`.

Inspect first. Some files may already be staged (for example the earlier `DefaultEngine.ini` default-map change), so review what is staged before adding more:

```bash
git status
git diff --stat
git diff --cached --stat
```

Stage only actual milestone files:

```bash
git add Game/Source/GrantMoney/Public/MapCameraPawn.h
git add Game/Source/GrantMoney/Private/MapCameraPawn.cpp
git add Game/Source/GrantMoney/Public/MapCameraPlayerController.h
git add Game/Source/GrantMoney/Private/MapCameraPlayerController.cpp
git add Game/Source/GrantMoney/Public/GrantMoneyGameMode.h
git add Game/Source/GrantMoney/Private/GrantMoneyGameMode.cpp
git add Game/Content/Input/
git add Game/Content/Blueprints/
git add Game/Content/Maps/PlayGame1.umap
git add Game/Config/DefaultEngine.ini
```

Notes:

- `PlayGame1.umap` and the `.uasset` files are binary. Confirm they are tracked by Git LFS (`git lfs ls-files` after staging) if the repository uses it.
- `DefaultEngine.ini` contains the GameMode and default-map settings. Check `git diff Game/Config/DefaultEngine.ini` so only intended lines are committed (Git may also warn about CRLF/LF line endings).
- Do **not** stage `Game/Binaries/`, `Game/Intermediate/`, `Game/Saved/` or `Game/DerivedDataCache/`.
- Do not stage `TileMapRenderer`, `MapArrays` or map-data files; they were intentionally not changed.

Review:

```bash
git status
git diff --cached --stat
git diff --cached
```

Suggested commit:

```text
GMS-76: add free-roaming perspective map camera with Enhanced Input and map-derived bounds
```

Suggested PR:

```text
GMS-76: Third-person map camera (free roam, walking height, playable-area bounds)
```
