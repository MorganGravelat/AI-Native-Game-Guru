# GMS-127 / GMS-128 – Enemy Base and First Slime Enemy
## Module 3 Sprint 1 – Starting Document

This is the **starting document** for the first two Module 3 tasks. It collects everything known before and during implementation, so a later editor can write the per-issue guides (same style as `GMS-106` and `GMS-111` in Module 2) without re-doing the investigation.

Status of this document: **implemented, compiled and reviewed. The developer play-tested the first version in the Editor and reported it worked. A pursuit review (Step 6) then made two small corrections, and a final checkpoint review (Step 7) inspected both classes against the acceptance criteria. Both were verified only by reading source and by headless simulation. The corrected build, and the manual Tests A to F in Step 7, have NOT yet been performed by the developer. No manual test is marked passed.** Sections marked `_you fill in_` are left blank on purpose and must only be filled in with what actually happened.

**What is and is not evidence in this document:**

| Kind | Meaning |
|---|---|
| Read from source | Taken from reading project or engine source. Not a test. |
| Compiled | UnrealBuildTool finished with `Result: Succeeded`. Says nothing about behaviour. |
| Developer reported | The developer played it in the Editor and said what they saw. |
| Headless simulation | A temporary automated test in a synthetic world (flat floor, stand-in player, no real map, no walls, no pause menu). Real numbers, but not the real game. |

---

## Jira Issues

| Key | Goal |
|---|---|
| GMS-127 | Create a modular C++ enemy base for future enemy types. |
| GMS-128 | Implement the first slime enemy with direct player pursuit. |

The Jira "Done When" lists for these two issues are not copied here. _you fill in_ from Jira. The acceptance criteria used for the final review (Step 7) came from the developer's review prompt, not from Jira.

### Course Capture Requirement

The Jira capture text for these two issues was not available. The requirement below is **the one carried by earlier issues (GMS-106, GMS-111)** and is assumed to apply. Confirm against Jira.

- Record short raw clips while building the feature, not only a finished demo.
- Save at least one screenshot for each meaningful step and use numbered filenames.
- Save useful AI prompts, corrections, and at least one verification result.
- Upload raw media to the shared media folder, not the Git repository.
- Documentation must describe what actually happened. Do not claim a test passed unless it was performed.

---

## Scope of This First Milestone

### Behaviour to prove

1. There is a reusable C++ enemy base class.
2. A slime derives from that enemy base.
3. The slime finds the current player at runtime.
4. The slime continuously moves directly toward the player.
5. Movement speed and stopping distance are configurable.
6. The reusable base contains no slime-specific assumptions.

### Explicitly out of scope

- Enemy spawning
- Wall or terrain climbing
- XP drops
- Damage and combat
- Death
- Slime emergence animation
- Weapons
- Difficulty scaling
- UI

### Do not modify

TileGen, `MapArrays`, `TileMapRenderer`, map-generation systems, camera systems, or unrelated gameplay systems.

Keep the architecture intentionally small. This milestone proves the enemy abstraction with one slime. It is not the design of the finished enemy system.

---

## The Opening Prompt (Preserved)

This is the prompt used to start the investigation. Keep it for the AI workflow log.

```text
We are beginning Module 3 of the Grant Money Unreal Engine C++ project.

Jira goals:
- GMS-127: Create a modular C++ enemy base for future enemy types.
- GMS-128: Implement the first slime enemy with direct player pursuit.

For this first milestone, DO NOT implement:
- enemy spawning
- wall/terrain climbing
- XP drops
- damage/combat
- death
- slime emergence animation
- weapons
- difficulty scaling
- UI

The only behavior we want to prove right now is:

1. There is a reusable C++ enemy base class.
2. A slime derives from that enemy base.
3. The slime finds the current player at runtime.
4. The slime continuously moves directly toward the player.
5. Basic movement speed and stopping distance are configurable.
6. The solution does not contain slime-specific assumptions in the reusable base.

Before writing code:

1. Inspect the repository structure.
2. Read only the smallest relevant set of files.
3. Identify the current player Character/Pawn class and how it is spawned/possessed.
4. Identify the project module name and existing Public/Private source structure.
5. Check whether any health, damage, enemy, movement, or targeting abstractions already exist.
6. Do not inspect generated Unreal folders or binary .uasset/.umap files.
7. Do not modify TileGen, MapArrays, TileMapRenderer, map-generation systems, camera systems, or unrelated gameplay systems.

Then report:

- files you inspected;
- what class represents the player;
- what existing systems can be reused;
- the minimum new classes/files you recommend;
- likely files that would be modified;
- any assumptions you still need to verify.

DO NOT MODIFY FILES YET.

Keep the architecture intentionally small. We are proving the enemy abstraction with one slime, not designing the entire finished enemy system.
```

---

# Starting Point (What Already Exists)

This is from reading the source before GMS-127. It is not a test result.

## Project Structure

| Item | Value |
|---|---|
| Unreal Engine version | 5.8 (`GrantMoney.uproject`) |
| Game module | `GrantMoney` (one Runtime module). API macro: `GRANTMONEY_API`. |
| Source root | `Game/Source/GrantMoney/` |
| Layout | `Public/*.h` and `Private/*.cpp`. Flat, except `Private/Maps/`. |
| Module dependencies | Core, CoreUObject, Engine, InputCore, EnhancedInput, UMG, Slate, SlateCore. No new dependency is needed for this milestone. |
| Default game mode | `BP_GrantMoneyGameMode` (Blueprint on top of `AGrantMoneyGameMode`), set in `Config/DefaultEngine.ini`. Values saved in the Blueprint can override C++ defaults. |

## Player

```text
BP_GrantMoneyGameMode (derives from AGrantMoneyGameMode)
   ├─ spawns AGrantMoneyPlayerCharacter on a safe map cell (GMS-106)
   │       └─ possessed by AMapCameraPlayerController (standard APlayerController flow)
   └─ spawns AMapCameraPawn (NOT possessed), the view target, which follows the character (GMS-111)
```

| Question | Answer |
|---|---|
| Player class | `AGrantMoneyPlayerCharacter`, derived from `ACharacter`. |
| Components | Default capsule, `CharacterMovementComponent`, and a cube placeholder mesh with no collision. The character has no camera of its own. |
| Movement | Camera-relative, through `AddMovementInput`. `bOrientRotationToMovement` is on. |
| Spawn | `AGrantMoneyGameMode::SpawnDefaultPawnFor_Implementation` uses `FPlayerSpawnResolver` and the map data to choose a safe cell. It falls back to the PlayerStart. |
| Possession | Standard. The controller is `AMapCameraPlayerController`. |
| Class used | `GetDefaultPawnClassForController_Implementation` always returns `PlayerCharacterClass`. |

## Existing Enemy-Related Abstractions

**None.** A search of `Source/GrantMoney` found no enemy, slime, health, damage, AI controller, navigation, team or faction code. The only "target" hits are camera follow code (`SetFollowTarget`, `FollowTarget`) and spring arm length, which are unrelated.

## Collision and Navigation Facts

- The ground is a generated collision slab. The tile planes are visual only.
- The ground slab uses a configurable `GroundCollisionProfile` (`ATileMapRenderer`).
- The ground slab and the player placeholder mesh both set `SetCanEverAffectNavigation(false)`, so the project has **no navigation mesh**. That suits direct pursuit.
- The project's custom collision profiles were not inspected. They are Editor settings (`DefaultEngine.ini` has no collision entries that matched).

---

# What Can Be Reused

| Existing piece | How it helps |
|---|---|
| `ACharacter` and `CharacterMovementComponent` | Movement, capsule collision, ground following and rotation toward movement. The player uses the same approach. |
| Placeholder mesh pattern in `AGrantMoneyPlayerCharacter` | The slime can use an engine basic shape (for example `/Engine/BasicShapes/Sphere`) through `ConstructorHelpers`, with collision disabled on the mesh and the capsule doing collision. |
| `UGameplayStatics::GetPlayerPawn(this, 0)` | Finds the current player at runtime, with no dependency on `AGrantMoneyPlayerCharacter` or the game mode. |
| Ground slab collision | Already works for the player, so the same capsule should walk on it. |
| Code style | `UE_LOG(LogTemp, …)`, comments only where they explain *why*, `UPROPERTY` categories, `GRANTMONEY_API`. |

---

# Recommended Minimum Design

Two new classes and four new files. No module, build or config changes.

| File | Class | Purpose |
|---|---|---|
| `Public/EnemyBase.h` | `AEnemyBase : ACharacter` (abstract) | Reusable enemy base. Declares configuration and the extension points. |
| `Private/EnemyBase.cpp` | | Implements target finding and direct movement. |
| `Public/SlimeEnemy.h` | `ASlimeEnemy : AEnemyBase` | First enemy type. |
| `Private/SlimeEnemy.cpp` | | Slime placeholder look, capsule size and default values. |

### `AEnemyBase`

- **Configurable properties:** movement speed and stopping distance, both `EditAnywhere`.
- **Target:** resolved at runtime through a protected virtual `FindTarget()` that defaults to the first player pawn. It must tolerate a null target (the player may not exist yet, or may be replaced) and look up again.
- **Movement:** every tick, a virtual direction function returns the flat direction toward the target, and zero inside the stopping distance. The result goes to `AddMovementInput`. The movement component's max walk speed is set from the configured speed.
- **Type knowledge:** only `AActor` or `APawn`. It must not reference the slime or the player character class.

### `ASlimeEnemy`

- Placeholder visuals, capsule size and default speed and stopping distance. Nothing else.
- Climbing, emergence and any other slime behaviour belong to later tickets.

### Files Likely Modified

**None required.** The new classes are self-contained, so existing files stay untouched.

For testing, the plan is to place a slime in the level by hand (Place Actors, or a Blueprint subclass). Spawning slimes from the game mode would be a spawning feature and is out of scope. If a code-based test spawn is ever wanted, the only file touched would be `GrantMoneyGameMode.cpp`, and that conflicts with "no spawning", so it is not recommended.

---

# Assumptions and Open Questions to Verify

These were untested when this document was first written. The outcomes are recorded under "What Actually Happened → Verification Results".

| # | Assumption or question | How to settle it |
|---|---|---|
| 1 | An engine basic shape (sphere) is acceptable as the slime placeholder, rather than a real asset. | Confirm with the developer. |
| 2 | A controller-less `ACharacter` moves correctly with `AddMovementInput`. The default `AutoPossessAI` is `PlacedInWorld` and the default AI controller class is `AAIController`, so a controller may be created anyway. The plan adds no AI controller yet. | Test in the Editor: the slime must move toward the player with and without an auto-created AI controller. Check rotation and acceleration settings for a pawn without a player controller. |
| 3 | The default capsule collides with the generated ground slab and walls the same way the player does. | Test. Collision profiles live in Editor settings and were not read. |
| 4 | Ticking every frame and caching the player pointer is simplest. The target must be re-resolved when it is null or destroyed. | Test: start Play with the slime already placed, since it can exist before the player pawn spawns. |
| 5 | A slime placed in the level exists before the player pawn spawns, so the base must handle a missing target at start. | Test, together with item 4. |
| 6 | With no navmesh, the slime moves in a straight line and slides along walls instead of pathfinding. This matches "directly toward the player" but should be stated as a limitation. | Observe and document. |
| 7 | Pausing is handled by `AMapCameraPlayerController`. Standard tick pausing should stop the slime. How the pause menu pauses the game was not inspected. | Test pause and resume with a slime chasing. |
| 8 | The project is C++-first. Expose the slime for placement, and add a Blueprint subclass (for example `BP_Slime`) only if the developer wants editor tuning. | Confirm with the developer. |

---

# Development and Documentation Workflow

```text
Record baseline
        ↓
Inspect existing architecture (read-only)   ← done, see above
        ↓
Plan the smallest change                    ← done, see above
        ↓
Implement one meaningful step
        ↓
Compile / test
        ↓
Capture screenshot or short video
        ↓
Tell the AI what actually happened
        ↓
Update documentation
        ↓
Review diff
        ↓
Suggested commit message
```

Source control rules for this module (from `CLAUDE.md`): one Jira issue, one branch, one pull request. Branch names follow `GMS-XXX-description`. Never commit or push unless explicitly told to. Do not commit generated Unreal folders or raw recordings.

**Branch decision (developer):** the usual rule is one Jira issue, one branch, one pull request. For this work **GMS-127 and GMS-128 are combined on a single branch**, because the slime exists only to prove the base class. The branch is `GMS-127-128` (read from git). It does not follow the `GMS-XXX-description` naming rule in `CLAUDE.md`; that is the developer's call and is only noted here. The pull request is still the developer's to create.

A second preference from the developer: **avoid changing files that already exist.** Everything in this milestone is new files only. The only existing file that changed is a Content asset that the Editor rewrote (see the `PlayGame1.umap` warning below).

---

# Steps and Their Status

Status uses the evidence kinds defined at the top. "Not reported" means the developer has not said either way.

| Step | Issue | Description | Status |
|---|---|---|---|
| 1 | Both | Create the development branch. | Done by the developer. Branch is `GMS-127-128` (read from git, not created by the AI). |
| 2 | Both | AI inspection of the repository. | Done (read from source). See Step 2. |
| 3 | Both | Review the proposal before accepting it. | Done. Developer accepted the new-files-only plan. |
| 4 | GMS-127 | Add `AEnemyBase` (header and source). | Done. See Step 4. |
| 5 | GMS-127 | Compile. | Compiled, `Result: Succeeded`. |
| 6 | GMS-128 | Add `ASlimeEnemy`, deriving from `AEnemyBase`. | Done. See Step 5. |
| 7 | GMS-128 | Compile. | Compiled, `Result: Succeeded`. |
| 8 | GMS-128 | Place a slime in the level and verify it finds and chases the player. | Developer reported it worked (first version). |
| 9 | GMS-128 | Verify speed and stopping distance are configurable. | Not reported separately. |
| 10 | GMS-128 | Verify the slime stops at the stopping distance and does not jitter or clip through walls. | Developer reported no jitter and no clipping through walls (first version). |
| 11 | GMS-128 | Verify pause and resume. | Not reported. |
| 12 | GMS-127 | Verify the base class contains no slime-specific assumptions. | Text search of `EnemyBase.h` and `EnemyBase.cpp` finds no `Slime`. Code review by AI only. |
| 13 | Both | Pursuit review (Step 6). | Done. Two corrections made, verified by headless simulation. |
| 14 | Both | Re-test the corrected version in the Editor. | **Pending.** Covered by Tests A to F in Step 7. |
| 15 | Both | Ask AI to review the final diff. | Done (Step 7). Source review and a headless multi-slime simulation. |
| 16 | Both | Run manual Tests A to F in the Editor. | **Pending.** None performed yet. |

---

# Media Capture Rules

- Short clips, about **10–30 seconds**. Voice-over can be added later.
- Record while building the feature, not only the finished result.
- Use numbered filenames so the editor can match footage to this document.
- Raw video goes in the shared media folder, **not in Git**.
- Failures are useful footage. Do not stage or delete them.
- The full numbered list is in "Master Capture Checklist" near the end. The per-step lists below use the same names.

### Recommended Media Folder Structure

Earlier issues used `Media/GMS102/`, `Media/GMS103/`, and so on. This issue is combined, so use one folder:

```text
Media/
└── GMS127-128/
    ├── 00-baseline-no-enemies.mp4
    ├── 00-baseline-no-enemies.png
    ├── 01-branch-created.png
    ├── 02-ai-inspection-findings.png
    ├── 03-player-class-and-spawn.png
    ├── 04-enemybase-header.png
    ├── 05-enemybase-details-panel.png
    ├── 06-enemybase-compile-success.png
    ├── 07-slimeenemy-code.png
    ├── 08-slime-compile-success.png
    ├── 09-slime-placed-in-level.png
    ├── 10-test-a-one-slime.mp4
    ├── 11-test-b-moving-player.mp4
    ├── 12-test-c-player-stops.mp4
    ├── 13-test-d-multiple-slimes.mp4
    ├── 14-test-e-no-valid-player.mp4
    ├── 14b-test-e-no-target-warning-log.png
    ├── 15-test-f-stop-restart-pie.mp4
    ├── 16-slowdown-distance-details.png
    ├── 17-fast-slime-slowdown-50.mp4
    ├── 18-fast-slime-slowdown-0-stutter.mp4
    ├── 19-no-aicontroller-in-outliner.png
    ├── 20-pursuit-review-findings.png
    ├── 21-decoupling-check.png
    ├── 22-multi-slime-bumping.mp4
    ├── 23-git-status-final.png
    └── 24-final-verification.png
```

Use these names, or update this list if you rename them.

---

# Step 1 – Create the Development Branch

One combined branch for GMS-127 and GMS-128 (see "Branch decision" above). The developer created it. It exists as `GMS-127-128`.

```bash
git branch --show-current
git status
```

## Capture

### Clip

```text
00-baseline-no-enemies.mp4
```

Ideally recorded **before** any enemy code. That moment has passed, so this is a retroactive substitute: open `PlayGame1`, remove or hide the placed slime **without saving the map**, press Play, and walk around for 10–15 seconds to show a world with no enemies. Skip it if that is awkward, and say in the video that the baseline is a recreation.

### Screenshot

```text
00-baseline-no-enemies.png
01-branch-created.png
```

- `00`: one frame from the clip above.
- `01`: the terminal showing `GMS-127-128` from `git branch --show-current`.

## Future Video Editor Use

This is the "before" shot for the whole issue.

Suggested narration:

> "So far the world is empty. There is a player and a map, and nothing that pushes back. This task adds the first thing that comes for you."

---

# Step 2 – Ask AI to Inspect the Repository

Do not edit anything yet. The prompt is preserved in full under "The Opening Prompt (Preserved)" near the top of this document.

What the AI found, in short (read from source, not tested):

- the player is `AGrantMoneyPlayerCharacter : ACharacter`, spawned by `AGrantMoneyGameMode` on a safe map cell and possessed by `AMapCameraPlayerController`;
- the module is `GrantMoney`, with a flat `Public/` and `Private/` layout;
- nothing for enemies, health, damage, targeting, AI controllers or navigation existed;
- the ground is a collision slab with no navigation mesh, which suits direct pursuit.

## Capture

```text
02-ai-inspection-findings.png
03-player-class-and-spawn.png
```

- `02`: the AI's reply to the opening prompt, showing the files inspected and the findings.
- `03`: `GrantMoneyPlayerCharacter.h` (the class declaration) beside the spawn override in `GrantMoneyGameMode.cpp`.

## Future Video Editor Use

Suggested narration:

> "Before writing an enemy, we ask what already exists. The answer is: a player that is already a Character, a ground that already has collision, and no enemy code at all. We don't need to invent a movement system. We need to reuse the one the player uses."

---

# Step 3 – Review the Proposal Before Accepting It

Check the AI's proposal against these questions. Where a decision is known it is filled in. Where it is not, it is left for the developer.

| Decision | Options to weigh | Decision |
|---|---|---|
| Base class type | `ACharacter`, `APawn` or `AActor` | `ACharacter`, as proposed. See Step 4. |
| Where the target comes from | A hard-coded player class, or a generic lookup | Generic: the first player's pawn, in an overridable function. |
| AI controller | Use one, or drive the character directly | None. Driven directly. A later change also disabled the automatic one (Step 6). |
| Existing files | Edit the game mode to spawn a test slime, or place one by hand | The developer prefers not to change existing files. A slime is placed by hand. |
| Placeholder visual | Engine sphere, or a real asset | Engine sphere. |
| Branches | One per issue, or combined | **Combined**, the developer's choice. The AI had cited the one-branch-per-issue rule. |
| Make `AEnemyBase` abstract | Now, or when the second enemy type exists | Not decided. Left placeable. _you fill in_ |
| Blueprint child for the slime | Create one now, or later | Not created. |

Reject changes that:

- put pursuit or targeting logic in Blueprint;
- put anything slime-specific in the base class;
- reference `TileMapRenderer`, `MapArrays` or any map coordinate;
- add spawning, health, damage, pathfinding or climbing;
- change the player, the camera or the game mode.

Record at least one place where you changed or rejected an AI suggestion. This issue has two: the branch decision above, and the Step 6 review, where the AI's own first version was corrected.

---

# Step 4 – GMS-127: Implement the Enemy Base

## AI Prompt

```text
Using what you just learned from the repository, implement the minimum reusable C++ enemy foundation for GMS-127.

Goal:
Create a reusable enemy base that future simple enemies can derive from, while keeping this milestone limited to player targeting and movement.

Requirements:

1. Create a C++ enemy base class using the most appropriate Unreal class for a moving world enemy.
   - Explain why you chose ACharacter, APawn, or AActor before implementing it.
   - Prefer Unreal's existing Character movement capabilities if they are appropriate.
   - Do not introduce Blueprint gameplay logic.

2. The enemy base should contain only shared behavior needed now:
   - configurable movement speed;
   - configurable stopping distance from the player;
   - a runtime reference to the current player target;
   - a reusable method for acquiring that target;
   - reusable movement-toward-target behavior;
   - safe handling when no player exists.

3. Do NOT add yet:
   - health;
   - damage;
   - attacks;
   - death;
   - XP;
   - loot;
   - enemy spawning;
   - obstacle climbing;
   - NavMesh pathfinding;
   - behavior trees;
   - AI Controllers unless there is a strong technical reason they are required for this extremely simple direct-pursuit prototype.

4. The base must not contain anything named or designed specifically for a slime.

5. Expose appropriate tuning properties to Unreal using UPROPERTY so we can adjust values in the Editor without recompiling, but keep actual gameplay logic in C++.

6. Prefer direct pursuit appropriate for a survivor game:
   enemy → current player position

We do NOT want conventional intelligent pathfinding yet.

7. Preserve all existing systems. Do not modify MapArrays, TileMapRenderer, map generation, camera behavior, or player movement unless absolutely required. If you believe an existing file must change, explain why before changing it.

Before editing:
- list the exact files you intend to create or modify;
- explain the responsibility of each.

Then implement the smallest version.

After implementation:
- show the files changed;
- summarize each change;
- identify anything that still requires Unreal Editor setup;
- give me exact manual verification steps;
- do NOT claim the feature works unless it was actually compiled and run.
```

## Why `ACharacter`

| Option | Verdict |
|---|---|
| `ACharacter` | **Chosen.** The enemy has to walk on the generated ground slab, collide with walls, and keep a capsule. `CharacterMovementComponent` already does all of that for the player, so the enemy reuses it instead of rewriting it. |
| `APawn` | Rejected. Movement, gravity and ground-following would have to be written by hand. |
| `AActor` | Rejected. No movement or collision model at all. |

## Files Created

Nothing existing was modified.

| File | Responsibility |
|---|---|
| `Source/GrantMoney/Public/EnemyBase.h` | Declares `AEnemyBase : ACharacter`: the tuning properties and the extension points. |
| `Source/GrantMoney/Private/EnemyBase.cpp` | Target finding and direct movement. |

## Behaviour of the First Version

- `MoveSpeed` (default 200 cm/s) and `StopDistance` (default 100 cm) are editable in the Editor.
- Every tick: if there is no valid target, look for one (`FindTarget`, which defaults to player 0's pawn). Then walk toward it on the ground plane with `AddMovementInput`, and stop inside `StopDistance`.
- `FindTarget` and `GetMoveDirection` are virtual, so a later enemy can change who it chases or how.
- With no player, nothing moves, one warning is logged, and it keeps looking quietly.
- No AI controller. `bRunPhysicsWithNoController` is set, because without it a controller-less character does not run its movement at all.
- The class was left placeable (not `Abstract`) so it could be tested by itself. Marking it `Abstract` is a one-word change once there is a second enemy type. **Not done yet.**

## Compile

`GrantMoneyEditor Win64 Development` with the UE 5.8 `Build.bat`: `Result: Succeeded` (about 18 seconds). Editor was not running.

## Capture

```text
04-enemybase-header.png
05-enemybase-details-panel.png
06-enemybase-compile-success.png
```

- `04`: `EnemyBase.h`, showing the properties and the two virtual extension points.
- `05`: the Details panel of a placed enemy, showing the `Enemy|Movement` category.
- `06`: the build output showing `Result: Succeeded`. Keep any failure screenshots too.

## Future Video Editor Use

Suggested narration:

> "We chose Character, not Pawn, because Unreal already knows how to walk a capsule across a floor. The base class only knows two things: who to chase, and how to step toward them. It has never heard of a slime."

---

# Step 5 – GMS-128: Implement the First Slime

## AI Prompt

```text
Now implement GMS-128: the first slime enemy.

The reusable enemy base already exists. Do not duplicate its targeting or movement logic.

Goal:
Create one concrete Slime enemy class that proves the enemy base can support derived enemy types.

Requirements:

1. Create a C++ Slime enemy derived from the reusable enemy base.

2. For this milestone, the Slime should add little or no unique gameplay behavior.
Its purpose is to prove:

    EnemyBase
        ↓
      Slime

and not:

    Slime = entire enemy system

3. The Slime should inherit:
- player acquisition;
- direct pursuit;
- movement speed handling;
- stopping distance behavior.

4. Slime-specific configuration may override sensible defaults such as:
- movement speed;
- stopping distance;
- future visual scale if appropriate.

5. Do NOT implement yet:
- ground emergence;
- squashing/stretching;
- wall climbing;
- damage;
- player collision damage;
- death;
- XP drops;
- sounds;
- animations;
- spawning system.

6. If a mesh or visual asset is needed to test the class, use the smallest reasonable temporary visual setup and clearly separate visual configuration from gameplay logic.

7. Avoid Blueprint gameplay code.
A Blueprint child may only be suggested if it is genuinely useful as an asset/configuration container for mesh/material assignment, not as the place where pursuit logic runs.

8. The Slime must remain useful even if the final slime mesh is replaced later.

Before modifying anything:
- tell me the exact files you will create/change.

Afterward:
- summarize the inheritance relationship;
- explain what lives in EnemyBase versus Slime;
- provide Unreal Editor steps for putting one slime in the PlayGame level;
- provide verification criteria;
- do not claim success without actual testing.
```

## Files Created

Nothing existing was modified.

| File | Responsibility |
|---|---|
| `Source/GrantMoney/Public/SlimeEnemy.h` | Declares `ASlimeEnemy : AEnemyBase` and its placeholder mesh component. |
| `Source/GrantMoney/Private/SlimeEnemy.cpp` | Slime defaults and the temporary visual. No tick, targeting or movement code. |

## Inheritance

```text
ACharacter
   └─ AEnemyBase      target finding, direct pursuit, MoveSpeed, StopDistance
         └─ ASlimeEnemy      slime defaults + placeholder visual only
```

| In `AEnemyBase` | In `ASlimeEnemy` |
|---|---|
| Tick, `FindTarget`, `AcquireTarget`, `GetMoveDirection`, `MoveTowardTarget` | No tick, targeting or movement code |
| `MoveSpeed` and `StopDistance` properties | Overrides their defaults: 250 cm/s and 80 cm |
| Character movement setup (no controller, face the direction of travel) | Capsule radius 30 and half height 30, so it is round |
| Nothing visual | A temporary `/Engine/BasicShapes/Sphere` at scale 0.6, with no collision |

The visual is its own component (`PlaceholderMesh`), so the mesh can be swapped or hidden without touching pursuit. No colour material was added, because the engine's basic material was not checked for a colour parameter. A Blueprint child is not needed yet. One would only be worth adding later to assign the real mesh and material.

## Compile

`Result: Succeeded` (about 6 seconds).

## Editor Steps (put one slime in the level)

1. Close any running Editor, or compile in it with Ctrl+Alt+F11. Open the project and load `PlayGame1`.
2. In the Content Browser, enable **Settings → Show C++ Classes**. Open `GrantMoney → Public`, and drag `SlimeEnemy` into the viewport.
3. Place it on playable ground, about 5 to 10 m from the player start and clear of walls. Snap it to the floor (End key) if it floats.
4. Press Play.

## Verification Criteria

1. The Output Log shows `EnemyBase: SlimeEnemy… is chasing GrantMoneyPlayerCharacter…`.
2. The slime moves in a straight line toward the player and keeps following as the player moves.
3. It stops about 80 cm from the player, without jitter, and does not push the player.
4. Changing `MoveSpeed` on the placed slime during Play changes its pace at once.
5. Changing `StopDistance` changes where it stops.
6. It stays on the ground.
7. It slides along or stops at walls. There is no pathfinding, which is expected.
8. Pause and resume stop and restart it.
9. `EnemyBase` has no reference to `Slime`.

## Result

Developer reported, after following these steps: *"I tested it like you said and it felt good with no jittering or clipping through walls it all just worked."* The individual criteria (speed edit, stop-distance edit, pause) were not itemised, so only the overall result is recorded. This was the version **before** the Step 6 corrections.

## Capture

```text
07-slimeenemy-code.png
08-slime-compile-success.png
09-slime-placed-in-level.png
```

- `07`: `SlimeEnemy.cpp`, showing how little it contains.
- `08`: the build output showing `Result: Succeeded`.
- `09`: the slime in `PlayGame1` and its Details panel.

The chase clip itself is `10-test-a-one-slime.mp4` (Step 7, Test A).

## Future Video Editor Use

Suggested narration:

> "The slime has no tick function. It has no targeting code. It sets two numbers and a placeholder sphere. Everything else it inherits. That is the point of a base class."

---

# Step 6 – Review the Pursuit Behaviour

## AI Prompt

```text
The EnemyBase + Slime implementation now exists.

Review ONLY the pursuit behavior.

The desired gameplay feel is a simple survivor-game enemy:

- continually presses toward the player;
- does not perform sophisticated pathfinding;
- does not orbit around the player;
- does not jitter when reaching the player;
- turns/moves smoothly enough for a prototype;
- stops or settles inside a configurable stopping radius instead of repeatedly overshooting;
- behaves correctly when the player moves away again.

Do not add obstacle climbing yet. That is a separate Jira task.

Inspect the current implementation and identify any problems with:

- DeltaTime/frame-rate independence;
- movement direction normalization;
- vertical Z movement;
- rotation behavior;
- CharacterMovement usage;
- repeated player lookup;
- null references;
- behavior when the player is destroyed/recreated;
- stopping-distance jitter.

Propose the smallest corrections.

Do not redesign the architecture unless there is an actual defect.

After making changes, explain:

1. what was wrong;
2. what changed;
3. why the correction is better;
4. what I should verify manually in Unreal.
```

## How the Review Was Done

Reading the code was not enough to answer "does it jitter?", so the AI measured instead. Engine source was read only to check defaults (`Pawn.cpp`, `CharacterMovementComponent.cpp`). Then a **temporary automated test** was written, run, and **deleted**. It is not part of the change.

- **How it ran:** `UnrealEditor-Cmd.exe` with `Automation RunTests GrantMoney.EnemyPursuitScratch`, headless (`-nullrhi`).
- **What it built:** a world made in code with a flat cube floor, a stand-in `ACharacter` as "the player" (possessed by a bare `APlayerController`), and a slime spawned at runtime. Fixed time steps.
- **What it measured:** arrival time, closest approach, final gap, and how often the slime's speed crossed zero (start/stop transitions) in a settled window.
- **What it is not:** the real map, the real player class, real walls, the pause menu, or a person playing. The numbers are real, but they come from a simulation.

The test harness itself had four problems that had to be fixed before the numbers meant anything. None were slime problems: (1) no game mode meant `BeginPlay` never ran, (2) the engine runs each tick function once per frame-counter value, so a tight loop must advance `GFrameCounter` or every actor ticks once, (3) a bare player controller is not local, so the stand-in player was moved by position instead of input, (4) creating and destroying several worlds crashed the test process, so one shared world was reused. The first run showed nothing moving at all, which looked like a slime bug and was not.

## Findings

| Area from the prompt | Verdict |
|---|---|
| DeltaTime / frame-rate independence | **No defect.** The code gives `CharacterMovement` a direction, and it scales by DeltaTime itself. Measured at 30, 60 and 144 fps: arrival 2.97, 2.93, 2.93 s (before) and 3.20 s at all three (after). |
| Direction normalization | **No defect.** `GetSafeNormal()` gives a unit vector, and zero inside the stop distance. |
| Vertical Z | **No defect.** Z is zeroed before the distance and direction are taken. This matters: the player capsule (half height 88) and slime capsule (half height 30) put the two origins about 58 cm apart vertically even when both stand on the floor. |
| Rotation | **No defect.** The engine rotates toward acceleration at 360°/s, and (read in `CharacterMovementComponent.cpp`) does not change rotation when there is no acceleration, so a stopped slime does not spin. It keeps facing its last direction of travel instead of the player. Invisible on a sphere, but worth knowing once a real mesh has a front. |
| `CharacterMovement` usage | **One defect** (Defect 2 below). |
| Repeated player lookup | **No defect.** The lookup happens only while there is no valid target. |
| Null references | **No defect.** Every use is behind `IsValid`, and the missing-target warning logs once. |
| Player destroyed and recreated | **Works in the simulation.** While the player was gone the target was `None` and the slime stopped (coasting 5.7 to 8.3 cm). When a new player appeared 600 cm away it picked it up and walked to it. |
| Stopping-distance jitter | **One defect** (Defect 1 below). |

## Defect 1 – Stop-and-go stutter at the stopping distance

**What was wrong.** Movement input was fully on outside `StopDistance` and fully off inside it. If the target moves away more slowly than the slime can move, the slime catches up, stops, falls behind, catches up again, and repeats every few frames. The slime also coasted 4 to 8 cm inside the stop radius while braking.

| Measurement (simulation) | Before | After |
|---|---|---|
| Player retreating at 100 cm/s, slime 600: start/stop transitions in 6 s | 42 | 0 |
| Same: speed standard deviation | 54.8 | 0.0 |
| Player retreating at 300 cm/s, slime 600: speed standard deviation | 95.6 | 0.0 |
| Stationary player: closest approach (stop distance is 80) | 72.6 to 75.8 | exactly 80.0 |
| Player circling at radius 300, 150 cm/s | not measured | steady gap 108, speed 141, 0 transitions |
| Slime 250 vs player at 600, then player stops: final gap | 75.8 | 80.0 |
| Arrival time over 720 cm at 250 cm/s | 2.93 s | 3.20 s |

**What changed.** New property `SlowdownDistance` (default 50 cm, `EditAnywhere`) on `AEnemyBase`. Within that distance beyond `StopDistance`, the enemy's max walk speed is scaled by `remaining distance ÷ SlowdownDistance`, reaching zero at `StopDistance`. A small helper, `GetFlatOffsetToTarget`, now holds the flat-offset calculation that `GetMoveDirection` used to do inline. Setting `SlowdownDistance` to 0 restores the old hard switch.

**Why it is better.** Speed now changes continuously with distance, so there is no boundary to flicker across. If the target retreats, the slime settles at whatever gap makes its speed match, instead of switching on and off. It also stops at the configured radius instead of inside it.

**Cost.** The last 50 cm is an ease-in, so arrival takes about 0.27 s longer over the test distance, and the slime reaches 80 cm asymptotically rather than at a sharp edge.

**Important note about the first Editor test.** With the default speeds (slime 250, player 600) the player always outruns the slime, so this stutter does not appear. The first test could not have shown it. It appears when the target is slower than the enemy: a slow or hesitating player, a player moving around inside the stop zone, or a later, faster enemy type.

## Defect 2 – Placed slimes got an AI controller, spawned ones would not

**What was wrong.** Unreal's default for a pawn is `AutoPossessAI = PlacedInWorld` (read in `Pawn.cpp`). So a slime dragged into the level got an `AAIController` automatically, but a slime spawned from code later (the future spawner) would get none. The base class was written for the no-controller case and the Editor test only ever exercised the controller case. The brief asked for no AI controllers unless required.

In the simulation both cases moved identically, so this did not break movement. It was a hidden difference, and an unused extra actor per placed enemy.

**What changed.** `AutoPossessAI = EAutoPossessAI::Disabled` in the `AEnemyBase` constructor. One line.

**Evidence.** A plain `ACharacter` placed in the simulated world got `AIController_1`. The slime got none, placed or spawned.

## What Was Not Changed

- `MaxWalkSpeed` is written every tick. This is intended: it lets `MoveSpeed` be tuned while playing, and the easing needs it.
- Rotation behaviour.
- Target refresh. A target that is still valid but no longer the player's pawn (the player possessing a different pawn while the old one still exists) would not be noticed. No such system exists yet, so it is recorded as a limitation, not fixed.
- `EnemyBase` is still not `Abstract`.

## Compile

After the corrections and after the temporary test was deleted: `Result: Succeeded`. `git status` shows only the four new source files, the new docs folder, and the modified map (see the warning below).

## Manual Re-Test Checklist (superseded by the Tests A to F in Step 7; the developer has not done this yet)

Compile first (close the Editor or press Ctrl+Alt+F11), then:

1. Select the placed slime. In Details, `Enemy|Movement` should show `MoveSpeed`, `StopDistance` and the new `SlowdownDistance` (50).
2. Press Play. The slime should chase and then **ease in** over the last half metre, stopping at about 80 cm without creeping inside the player.
3. Check the **World Outliner** during Play. There should be no `AIController` actor for the slime. Before the correction there was one.
4. To see Defect 1: during Play, set the slime's `MoveSpeed` to about 800 so it is faster than the player. Walk away in a straight line. The slime should trail at a steady gap with no stutter. Then set `SlowdownDistance` to 0 and walk away again. The stutter should appear. Set it back to 50. This also makes a good video clip.
5. Walk in a circle around the slime when it has stopped. It should follow without jitter and should not spin in place.
6. Pause and resume while it chases.
7. Stop and start Play a few times.
8. Check that the slime still stays on the ground and does not clip through walls.

## Capture

```text
16-slowdown-distance-details.png
17-fast-slime-slowdown-50.mp4
18-fast-slime-slowdown-0-stutter.mp4
19-no-aicontroller-in-outliner.png
20-pursuit-review-findings.png
```

- `16`: the Details panel showing the `SlowdownDistance` property.
- `17` and `18`: the same walk-away test with a fast slime (`MoveSpeed` about 800), once with `SlowdownDistance` 50 and once with 0, for a before/after comparison.
- `19`: the World Outliner during Play with no `AIController` for the slime.
- `20`: the AI's reply to the Step 6 review prompt.

## Future Video Editor Use

Suggested narration:

> "Our first test passed, so why change anything? Because the first test couldn't have failed. The player is faster than the slime, so the slime never had to stop and start at the boundary. We measured it instead of trusting it. A hard on-off switch at a distance becomes a stutter the moment the target moves slowly. The fix is a gentle slowdown, and one number to tune it."

---

# Step 7 – Final Checkpoint Review of GMS-127 and GMS-128

## AI Prompt

```text
Review GMS-127 and GMS-128 against their current implementation.

Do NOT add new features.

Acceptance criteria for this checkpoint:

Enemy base:
- reusable C++ class exists;
- no slime-specific assumptions;
- target acquisition is reusable;
- movement speed is configurable;
- stopping distance is configurable;
- missing-player state is handled safely.

Slime:
- derives from the enemy base;
- acquires the actual player at runtime;
- follows a moving player;
- multiple manually placed slimes can pursue the player simultaneously;
- slime does not duplicate shared pursuit logic.

Explicitly inspect for:
- unnecessary Tick work;
- GetPlayerCharacter/GetPlayerPawn being called unnecessarily every frame;
- accidental hardcoded player class assumptions;
- unsafe raw references;
- duplicated movement code;
- dependencies on map-specific coordinates;
- Blueprint-only gameplay;
- unrelated file modifications.

Give me a manual Unreal test sequence:

Test A — One slime
Test B — Moving player
Test C — Player stops
Test D — Multiple slimes
Test E — No valid player
Test F — stop and restart PIE

For each test state:
Action → Expected Result.

Do not mark any test passed unless there is evidence it was actually performed.
```

The same message also asked for the media lists, naming and all earlier document sections to be included in this document. That is why this document now has the capture rules, folder list, master checklist, voice-over outline and Definition of Done.

## How the Review Was Done

- **Read:** all four source files in full.
- **Searched** (case-insensitive) `EnemyBase.h`/`.cpp` for `slime`, the player class name, `MapArrays`, `TileMap`, `Blueprint`, `GetPlayer` and numeric vector literals. Searched `SlimeEnemy.h`/`.cpp` for `Tick`, `FindTarget`, `GetPlayer`, `AddMovementInput`, `GetMoveDirection`, `MaxWalkSpeed` and `Blueprint`. Searched all four for `static`.
- **Git:** `git status`, `git diff --stat`, `.gitattributes` for LFS rules, and the current branch.
- **A second temporary headless test** (deleted afterwards, same method as Step 6), to get evidence for the two criteria that reading cannot settle: several slimes at once, and no player at all. Five slimes, a flat floor, a stand-in `ACharacter` as the player, fixed time steps. It is a simulation: no real map, walls, player class, pause menu, or person playing. The earlier harness problems (Step 6) were already known and handled, so this run needed no debugging of the harness.
- **Built** the project again after deleting the test: `Result: Succeeded`.

## Acceptance Criteria

The evidence column uses the kinds defined at the top: read from source, compiled, developer reported, headless simulation. **No manual Editor test of the corrected build has been performed.**

### Enemy base

| Criterion | Status | Evidence |
|---|---|---|
| Reusable C++ class exists | Met | `AEnemyBase : ACharacter`, `UCLASS`, exported with `GRANTMONEY_API`. Read from source and compiled. Not `Abstract`, so it can still be placed on its own. |
| No slime-specific assumptions | Met | The search of `EnemyBase.h` and `EnemyBase.cpp` finds no `slime` at all. Read from source. |
| Target acquisition is reusable | Met | Virtual `FindTarget` (defaults to the first player's pawn) plus `AcquireTarget`. Read from source. Worked with a stand-in player in the simulation, and with the real player in the developer's first Editor test. |
| Movement speed is configurable | Met by source | `MoveSpeed` is `EditAnywhere` and applied every tick. The developer has not said they edited it. |
| Stopping distance is configurable | Met by source | `StopDistance` is `EditAnywhere`. The simulation used the slime's default (80) only, and never changed it. |
| Missing-player state is handled safely | Met in simulation | Five slimes with no player for 2 s: no target, moved 0.00 cm, one warning each, no crash. After the player was destroyed: targets cleared, one warning each again. **Not tested in the real game.** |

### Slime

| Criterion | Status | Evidence |
|---|---|---|
| Derives from the enemy base | Met | `ASlimeEnemy : AEnemyBase`. Read from source. |
| Acquires the actual player at runtime | Partly evidenced | Developer reported the first version chasing the real player. The simulation used a stand-in. The corrected build has not been run in the Editor. |
| Follows a moving player | Simulation only | Circling player (radius 300 to 400 cm) and retreating player: the slime follows with a steady gap. The developer's first report ("it all just worked") does not itemise a moving player. |
| Multiple placed slimes pursue at once | Simulation only, with a caveat | All five slimes targeted the player and moved at once. Four of five reached the stop ring in 3.2 to 3.3 s. One was blocked by the others. See "Multi-Slime Findings". Not tested in the Editor. |
| Slime does not duplicate pursuit logic | Met | The search of the slime files finds no `Tick`, `FindTarget`, `GetPlayer`, `AddMovementInput`, `GetMoveDirection`, `MaxWalkSpeed` or `Blueprint`. Read from source. |

## Explicit Inspection

| Item | Finding | Changed? |
|---|---|---|
| Unnecessary Tick work | Every enemy ticks every frame, which is needed to follow a moving player. Per tick: a validity check, the flat offset to the target **computed twice** (once in `MoveTowardTarget`, once inside `GetMoveDirection`), a speed write and one `AddMovementInput`. A few vector operations. Fine for a prototype. Could be consolidated if hundreds of enemies are ever placed. | No |
| `GetPlayerPawn` called every frame | **No while a target is held.** The target is cached, and the lookup happens only when there is no valid target. While no player exists, each slime does one lookup per frame (a short loop over the player controllers) and logs one warning. A timer-based retry would be tidier. | No |
| Hardcoded player class | **None.** `FindTarget` returns `UGameplayStatics::GetPlayerPawn(this, 0)`, a plain `APawn`. Neither enemy file includes or names `AGrantMoneyPlayerCharacter`. The only assumption is player index 0, which is correct for a single-player game. | No |
| Unsafe raw references | **None.** `Target` is a `TObjectPtr<AActor>` `UPROPERTY` (`Transient`), so it is garbage-collection safe and is checked with `IsValid` before every use. The public `GetTarget()` returns a raw pointer for the caller to read, which callers should check with `IsValid` before use. `PlaceholderMesh` is also a `TObjectPtr` `UPROPERTY`. | No |
| Duplicated movement code | **None across classes.** Exactly one `AddMovementInput` call exists, in the base. The only repetition is the double flat-offset calculation noted above. | No |
| Map-specific coordinates | **None.** No `MapArrays`, `TileMapRenderer` or map coordinates appear. The slime's constants are its own capsule size and placeholder scale. | No |
| Blueprint-only gameplay | **None.** All behaviour is C++. The three tuning properties are `BlueprintReadWrite`, which is broader than needed (`EditAnywhere` alone is enough for Editor tuning) and lets Blueprints change them at runtime. There are no Blueprint-callable functions or events. | No. Developer's call. |
| Unrelated file modifications | **One.** `Game/Content/Maps/PlayGame1.umap` shows as modified. `git diff --stat` shows 2 lines changed in the file, which is its Git LFS pointer. That is almost certainly the placed slime saved by the Editor. Its contents were not read. All source changes are new files. | No. Developer decides. |
| State that could leak between Play sessions | **None.** The only `static` in the four files is the standard constructor asset lookup in `SlimeEnemy.cpp`. | No |

## Multi-Slime Findings (simulation)

Setup: five slimes. Three stacked on one side (starting at 800 cm, with two others 120 cm either side of it), one on a second side, one on a third. The player is stationary, then circles at radius 400 cm and 150 cm/s for 30 s, then stops.

| Run | Result |
|---|---|
| No player for 2 s | No slime had a target. Maximum movement 0.00 cm. One warning per slime. |
| Stationary player | Slimes 1 to 4 settled at exactly 80.0 cm after 3.20 to 3.32 s, with speed 0 and no start/stop transitions. Slime 0 (the front of the stack) never reached the ring and settled at 97 cm. The closest any two slimes came was 60.0 cm, which is two slime capsules touching. |
| Circling player, slimes block each other | Slime 0: speed sd 3.6. Slime 2: sd 12.6. Slimes 1, 3, 4: sd 49.9, 52.4, 54.8 cm/s, with 4, 0 and 2 start/stop transitions in the last 10 s. Mean speeds 132 to 161 cm/s. |
| Circling player, slimes **do not** block each other | All five identical: gap 109.0, speed 145.1, sd 0.0, no transitions. Closest pair 0.0 cm (all stacked on one spot). |
| Player stops | Four slimes settled at exactly 80.0 cm. One stopped at 124.3 cm with speed 0, queued behind another slime. |

**What this means.** With one slime the pursuit is steady (Step 6). With several, the speed wobble disappears completely when slimes stop blocking each other, so the cause is slimes bumping into one another, not the pursuit logic. Slimes that arrive late queue behind the ones in front, because direct pursuit has no sidestep or separation.

**Is it a defect?** Not against this milestone's criteria. Enemy-to-enemy avoidance was never asked for, and adding it would be a new feature. It is a visible behaviour though, so it is recorded as a limitation and Test D tells you what to look for. Options for a future ticket (**none implemented**): a different collision response between enemies, a small separation push, or spacing rules for the future spawner.

One run (the player stopping, after the collision toggle) was contaminated by the test's own change and its numbers are not used. The "player stops" row above is from the first, clean run.

## Optional Tidy-Ups (none done)

These need a developer decision. None is a defect.

1. Narrow the three `BlueprintReadWrite` properties to `EditAnywhere`.
2. Mark `AEnemyBase` as `Abstract` once there is a second enemy type, or now.
3. Compute the flat offset once per tick.
4. Retry target lookup on a timer while no player exists.
5. Decide what to do about slimes blocking each other, in a future ticket.

## Manual Unreal Test Sequence

**Nothing below has been performed. Every status is "Not performed" until the developer runs it and reports.** Record what you actually see.

### Setup (once)

1. Close the Editor if it is open, or compile in it with Ctrl+Alt+F11. Open the project and load `PlayGame1`.
2. In the Output Log, filter on `EnemyBase`. This shows the `chasing` and `no target` lines.
3. Keep `show collision` handy in the console. It shows the capsules.
4. Do not save the map after testing unless you mean to keep the slimes you placed.

### Test A — One slime

**Setup:** exactly one slime, placed 5 to 10 m from the player start on playable ground.

| # | Action | Expected Result |
|---|---|---|
| 1 | Press Play and read the filtered Output Log. | A line `EnemyBase: SlimeEnemy_… is chasing GrantMoneyPlayerCharacter_…`. If a `has no target yet` warning appears, it appears at most once, before the `chasing` line. |
| 2 | Stand still and watch. | The slime walks in a straight line at about 250 cm/s, eases in over the last half metre, and stops about 80 cm from the player. No jitter, no overlap with the player, no pushing it. |
| 3 | Type `show collision`. | The slime capsule sits on the ground and does not overlap the player capsule. |
| 4 | Open the World Outliner while playing. | No `AIController` actor for the slime. |
| 5 | Eject (F8), select the slime in the Outliner, set `MoveSpeed` to 500. | Its pace changes immediately. |
| 6 | Set `StopDistance` to 300. | It settles about 300 cm away instead. |

**Status: Not performed.** Evidence so far: the developer reported the first version worked (not itemised). The corrected build: simulation only.
**Capture:** `10-test-a-one-slime.mp4`, and `19-no-aicontroller-in-outliner.png` for step 4.

### Test B — Moving player

**Setup:** one slime, as in Test A.

| # | Action | Expected Result |
|---|---|---|
| 1 | Walk away in a straight line. | The slime follows, always heading at where you are now. You outrun it (600 vs 250 cm/s), so the gap grows. It never stops while you are far away. |
| 2 | Walk in a circle around the slime. | It turns smoothly and follows. It does not spin in place, orbit, or jitter. |
| 3 | Reverse direction suddenly. | It turns toward the new position without an oscillating overshoot. |
| 4 | Walk along a wall with the slime behind you. | It slides along or stops at the wall. It does not pass through it. It does not path around it, which is expected. |
| 5 | Optional, for the before/after clip: set `MoveSpeed` to about 800 and walk away in a line. | The slime catches up and trails at a steady gap with no stutter. Then set `SlowdownDistance` to 0 and repeat: stop-and-go stutter is expected to return. Set it back to 50. |

**Status: Not performed.** Evidence so far: simulation (retreating and circling player, steady results).
**Capture:** `11-test-b-moving-player.mp4`, and the optional pair `17-fast-slime-slowdown-50.mp4` and `18-fast-slime-slowdown-0-stutter.mp4`.

### Test C — Player stops

**Setup:** one slime.

| # | Action | Expected Result |
|---|---|---|
| 1 | Run away, then stop completely and wait 10 s. | The slime catches up, eases in, and stops about 80 cm away. It stays stopped. No creeping or twitching. |
| 2 | Step about 1 m away. | It resumes and stops again at about 80 cm. |
| 3 | Watch its facing (set `PlaceholderMesh` hidden or look at the capsule direction). | It faces its last direction of travel and does not spin. This is expected. |

**Status: Not performed.** Evidence so far: simulation (settles at exactly 80.0 cm, speed 0).
**Capture:** `12-test-c-player-stops.mp4`.

### Test D — Multiple slimes

**Setup:** place four or five slimes in different directions and distances (at least 5 m from the player, at least 1 m from each other), with at least two on the same side.

| # | Action | Expected Result |
|---|---|---|
| 1 | Press Play. | One `chasing` line per slime. All start moving toward the player at the same time. |
| 2 | Stand still. | All head to the player. The first to arrive stop at about 80 cm. Later ones may queue behind them. None pass through each other or the player. None fall through the floor. Frame rate stays normal. |
| 3 | Walk in a circle. | All follow. Some may bump each other and show uneven speed. That was seen in the simulation. Note what you see. It only counts as a problem if a slime stays stuck once the player moves away, or if a lone slime jitters. |
| 4 | Walk far away, then come back. | Slimes that were queued free themselves and follow. |

**Status: Not performed.** Evidence so far: simulation (see "Multi-Slime Findings").
**Capture:** `13-test-d-multiple-slimes.mp4`, and `22-multi-slime-bumping.mp4` if you see bumping.

### Test E — No valid player

The project has no respawn system, so the cleanest way to have no player is Simulate. Both methods below are **suggestions that have not been tried**. If neither works, record that and do not invent a result.

| # | Action | Expected Result |
|---|---|---|
| 1 | Method 1: press **Alt+S** (Simulate In Editor) with slimes placed, instead of Play. | No player pawn exists. Each slime logs `has no target yet; it will keep looking.` **once**. No slime moves. No repeating log spam, no crash, no red errors from `EnemyBase`. |
| 2 | Method 2: press Play, press F8 to eject, select the player pawn in the Outliner and delete it. | The slimes stop within a few centimetres and log one warning each. No crash. The player does not come back, so nothing re-acquires. |
| 3 | Leave it running for 30 s. | Still no movement, no growth in the log. |

Re-acquiring a new player was shown only in the simulation (Step 6). It cannot be shown in the Editor without a respawn system.

**Status: Not performed.** Evidence so far: simulation only (no target, no movement, one warning each).
**Capture:** `14-test-e-no-valid-player.mp4` and `14b-test-e-no-target-warning-log.png`.

### Test F — Stop and restart PIE

| # | Action | Expected Result |
|---|---|---|
| 1 | Press Play, confirm the chase, stop. Repeat three times. | Each run logs one `chasing` line per slime and the chase works every time. Nothing carries over. |
| 2 | During one run, change a slime's `MoveSpeed`, then stop and play again. | The edit is gone: the slime is back to its saved values. This is normal Unreal behaviour. |
| 3 | Stop Play while the slimes are mid-chase. | A clean stop, with no crash and no `Accessed None` errors. |
| 4 | After the third run, search the Output Log for `Error`. | Nothing from `EnemyBase` or `SlimeEnemy`. |

**Status: Not performed.** Evidence so far: none from a run. By source: the enemy code holds no mutable static state, which is why nothing should carry over.
**Capture:** `15-test-f-stop-restart-pie.mp4`.

### Result Summary (fill in only from what you observed)

| Test | Status | Notes |
|---|---|---|
| A — One slime | Not performed | _you fill in_ |
| B — Moving player | Not performed | _you fill in_ |
| C — Player stops | Not performed | _you fill in_ |
| D — Multiple slimes | Not performed | _you fill in_ |
| E — No valid player | Not performed | _you fill in_ |
| F — Stop and restart PIE | Not performed | _you fill in_ |

If a test fails, keep the footage and write down what you saw. Failures are useful for the course.

## Capture

```text
21-decoupling-check.png
22-multi-slime-bumping.mp4
23-git-status-final.png
24-final-verification.png
```

- `21`: a search of `EnemyBase.h` and `EnemyBase.cpp` for `slime`, `MapArrays` and `TileMap` returning nothing.
- `22`: slimes bumping into each other, if you see it (optional).
- `23`: `git status` showing only the four new source files, the docs folder, and the modified map.
- `24`: the Output Log with no errors from the enemy classes, beside the filled-in Result Summary.

## Future Video Editor Use

Suggested narration:

> "Passing one test is easy. We went back and asked: what did that test not prove? One slime is steady. Five slimes bump into each other and wobble, and the wobble goes away the moment we switch off slime-against-slime collision. That tells us the pursuit logic is fine and crowd behaviour is the next problem, not this one."

---

# What Actually Happened (Fill In During Development)

## Deviations From the Plan

| Plan | What actually happened | Why |
|---|---|---|
| One branch per Jira issue | GMS-127 and GMS-128 are combined on one branch, `GMS-127-128` | Developer's decision for this work. |
| `AEnemyBase` described as abstract in the starting plan | Left placeable, not `Abstract` | It needed to be testable alone first. The change was not made yet. |
| Two classes, four files, nothing else | Same, plus two temporary automated test files (Step 6 and Step 7), each deleted after use | Only for measurements. Neither was committed or left in the project. |
| Place a slime and play-test | Developer did this for the first version only | The Step 6 corrections have not been play-tested by the developer. |
| Final checkpoint review (Step 7) with no new features | Done. Nothing in the source was changed during it. | The review found no defect against the acceptance criteria. It found one behaviour to note (slimes bumping each other) and five optional tidy-ups, all left for the developer. |
| Manual Tests A to F | **Not performed** | Waiting on the developer. |
| Instruction given to the developer to check that Auto Possess AI is `Disabled` (the default) | **That instruction was wrong.** The engine default is `PlacedInWorld`. | See "AI Mistakes". The base class now sets it to `Disabled` itself. |

## Files Changed

All four source files are new. No existing source file was modified.

| File | Change |
|---|---|
| `Source/GrantMoney/Public/EnemyBase.h` | New. `AEnemyBase : ACharacter` with `MoveSpeed`, `StopDistance`, `SlowdownDistance`, `GetTarget`, and the virtual `FindTarget` and `GetMoveDirection`. |
| `Source/GrantMoney/Private/EnemyBase.cpp` | New. Target acquisition, flat direction, eased direct pursuit, controller-less setup, `AutoPossessAI` disabled. |
| `Source/GrantMoney/Public/SlimeEnemy.h` | New. `ASlimeEnemy : AEnemyBase` and the placeholder mesh component. |
| `Source/GrantMoney/Private/SlimeEnemy.cpp` | New. Slime defaults (250 cm/s, 80 cm), round capsule, placeholder sphere. |

Not touched: `TileMapRenderer`, `MapArrays`, `MapBoundaryGenerator`, the camera classes, `GrantMoneyPlayerCharacter`, `GrantMoneyGameMode`, `GrantMoney.Build.cs`, any ini file.

### Warning: `PlayGame1.umap` Is Modified

`git status` shows `Game/Content/Maps/PlayGame1.umap` as modified. That is almost certainly the placed slime, saved by the Editor. This was not read or checked (it is a binary asset), so what exactly changed in it is unknown. Decide before committing whether the slime belongs in the committed map or should be removed, and remember that Unreal binary assets go through Git LFS. This is the one existing file that differs from the "no existing files" goal.

## Verification Results

Only results the developer reported are marked as confirmed. Simulation results are marked as such.

| Criterion | Result | Evidence |
|---|---|---|
| Project compiles | Confirmed | `Result: Succeeded`, after every step including the final clean build |
| Slime finds the player and chases it (first version) | Confirmed by developer | "it all just worked" |
| No jitter (first version) | Confirmed by developer | "no jittering" |
| No clipping through walls (first version) | Confirmed by developer | "no … clipping through walls" |
| Speed and stop distance editable in the Editor | Not itemised | Properties are `EditAnywhere`. Developer has not said they changed them. |
| Pause and resume | Not reported | Not tested by the AI either. How the pause menu pauses the game was not inspected. |
| Frame-rate independence | Headless simulation | Arrival time 2.97 / 2.93 / 2.93 s (before), 3.20 s at all three (after), at 30 / 60 / 144 fps |
| No stop-and-go stutter at the stop boundary | Headless simulation | 0 transitions, speed sd 0.0, after the correction |
| Stops at the configured radius | Headless simulation | Closest approach exactly 80.0 after the correction |
| Follows a circling player without orbiting | Headless simulation | Steady gap 108, steady speed 141 |
| Recovers when the player is destroyed and recreated | Headless simulation | Stopped while gone, then walked to the new player |
| No AI controller on placed or spawned slimes | Headless simulation, plus engine source | Slime had none placed or spawned. A plain placed `ACharacter` got one. |
| Step 6 corrections in the real game | **Not tested** | Developer re-test pending |
| Base class has no slime-specific assumptions | Confirmed by text search | `EnemyBase.h` and `EnemyBase.cpp` contain no `slime` (case-insensitive), re-checked in Step 7 |
| Slime contains no duplicated pursuit logic | Confirmed by text search | No `Tick`, `FindTarget`, `GetPlayer`, `AddMovementInput`, `GetMoveDirection` or `MaxWalkSpeed` in the slime files |
| No hardcoded player class, map coordinates or Blueprint-only gameplay | Confirmed by text search and reading | Step 7 inspection table |
| Several slimes pursue at once | Headless simulation | Five slimes all chased. Four reached the ring (80.0 cm); one was blocked by the others (97 cm, then 124 cm in a later run). Not tested in the Editor. |
| No player: nothing moves, no crash, one warning each | Headless simulation | Five slimes, 2 s with no player: 0.00 cm moved; again after the player was destroyed |
| Manual Tests A to F | **Not performed** | See Step 7. Every status there is "Not performed". |

Assumptions from the investigation, and what happened to each:

| # | Assumption | Outcome |
|---|---|---|
| 1 | Engine sphere is acceptable as a placeholder | Used. Developer did not object. |
| 2 | Controller-less `ACharacter` moves with `AddMovementInput` | Confirmed in simulation. Also found that placed pawns get an AI controller by default, which was removed. |
| 3 | Default capsule collides with the ground slab and walls like the player | Developer reported no clipping through walls and no falling through. |
| 4 | Cache the target, re-resolve when it is gone | Works in simulation. |
| 5 | A placed slime exists before the player pawn | Handled by the missing-target path. Developer's placed slime worked. |
| 6 | No navmesh, so straight-line movement | Matches the brief. Behaviour against walls: developer reported no clipping. |
| 7 | Pause stops the slime | Not tested. |
| 8 | No Blueprint subclass needed | Held. None was made. |

## Observations (Not Pass or Fail)

- The slime faces its last direction of travel, not the player, when stopped. Nothing shows it on a sphere.
- A slime that is faster than the player trails at a steady gap larger than the stop distance. Measured: 88 cm behind a player walking at 100 cm/s, and 105 cm behind one at 300 cm/s (slime 600, stop distance 80). The eased approach makes this the natural result, not a bug.
- With `SlowdownDistance` at 0 the old hard on-off behaviour returns, which is useful for comparison clips.
- The headless test log showed the tile renderer building tiles, so the Editor appears to load the project's default map in the background. Harmless noise in the test log.
- In the five-slime simulation the uneven speed disappeared when slime-against-slime blocking was switched off, so it comes from slimes bumping into each other.
- With slimes allowed to overlap, all five followed a circling player with identical results (gap 109.0, speed 145.1), so the pursuit logic itself behaves identically for every slime.

## AI Workflow Log

| # | Prompt | Result |
|---|---|---|
| 1 | Opening prompt: inspect the repository and report, no edits. | Files inspected, player class, nothing reusable for enemies, minimum design, assumptions. Recorded above. |
| 2 | "Write these findings into a starting file for Module 3 Sprint 1…" | This document. |
| 3 | Step 4 prompt: implement `AEnemyBase`. | Explained `ACharacter`, created two files, compiled. |
| 4 | "Are you able to build the project yourself?" and "Is there anything left…" | Confirmed the AI can compile but not play. Confirmed nothing existing was modified. |
| 5 | Step 5 prompt: implement the slime. | Created two files, compiled, gave Editor steps and criteria. |
| 6 | Developer reported the first play-test result. | Recorded in Step 5. |
| 7 | Step 6 prompt: review the pursuit behaviour. | Measured with a temporary headless test, made two corrections, rebuilt, deleted the test. |
| 8 | Step 7 prompt: final checkpoint review, no new features, with a manual test sequence A to F and the media lists. | Read and searched the source, checked git, ran a second temporary headless test (five slimes, no player), changed no source, deleted the test, rebuilt. Wrote Step 7 and restored the capture and review sections from the earlier documents. |

## AI Mistakes, Corrections and Surprises

- **Wrong claim about Auto Possess AI.** When giving the Editor steps for the slime, the AI said Auto Possess AI is `Disabled` by default. It is `PlacedInWorld`. The AI found this during the Step 6 review by reading `Pawn.cpp`, and told the developer. The base class now sets it to `Disabled`, so the original instruction no longer matters. Reading the engine default instead of recalling it is the lesson.
- **The first test could not have caught the stutter.** The default speeds made the player faster than the slime, so the boundary stutter never showed. The review found it only by measuring a slower-moving target.
- **The test harness looked like a slime bug.** The first runs showed nothing moving, even the stand-in player. Four separate harness problems (no `BeginPlay`, frame counter not advancing, non-local controller, world teardown crash) accounted for it. The temporary log lines that found the frame-counter problem were removed from `EnemyBase.cpp` afterwards.
- **The AI did not claim a test passed when it had not run.** The final message of Step 4 and Step 5 said the code compiled but was untested. The first Editor result came from the developer.
- **Scope held.** The review changed only `EnemyBase`. It left rotation, target refresh and `Abstract` alone, and said why.
- **A suspicious result was isolated, not guessed at.** In the Step 7 simulation, three of five circling slimes showed uneven speed. Instead of blaming the pursuit code, the AI switched off slime-against-slime blocking in the test and re-ran. The unevenness went to exactly zero, which pointed at slimes bumping each other. The number from a later run that the toggle had contaminated was thrown away, and the document says so.
- **A first-run harness error was not repeated.** The Step 7 test reused what Step 6 had learned (advance the frame counter, call `NotifyBeginPlay`, one shared world), and needed no debugging.
- **Untried methods are labelled as untried.** Test E suggests Simulate (Alt+S) and deleting the pawn in the Outliner. The AI has not run either, and the document says so instead of presenting them as known to work.
- **Test status was not inflated.** All six manual tests are "Not performed". The developer's earlier "it all just worked" is recorded as covering only the first version, and is not itemised.

## Known Limitations

- No navigation mesh, so no pathfinding around obstacles. The slime slides along or stops at walls.
- No spawning. Slimes are placed by hand.
- No damage, combat, death, XP or UI.
- A target that is still valid but no longer the player's pawn is not re-resolved. Matters only if a future system swaps the player's pawn without destroying the old one.
- Facing is the last direction of travel, not the player, while stopped.
- Easing makes arrival slightly slower and approach asymptotic. `SlowdownDistance` 0 disables it.
- The Step 6 corrections have only been verified in a headless simulation, not in the real game.
- Pause and resume were never tested.
- `AEnemyBase` is not yet `Abstract`, and its three tuning properties are `BlueprintReadWrite`, which is broader than needed.
- **Slimes bump into each other.** There is no separation or avoidance, so slimes that arrive late queue behind the ones in front, and several circling slimes show uneven speed. Seen only in simulation. A future ticket could address it.
- A missing player causes one target lookup per slime per frame until one appears. Cheap, but not free with many enemies.
- Pursuit does a small amount of duplicate work per tick (the flat offset is computed twice).
- Test E's methods for removing the player in the Editor are untried suggestions.
- The placed slimes saved in `PlayGame1.umap` have not been decided on (see the warning under "Files Changed").

## Suggested Commit Messages

One combined branch. Only commit after the developer's re-test and a diff review. Never commit or push without being told to.

```text
GMS-127/GMS-128: Add enemy base and first slime enemy

- AEnemyBase (ACharacter): finds the player at runtime and walks straight
  toward it, with configurable MoveSpeed, StopDistance and SlowdownDistance
- Eased approach so the enemy settles at StopDistance without stop-and-go
  stutter when the target moves away slowly
- Enemies are controller-less (AutoPossessAI disabled) so placed and spawned
  enemies behave the same
- ASlimeEnemy derives from AEnemyBase and only sets defaults and a placeholder
  sphere
- No changes to TileGen, MapArrays, the camera, or the player character

No pathfinding, spawning, damage or death yet.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>
```

If `PlayGame1.umap` is committed, mention the placed slime in the message. Decide first (see the warning under "Files Changed").

---

# Known Risks and Open Questions

These came from reading the source and from the simulations. Confirm each during testing and record what you find.

| Risk | Why it matters |
|---|---|
| Corrected build not play-tested | The first Editor result covers the version before the `SlowdownDistance` and `AutoPossessAI` changes. Tests A to F are pending. |
| Slimes bump into each other | No separation or avoidance. Late arrivals queue, and circling slimes show uneven speed (simulation). Expect to see it in Test D. |
| Straight-line movement against walls | No pathfinding. A slime behind a wall presses against it until the player moves. Expected for this milestone. |
| Placed slimes in `PlayGame1.umap` | The map is modified in git. Decide whether the slimes belong in the commit. |
| Pause and resume | Never tested. How the pause menu pauses the game was not inspected. |
| Test E removal methods | Simulate (Alt+S) and deleting the pawn in the Outliner are untried suggestions. |
| Easing changes arrival feel | The last 50 cm is an ease-in. If it feels floaty, lower `SlowdownDistance`. 0 turns it off, and brings back stutter against a slower-moving target. |
| `AEnemyBase` is placeable | A bare base instance chases with no mesh. Make it `Abstract` once a second type exists. |
| Facing while stopped | The slime faces its last direction of travel, not the player. Will matter once there is a mesh with a front. |
| Collision profiles | Never inspected (Editor settings). Walls and ground behaved correctly in the developer's first test. |

---

# Final Review (End-of-Task Check)

This is a summary of Step 7. It reviews GMS-127 and GMS-128 together, on branch `GMS-127-128`. Nothing here has been committed or pushed.

## Diff Summary

```text
Game/Source/GrantMoney/Public/EnemyBase.h        new, 65 lines
Game/Source/GrantMoney/Private/EnemyBase.cpp     new, 112 lines
Game/Source/GrantMoney/Public/SlimeEnemy.h       new, 29 lines
Game/Source/GrantMoney/Private/SlimeEnemy.cpp    new, 39 lines
Game/Content/Maps/PlayGame1.umap                 modified (Git LFS pointer, 2 lines)
docs/course/Module3-CreatingChallengeAndReward/Sprint1/   new
```

## Review Checklist

| Check | Result |
|---|---|
| Unrelated changes | None in the source. See the warning below about `PlayGame1.umap`. |
| Unnecessary refactoring | None. No existing code was moved or renamed. |
| Duplicated systems | None. One `AddMovementInput` call, in the base. The slime has no pursuit code. |
| Hardcoded map-specific logic | None. No `MapArrays`, `TileMapRenderer` or map coordinates. |
| Hardcoded player class | None. `GetPlayerPawn(this, 0)` returns a plain pawn. |
| Changes that belong to another Jira issue | None. No spawning, health, damage, climbing, pathfinding, UI or camera change. |
| Blueprint-only gameplay | None. All behaviour is C++. Three properties are `BlueprintReadWrite` (optional tidy-up). |
| Documentation claims not actually verified | Checked. Every manual test is marked "Not performed", simulation results are labelled as simulation, and the developer's first report is recorded as covering only the first version. |

## Warning: `PlayGame1.umap` Is Modified

`git status` shows `Game/Content/Maps/PlayGame1.umap` as modified. `git diff --stat` shows 2 lines changed, which is the Git LFS pointer file (`*.umap` is tracked by LFS in `.gitattributes`). It is almost certainly the slime or slimes you placed, saved by the Editor. The map's contents were not read, because it is a binary asset.

Before committing, decide:

1. **Keep it** if you want the placed slimes in the committed level. Say so in the commit message and make sure LFS is set up so the asset uploads correctly.
2. **Discard it** if you only placed slimes for testing. Close the Editor first (it can re-save the file), then run `git restore Game/Content/Maps/PlayGame1.umap`.
3. Check `git status` again so only the intended files remain.

## Implementation Summary

`AEnemyBase` is a `ACharacter` that finds the first player's pawn at runtime and walks straight toward it on the ground plane, using the normal character movement. Speed, stopping distance and a short slowdown zone are Editor-tunable. It has no AI controller, no pathfinding and no knowledge of any specific enemy. `ASlimeEnemy` derives from it and only sets slime defaults (250 cm/s, stop at 80 cm), a round capsule and a placeholder sphere. A pursuit review added the slowdown zone (to stop stutter against a slowly-moving target) and disabled the automatic AI controller. A final review found no defect against the acceptance criteria and noted that slimes bump into each other.

## Reproducible Steps (what actually happened)

1. The developer created the branch `GMS-127-128`.
2. Asked the AI to inspect the repository (read-only) and report.
3. Wrote the findings into the starting document.
4. Reviewed the proposal. Chose `ACharacter`, no AI controller, new files only, one combined branch.
5. Implemented `AEnemyBase` (two new files). Compiled: succeeded.
6. Implemented `ASlimeEnemy` (two new files). Compiled: succeeded.
7. Developer placed a slime in `PlayGame1` and played. Reported it worked, with no jitter and no clipping through walls.
8. Asked for a pursuit review. The AI measured with a temporary headless test, added `SlowdownDistance`, disabled `AutoPossessAI`, rebuilt, and deleted the test.
9. Asked for a final checkpoint review. The AI read and searched the source, ran a second temporary headless test with five slimes and no player, changed no source, deleted the test, and rebuilt.
10. Manual Tests A to F: **not yet performed.**

## Screenshot and Video References

Whether each file below has been captured is for you to confirm. This table only lists what the plan calls for. Nothing has been captured as far as the AI knows. No `Media/GMS127-128` folder exists.

| File | Status |
|---|---|
| `00-baseline-no-enemies.mp4` / `.png` | _you confirm_ (retroactive) |
| `01-branch-created.png` | _you confirm_ |
| `02-ai-inspection-findings.png` | _you confirm_ |
| `03-player-class-and-spawn.png` | _you confirm_ |
| `04-enemybase-header.png` | _you confirm_ |
| `05-enemybase-details-panel.png` | _you confirm_ |
| `06-enemybase-compile-success.png` | _you confirm_ |
| `07-slimeenemy-code.png` | _you confirm_ |
| `08-slime-compile-success.png` | _you confirm_ |
| `09-slime-placed-in-level.png` | _you confirm_ |
| `10-test-a-one-slime.mp4` | _you confirm_ |
| `11-test-b-moving-player.mp4` | _you confirm_ |
| `12-test-c-player-stops.mp4` | _you confirm_ |
| `13-test-d-multiple-slimes.mp4` | _you confirm_ |
| `14-test-e-no-valid-player.mp4` and `14b-test-e-no-target-warning-log.png` | _you confirm_ |
| `15-test-f-stop-restart-pie.mp4` | _you confirm_ |
| `16-slowdown-distance-details.png` | _you confirm_ |
| `17-fast-slime-slowdown-50.mp4` and `18-fast-slime-slowdown-0-stutter.mp4` | _you confirm_ (optional pair) |
| `19-no-aicontroller-in-outliner.png` | _you confirm_ |
| `20-pursuit-review-findings.png` | _you confirm_ |
| `21-decoupling-check.png` | _you confirm_ |
| `22-multi-slime-bumping.mp4` | _you confirm_ (optional, only if you see it) |
| `23-git-status-final.png` | _you confirm_ |
| `24-final-verification.png` | _you confirm_ |

## Voice-Over Outline

> **The problem (over `00-baseline-no-enemies.mp4`).** "The world is empty. There is a player and a map, and nothing that comes for you."
>
> **Reuse, don't replace (over `02` and `03`).** "We ask the AI to inspect the project first. The player is already a Character, and the ground already has collision. We don't need a new movement system."
>
> **The base class (over `04` and `05`).** "The enemy base knows two things: who to chase, and how to step toward them. It has never heard of a slime. Speed and stopping distance are numbers you can change in the Editor."
>
> **The slime (over `07` and `09`).** "The slime has no tick function and no targeting code. It sets a few defaults and a placeholder sphere. Everything else is inherited. That is the point of a base class."
>
> **The result (over `10`, `11` and `12`).** "It finds the player, follows, and stops at a set distance."
>
> **The review (over `17`, `18`, `19` and `20`).** "The first test passed, so why look again? Because it could not have failed. The player always outruns the slime, so we never saw it stutter against a slower target. We measured it, found a stop-and-go problem, and fixed it with a short slowdown zone."
>
> **Crowds (over `13` and `22`).** "One slime is steady. Several bump into each other. We checked that the pursuit code wasn't the cause by letting them overlap, and the wobble vanished. That is a crowd problem for a later ticket, not this one."
>
> **Robustness (over `14` and `15`).** "No player? Nothing moves, nothing crashes, and each slime says so once. Stop and restart? Nothing carries over."
>
> **Closing.** "One base class, one slime, no pathfinding, and no changes to the player, the map or the camera."

---

# Media Capture Rules (Recap)

- Short clips, about **10–30 seconds**.
- Record while building, not only the finished result.
- Use the numbered names below so the editor can match footage to this document.
- Raw video goes in the shared media folder, **not in Git**.
- Failures are useful footage. Do not stage or delete them.

# Master Capture Checklist

Check each one off when the file exists in the shared media folder. **Nothing here has been captured yet, as far as the AI knows.** Folder: `Media/GMS127-128/`.

| File | What to show | Notes |
|---|---|---|
| `00-baseline-no-enemies.mp4` / `.png` | The world with no enemy: walk around for 10–15 s | Retroactive: remove or hide the slime **without saving the map** |
| `01-branch-created.png` | Terminal with `git branch --show-current` showing `GMS-127-128` | |
| `02-ai-inspection-findings.png` | The AI's reply to the opening prompt | |
| `03-player-class-and-spawn.png` | `GrantMoneyPlayerCharacter.h` and the game mode spawn override | |
| `04-enemybase-header.png` | `EnemyBase.h`: the properties and the two virtual functions | |
| `05-enemybase-details-panel.png` | Details panel of a placed enemy, `Enemy|Movement` category | |
| `06-enemybase-compile-success.png` | Build output with `Result: Succeeded` | Keep failures too |
| `07-slimeenemy-code.png` | `SlimeEnemy.cpp`, showing how little it contains | |
| `08-slime-compile-success.png` | Build output with `Result: Succeeded` | |
| `09-slime-placed-in-level.png` | The slime in `PlayGame1` and its Details panel | |
| `10-test-a-one-slime.mp4` | Test A: Play, the chase, the stop, `show collision` | Main result clip |
| `11-test-b-moving-player.mp4` | Test B: walk away, circle, reverse | |
| `12-test-c-player-stops.mp4` | Test C: stop, wait, step away, stop again | |
| `13-test-d-multiple-slimes.mp4` | Test D: four or five slimes at once | |
| `14-test-e-no-valid-player.mp4` | Test E: no player, slimes idle | Say which method you used |
| `14b-test-e-no-target-warning-log.png` | The Output Log with one warning per slime | |
| `15-test-f-stop-restart-pie.mp4` | Test F: three starts and stops | |
| `16-slowdown-distance-details.png` | Details panel with `SlowdownDistance` | |
| `17-fast-slime-slowdown-50.mp4` | Fast slime (`MoveSpeed` about 800) trailing smoothly | Optional pair, for the before/after |
| `18-fast-slime-slowdown-0-stutter.mp4` | The same test with `SlowdownDistance` 0, showing stutter | Optional pair |
| `19-no-aicontroller-in-outliner.png` | World Outliner during Play with no `AIController` for the slime | |
| `20-pursuit-review-findings.png` | The AI's reply to the Step 6 review | |
| `21-decoupling-check.png` | A search of `EnemyBase` for `slime`, `MapArrays`, `TileMap` finding nothing | |
| `22-multi-slime-bumping.mp4` | Slimes bumping and wobbling | Optional, only if you see it |
| `23-git-status-final.png` | `git status` before committing | |
| `24-final-verification.png` | Output Log with no enemy errors, beside the filled-in Result Summary | |

### Suggested Recording Order

1. `00` first, if you still want it, before anything else changes the map.
2. `01`, `02` and `03` from the investigation (screenshots only).
3. `04` to `09` are screenshots of the code and the placed slime.
4. Record `10` to `15` (Tests A to F) from the corrected build.
5. Record `16` to `19` during or just after Test B and Test A.
6. `20` and `21` from the review replies and a search.
7. `22` if you see bumping in Test D.
8. `23` and `24` last.

---

# Suggested Video Structure

## Section 1 – The Problem

Use:

```text
00-baseline-no-enemies.mp4
```

Explain that the world is empty and nothing pushes back.

## Section 2 – Reuse, Don't Replace

Use:

```text
02-ai-inspection-findings.png
03-player-class-and-spawn.png
```

Explain that the player is already a Character, and the project already has the movement and collision an enemy needs.

## Section 3 – The Base Class

Use:

```text
04-enemybase-header.png
05-enemybase-details-panel.png
06-enemybase-compile-success.png
```

Explain what the base class knows, and what it deliberately does not.

## Section 4 – The Slime

Use:

```text
07-slimeenemy-code.png
08-slime-compile-success.png
09-slime-placed-in-level.png
10-test-a-one-slime.mp4
```

Show how little the slime contains, then show it chasing.

## Section 5 – Following and Stopping

Use:

```text
11-test-b-moving-player.mp4
12-test-c-player-stops.mp4
```

Show it following a moving player and settling when the player stops.

## Section 6 – The Review

Use:

```text
16-slowdown-distance-details.png
17-fast-slime-slowdown-50.mp4
18-fast-slime-slowdown-0-stutter.mp4
19-no-aicontroller-in-outliner.png
20-pursuit-review-findings.png
```

Explain why a passing test is not the end, show the stutter and its fix, and the hidden AI controller.

## Section 7 – Crowds and Edge Cases

Use:

```text
13-test-d-multiple-slimes.mp4
22-multi-slime-bumping.mp4
14-test-e-no-valid-player.mp4
14b-test-e-no-target-warning-log.png
15-test-f-stop-restart-pie.mp4
21-decoupling-check.png
```

Show several slimes, say honestly what they do when they meet, then show no-player and restart behaviour and the decoupling check.

Suggested closing narration:

> "We now have a reusable enemy base and one slime that proves it. It follows the player, stops at a set distance, and doesn't depend on the map, the camera or any one player class. What it doesn't do yet is spawn, climb, hurt you or avoid its friends. Those are next."

---

# AI Workflow Material to Preserve

Do not save only the successful prompts. Also keep examples where:

- the AI gave you wrong information (it said Auto Possess AI defaults to `Disabled`, and it is `PlacedInWorld`);
- a test result looked like a bug but was not (the first headless runs showed nothing moving, which was the test harness);
- the first test could not have failed, and a second look found a real problem (the stutter);
- you changed the AI's proposed design (one combined branch, new files only);
- a clue was isolated instead of guessed at (slime wobble disappeared when slimes stopped blocking each other).

A useful lesson is:

```text
We inspected the project
→ AI proposed a solution
→ we tested it
→ something was missed
→ we measured to find why
→ corrected the implementation
→ verified the final result, and labelled what we did not verify
```

---

# End-of-Task AI Prompt Template

```text
GMS-127 and GMS-128 are now at their final verification stage.

Actual results:

[PASTE THE REAL TEST RESULTS FOR TESTS A TO F]

Review only the changes associated with GMS-127 and GMS-128.

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

# GMS-127 / GMS-128 Definition of Done

```text
[ ] A reusable C++ enemy base class exists (AEnemyBase).
[ ] The base contains nothing slime-specific.
[ ] Target acquisition is reusable (FindTarget / AcquireTarget).
[ ] Movement speed is configurable in the Editor.
[ ] Stopping distance is configurable in the Editor.
[ ] A missing player is handled safely (no crash, no movement, one warning).
[ ] ASlimeEnemy derives from the base and adds no pursuit logic.
[ ] The slime finds the real player at runtime.
[ ] The slime follows a moving player.
[ ] Several placed slimes pursue the player at once.
[ ] The slime stops at the stopping distance without jitter.
[ ] Stopping and restarting Play behaves the same each time.
[ ] No TileGen, MapArrays or map-specific coupling.
[ ] No spawning, health, damage, death, XP, climbing, pathfinding or UI.
[ ] Project compiles.
[ ] Tests A to F performed and results recorded.
[ ] The PlayGame1.umap change is decided on (kept or discarded).
[ ] Numbered screenshots exist for each step.
[ ] Numbered raw clips exist in the shared media folder (not in Git).
[ ] AI prompts and corrections were preserved.
[ ] At least one verification result is recorded.
[ ] Documentation reflects what actually happened.
```

---

# Sources Used for This Document

- `CLAUDE.md`
- `Game/GrantMoney.uproject`, `Game/Config/DefaultEngine.ini` (game mode lines only)
- `Game/Source/GrantMoney/GrantMoney.Build.cs`
- `Public/GrantMoneyPlayerCharacter.h`, `Private/GrantMoneyPlayerCharacter.cpp`
- `Public/GrantMoneyGameMode.h`, `Private/GrantMoneyGameMode.cpp`
- `Public/MapCameraPlayerController.h`
- Keyword search of `Source/GrantMoney` for enemy, health, damage, target, AI and navigation terms, and a search of `TileMapRenderer.cpp` for collision lines
- Document style references: `Module2-BringingTheWorldToLife/Sprint1/GMS-106-ScriptDocumentation.md` and `Sprint2/GMS-111-ScriptDocumentation.md`
- UE 5.8 engine source, read only to check defaults and behaviour for the Step 6 review: `Pawn.cpp` (`AutoPossessAI` default and when it applies), `CharacterMovementComponent.cpp` (braking and friction defaults, no-controller conditions, rotation with no acceleration), `World.cpp` and `LevelTick.cpp` (how a world ticks, used to debug the test harness)
- `Private/EnvironmentPlacerTests.cpp`, for the project's automation-test conventions (the temporary tests followed them and were deleted)
- Step 7 additionally: the four new source files read in full, case-insensitive text searches of them (listed under "How the Review Was Done"), `git status`, `git diff --stat`, `.gitattributes` (LFS rules for `*.umap` and `*.uasset`), `git branch --show-current`, and a second temporary headless test (five slimes, no player, circling player, a collision-isolation re-run). That test was deleted after use.
- Earlier-document structure reused for the capture, review and wrap-up sections: `Module2-BringingTheWorldToLife/Sprint2/GMS-111-ScriptDocumentation.md` (media rules, folder list, master checklist, video structure, AI workflow guidance, end-of-task prompt, Definition of Done, final review) and `Sprint1/GMS-106-ScriptDocumentation.md` (per-step capture tables, AI workflow log)
