# OUTPUT A — Milestone Change Report
## Third-Person / Free-Roaming Map Camera (GMS-76)

> **Status:** Preparation and implementation are complete for the free-roaming camera. Items that were not run are listed under **Verification Status**. Nothing is described as passing unless it was actually built, run, or inspected.

## Milestone Summary

**Before:** the generated TileGen world was viewed only through a temporary orthographic `CameraActor` placed in `PlayGame1` (looking straight down from Z = 30000). There was no player input, no Pawn, no PlayerController configuration and no camera architecture. The earlier editor-preparation step had created five Enhanced Input assets and three empty C++ class skeletons.

**After:** the game starts with a perspective camera that moves like a player walking over the map.

- `BP_GrantMoneyGameMode` (a data-only Blueprint child of `AGrantMoneyGameMode`) selects `MapCameraPawn` and `MapCameraPlayerController`.
- `AMapCameraPlayerController` registers `IMC_MapCamera` for the local player and captures the mouse.
- `AMapCameraPawn` owns a `SceneRoot → SpringArm → Camera` rig and binds the four existing Input Actions: WASD moves on the ground plane relative to the camera yaw, mouse X turns, mouse Y changes clamped pitch, the wheel zooms with clamped, eased arm length. Movement is frame-rate independent.
- The Pawn is held at walking height above the ground plane (`GroundZ + HeightAboveGround`).
- X/Y movement is limited to the **playable area of the map the renderer is showing**, derived at start from the same data the border walls use. For Level 2 the log shows `X 400..5900 Y 400..5900`. Manual bounds remain as a fallback.
- An optional `FollowTarget` lets a future player pawn drive the camera.

**Why it matters architecturally:**

- Input assets, controller, pawn and GameMode each have one job. The renderer is not involved in camera behavior.
- The camera learns the map extent through existing public data (`ATileMapRenderer::LevelToRender` and `FMapArrays::GetMap`) without any change to the renderer or `MapArrays`.
- Ground collision, camera/wall collision and the player pawn were deliberately **not** added; the follow-target hook is the seam for that later work.

## Files Created

Paths are relative to the repository root.

| File | Purpose |
|------|---------|
| `Game/Source/GrantMoney/Public/MapCameraPawn.h` | Camera Pawn: components, input assets, tuning properties, bounds, height lock, follow target. |
| `Game/Source/GrantMoney/Private/MapCameraPawn.cpp` | Rig construction, Enhanced Input bindings, per-frame movement/zoom, bounds derivation, constraints. |
| `Game/Source/GrantMoney/Public/MapCameraPlayerController.h` | Controller that registers the camera mapping context. |
| `Game/Source/GrantMoney/Private/MapCameraPlayerController.cpp` | Adds `IMC_MapCamera`, hides the cursor, sets game-only input. |
| `Game/Source/GrantMoney/Public/GrantMoneyGameMode.h` | Empty C++ GameMode base (skeleton from the preparation step; not edited during implementation). |
| `Game/Source/GrantMoney/Private/GrantMoneyGameMode.cpp` | Matching implementation stub. |
| `Game/Content/Input/IA_CameraMove.uasset` | Axis2D movement action (created in the editor during preparation). |
| `Game/Content/Input/IA_CameraTurn.uasset` | Axis1D yaw action. |
| `Game/Content/Input/IA_CameraLookUp.uasset` | Axis1D pitch action. |
| `Game/Content/Input/IA_CameraZoom.uasset` | Axis1D zoom action. |
| `Game/Content/Input/IMC_MapCamera.uasset` | Maps WASD, mouse X/Y and wheel to the actions (8 mappings, 3 Negate and 2 Swizzle modifiers). |
| `Game/Content/Blueprints/BP_GrantMoneyGameMode.uasset` | Blueprint child of `GrantMoneyGameMode` selecting the Pawn and Controller (data only). |
| `docs/course/Module1-BuildingAWorld/Sprint3/ThirdPersonCamera_EditorPrep_*.md` | This report, the developer guide and the recording script. |

## Files Modified

| File | What Changed | Why |
|------|--------------|-----|
| `Game/Config/DefaultEngine.ini` | `GlobalDefaultGameMode` and `GlobalDefaultServerGameMode` set to `/Game/Blueprints/BP_GrantMoneyGameMode.BP_GrantMoneyGameMode_C`. The default-map lines (`GameDefaultMap`, `EditorStartupMap` = `PlayGame1`) were made by the developer earlier and may already be staged. | Makes the Blueprint GameMode the project default. |
| `Game/Content/Maps/PlayGame1.umap` (binary) | World Settings GameMode Override changed to `BP_GrantMoneyGameMode`; `CameraActor_0` **Auto Activate for Player** set to Disabled. | A level override beats the project default; the old camera would otherwise keep Player 0's view. |

Both edits were made by the developer in the editor and then confirmed by reading the saved file. The file is binary, so `git diff` will show only that it changed; confirm the result in the editor.

## Files Not Changed

| Item | Why it was preserved |
|------|----------------------|
| `Game/Source/GrantMoney/Public/TileMapRenderer.h` and `Private/TileMapRenderer.cpp` | The camera reads `LevelToRender` (already public); the renderer stays responsible for world presentation only. |
| `Game/Source/GrantMoney/Public/MapArrays.h` and `Private/MapArrays.cpp` | The camera uses the existing `FMapArrays::GetMap`; no new functions were added. |
| `Game/Source/GrantMoney/Private/MapBoundaryGenerator.cpp` / `Public/MapBoundaryGenerator.h` | Used as the definition of "playable cell"; walls and their collision are unchanged. |
| `Game/Source/GrantMoney/Private/Maps/Map1_*.cpp/.h` (TileGen exports) | Generated data. |
| `Game/Content/Materials/M_Tile`, `Game/Content/Tiles/` | No material or texture work. |
| `Game/GrantMoney.uproject` | Enhanced Input is already available; no plugin change was needed. |
| Tile and ground collision | Intentionally left disabled; collision is the next milestone. |
| `Game/Content/Maps/PlayGame.umap`, `PlayGame2.umap`, `NewMap.umap` | Not the active level (`PlayGame.umap` is a redirector to `PlayGame2`). |
| `CameraActor_0` (still in `PlayGame1`) | Kept as a fallback; only its auto-activation was disabled. |
| `Game/Binaries/`, `Game/Intermediate/`, `Game/Saved/`, `Game/DerivedDataCache/` | Generated by builds and runs; do not stage. |

## Verification Status

| Check | Result |
|---|---|
| C++ builds (`GrantMoneyEditor Win64 Development`, Unreal Build Tool) | **Passed** after each code change |
| Input assets and mappings match the plan | **Verified** by reading the assets |
| Game class at runtime is `BP_GrantMoneyGameMode_C` | **Verified** in the log and in the saved World Settings |
| Exactly one `MapCameraPawn` and one `MapCameraPlayerController`, zero `DefaultPawn` | **Verified** with a headless launch and `obj list` |
| WASD, yaw, pitch, zoom, walking height, view ownership | **Developer play-test**: works and feels good (no measurements or recordings) |
| Derived bounds | **Verified** in the log: Level 2, X 400..5900, Y 400..5900 |
| Camera stops near the border | **Developer play-test**: "relatively close to the border" |
| Camera blocked by border walls | **No**; expected, because the Pawn has no collision |
| `FollowTarget` / `SetFollowTarget` | **Compiled only; not run** |
| Gamepad, packaged build, automated tests | **Not run** |

## Problems Found and Corrected

| Problem | Correction |
|---|---|
| The plan referred to `PlayGame`, a redirector to a different map. | Use `PlayGame1`, the level that contains the renderer, PlayerStart and old camera. |
| The old camera's setting was called "Auto Receive Input" in an AI answer. | The correct name is **Auto Activate for Player**. |
| `PlayGame1` still overrode the GameMode with the C++ class. | Developer changed the override to `BP_GrantMoneyGameMode`. |
| The first camera floated far above the map. | Added a walking-height lock and retuned defaults (arm 450, pitch −25°, speed 600, zoom 200–1500). |
| Header changes cannot be built with the editor open. | Editor was closed before building. |

## Important Systems Intentionally Not Changed Yet

- Tile, ground and wall collision against the camera or a player.
- Player character, character movement and camera-follow wiring (hook exists, unused).
- Per-cell playable-area limits (the limit is the bounding rectangle of playable cells).
- Runtime map switching (bounds are derived once at `BeginPlay`).
- Environment population and the exported per-map `Collision` arrays (not yet exposed through `MapArrays`).

## Git Staging Recommendation

Do **not** stage blindly. First run:

```bash
git status
git diff --stat
git diff --cached --stat
```

The branch is `GMS-76-third-person-camera`. Part of `DefaultEngine.ini` (the default-map lines) may already be staged, so check `git diff --cached` before adding it again.

Stage only the milestone files:

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
git add docs/course/Module1-BuildingAWorld/Sprint3/ThirdPersonCamera_EditorPrep_ChangeReport.md
git add docs/course/Module1-BuildingAWorld/Sprint3/ThirdPersonCamera_EditorPrep_DeveloperGuide.md
git add docs/course/Module1-BuildingAWorld/Sprint3/ThirdPersonCamera_EditorPrep_Script.md
```

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

Do not commit, push or merge automatically.
