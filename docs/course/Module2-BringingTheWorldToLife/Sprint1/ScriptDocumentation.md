```md
# Module 2 – Sprint 1 Development and Recording Guide
## Bringing the World to Life

### Sprint Goal

The goal of Module 2 Sprint 1 is to introduce the first real playable character into the generated Grant Money world.

By the end of this sprint, the project should support:

1. A C++ player character that spawns and is possessed by the player.
2. Camera-relative movement using Unreal Engine Enhanced Input.
3. Collision between the player and the generated world/map boundaries.
4. A reproducible AI-assisted development process that can be turned into instructional course material.

This sprint is intentionally limited to the **player foundation**.

Do not add combat, enemies, XP, pickups, advanced animation, or a fully player-following camera during this sprint.

The Module 1 camera should remain intact wherever possible. Sprint 1 only needs movement to understand the **orientation of the active camera**.

---

# Development and Documentation Workflow

Each Jira issue should be treated as its own development milestone.

The recommended workflow is:

```text
Inspect existing architecture
        ↓
Plan the smallest change
        ↓
Record baseline
        ↓
Implement one meaningful step
        ↓
Compile/test
        ↓
Capture screenshot or short video
        ↓
Tell the AI what actually happened
        ↓
Update documentation
        ↓
Review diff
        ↓
Commit / PR
```

Do not wait until the end of the sprint to recreate the development process from memory.

The development process itself is part of the course material.

---

# Media Capture Rules

Record short clips while actually building the feature.

Do not record only a demonstration of the finished feature.

Most clips only need to be approximately:

```text
10–30 seconds
```

Long recordings are usually unnecessary because the final instructional video can use voice-over narration.

Screenshots should show important configuration, code, or verification states.

Use numbered filenames so the future video editor can easily match documentation to footage.

Example:

```text
01-create-character-class.mp4
01-create-character-class.png

02-character-code.png

03-character-spawned.mp4
03-character-spawned.png
```

Raw videos should be stored in the team's shared media folder rather than committed to Git.

---

# Recommended Media Folder Structure

```text
Module2/
└── Sprint1/
    ├── GMS-102/
    │   ├── 00-baseline.mp4
    │   ├── 00-baseline.png
    │   ├── 01-existing-camera-architecture.png
    │   ├── 02-create-character-class.mp4
    │   ├── 02-create-character-class.png
    │   ├── 03-character-code.png
    │   ├── 04-player-spawned.png
    │   ├── 05-possession-proof.png
    │   └── 06-player-in-world.mp4
    │
    ├── GMS-103/
    │   ├── 00-character-no-movement.mp4
    │   ├── 01-ia-move.png
    │   ├── 02-mapping-context.png
    │   ├── 03-input-binding-code.png
    │   └── 04-camera-relative-movement.mp4
    │
    └── GMS-104/
        ├── 00-before-collision.mp4
        ├── 01-boundary-collision-code.png
        ├── 02-ground-collision.mp4
        └── 03-wall-collision.mp4
```

---

# GMS-102 – Playable C++ Character Spawns and Is Possessed

## Goal

Create the minimum C++ player-character architecture required for Grant Money.

The completed milestone should prove that:

- a C++ player character exists;
- Unreal spawns the character when gameplay begins;
- Player 0 possesses the character;
- the character has standard Unreal character collision and movement components;
- the existing generated world continues to render;
- the existing pause system continues working;
- the Module 1 camera is not unnecessarily rewritten.

Movement input is **not part of this issue**.

---

# Step 1 – Create the Development Branch

Start from the latest version of `main`.

```bash
git checkout main
git pull
git checkout -b GMS-102-playable-player-character
git status
```

Do not begin implementation on an old Module 1 branch.

---

## Capture

Record the existing game before making changes.

### Clip

```text
00-baseline.mp4
```

Record approximately 10–20 seconds showing:

- `PlayGame` launching;
- the generated TileGen world;
- the existing camera;
- the current state before a player character exists.

### Screenshot

```text
00-baseline.png
```

Show the current world/camera state.

---

## Future Video Editor Use

Use this clip near the beginning of the GMS-102 instructional section.

Suggested narration:

> "At the start of Module 2, our generated world already exists, but the player does not yet control a character. Our first goal is to introduce a proper Unreal C++ character without breaking the systems created in Module 1."

A useful visual sequence is:

```text
00-baseline.mp4
        ↓
Title / explanation
        ↓
Character creation
        ↓
Final possessed character
```

This creates a clear **before → implementation → after** structure.

---

# Step 2 – Ask AI to Inspect the Existing Architecture

Before changing code, have the coding AI inspect the existing project structure.

The goal is to understand how the current:

- GameMode;
- PlayerController;
- pawn;
- camera;
- pause system

are connected.

Do not ask the AI to immediately rewrite the project.

---

## AI Prompt

```text
We are starting Jira GMS-102: "Playable C++ character spawns and is possessed."

Follow the repository development rules.

Before editing anything, inspect only the smallest relevant set of C++ and configuration files.

Determine:

1. What GameMode is currently used by PlayGame.
2. What PlayerController is currently used.
3. What object currently owns or controls the Module 1 camera.
4. What the current DefaultPawnClass is.
5. How the pause system is connected to the PlayerController or pawn.
6. Whether a player-character class already exists.
7. Which exact files would need to change to introduce an ACharacter-derived playable character.

Do not modify files yet.

Do not inspect binaries, .uasset files, generated Unreal directories, media, or unrelated source files.

The acceptance criteria are:

- a C++ player character exists and compiles;
- PlayGame spawns and possesses it;
- it has a collision capsule and CharacterMovementComponent;
- the TileGen renderer still works;
- the existing pause system still works;
- the architecture can later support animation and combat.

Report:

- the current architecture;
- the minimum proposed change;
- likely files affected;
- any risk to the existing camera.
```

---

# Step 3 – Identify the Current Camera Architecture

Before changing possession, determine what currently controls the camera.

This is important because the Module 1 camera may currently depend on the existing pawn.

A good architecture for Sprint 1 is conceptually:

```text
PlayerController
    │
    ├── possesses → GrantMoneyPlayerCharacter
    │
    └── receives view from → Existing Gameplay Camera
```

The character does **not** need a new follow camera yet.

A full player-follow camera is later Module 2 work.

---

## Capture

### Screenshot

```text
01-existing-camera-architecture.png
```

Capture the most useful evidence showing how the current camera is implemented.

Possible examples:

- camera controller C++ source;
- PlayerController camera logic;
- spring arm/camera code;
- existing camera actor configuration.

---

## Future Video Editor Use

Use this screenshot while explaining why the team inspected the existing architecture before creating the character.

Suggested narration:

> "Before changing possession, we first asked the AI to inspect the existing camera and controller architecture. This prevents a common AI-assisted development mistake: creating a new system without understanding the one already in the project."

This screenshot is especially useful for the **AI Workflow Review** later.

---

# Step 4 – Create the C++ Character Class

In Unreal Engine:

```text
Tools
→ New C++ Class
→ Character
```

Choose:

```text
Character
```

rather than:

```text
Pawn
```

A useful project-specific class name is:

```text
GrantMoneyPlayerCharacter
```

unless the project already uses another naming convention.

---

## Why Use ACharacter?

`ACharacter` already provides the standard gameplay foundation required for a walking player.

It includes systems such as:

```text
CapsuleComponent
SkeletalMeshComponent
CharacterMovementComponent
```

Using `ACharacter` prevents the team from rebuilding normal Unreal character functionality unnecessarily.

---

## Capture

### Video

```text
02-create-character-class.mp4
```

Record the Unreal class wizard while selecting:

```text
Character
```

and creating the new C++ class.

Approximately 10–20 seconds is sufficient.

### Screenshot

```text
02-create-character-class.png
```

Capture the class wizard or the newly created class inside the project.

---

## Future Video Editor Use

This should be one of the most important instructional clips in the GMS-102 video.

Use the clip while the narration explains:

> "Rather than starting from a generic Pawn, we selected Unreal's Character base class because it already includes the standard components required for a walking player."

If necessary, pause or zoom the video around the **Character** selection.

---

# Step 5 – Ask AI to Implement Only the Minimum Character Foundation

Now that the C++ class exists, allow Copilot/Claude to help implement GMS-102.

Do not ask it to build movement yet.

---

## AI Prompt

```text
We have now created the ACharacter-derived class GrantMoneyPlayerCharacter for GMS-102.

Implement only the minimum player-character foundation required by GMS-102.

Requirements:

- Preserve the existing GameMode, PlayerController, pause system, TileMapRenderer, MapArrays, and camera architecture whenever possible.
- Do not implement movement input yet. That belongs to GMS-103.
- Do not implement animation logic yet.
- Do not implement combat.
- Do not convert the camera into a player-follow camera.
- Do not introduce Blueprint gameplay logic.
- Use the existing PlayerController if one already exists.
- Ensure the character uses the normal ACharacter capsule and CharacterMovementComponent.
- Configure only sensible minimum defaults required for the character to safely exist in the world.
- Make PlayGame spawn this character as the player's pawn and have Player 0 possess it.
- Preserve the existing active camera view if possible.

Before editing, tell me exactly which files you will change and why.

After editing:

1. list every file changed;
2. explain why each file changed;
3. give me the exact Unreal compile and test procedure;
4. do not claim anything successfully compiled or ran until I confirm it.
```

---

# Step 6 – Review the Proposed Changes Before Accepting Them

The AI should normally make a relatively small change.

Likely files may include:

```text
GrantMoneyPlayerCharacter.h
GrantMoneyPlayerCharacter.cpp
ExistingGameMode.h
ExistingGameMode.cpp
```

The existing PlayerController may also require modification depending on how the camera currently works.

The AI should **not** suddenly rewrite unrelated systems such as:

```text
TileMapRenderer
MapArrays
MapEnvironmentPopulator
TileGen generated files
```

If it attempts a large unrelated refactor, stop and ask it to reduce the implementation to only what GMS-102 requires.

---

# Step 7 – Compile the Character Foundation

Compile the project using the team's normal Unreal C++ build workflow.

The initial checkpoint is:

```text
Project compiles
        ↓
Editor opens
        ↓
PlayGame loads
        ↓
Generated map still renders
```

Do not move on to additional features until the project builds successfully.

---

## Capture

### Screenshot

```text
03-character-code.png
```

Show a useful portion of the new character class.

Good content includes:

- constructor;
- capsule configuration;
- CharacterMovement configuration;
- relevant class declaration.

Avoid showing hundreds of lines of unrelated code.

### Optional Clip

```text
03-character-compiles.mp4
```

A very short recording can show:

- successful build;
- Unreal Editor reopening;
- project successfully entering Play.

---

## Future Video Editor Use

Use the code screenshot while explaining what Unreal's `ACharacter` gives the project automatically.

The successful compilation clip can be used as a quick transition:

```text
Character implementation
        ↓
Compile
        ↓
Verify in gameplay
```

---

# Step 8 – Update the Running Documentation

After compilation is confirmed, tell the AI exactly what succeeded.

---

## AI Prompt

```text
The project compiled successfully.

Update the running GMS-102 task documentation with:

- what we created;
- why ACharacter was selected instead of APawn;
- files changed;
- screenshot 02-create-character-class.png;
- screenshot 03-character-code.png;
- verification that the project compiled.

Do not say that spawning or possession works yet because we have not verified that.
```

This is important because the AI should only document tests that actually occurred.

---

# Step 9 – Configure the Character to Spawn

The project's existing GameMode should normally define which pawn the player receives.

Conceptually, the relevant configuration may resemble:

```cpp
DefaultPawnClass = AGrantMoneyPlayerCharacter::StaticClass();
```

The exact implementation should be based on the existing GameMode rather than creating a duplicate GameMode.

For initial GMS-102 testing, a correctly positioned Unreal `PlayerStart` is acceptable.

The reusable/generated-map spawn system is handled separately by GMS-106.

Do not over-engineer spawning inside GMS-102.

---

# Step 10 – Verify Player Possession

Run the game.

Verify that:

```text
Player 0
   ↓
possesses
   ↓
GrantMoneyPlayerCharacter
```

Useful verification methods include:

- Unreal World Outliner;
- Unreal debugger;
- log output;
- PlayerController inspection.

A temporary diagnostic log can also be useful.

Example concept:

```cpp
UE_LOG(
    LogTemp,
    Log,
    TEXT("GrantMoneyPlayerCharacter possessed by %s"),
    *GetNameSafe(GetController())
);
```

Prefer an existing project log category if one already exists.

---

## AI Prompt

```text
The character compiles.

Now help me verify GMS-102 rather than adding new features.

Add or identify the minimum useful logging/debugging needed to prove:

1. GrantMoneyPlayerCharacter spawned.
2. Player 0 possesses it.
3. The existing camera remains the active view.
4. TileMapRenderer still rendered the world.
5. The pause system still functions.

Avoid permanent debug code unless it has ongoing value.

Tell me exactly what I should look for in Unreal while testing.
```

---

# Step 11 – Capture Spawn and Possession Evidence

## Screenshot

```text
04-player-spawned.png
```

Show the player character visibly existing in the generated level.

---

## Screenshot

```text
05-possession-proof.png
```

Show useful proof that the PlayerController possesses `GrantMoneyPlayerCharacter`.

This could be:

- debugger state;
- World Outliner;
- log output;
- controller information.

---

## Video

```text
06-player-in-world.mp4
```

Record approximately 15–30 seconds showing:

1. starting Play;
2. generated world loading;
3. player character appearing;
4. existing camera remaining functional;
5. pause menu still working if practical.

The player does not need to walk yet.

---

## Future Video Editor Use

This is the final result clip for GMS-102.

Suggested sequence:

```text
00-baseline.mp4
    "No player character yet"

02-create-character-class.mp4
    "Create ACharacter"

03-character-code.png
    "Minimal C++ implementation"

05-possession-proof.png
    "Verify possession"

06-player-in-world.mp4
    "Final GMS-102 result"
```

This creates a complete mini-lesson without requiring the developer to narrate while working.

---

# Step 12 – Final GMS-102 Verification

Verify each acceptance criterion individually.

```text
[ ] C++ player-character class exists.
[ ] Project compiles successfully.
[ ] PlayGame spawns the player character.
[ ] Player 0 possesses the character.
[ ] Character uses normal ACharacter capsule collision.
[ ] CharacterMovementComponent exists.
[ ] TileMapRenderer still generates the map.
[ ] Existing camera still functions.
[ ] Pause menu still functions.
[ ] No GMS-103 movement system has been added prematurely.
```

---

# Step 13 – Ask AI to Review the Final Diff

Before committing, have the AI inspect the changes.

---

## AI Prompt

```text
GMS-102 verification results:

[PASTE THE ACTUAL PASS/FAIL RESULTS HERE]

Review the diff for GMS-102 only.

Check specifically for:

- unnecessary changes;
- unrelated refactors;
- duplicated GameMode logic;
- duplicated PlayerController logic;
- Blueprint gameplay logic;
- coupling to TileGen-generated filenames;
- changes that belong to GMS-103 or a later sprint.

Then generate the final GMS-102 milestone report containing:

- implementation summary;
- files changed and why;
- step-by-step reproduction procedure;
- AI prompts used;
- AI mistakes or corrections;
- actual verification results;
- screenshot and recording references;
- known limitations;
- suggested commit message;
- short instructional voice-over outline.

Do not commit or push.
```

---

# GMS-102 – As-Built Record

This section records what actually happened while implementing GMS-102. It differs from the plan above in a few places. Only results the developer confirmed are marked as passed.

## Deviations From the Plan

| Plan | What actually happened | Why |
|---|---|---|
| Step 9: set `DefaultPawnClass` in the GameMode | The GameMode overrides `GetDefaultPawnClassForController_Implementation` to return the character class (`DefaultPawnClass` is also set in the constructor) | `BP_GrantMoneyGameMode` has `MapCameraPawn` saved as its default pawn. A saved Blueprint value overrides a C++ constructor default. |
| "Existing camera still functions" | The camera view is preserved but static. Pan, look and zoom input is inactive. | The camera is now an unpossessed `AMapCameraPawn`, and that pawn only binds its input when possessed. |
| Ground collision belongs to GMS-104 | A basic ground collision was added during GMS-102 | The character fell through the map (`TileMapRenderer` set tiles to `NoCollision`), so the capsule could not be verified. Boundary walls were already `BlockAll`. GMS-104 now only needs to review and extend it. |

## Files Changed

| File | Change |
|---|---|
| `Source/GrantMoney/Public/GrantMoneyPlayerCharacter.h`, `Private/GrantMoneyPlayerCharacter.cpp` | `ACharacter` subclass. No tick, no input. Controller rotation off. `PossessedBy` log. |
| `Source/GrantMoney/Public/GrantMoneyGameMode.h`, `Private/GrantMoneyGameMode.cpp` | `PlayerCharacterClass` and `CameraPawnClass` properties. Pawn class override. In `HandleStartingNewPlayer_Implementation`, turns off auto camera management, spawns an unpossessed `AMapCameraPawn` at the PlayerStart (yaw only), and sets it as the view target. Logs an error and falls back to the character's view if the spawn fails. |
| `Source/GrantMoney/Public/TileMapRenderer.h`, `Private/TileMapRenderer.cpp` | `BuildGroundCollision` creates hidden collision slabs under each rendered tile. New Collision settings: `bGenerateGroundCollision`, `GroundCollisionThickness`, `GroundCollisionProfile`. |

Not touched: `MapCameraPlayerController`, `PauseMenuWidget`, `MapCameraPawn`, `MapArrays`, `MapBoundaryGenerator`, `Build.cs`, ini files.

## Architecture After GMS-102

```text
BP_GrantMoneyGameMode (derives from AGrantMoneyGameMode)
   ├─ spawns AGrantMoneyPlayerCharacter at the PlayerStart
   │       └─ possessed by MapCameraPlayerController (existing, unchanged)
   │              └─ owns the pause system (unchanged)
   └─ spawns AMapCameraPawn (unpossessed) at the PlayerStart
           └─ is the controller's view target (static view)

ATileMapRenderer
   ├─ tile planes (visual only)
   ├─ ground collision slabs (hidden, BlockAll)
   └─ boundary walls (BlockAll)
```

## Compile and Test Procedure

1. Close the Unreal Editor. Live Coding does not reliably pick up new classes or constructor changes.
2. Build `GrantMoneyEditor` (Development Editor, Win64) in Visual Studio. Fix any errors.
3. Reopen the Editor and open `PlayGame1`.
4. Check World Settings: the GameMode Override is `BP_GrantMoneyGameMode`. One PlayerStart exists.
5. Select the `TileMapRenderer` actor and click Rebuild Map. Check the Collision category in Details.
6. Press Play. Check the Output Log (see the log lines below).
7. Press F8 to eject. In the World Outliner, confirm `GrantMoneyPlayerCharacter_0` and a separate `MapCameraPawn`.
8. Run `show Collision` to see the capsule and the ground slabs.
9. Confirm the tiles render, the view matches the Module 1 start, and the pause menu opens, resumes and quits.
10. Turn `bGenerateGroundCollision` off and replay. The character should fall, which confirms the setting works. Turn it back on.

### Log Lines to Look For

```text
LogLoad: Game class is 'BP_GrantMoneyGameMode_C'
LogTemp: GrantMoneyPlayerCharacter: GrantMoneyPlayerCharacter_0 possessed by MapCameraPlayerController_0.
LogTemp: Warning: TileMapRenderer finished. Created 2896 tiles.
LogTemp: TileMapRenderer: Created 2896 ground collision slabs.
LogTemp: Warning: MapBoundaryGenerator: Created 304 wall segments.
LogTemp: MapCameraPawn: Level 2 bounds X 400..5900 Y 400..5900
```

There should be no `no pawn was spawned` or `camera pawn could not be spawned` errors.

## Verification Results

| Criterion | Result | Evidence |
|---|---|---|
| C++ player character exists and compiles | Confirmed by developer | Built and ran in the Editor |
| PlayGame spawns and possesses it | Confirmed by developer | Possession log line, and the character in the Outliner |
| Capsule and `CharacterMovementComponent` | Capsule confirmed visible. `CharacterMovement` is inherited from `ACharacter`. | Outliner and `show Collision` |
| Character stands on the map | Confirmed by developer | Capsule visible and not falling after the ground collision was added |
| TileGen renderer still works | Confirmed by log | 2896 tiles, 304 wall segments |
| Existing camera view preserved | Camera pawn spawned (bounds log). Static view. | Not independently re-checked against the Module 1 start view |
| Pause system still works | Reported working in the first test | Re-confirm after the ground collision change |
| No GMS-103 movement added | By design | No input code on the character |

## AI Workflow Log

| # | Prompt | Result |
|---|---|---|
| 1 | Read-only investigation: seven questions about the GameMode, controller, camera, default pawn, pause system, any existing character, and files to change. | Architecture report. The AI said it could not read Blueprint assets, so the Blueprint-held values were inferred. |
| 2 | Developer answered from the Editor: default pawn is `MapCameraPawn`, controller is `MapCameraPlayerController`, GameMode override is `BP_GrantMoneyGameMode`, a PlayerStart exists. | Confirmed the inferred architecture. |
| 3 | Implement only the minimum character foundation. List files before editing. Do not claim compile or run results. | Character, GameMode and camera-preserving spawn. |
| 4 | "`show Collision` doesn't show the capsule." | Explained the falling character and the missing floor, and suggested a static check. |
| 5 | "Set up a very basic collision system that can be iterated on." | Ground collision slabs with Details settings. |
| 6 | "Is the player becoming possessed?" (log pasted) | Confirmed from the `PossessedBy` log line. |

## AI Mistakes, Corrections and Surprises

- **Blueprint overrides C++ defaults.** Setting `DefaultPawnClass` in C++ would not have worked. This was caught during planning and handled with the override.
- **Camera lost its input.** A possessed character means the unpossessed camera pawn no longer receives pan, look and zoom input. This was flagged before implementation and accepted as a known limitation.
- **The capsule did not show up.** The tiles had `NoCollision`, so the character fell. The first diagnosis was a display issue. The log's possession line proved the spawn was fine, and the actual cause was the missing floor.
- **Scope pulled in from GMS-104.** Basic ground collision had to be added early so the character could exist safely in the world.

## Media Capture Plan

Nothing below has been captured yet unless noted. Numbering follows the folder structure at the top of this document. The three extra items are marked "new".

| File | What to show |
|---|---|
| `00-baseline.mp4`, `00-baseline.png` | `PlayGame` before the change: generated world, existing camera, no character. This must be recorded from a branch or build before these code changes. |
| `01-existing-camera-architecture.png` | Code or Details view of `AMapCameraPawn` (spring arm and camera components), or the architecture report from prompt 1. |
| `02-create-character-class.mp4`, `.png` | The Editor's new C++ class dialog creating `GrantMoneyPlayerCharacter` (parent `Character`). |
| `03-character-code.png` | The final `GrantMoneyPlayerCharacter.h` and `.cpp`. |
| `04-player-spawned.png` | The character capsule in the generated level (use `show Collision`, since there is no mesh yet). |
| `05-possession-proof.png` | The Output Log line `GrantMoneyPlayerCharacter_0 possessed by MapCameraPlayerController_0`, ideally with the Outliner visible. |
| `06-player-in-world.mp4` | 15–30 seconds: Play, world loads, capsule visible, static camera view, pause and resume. |
| `07-character-falls.mp4` (new) | 10–15 seconds with `bGenerateGroundCollision` off: the capsule falls out of the world. Reproducible, so it can be recorded at any time. |
| `08-ground-collision-settings.png` (new) | The `TileMapRenderer` Details panel with the Collision category. |
| `09-show-collision.png` (new) | `show Collision` with the ground slabs and the capsule standing on them. |

## Voice-Over Outline

> **Intro (over `00-baseline.mp4`).** "At the start of Module 2 our generated world exists, but the player is only a camera. Our first goal is a real Unreal C++ character, without breaking what we built in Module 1."
>
> **Investigate (over `01-existing-camera-architecture.png`).** "Before changing anything we ask the AI to inspect the project. It can't read Blueprint assets, so we check those settings in the Editor ourselves."
>
> **Create the class (over `02-create-character-class.mp4` and `03-character-code.png`).** "We create a C++ `ACharacter`. It gives us the collision capsule and the movement component for free, and it is the right base for animation and combat later. We keep it minimal."
>
> **Spawn and possess (over `05-possession-proof.png`).** "The GameMode tells Unreal to spawn this character, and the existing controller possesses it. The log confirms it. A Blueprint can override C++ defaults, so we override the pawn class in code."
>
> **Keep the view.** "The character has no camera, so we keep the original camera as a separate actor and make it the view. The view doesn't change."
>
> **The failure (over `07-character-falls.mp4`).** "The capsule falls out of the world. Our tiles are only visual and have no collision."
>
> **The fix (over `08-ground-collision-settings.png` and `09-show-collision.png`).** "We add a simple invisible floor to the tile renderer, with settings we can change later."
>
> **Result (over `06-player-in-world.mp4`).** "The character stands on the map. The tiles still render and pause still works. Next up: movement."

## Known Limitations

- The camera view is static until its input is re-routed (GMS-103 or later camera work).
- The ground collision is a flat placeholder. GMS-104 should review it, not rebuild it.
- The character has no visible mesh yet.
- If the camera pawn fails to spawn, the fallback uses the character's view, which has no camera.
- Auto camera management is only turned off at login, so a respawn may switch the view onto the character.

## Suggested Commit Message

```text
GMS-102: Add playable C++ character spawned and possessed in PlayGame

- Add AGrantMoneyPlayerCharacter (ACharacter) with minimal defaults
- GrantMoneyGameMode spawns the character via GetDefaultPawnClassForController
  and keeps an unpossessed AMapCameraPawn as the view target
- TileMapRenderer: add optional invisible ground collision slabs under each tile
  (bGenerateGroundCollision, GroundCollisionThickness, GroundCollisionProfile)

Movement input, animation, combat, and the follow camera are not part of this change.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>
```

Branch: `GMS-102-playable-player-character`. Do not commit or push until the pause check is re-confirmed and the diff has been reviewed.

---

# GMS-103 – Camera-Relative Enhanced Input Movement


After GMS-102 has been reviewed and merged, start GMS-103 from the updated `main` branch.

---

# Step 1 – Create the GMS-103 Branch

```bash
git checkout main
git pull
git checkout -b GMS-103-camera-relative-movement
```

---

# Step 2 – Capture the Starting State

Record the character from GMS-102.

The character should spawn and be possessed but should not yet have the new camera-relative movement system.

---

## Capture

```text
00-character-no-movement.mp4
```

Record approximately 10–20 seconds.

---

## Future Video Editor Use

Use this to introduce the second problem:

> "The character now exists and is possessed, but it still needs a gameplay movement system."

---

# Step 3 – Ask AI to Inspect the Existing Input Architecture

Do not immediately create a second input system.

First determine whether the project already uses Unreal's Enhanced Input framework.

---

## AI Prompt

```text
We are starting GMS-103: camera-relative player movement using Enhanced Input.

Follow the repository rules.

Inspect only the relevant player, PlayerController, camera, Enhanced Input, and configuration source.

Do not edit yet.

Determine:

- what Enhanced Input system already exists;
- where mapping contexts are currently added;
- whether a movement Input Action already exists;
- how the active gameplay camera's rotation can be queried without tightly coupling the player to a specific camera class;
- the minimum files that should change.

The movement must be relative to the ACTIVE CAMERA'S yaw, not fixed world X/Y.

Do not implement player-follow camera behavior.
Do not implement animation.
Do not change TileGen.
Do not change MapArrays.

Return a proposed implementation before modifying anything.
```

---

# Step 4 – Create or Verify the Movement Input Action

If suitable Enhanced Input assets do not already exist, create:

```text
IA_Move
```

Recommended type:

```text
Axis2D
```

The project may also need a mapping context such as:

```text
IMC_Player
```

Example mapping concept:

```text
W → Y +1
S → Y -1
A → X -1
D → X +1
```

Because Unreal Input Action and Mapping Context assets are `.uasset` files, these should normally be configured manually through the Unreal Editor rather than asking the AI to directly modify the binary assets.

---

## Capture

### Screenshot

```text
01-ia-move.png
```

Show the `IA_Move` Input Action configuration.

### Screenshot

```text
02-mapping-context.png
```

Show the WASD mappings inside the input mapping context.

---

## Future Video Editor Use

Use these screenshots while explaining Unreal Enhanced Input.

Suggested narration:

> "Instead of handling each keyboard key directly in gameplay code, we use Unreal's Enhanced Input system. A two-dimensional Move action allows the character to receive horizontal and vertical movement as one reusable input value."

---

# Step 5 – Implement Camera-Relative Movement

The basic concept is:

```text
Player presses W
        ↓
Read active camera yaw
        ↓
Calculate camera forward direction
        ↓
Move character forward relative to that view
```

Conceptual code may resemble:

```cpp
FVector2D Input = Value.Get<FVector2D>();

FRotator CameraRotation = /* active gameplay camera rotation */;
FRotator YawRotation(0.0f, CameraRotation.Yaw, 0.0f);

FVector Forward =
    FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

FVector Right =
    FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

AddMovementInput(Forward, Input.Y);
AddMovementInput(Right, Input.X);
```

The exact implementation should be based on the existing player and camera architecture.

The important design rule is:

```text
W means "forward relative to what the player sees"
```

rather than:

```text
W always means world +X
```

---

## Capture

### Screenshot

```text
03-input-binding-code.png
```

Show the important portion of the movement/input code.

---

# Step 6 – Verify Camera-Relative Movement

Run the game.

Test:

```text
W
A
S
D
W + D
W + A
S + D
S + A
```

If the camera orientation can currently change, rotate/change the camera and verify that movement follows the camera's direction.

Also verify that diagonal movement is not significantly faster than straight movement.

---

## Capture

### Video

```text
04-camera-relative-movement.mp4
```

This should be one of the main Sprint 1 instructional clips.

Record:

1. character moving forward;
2. sideways movement;
3. diagonal movement;
4. camera orientation changing if available;
5. movement remaining relative to the current camera view.

---

## Future Video Editor Use

This is the primary result clip for GMS-103.

Suggested narration:

> "The important difference is that movement is calculated from the active camera's yaw. This means pressing W moves the character forward from the player's perspective rather than toward a fixed world axis."

If possible, show the camera orientation changing immediately before demonstrating movement.

---

# GMS-104 – Generated World and Boundary Collision

Once GMS-103 is complete and merged, create a separate branch for collision.

Note: a basic ground collision (hidden slabs under each rendered tile, with `bGenerateGroundCollision`, `GroundCollisionThickness` and `GroundCollisionProfile` on `ATileMapRenderer`) was already added during GMS-102 so the character could stand on the map. See the GMS-102 As-Built Record. This issue should review and extend that work, not rebuild it. Boundary walls were already `BlockAll`.

```bash
git checkout main
git pull
git checkout -b GMS-104-player-world-collision
```

---

# Step 1 – Capture Existing Collision Behavior

Before making changes, intentionally test the character against:

- map boundaries;
- generated ground;
- major world geometry.

---

## Capture

```text
00-before-collision.mp4
```

Record any visible incorrect behavior.

Useful examples include:

- player walking through a boundary;
- player falling through the floor;
- player entering invalid map space.

---

## Future Video Editor Use

Failed behavior is excellent instructional material.

Use this clip before showing the collision implementation.

Suggested narration:

> "Movement alone is not enough. Our generated world also needs to behave like physical space. Before adding collision, the character could interact incorrectly with the map boundary."

---

# Step 2 – Ask AI to Inspect Existing Collision Systems

---

## AI Prompt

```text
We are starting GMS-104: player collision with the generated world and map boundaries.

Inspect only:

- GrantMoneyPlayerCharacter;
- TileMapRenderer;
- the existing generated boundary implementation;
- relevant collision configuration.

Do not edit yet.

Determine why the player currently can or cannot:

- stand on the generated ground;
- pass through boundary walls;
- pass through major world geometry.

Preserve the existing data-driven boundary generation.

Do not introduce hardcoded level coordinates.
Do not rewrite MapArrays.
Do not add map-specific logic.

Propose the smallest collision changes necessary.
```

---

# Step 3 – Implement the Minimum Collision Changes

The player should continue using the `ACharacter` capsule for collision.

Generated boundaries should continue coming from the existing map/boundary system.

Avoid code such as:

```text
if Level1:
    wall = coordinate X
```

Collision should follow the generated world's data-driven structure.

---

## Capture

### Screenshot

```text
01-boundary-collision-code.png
```

Show the most important collision-related code or configuration.

---

# Step 4 – Verify Ground Collision

Confirm that the character:

```text
spawns
→ stands on the world
→ moves normally
→ does not fall through the generated ground
```

---

## Capture

```text
02-ground-collision.mp4
```

A short clip should show the player moving normally across the generated ground.

---

# Step 5 – Verify Boundary Collision

Walk directly into the generated map boundary.

The player should stop instead of moving into invalid map space.

---

## Capture

```text
03-wall-collision.mp4
```

Record approximately 10–20 seconds showing:

1. character approaching the boundary;
2. character walking directly into it;
3. collision preventing the player from passing through;
4. character moving along the boundary afterward.

---

## Future Video Editor Use

This is the strongest GMS-104 result clip.

A good editing pattern is:

```text
00-before-collision.mp4
        ↓
01-boundary-collision-code.png
        ↓
03-wall-collision.mp4
```

Suggested narration:

> "Rather than adding hardcoded invisible walls for each level, we reused the data-driven boundary system from Module 1. The player's collision capsule now interacts with those generated boundaries automatically."

---

# Sprint 1 Final Verification

Before considering the programming sprint complete, verify the full sequence.

```text
GMS-102
Player character exists
        ↓
Player spawns
        ↓
Player is possessed

GMS-103
        ↓
Enhanced Input movement
        ↓
Movement relative to active camera

GMS-104
        ↓
Ground collision
        ↓
Generated boundary collision
```

The completed experience should be:

```text
Launch PlayGame
        ↓
Generated world loads
        ↓
Player character appears
        ↓
PlayerController possesses character
        ↓
WASD moves relative to camera
        ↓
Character remains on generated ground
        ↓
Generated boundaries stop character
```

---

# Final Sprint 1 Media Package

The future video editor should receive:

```text
Task documentation
AI prompts
AI corrections/failures
Numbered screenshots
Numbered raw clips
Final verification results
Git/PR references
Known limitations
```

The editor should not need to reconstruct the programming process by interviewing the developer afterward.

---

# Suggested Sprint 1 Instructional Video Structure

## Section 1 – Starting Point

Use:

```text
GMS-102/00-baseline.mp4
```

Explain that Module 1 created the world and camera, but no playable character exists yet.

---

## Section 2 – Understanding the Existing Architecture

Use:

```text
GMS-102/01-existing-camera-architecture.png
```

Explain how AI was used to inspect the current GameMode, PlayerController, pawn, and camera before modifying anything.

---

## Section 3 – Creating the Player Character

Use:

```text
GMS-102/02-create-character-class.mp4
GMS-102/03-character-code.png
```

Explain why `ACharacter` was chosen.

---

## Section 4 – Spawning and Possession

Use:

```text
GMS-102/05-possession-proof.png
GMS-102/06-player-in-world.mp4
```

Explain Unreal's PlayerController → Pawn possession relationship.

---

## Section 5 – Enhanced Input

Use:

```text
GMS-103/01-ia-move.png
GMS-103/02-mapping-context.png
GMS-103/03-input-binding-code.png
```

Explain how `IA_Move` and the mapping context translate WASD into a 2D movement value.

---

## Section 6 – Camera-Relative Movement

Use:

```text
GMS-103/04-camera-relative-movement.mp4
```

Explain how movement uses the active camera's yaw rather than fixed world coordinates.

---

## Section 7 – Collision

Use:

```text
GMS-104/00-before-collision.mp4
GMS-104/01-boundary-collision-code.png
GMS-104/03-wall-collision.mp4
```

Explain the difference between having movement and having a physically valid playable world.

---

## Section 8 – Final Result

Use portions of:

```text
GMS-102/06-player-in-world.mp4
GMS-103/04-camera-relative-movement.mp4
GMS-104/03-wall-collision.mp4
```

Create one short montage showing:

```text
Spawn
→ Possession
→ Movement
→ Collision
```

Suggested closing narration:

> "At the end of Sprint 1, the generated world is no longer just something we can inspect. It now contains a real Unreal C++ player character that can spawn, receive input, move relative to the gameplay camera, and interact physically with the boundaries generated from our map data."

---

# AI Workflow Material to Preserve

Do not save only the successful final prompts.

Also preserve examples where:

- the AI misunderstood the existing architecture;
- the AI attempted an unnecessary refactor;
- a generated solution failed to compile;
- camera possession behaved differently than expected;
- input mappings were incorrect;
- collision behaved incorrectly;
- the human developer changed the AI's proposed design.

These moments are valuable for the later **AI Workflow Review**.

A useful course lesson is not:

> "The AI generated the correct answer immediately."

A much more realistic lesson is:

```text
We inspected the project
→ AI proposed a solution
→ we tested it
→ something was wrong
→ we identified why
→ corrected the prompt or implementation
→ verified the final result
```

That is the workflow future students should learn.

---

# End-of-Task AI Prompt Template

Use this after each Jira issue.

```text
This Jira issue is now at its final verification stage.

Actual results:

[PASTE THE REAL TEST RESULTS]

Review only the changes associated with this Jira issue.

Check for:

- unrelated changes;
- unnecessary refactoring;
- duplicated systems;
- hardcoded map-specific logic;
- changes that belong to another Jira issue;
- documentation claims that were not actually verified.

Then create:

1. implementation summary;
2. files changed and why;
3. reproducible development steps;
4. AI prompts used;
5. AI mistakes/corrections;
6. actual verification results;
7. screenshot and video references;
8. known limitations;
9. suggested Git commit message;
10. instructional voice-over outline.

Do not invent missing steps.
Do not claim tests passed unless I explicitly confirmed them.
Do not commit or push.
```

---

# Sprint 1 Definition of Done

Sprint 1 is ready for review when:

```text
[ ] GMS-102 is complete.
[ ] Player character spawns.
[ ] PlayerController possesses the player.
[ ] Existing camera still functions.

[ ] GMS-103 is complete.
[ ] Enhanced Input controls movement.
[ ] WASD movement is camera-relative.
[ ] Diagonal movement behaves correctly.
[ ] Pause/resume does not break controls.

[ ] GMS-104 is complete.
[ ] Player stands on generated ground.
[ ] Player cannot pass through generated boundaries.
[ ] No map-specific boundary coordinates were introduced.

[ ] Project compiles.
[ ] Packaged/QA verification is ready for GMS-107.
[ ] Numbered screenshots exist.
[ ] Numbered raw video clips exist.
[ ] AI prompts and corrections were preserved.
[ ] Documentation reflects what actually happened.
```
```