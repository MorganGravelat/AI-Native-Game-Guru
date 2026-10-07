# VIDEO SCRIPT — Third-Person / Free-Roaming Map Camera Foundation
## Module 1 — Building a World

> **Script status:** Complete through the free-roaming camera prototype. Steps 1–16 are the preparation recorded before Copilot wrote code. Steps 17–24 follow what actually happened: inspection, the camera rig, editor wiring, Enhanced Input, possession checks, the walking-height retune, map-derived bounds and the follow target.
>
> Facts in this script come from the repository, the build and run logs, and the developer's own play-test. **Verified** means a build, log line or file inspection backs it. **Developer play-test** means the developer ran the game and reported the result; nothing was timed or recorded. The follow target and gamepad support were **not** run. Do not present anything as tested that is not marked here as tested.

# Opening

### SAY

Welcome back to Grant Money. At this point, our generated world exists and the renderer can build a TileGen map from C++ data. We have also used a temporary orthographic Camera Actor to verify the world from above.

In this part, we are going to prepare the project for a reusable perspective camera system.

The exact camera class names and exact map dimensions are not the important lesson. The reusable method is to define the input contract, decide the camera behavior, establish world limits, and create clear C++ ownership before we ask AI to write implementation code.

We are going to let AI help with the repetitive C++ work, but we are not going to ask it to invent our design.

# Acceptance Criteria

### SAY

At the end of this preparation part:

- Enhanced Input is ready for camera movement, turn, look, and zoom.
- A camera Pawn class exists.
- A PlayerController class exists.
- A GameMode class exists.
- A PlayerStart provides a predictable spawn location.
- We have decided the camera's initial perspective, pitch, zoom, movement speed, and map boundaries.
- We have deliberately left ground collision unchanged.
- We have kept the existing Camera Actor as a fallback until the replacement is proven.
- We have a bounded Copilot prompt ready to implement the smallest working camera.

We are not claiming movement works yet. That comes after AI generates the code and we compile and test it.

# Engineering Principle

### SAY

Here is an engineering principle to remember:

**Define the contract before you automate the implementation.**

If we simply tell AI, "make a third-person camera," the AI has to guess whether the camera follows a character, moves freely, uses old input mappings, uses Enhanced Input, rotates the Pawn, rotates a spring arm, or stays inside the generated map.

Instead, we will make those decisions first. Then AI has a narrow engineering problem to solve.

# Prerequisites

### SAY

Before starting, confirm that the Unreal C++ project builds, `PlayGame1` opens (not `PlayGame`, which in this repository is only a redirector to a different map), the TileGen world renderer works, the map textures display correctly, and the current debugging camera can still show the generated world.

The map renderer stays untouched during this setup. We are adding a new camera system beside it, not rewriting it.

---

# Step 1 — Protect the Working Baseline

### SAY

Before we change camera ownership or add C++ classes, we are going to protect the known-good world-rendering state.

### SHOW

Show the Git branch and current `PlayGame1` result.

### DO

In Git Bash:

```bash
git checkout main
git pull
git lfs pull
git status
git checkout -b GMS-XX-map-camera-controller
```

Replace the Jira number before recording the final version.

### AI PROMPT

No AI is needed for this step.

### VERIFY

Confirm you are on the feature branch and the existing map still works.

---

# Step 2 — Create a Dedicated Input Folder

### SAY

Input is its own responsibility. We are going to create the Unreal assets first so the future C++ code binds to known names instead of inventing them.

### SHOW

Open the Content Drawer.

### DO

1. Press `Ctrl + Space`.
2. Under `Content`, create:

```text
Input
```

### VERIFY

Confirm:

```text
Content/Input/
```

exists.

---

# Step 3 — Verify Enhanced Input

### SAY

We are using Unreal's Enhanced Input system. These are editor assets, which is allowed even though gameplay behavior remains in C++.

### SHOW

Open `Edit → Plugins` and search for Enhanced Input.

### DO

1. Verify **Enhanced Input** is enabled.
2. If enabling it now, restart Unreal.

### VERIFY

Confirm Input Actions and Input Mapping Contexts can be created.

---

# Step 4 — Create the Camera Input Actions

### SAY

We will define four behaviors: movement, horizontal turning, vertical look, and zoom. These asset names become a stable contract for the C++ implementation.

### SHOW

`Content/Input`

### DO

Create:

```text
IA_CameraMove
IA_CameraTurn
IA_CameraLookUp
IA_CameraZoom
```

Set:

```text
IA_CameraMove   → Axis2D
IA_CameraTurn   → Axis1D
IA_CameraLookUp → Axis1D
IA_CameraZoom   → Axis1D
```

### AI PROMPT

No code generation yet. If needed, ask:

```text
Explain why camera movement is best represented as an Axis2D Enhanced Input
Action while yaw, pitch, and zoom are Axis1D. Do not write implementation
code yet.
```

### VERIFY

Open each asset and confirm the Value Type.

---

# Step 5 — Create `IMC_MapCamera`

### SAY

The Input Actions describe behaviors. The Mapping Context describes which physical controls produce those behaviors.

### SHOW

Create `IMC_MapCamera` under `Content/Input`.

### DO

Map:

```text
W → IA_CameraMove
S → IA_CameraMove
A → IA_CameraMove
D → IA_CameraMove

Mouse X → IA_CameraTurn
Mouse Y → IA_CameraLookUp
Mouse Wheel Axis → IA_CameraZoom
```

Configure the movement vector conceptually as:

```text
W → (0,+1)
S → (0,-1)
A → (-1,0)
D → (+1,0)
```

For W/S, use **Swizzle Input Axis Values** to route the key into Y.
For S, add **Negate**.
For A, add **Negate**.

### AI PROMPT

```text
I am configuring an Unreal Enhanced Input Axis2D action for WASD. Explain
which Swizzle and Negate modifiers I should use so W=(0,+1), S=(0,-1),
A=(-1,0), D=(+1,0). Do not write C++ yet.
```

### VERIFY

Inspect the mappings one by one before moving on.

---

# Step 6 — Create the Camera Pawn Skeleton

### SAY

Now we create the object that will eventually own our spring arm and perspective camera.

### SHOW

`Tools → New C++ Class`

### DO

1. Choose **Pawn**.
2. Name it:

```text
MapCameraPawn
```

3. Create the class.

Do not write the movement code yet.

### SAY

The intended component hierarchy will be:

```text
MapCameraPawn
    SceneRoot
        ↓
    SpringArm
        ↓
    Camera
```

The spring arm gives us a convenient zoom distance and later helps with camera obstruction.

### VERIFY

Confirm the new C++ class appears in Visual Studio and Unreal.

---

# Step 7 — Create the PlayerController Skeleton

### SAY

We are keeping input ownership separate from camera movement. This is deliberate.

### DO

Create a C++ `PlayerController` named:

```text
MapCameraPlayerController
```

### SAY

The PlayerController will be responsible for the Enhanced Input mapping context and player ownership. The Pawn will be responsible for camera movement and camera mechanics.

### VERIFY

Confirm the generated files exist.

---

# Step 8 — Create the GameMode Skeleton

### SAY

We also want Unreal to know which Pawn and PlayerController should exist when the game starts.

### DO

Create a C++ `GameModeBase` named:

```text
GrantMoneyGameMode
```

### SAY

Later the AI implementation will configure this GameMode to use our camera Pawn and PlayerController.

### VERIFY

Confirm Unreal recognizes the class.

---

# Step 9 — Full Build

### SAY

We just created new reflected C++ classes. This is a good time for a full build rather than relying on Live Coding.

### SHOW

Close Unreal. Open Visual Studio.

### DO

Select:

```text
Development Editor | x64
```

Then:

```text
Build → Build Solution
```

or press:

```text
Ctrl + Shift + B
```

### VERIFY

Do not continue until the classes compile successfully.

If they do not compile, capture the first real error and fix that before asking AI to build on top of a broken baseline.

---

# Step 10 — Set the GameMode

### SAY

Now we establish the startup ownership chain.

### SHOW

`World Settings` and `Project Settings → Maps & Modes`.

### DO

Use either a level-specific `GameMode Override` in `PlayGame1` or set the project default GameMode, and document which approach the project uses. Remember that a level override beats the project default; we will see why that matters in Step 19.

Document which approach the project uses.

### VERIFY

Confirm the chosen GameMode remains selected after saving.

---

# Step 11 — Add PlayerStart

### SAY

A Pawn needs a predictable place to spawn.

The current example world is a 64 by 64 map with 100 Unreal units between tile positions. That puts the approximate center near 3150, 3150.

### SHOW

Place a `PlayerStart` in `PlayGame1` (the level already contained one at 3150, 3150, 600).

### DO

Start with approximately:

```text
X = 3150
Y = 3150
Z = 600
```

### SAY

The Z value is only a first test value. We will tune the final perspective after the camera exists.

### VERIFY

Confirm the PlayerStart sits over the generated map area.

---

# Step 12 — Decide the Camera Feel Before Coding

### SAY

Now we reach the most important design step before Copilot.

We are going to define what the camera should feel like.

For this first implementation, the camera is a **free-roaming perspective map camera**. It is not yet following the actual survivor character.

### SHOW

Display these target values on screen:

```text
Perspective camera

Pawn height:
400–800 units

Spring arm:
800–1200 units

Pitch:
approximately -35° to -50°

Movement:
WASD over the X/Y map plane

Turn:
Mouse X

Look:
Mouse Y, clamped

Zoom:
Mouse wheel changes TargetArmLength

Move speed:
approximately 1200–2000 uu/sec

Camera collision:
enabled on spring arm
```

### SAY

These are tuning targets. We are going to expose them as editable settings where practical instead of hiding them as unexplained constants.

*(Update after implementation: the first build used a high, strategy-style view, and we later retuned it to a walking-height view. We also left spring-arm collision off because collision is deliberately out of scope. See Steps 18 and 22.)*

### AI PROMPT

```text
Review this desired camera feel and identify the minimum SpringArm and Camera
properties an Unreal C++ Pawn should expose for tuning. Do not implement
the class yet. Keep the design appropriate for a free-roaming perspective
map camera, not a character-follow camera.
```

### VERIFY

Do not ask Copilot to code until the team agrees this is the intended first camera behavior.

---

# Step 13 — Define Camera Movement Boundaries

### SAY

A free camera can reveal the empty outside of the generated map unless we deliberately limit it.

Our renderer currently places a 64 by 64 grid with 100 Unreal units between cells.

That means this current map occupies approximately zero through 6300 on both X and Y.

### SHOW

Display:

```text
Min X = 0
Max X = 6300

Min Y = 0
Max Y = 6300
```

### DO

Write these down as the first test bounds.

Optionally reserve a margin of about:

```text
200–400 units
```

depending on how much outside space the camera should reveal.

### SAY

We are not saying 6300 should live forever in the architecture.

The reusable design is:

```text
Map dimensions
    ↓
World extent
    ↓
Camera bounds
```

For our first implementation, configurable editor properties are enough. Later, the camera can derive them from selected map data.

### AI PROMPT

```text
When you implement MapCameraPawn, expose configurable X/Y world bounds.
Use approximately 0..6300 for the first GrantMoney defaults because the
current map is 64x64 at 100-unit spacing. Keep the design ready for those
bounds to be derived from map data later. Do not change TileMapRenderer.
```

### VERIFY

The team knows what the first valid movement rectangle is before code is generated.

---

# Step 14 — Do Not Add Ground Collision for the Camera

### SAY

Here is another scope decision.

Our current TileMapRenderer uses visual tile planes without collision. We are not going to turn all those tile planes into collision geometry just because our camera is now perspective.

The camera floats above the world. It does not need a walkable floor.

Ground collision becomes important when the actual player character arrives. That is a separate problem and deserves a deliberate solution.

### SHOW

Show the current renderer or Details information demonstrating that tile collision is currently disabled.

### AI PROMPT

```text
Do not modify TileMapRenderer collision while implementing the camera. The
free-roaming camera does not need a walkable floor. Keep collision work
outside this milestone.
```

### VERIFY

No renderer/collision files are changed merely to support the camera.

---

# Step 15 — Keep the Old Camera as a Fallback

### SAY

We also keep our existing orthographic Camera Actor for now.

A known-good observation tool is useful while replacing the camera system.

The order will be:

```text
Keep old camera
    ↓
Build new camera Pawn
    ↓
Verify possession
    ↓
Verify movement
    ↓
Verify look
    ↓
Verify zoom
    ↓
Verify bounds
    ↓
Disable old camera
```

### SHOW

Select the existing Camera Actor and show its **Auto Activate for Player** setting (Details panel, Auto Player Activation).

### SAY

We will only disable Auto Activate for Player—or remove the Actor—after the new camera is proven.

If two cameras compete for Player 0, debugging becomes harder because we no longer know which system owns the viewport.

### AI PROMPT

```text
Do not delete or modify the existing CameraActor automatically. Implement
the new Pawn/controller first. After runtime verification, give me manual
steps for disabling the old CameraActor's Player 0 auto-activation.
```

### VERIFY

The existing Camera Actor remains available before AI implementation begins.

---

# Step 16 — Give Copilot the Bounded Implementation Prompt

### SAY

Now the project is ready for AI.

Notice what we have already decided for it:

- which input system we use;
- which Input Actions exist;
- which classes own which responsibilities;
- what the camera should feel like;
- where the camera starts;
- how far it can move;
- what it is not allowed to change.

That changes Copilot from a designer guessing at our requirements into an implementation specialist working against a contract.

### SHOW

Paste the full implementation prompt from the Developer Guide into Copilot Agent Mode.

### DO

Ask Copilot to inspect first and report:

```text
Existing architecture
Files to modify
Implementation plan
Assumptions needing confirmation
```

Review that answer **before** authorizing edits.

### VERIFY

Before letting the agent proceed, confirm:

- it found the existing Input Actions;
- it found the correct C++ class skeletons;
- it intends to leave TileMapRenderer and MapArrays unchanged;
- it understands the initial 0..6300 world bounds are configurable defaults;
- it is not adding Blueprint gameplay;
- it is not adding player-character movement;
- it is not enabling tile collision.

---

# Step 17 — Inspect Before Editing

### SAY

Now we hand the work to AI, but the first instruction is not "write code." The first instruction is "inspect and report."

We want to see whether the AI understands our actual project before it changes anything.

### SHOW

The Copilot response listing the real paths and the current contents of the skeleton classes.

### AI PROMPT

```text
Inspect the current Unreal C++ project before editing. Find the existing
MapCameraPawn, MapCameraPlayerController, GrantMoneyGameMode, the Enhanced
Input assets, the current temporary CameraActor, PlayerStart, TileMapRenderer,
and MapArrays. Report the actual paths and current class contents. Do not
modify anything yet. Identify any mismatch between the editor assets and
the intended input names.
```

### DO

Read the report carefully. In our project the inspection found:

- the three C++ skeletons were empty stock classes;
- all five input assets existed with the expected value types and mappings;
- the zoom mapping uses `MouseScrollUp` and `MouseScrollDown`, not a single Mouse Wheel Axis key, which does not matter because the code binds to the action;
- `PlayGame.umap` is only a redirector to a different map. The real gameplay level is `PlayGame1`, and it contains the renderer, `PlayerStart_0` and the old `CameraActor_0`;
- the old camera had its Player 0 auto-activation turned on;
- `PlayGame1` World Settings still overrode the GameMode with the C++ class.

### VERIFY

Nothing was modified. The mismatches were documented, and the plan and guide were corrected to use `PlayGame1`.

---

# Step 18 — Implement Only the Camera Rig

### SAY

Next we ask for one small piece: the camera rig. No input yet.

### AI PROMPT

```text
Implement only the AMapCameraPawn camera rig. Add a SceneRoot,
USpringArmComponent and UCameraComponent. Use a perspective camera. Expose
initial arm length, pitch, zoom bounds and camera-tuning properties. Do not
bind input yet and do not modify TileMapRenderer, MapArrays, collision, or
the level. Report changed files.
```

### DO

Review the diff. The result:

- `MapCameraPawn.h/.cpp` changed only;
- the arm is driven by its own pitch, not by the controller;
- spring-arm collision test is **off**, because ground collision is not part of this milestone;
- tuning properties are `EditAnywhere`.

Build:

```text
GrantMoneyEditor | Win64 | Development
```

### VERIFY

The build reported `Result: Succeeded`. At this point the Pawn is not yet selected by any GameMode, so nothing visible changes in the game.

---

# Step 19 — Wire the Startup Chain in the Editor

### SAY

This part is editor work, so we do it ourselves rather than asking AI to edit binary assets.

### SHOW

`BP_GrantMoneyGameMode`, the level's World Settings and the old Camera Actor.

### DO

1. Create a Blueprint class based on `GrantMoneyGameMode` named `BP_GrantMoneyGameMode` under `Content/Blueprints`. It is a data-only configuration asset; gameplay stays in C++.
2. Set **Default Pawn Class** to `MapCameraPawn` and **Player Controller Class** to `MapCameraPlayerController`.
3. Make it the project default GameMode (`DefaultEngine.ini` records `GlobalDefaultGameMode` and `GlobalDefaultServerGameMode`).
4. Open `PlayGame1` → World Settings and set **GameMode Override** to `BP_GrantMoneyGameMode` (or clear it). A level override beats the project default.
5. Select the old `CameraActor`. Search its Details for **Auto Activate** and set **Auto Activate for Player** to **Disabled**. Keep the Actor in the level as a fallback.
6. Save the level.

### SAY

There was a small trap here. An AI answer had called that property "Auto Receive Input", which is a Pawn setting. We could not find it on the Camera Actor, which is how we knew the name was wrong. The correct label is **Auto Activate for Player**.

### VERIFY

Press Play. The developer reported that the view was now hovering above the level, coming from the new Pawn rather than the orthographic camera.

---

# Step 20 — Bind Enhanced Input and Add Movement Bounds

### SAY

Now the behavior. We give AI the actual asset names and the constraints.

### AI PROMPT

```text
Using the existing IA_CameraMove, IA_CameraTurn, IA_CameraLookUp,
IA_CameraZoom and IMC_MapCamera assets, implement the input binding through
the existing PlayerController/Pawn architecture. Preserve C++-only gameplay.
Do not invent new Input Action names. Movement must be frame-rate independent.
Clamp pitch and zoom. Report any asset path that must be verified by a human.

Add configurable X/Y camera movement bounds to MapCameraPawn. For the first
GrantMoney test, use editor-exposed defaults around 0..6300 for X and Y,
because the current map is 64x64 with 100-unit spacing. Keep the values
editable and explain how the architecture could later derive them from map
data. Do not modify the renderer to implement this.
```

### SHOW

The two classes:

- `AMapCameraPlayerController` adds `IMC_MapCamera` to the local player's Enhanced Input subsystem in `SetupInputComponent`, hides the cursor and sets game-only input.
- `AMapCameraPawn` binds the four actions in `SetupPlayerInputComponent`. Movement is applied in `Tick` with `DeltaTime`, so it is frame-rate independent. Mouse deltas are intentionally not multiplied by `DeltaTime`. Pitch and arm length are clamped.

### SAY

The prompt also asked the AI to tell us which asset paths a human must verify. The classes load `/Game/Input/IA_Camera*` and `/Game/Input/IMC_MapCamera` by path. If we rename an asset, we must update those paths, and a missing asset shows up as a log error, not as a crash.

### DO

Close the editor, build, reopen, press Play.

### VERIFY

The build succeeded. The developer play-tested and reported that WASD, mouse look, zoom and the bounds worked and felt good. Mouse sensitivity was left at 1.0 and was not numerically tuned.

---

# Step 21 — Verify the Possession Chain

### SAY

"It works on my screen" is not the same as "I know why it works." So we verify the startup chain.

### AI PROMPT

```text
The new camera code compiles, but verify the startup/possession chain:
GrantMoneyGameMode → MapCameraPlayerController → MapCameraPawn →
CameraComponent. Explain how to diagnose a case where the old CameraActor is
still active or Player 0 is viewing the wrong camera. Do not delete or modify
assets automatically. Give manual Unreal checks first.
```

The prompt we actually sent was cut off after `MapCameraPawn →`. The AI inferred the rest. Always read your prompt before sending it.

### DO

The AI checked:

- `DefaultEngine.ini` default GameMode;
- the level's GameMode override in the saved map;
- the Blueprint's Pawn and Controller defaults;
- that the old camera's auto-activation was no longer saved as Player 0;
- the Play-in-Editor log line `Game class is 'BP_GrantMoneyGameMode_C'`;
- and a headless launch listing live objects.

### VERIFY

The headless run showed one `MapCameraPawn_0`, one `MapCameraPlayerController_0`, one `BP_GrantMoneyGameMode_C_0` and zero `DefaultPawn`. That run has no renderer, so the final proof that the viewport uses the Pawn's camera is the developer's play-test.

---

# Step 22 — Tune for the Intended Feel

### SAY

At this point the camera worked mechanically but not experientially. The developer's note was that it floated well above the level. We wanted a camera that behaves as if it were attached to a player walking around.

The cause was arithmetic. The Pawn spawned at the PlayerStart's Z of 600, then the spring arm added about 1,230 more units with a 1500-unit arm at −55° pitch. The camera was near Z = 1800 over a ground plane at Z = 0.

### AI PROMPT

```text
The camera currently floats far above the level. Put it just above the ground
so it behaves as if attached to a player walking around the level. Lock the
Pawn's height to a configurable ground Z plus a configurable height, retune the
default arm length, pitch, zoom range and move speed for a walking-player view,
and keep every value editable in the Details panel. Do not add collision and do
not modify TileMapRenderer, MapArrays or any map. Tell me anything I must do in
the editor before building.
```

### DO

The new defaults: ground Z 0, pivot height 100, arm 450, pitch −25°, zoom 200–1500, move speed 600. The camera starts about 290 units above the ground.

The AI told us to close the editor first because header changes cannot be hot-reloaded. We forgot and the editor was open, so the build was deferred until we closed it.

### VERIFY

The build succeeded and the developer reported that the view looked great. We did not measure the exact camera height in game.

---

# Step 23 — Derive Bounds from the Map and Add a Follow Target

### SAY

Two follow-up improvements, both chosen by the engineer.

First, the camera's 0..6300 bounds were a magic number. The playable area is actually smaller and irregular. We now derive the bounds from the same data the border generator uses.

Second, the final game will have a player character. Instead of rewriting the camera later, we add an optional follow target now and leave free-roam as a debug mode.

### AI PROMPT

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

### SHOW

How it works:

- The Pawn finds the renderer in the world and reads its public `LevelToRender`.
- It asks `FMapArrays::GetMap` for the map data and finds the min and max X/Y of cells that are valid and have a ground tile.
- It multiplies by the tile size of 100 and uses those as bounds.

### VERIFY

The log reported `MapCameraPawn: Level 2 bounds X 400..5900 Y 400..5900`. The developer reported that the camera now stops relatively close to the border. We then noticed that the camera can still pass through the border walls, because the Pawn has no collision. That is expected and is the subject of the next milestone.

The follow target compiles but has **not** been exercised, because nothing exists to follow yet.

---

# Step 24 — Review What Changed

### SAY

Before we finish, we look at exactly which files changed and which did not.

### DO

```bash
git status
git diff --stat
git diff --cached --stat
```

Expected touched areas: the three camera classes, `BP_GrantMoneyGameMode`, the five input assets, `PlayGame1.umap`, and `DefaultEngine.ini`.

Expected **not** touched: `TileMapRenderer`, `MapArrays`, `MapBoundaryGenerator`, generated map arrays, materials and textures.

Do not stage `Binaries`, `Intermediate`, `Saved` or `DerivedDataCache`.

### VERIFY

The change report lists every file and gives the exact `git add` commands. Nothing is committed automatically.

---

# Engineering Discussion

### SAY

Here is an engineering discussion.

It would be tempting to let the AI create all of these systems from a one-line prompt. That might even produce something that moves a camera.

But it would be difficult to teach, difficult to review, and likely to mix unrelated responsibilities.

By creating the input contract and startup architecture ourselves, we gave AI a stable boundary. The AI inspected first, we corrected the plan from what it found, and every later prompt named the exact assets and listed what must not change.

The same method applies outside this project:

```text
Decide behavior
    ↓
Define inputs and ownership
    ↓
Give AI a narrow task
    ↓
Compile and test
    ↓
Correct assumptions
    ↓
Tune against the intended experience
```

A second lesson is that "works" and "right" are different. The first camera worked and was wrong for the experience we wanted, and we only found that by playing it.

A third lesson is scope discipline. We did not add collision or a player character even when the camera obviously wanted them. We left a follow-target hook and recorded the walls limitation instead.

---

# Common Mistakes

### SAY

Here are some common mistakes to watch for.

First, creating the camera code before deciding which Input Actions exist. This encourages hardcoded keys and inconsistent names.

Second, allowing both the old Camera Actor and new camera Pawn to fight for Player 0. The setting you need is **Auto Activate for Player** on the Camera Actor, not "Auto Receive Input".

Third, setting a new default GameMode but forgetting that the level has its own GameMode Override. In our project the level still pointed at the old class, so the project default was ignored there until the override was changed.

Fourth, trusting a level by its name. In our repository `PlayGame` is only a redirector to another map. The level with the renderer is `PlayGame1`.

Fifth, changing the TileMapRenderer to solve camera problems. Rendering the map and navigating the map are separate responsibilities. We read the data the renderer already exposes instead.

Sixth, adding collision to thousands of visual tiles even though the camera does not need a floor.

Seventh, hardcoding a boundary such as 6300 without explaining where it came from. We replaced it with bounds derived from map data, and kept the manual values as a fallback.

Eighth, building with Unreal Editor open after changing a reflected header. Close the editor, build, then reopen.

Ninth, assuming that a wall with collision will stop a mover that has no collision of its own.

---

# Verification Checkpoint

### SAY

Now let's check what is actually verified.

### SHOW / VERIFY

Preparation:

```text
[x] Content/Input exists with 4 Input Actions and IMC_MapCamera
[x] IA_CameraMove = Axis2D
[x] IA_CameraTurn / LookUp / Zoom = Axis1D
[x] WASD, mouse and wheel mappings present (verified by reading the assets)
[x] MapCameraPawn, MapCameraPlayerController and GrantMoneyGameMode exist
[x] PlayerStart exists in PlayGame1
[x] TileMapRenderer and MapArrays unchanged
[x] old CameraActor still in the level (auto-activation disabled)
```

Implementation:

```text
[x] full C++ build succeeds (Unreal Build Tool, Development Editor Win64)
[x] game class at runtime is BP_GrantMoneyGameMode_C (log)
[x] one MapCameraPawn and one MapCameraPlayerController spawn, zero DefaultPawn (headless obj list)
[x] WASD, yaw, pitch, zoom (developer play-test: works)
[x] Pawn camera is the viewport camera (developer play-test)
[x] walking-height placement (developer play-test: looks great)
[x] bounds derived from map: Level 2 X 400..5900, Y 400..5900 (log)
[x] camera stops near the border (developer play-test)
[ ] camera blocked by the border walls (it is not; expected)
[ ] follow target exercised (not run)
[ ] gamepad (not run)
[ ] packaged build (not run)
```

Do not mark the unchecked items as passed.

---

# AI Review

### SAY

Let's review what AI actually did, what we decided, and what had to be corrected.

**What AI proposed or did:** inspected the project and reported real paths; wrote the camera rig, input binding, bounds, height lock, map-derived bounds and follow target; built the project; checked the possession chain from the saved assets and a headless run.

**What the engineer decided:** the responsibilities of each class; using the existing Input Actions; no Blueprint gameplay; no collision yet; a Blueprint GameMode as configuration; keeping the old camera until the new one worked; the walking-player feel; deriving bounds from map data; adding a follow target instead of a player pawn for now.

**What had to be corrected:** the plan named `PlayGame` but the real level is `PlayGame1`; an AI answer used the wrong property name for the old camera; the level GameMode override masked the project default; the first camera floated too high; a build was attempted while the editor was open.

**What was actually tested:** builds, log lines, a headless object list, and the developer's own play-test.

**What remains unverified:** the follow target, gamepad, packaged builds, collision, and fine tuning of every value.

---

# Engineering Pause

### SAY

Here is an engineering pause.

Consider this question:

**If an AI can generate a camera class in seconds, why spend time defining input assets, boundaries, ownership and what must not change?**

Because fast code generation does not remove the cost of wrong assumptions.

Then a second question:

**Our camera worked after the first implementation. Why change it?**

Because the goal was never "a camera that moves." The goal was a camera that feels like it follows a walking player. The only way to learn that was to play it.

---

# Suggested Screenshots

None were captured during the work. Suggested filenames:

```text
camera_01_input_assets.png
camera_02_imc_mappings.png
camera_03_world_settings_gamemode.png
camera_04_bp_gamemode_classes.png
camera_05_old_camera_auto_activate.png
camera_06_first_run_too_high.png
camera_07_walking_height.png
camera_08_pawn_details.png
camera_09_bounds_log.png
camera_10_border_edge.png
```

---

# Completion Criteria

### SAY

This milestone is complete for the free-roaming prototype when:

- the editor input assets exist and match the plan;
- the three C++ classes compile;
- `BP_GrantMoneyGameMode` is the GameMode in both the project settings and the level;
- the Pawn is possessed and its camera is the viewport camera;
- WASD, yaw, pitch and zoom work, with pitch and zoom clamped;
- the camera sits at walking height;
- movement stays inside bounds derived from the playable map area;
- ground and wall collision remain out of scope;
- the old camera remains available as a fallback;
- and the change report lists every touched file.

This was met in the editor by developer play-test and by the build and log evidence above. The follow target, gamepad and packaged builds are not part of this completion claim.

# Next Milestone

### SAY

Next, we add the player character: a Pawn with collision that can walk the map and be stopped by the border walls and tile collision data. Then we point the camera at it with `SetFollowTarget`, and decide whether the camera stays a separate Pawn or becomes part of the character.

We will not tune the camera further until the character exists.
