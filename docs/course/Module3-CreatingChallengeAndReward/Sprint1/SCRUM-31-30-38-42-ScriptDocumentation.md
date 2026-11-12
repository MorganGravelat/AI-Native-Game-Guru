# SCRUM-31 / SCRUM-30 / SCRUM-38 / SCRUM-42 – Spawning, Emergence and the Level Timer
## Module 3 Sprint 1 – Development and Recording Guide (combined milestone)

This guide covers the first vertical-slice milestone: **a timed survival run with automatic slime spawning.** It combines SCRUM-31, SCRUM-30, SCRUM-38 and the foundation of SCRUM-42 into one implementation sequence. It builds three things and joins them:

- **A run controller** (`ARunController`): one authoritative owner of the run's state (`Starting`, `Active`, `BossPhase`, `Won`, `Lost`) and its survival clock (180 s by default). There is no score here: the character's level, once implemented, is the run's progress readout.
- **A reusable enemy spawner** (`AEnemySpawner`): spawns an enemy class around the player, ramping up from a slow, small start to a fast, large cap as the run's survival time goes on, on validated ground only.
- **Emergence** (`UEnemyEmergenceComponent` + hooks on `AEnemyBase`): a spawned slime rises out of the ground, and only starts chasing, hurting and being targeted when it is up.
- **A minimal HUD** (`ARunHUD`): countdown, run state and enemy count, so the milestone can be checked by eye straight away.

- **Part A:** change report
- **Part B:** developer implementation guide
- **Part C:** AI workflow log
- **Part D:** video voice-over script
- **Media Capture List:** every screenshot and clip to record, with file names

**Status:** the code, tests and this document are written. **The project builds, and the full `GrantMoney.*` Automation suite passes: 100 of 100, 27 of them new** (run by the AI session on this machine, 2026-11-12). The real level was also started headless (see §9). **Nothing has been looked at on screen yet:** the HUD layout, the emergence animation, the spawn ramp and the camera view are pending the developer's PIE checks in §9. No media has been captured.

**Revision note (2026-11-12, same day):** after the first pass, the developer asked for two changes, both applied and re-tested: (1) the run no longer tracks a score — the character's level will be the run's progress readout once it exists; (2) the spawner no longer spawns at a fixed rate/batch/cap — it now ramps from slow-and-small to fast-and-large as the run's survival time goes on (§6A has the designer-facing explanation). See the new Part C entry for the follow-up prompt.

**What is and is not evidence in this document:**

| Kind | Meaning |
|---|---|
| Read from source | Taken from reading project or engine source. Not a test. |
| Compiled | UnrealBuildTool finished with `Result: Succeeded`. Says nothing about behaviour. |
| Automation test | A `GrantMoney.*` Automation test passed in a headless editor run, in a synthetic empty world (or, where stated, with the real level-1 map built by the real renderer). |
| Headless run | The real project, game mode and level started with `-game -nullrhi` and the Output Log was read. Proves wiring and logic; shows nothing on screen. |
| Developer reported | The developer played it in the Editor and said what they saw. |

At the time of writing, **"Read from source", "Compiled", "Automation test" and "Headless run" apply.** "Developer reported" does not yet.

---

## Jira

The four tickets were implemented as one sequence. **The ticket texts were not available to the AI**, so the criteria below come from the combined request, not from Jira. Check each against its ticket and assign it before closing the tickets.

| Key | Where it lands in this milestone |
|---|---|
| SCRUM-31 (named in SCRUM-33 as "the enemy spawner") | `AEnemySpawner`, `FSpawnArena` / `FEnemySpawnPlacement` |
| SCRUM-30 / SCRUM-38 | Emergence (`UEnemyEmergenceComponent`, `AEnemyBase::BeginEmergence`) and the run timer / spawn flow. Which of the two is which was not confirmed. |
| SCRUM-42 (foundation only) | `ARunController` states, events, `BossPhase` entry. The boss itself is **not** built. |

**Done when, and where each one is met:**

| Criterion | Met by | Evidence |
|---|---|---|
| The run starts after the playable map and the player are ready | `AGrantMoneyGameMode::CheckRunReady` waits for `ATileMapRenderer::IsMapReady()` and a player pawn, then calls `StartRun` | Headless run: map built, then `run starting`, then `Starting -> Active` |
| One authoritative run controller with Starting, Active, BossPhase, Won, Lost | `ARunController`, `EGrantMoneyRunState`, one transition table in `TryTransition` | Tests `Run.RunController.*` |
| Elapsed time, remaining time from an editable duration (180 s) | `RunDurationSeconds`, `GetElapsedSeconds`, `GetRemainingSeconds` | Test `Clock` |
| Pause freezes gameplay time | The clock moves only in `Tick` and only when `UWorld::IsPaused()` is false | Test `PauseFreezesTime` (real world ticks while paused) |
| At zero, enter BossPhase exactly once; normal spawning stops | `AdvanceRun` → `TryTransition(BossPhase)` once; `IsNormalSpawningAllowed()` is true only in `Active` | Tests `BossPhaseOnce`, `FollowsRunStateAndPause` |
| State and timer events for HUD and boss | `OnStateChanged`, `OnTimeChanged` (per whole second), `OnBossPhaseStarted` | Test `BossPhaseOnce`, `Clock` |
| Reusable spawner: class, min/max distance, a difficulty ramp (interval, batch size, cap) | `AEnemySpawner` properties, `GetDifficultyAlpha` / `GetCurrentSpawnInterval` / `GetCurrentBatchSize` / `GetCurrentMaxActiveEnemies` | Tests `Run.EnemySpawner.*` |
| Positions validated against map data, ground height and capsule clearance | `TryPlaceAt`: `FSpawnArena::IsFootprintPlayable`, downward trace, padded capsule overlap | Tests `StaysOnPlayableCellsAtMapEdges`, `RejectsBlockedAndRaisedGround`, `Map.TileMapRenderer.SpawnerOnRealMapEdge` |
| Bounded retries; skip when nothing valid | `MaxPlacementAttempts` per enemy | Test `SkipsWhenNoValidPosition` (15 attempts for 3 enemies at 5 each) |
| Emerging enemies count toward the cap; slots free when removed | `SpawnedEnemies` + `IsActiveEnemy` (alive and not being destroyed) | Tests `RingAndCap`, `ReplacesDeadEnemies` |
| Slime is the first type; collision root at the ground spot; mesh grows from flat; bottom stays near the ground | `ASlimeEnemy::GetEmergenceVisual`, `UEnemyEmergenceComponent::ComputePose` | Tests `Emergence.PoseMath`, `AnimatesVisual`, `EnemySpawner.SlimeEmerges` |
| Pursuit, contact damage and weapon targeting start after emergence | `AEnemyBase::Tick` returns while `bIsEmerging`; `CanBeTargeted()` used by `UStarterWeaponComponent` | Tests `EnemyBase.EmergenceGatesPursuitAndDamage`, `StarterWeapon.SkipsEmergingEnemies` |
| Works with a replacement mesh later | The animation scales whatever component `GetEmergenceVisual()` returns, relative to its own rest pose and bounds | Test `AnimatesVisual` (any scene component); see §6A |
| Spawning starts slow and small, ramps up to fast and large over the run | `GetDifficultyAlpha` (0 at survival time 0, 1 at `DifficultyRampSeconds`) lerps `SpawnIntervalAtStart/End`, `BatchSizeAtStart/End`, `MaxActiveEnemiesAtStart/End`; optional `DifficultyCurve` reshapes it | Test `DifficultyRampsOverTime` |
| Minimal countdown and run-state label | `ARunHUD` | Clock and state: tests. **Look on screen: pending.** |

**Related tickets:** SCRUM-33 (stats, health, death), SCRUM-32 (starter weapon), SCRUM-35 (pickups, drops), SCRUM-53 (stat boosts).

---

# Part A – Change Report

## Milestone Summary

Starting the level now starts a three-minute run. After a one-second *Starting* beat the run is *Active*: slimes rise out of the ground 9 to 15 m from the player, spawning slowly and in small batches at first (one every 3.5 s, up to 6 alive) and ramping up over the first 2.5 minutes of survival time to fast, large batches (every 0.75 s, up to 30 alive), then holding there. Dead slimes free their slot and are replaced on the next batch. When the clock hits 0:00 the run enters *Boss Phase* once and spawning stops. A text readout at the top of the screen shows the clock, the state and the enemy count.

## Files Created

| File | Purpose |
|---|---|
| `Public/RunController.h`, `Private/RunController.cpp` | `EGrantMoneyRunState`, `ARunController`: state machine, clock, events. |
| `Public/EnemySpawnPlacement.h`, `Private/EnemySpawnPlacement.cpp` | `FSpawnArena` (a TileGen map plus its world transform: cell lookup, playable test, circle-footprint test) and `FEnemySpawnPlacement` (even-area ring sampling). Pure data and maths. |
| `Public/EnemySpawner.h`, `Private/EnemySpawner.cpp` | `AEnemySpawner`, `FEnemySpawnBatchResult`. |
| `Public/EnemyEmergenceComponent.h`, `Private/EnemyEmergenceComponent.cpp` | `UEnemyEmergenceComponent`: grows a visual from flat to normal with its bottom held. |
| `Public/RunHUD.h`, `Private/RunHUD.cpp` | `ARunHUD`: canvas readout. `// NO-TEST:` (drawing only). |
| `Private/RunControllerTests.cpp` | 6 tests, `GrantMoney.Run.RunController.*` |
| `Private/EnemySpawnPlacementTests.cpp` | 4 tests, `GrantMoney.Run.SpawnPlacement.*` |
| `Private/EnemyEmergenceComponentTests.cpp` | 3 tests, `GrantMoney.Run.Emergence.*` |
| `Private/EnemySpawnerTests.cpp` | 9 tests, `GrantMoney.Run.EnemySpawner.*` |
| `Private/TileMapRendererTests.cpp` | 2 tests, `GrantMoney.Map.TileMapRenderer.*` (real level-1 map) |
| `Private/Tests/RunTestListener.h`, `Private/Tests/EmergenceTestTypes.h` | Test helpers: event recorder, and an enemy that chases a chosen actor. |
| `docs/course/Module3-CreatingChallengeAndReward/Sprint1/SCRUM-31-30-38-42-ScriptDocumentation.md` | This document. |

## Files Modified

| File | Change | Why |
|---|---|---|
| `Public/TileMapRenderer.h`, `Private/TileMapRenderer.cpp` | `IsMapReady()` (set when `BuildMap` finishes, cleared by `ClearGeneratedComponents`); `static GetCellSize()`. | The game mode must wait for the ground slabs before the run starts, and the spawner's ground trace needs them. `TileSize` was private. |
| `Public/EnemyBase.h`, `Private/EnemyBase.cpp` | `Emergence` component; `BeginEmergence()`, `IsEmerging()`, `CanBeTargeted()`, virtual `GetEmergenceVisual()`. `Tick` returns early while emerging. A death cancels emergence. | Emergence is general enemy behaviour, not slime-only. |
| `Public/SlimeEnemy.h`, `Private/SlimeEnemy.cpp` | `GetEmergenceVisual()` returns `PlaceholderMesh`. | The slime's visual is what rises. |
| `Private/StarterWeaponComponent.cpp`, `Public/StarterWeaponComponent.h` (comment) | `IsLivingEnemy` also requires `CanBeTargeted()`. | Weapons must not target an emerging slime. |
| `Public/GrantMoneyGameMode.h`, `Private/GrantMoneyGameMode.cpp` | `RunControllerClass`, `EnemySpawnerClass`, `HUDClass = ARunHUD`, `GetRunController()`, `PrepareRun` / `IsRunReady` / `CheckRunReady`. The HUD class is also forced on the player controller in `HandleStartingNewPlayer`. | Creates the run pieces (or uses ones placed in the level) and starts the run when ready. |
| `Private/EnemyBaseTests.cpp`, `Private/StarterWeaponComponentTests.cpp` | 3 new tests (2 + 1). | `CLAUDE.md`: changed logic gets tests. |
| `Public/RunController.h`, `Private/RunController.cpp`, `Private/RunControllerTests.cpp` | Removed `ScorePerSecond` / `GetScore()`; updated the class comment and the `TryTransition` log line. | Developer: the character's level is the run's progress readout, not a separate score. |
| `Public/EnemySpawner.h`, `Private/EnemySpawner.cpp` | `SpawnInterval` / `BatchSize` / `MaxActiveEnemies` replaced by `*AtStart` / `*AtEnd` pairs, `DifficultyRampSeconds`, an optional `DifficultyCurve`, and `GetDifficultyAlpha` / `GetCurrentSpawnInterval` / `GetCurrentBatchSize` / `GetCurrentMaxActiveEnemies`; `AdvanceSpawning` and `SpawnBatch` use the current (ramped) values. Added `SetElapsedSecondsOverride` for tests and tools. `ResolveRunController` / `IsSpawningAllowed` are now `const`. | Developer: spawning should start slow and speed up as the level goes on, not run at a fixed rate up to a fixed cap. |
| `Private/RunHUD.cpp`, `Public/RunHUD.h` | Dropped the score line; the enemy count reads `GetCurrentMaxActiveEnemies()` instead of the old fixed `MaxActiveEnemies`. | Same. |
| `Private/EnemySpawnerTests.cpp`, `Private/TileMapRendererTests.cpp` | Existing tests set the `*AtStart`/`*AtEnd` pairs to the same value where an exact, non-ramped count was being asserted; one new test, `DifficultyRampsOverTime`. | Keep deterministic assertions; cover the new ramp. |

## Files Not Changed

- `FMapArrays`, `FPlayerSpawnResolver`, `FMapBoundaryGenerator`, `UContactDamageComponent`, `UAttackHitComponent`, `UHealthComponent`, `AMagicOrbProjectile`, `APickupBase`, the pause menu and controller. Pause already freezes the world, which is all the run needs.
- No `.uasset` / `.umap` was created or edited by this work. (`Game/Content/Maps/PlayGame1.umap` already showed as modified before this work started.) Config, `.uproject` and `GrantMoney.Build.cs` are untouched.

---

# Part B – Developer Implementation Guide

## 1. Objective

At the end:
- starting the level starts a run: *Starting* (1 s) → *Active* (180 s) → *Boss Phase*;
- slimes rise out of valid ground around the player, spawning slowly and in small batches at first and ramping up to fast, large batches as survival time goes on, up to an editable cap, and are replaced as they die;
- pausing freezes the clock and the spawning;
- at 0:00 the run enters *Boss Phase* once and normal spawning stops;
- a text readout shows clock, state and enemy count;
- every number, including the ramp's start/end values and how long it takes, is editable in the Details panel.

## 2. Why This System Exists

**Before this ticket** the level had a map, a player with a weapon, slimes that chase and a drop system, but slimes only existed if hand-placed, and nothing owned "the run".

**Chosen architecture:**

- **One owner of run state.** `ARunController` is the only thing that knows the state and the clock. HUD, spawner and (later) the boss read it or listen to it. No other class keeps its own timer for the run.
- **Time is game time.** The clock and the spawn timer advance in `Tick`. A paused world does not tick actors, so pause freezes both with no pause-specific code; a guard on `IsPaused()` keeps that true even if something ticks them directly.
- **Spawner is data plus two checks.** The spawner knows nothing about slimes, the map file or the run's internals. It takes an `EnemyClass`, asks `FSpawnArena` whether a spot is on the map, traces for ground, and overlap-tests the capsule.
- **Map access goes through the existing boundary.** `FSpawnArena` is built from `FMapArrays::GetMap` and the `ATileMapRenderer` transform, with the same playable-cell rule as `FPlayerSpawnResolver` and `FMapBoundaryGenerator` (`Valid != 0` and `Ground != 0`). No generated TileGen file name or coordinate appears in gameplay code.
- **Emergence is presentation only.** The capsule (collision root) is placed first; only the *visual* animates. `AEnemyBase` decides what is switched off while it runs.
- **Bottom held, by bounds.** Scaling a mesh about its centre sinks its bottom into the ground. The component reads the visual's local bottom from its bounds and moves the visual up as it flattens, so the bottom stays put for any mesh.
- **Difficulty is a function of survival time, not a timer of its own.** The spawner asks the run controller "how much survival time has passed" and lerps between an "AtStart" and an "AtEnd" value for each of interval, batch size and cap. No separate ramp clock exists to drift out of sync with the run, and several spawners with different enemy classes ramp together automatically.

**Alternatives considered:**

| Alternative | Decision |
|---|---|
| Run state on the game mode | **Rejected.** A separate actor can be placed in a level with its own duration, and tests can create one without a game mode. The game mode only creates it and starts it. |
| A world subsystem for the run | **Rejected for now.** The spawner finds the one controller by itself (throttled lookup), the HUD does the same. A subsystem is the next step if more systems need it. |
| Timers (`FTimerManager`) for the clock and spawning | **Rejected.** Tick-based accumulation is easy to drive directly from tests and has the same pause behaviour. |
| Spawn by `NavMesh` projection | **Not used.** The existing player-start code trusts map data plus an overlap test, so the spawner follows the same pattern. |
| Animate the mesh in the Slime's own `Tick` | **Rejected.** A component is reusable by any enemy and any mesh type. |
| UMG widget for the HUD | **Deferred.** `AHUD` canvas drawing needs no asset and is explicitly temporary. |
| One global enemy cap | **Per spawner for now.** Each spawner counts its own enemies. A global cap is a later decision (§10). |
| A fixed spawn rate for the whole run | **Rejected (revised after the first pass).** The developer asked for a slow start that speeds up, so interval/batch/cap are now each a lerp between an "AtStart" and an "AtEnd" value over `DifficultyRampSeconds`, not constants. |
| A score on the run controller | **Removed (revised after the first pass).** The developer's plan is for the character's level (once implemented) to be the run's progress readout, so a separate score would be redundant. |

## 3. Prerequisites

- SCRUM-33, SCRUM-32, SCRUM-35 and SCRUM-53 merged (this builds on the health, weapon, drop and stat systems).
- Unreal Engine 5.8, Editor **closed** when running `scripts/run_tests.py` (Live Coding breaks the build).
- A level with an `ATileMapRenderer` (the existing levels have one).

## 4. Architecture Overview

```
Level starts
  GameMode.HandleStartingNewPlayer ─► player pawn + camera
        └─► PrepareRun ─► ARunController (placed one, else spawned)   AEnemySpawner (placed one, else spawned)
                      └─► poll every 0.1 s until TileMapRenderer.IsMapReady() && player pawn
                                └─► RunController.StartRun()

ARunController (Tick, game time)
  Starting ──StartRun + StartDelaySeconds──► Active ──remaining = 0──► BossPhase ──► Won
                                               │                         │
                                               └──────────► Lost ◄───────┘
  events: OnStateChanged(old,new), OnTimeChanged(elapsed,remaining) per whole second, OnBossPhaseStarted

AEnemySpawner (Tick, game time)               ARunHUD (canvas)
  allowed only while Run is Active              reads state, remaining, spawner count
  difficulty = GetDifficultyAlpha(): 0 at survival time 0, 1 at DifficultyRampSeconds (lerped, or
    reshaped by an optional DifficultyCurve); holds at 1 after that
  every GetCurrentSpawnInterval() [lerp(IntervalAtStart, IntervalAtEnd, difficulty)]: SpawnBatch
    for each of GetCurrentBatchSize() [lerp(BatchSizeAtStart, BatchSizeAtEnd, difficulty)] (stop at
    GetCurrentMaxActiveEnemies() [lerp(CapAtStart, CapAtEnd, difficulty)]):
      up to MaxPlacementAttempts times:
        random point in ring [Min,Max] around player
        1. FSpawnArena.IsFootprintPlayable  (map data, whole capsule + padding)
        2. trace down: solid, flat, within GroundHeightTolerance of tile height
        3. padded capsule overlap vs Pawn channel (walls, props, other enemies)
      success: SpawnActor(EnemyClass at ground + half-height) ─► BeginEmergence()
      failure: skip this enemy (no forced spawn)

Enemy (ASlimeEnemy)
  BeginEmergence ─► UEnemyEmergenceComponent.Begin(PlaceholderMesh): flat + wide ─► normal over Duration, bottom held
     while emerging: no chase, no contact damage, CanBeTargeted() = false  (weapon skips it)
     OnFinished: pursuit, contact damage and targeting turn on
```

**Defaults** (all editable in the Details panel):

| Where | Property | Default |
|---|---|---|
| Run controller | `RunDurationSeconds` | 180 s |
| | `StartDelaySeconds` | 1 s (the visible *Starting* beat; counts as no survival time) |
| Spawner | `EnemyClass` | `ASlimeEnemy` |
| | `MinSpawnDistance` / `MaxSpawnDistance` | 900 / 1500 cm (ground plane, from the player) |
| | `SpawnIntervalAtStart` / `SpawnIntervalAtEnd` | 3.5 s / 0.75 s |
| | `BatchSizeAtStart` / `BatchSizeAtEnd` | 1 / 4 |
| | `MaxActiveEnemiesAtStart` / `MaxActiveEnemiesAtEnd` | 6 / 30 |
| | `DifficultyRampSeconds` | 150 s (reaches full difficulty with 30 s of the default 180 s run left) |
| | `DifficultyCurve` | None (straight-line ramp) |
| | `MaxPlacementAttempts` | 12 per enemy |
| | `GroundClearance` / `ClearancePadding` | 2 cm / 5 cm |
| | `GroundTraceHalfHeight` / `GroundHeightTolerance` | 400 cm / 30 cm |
| | `GroundTraceChannel` | Visibility (the tile ground blocks it) |
| | `bRequireActiveRun` | true |
| Emergence (on every enemy) | `Duration` | 0.8 s |
| | `FlattenedScale` | (1.25, 1.25, 0.1) × normal scale |

## 5. AI-Assisted Planning

### AI Prompt
The developer's request (Part C, row 1): build the foundation of a timed survivor run with automatic slime spawning; inspect `GrantMoneyGameMode`, `PlayerSpawnResolver`, `MapArrays`, `TileMapRenderer`, `EnemyBase`, `SlimeEnemy`; start the run after the playable map and player are ready; one authoritative run controller with five states, an editable 180 s duration, pause freezing time, a single transition into `BossPhase`, and events for HUD and boss; a reusable spawner with editable class, distances, interval, batch size and cap, validated positions, bounded retries and cap accounting; slime emergence with the bottom held near the ground and gameplay enabled afterwards; a minimal countdown and state label; score from survival time; verification list; documentation.

### Expected AI Output
A map of the existing classes to the requirements, the gaps, and the smallest set of new classes, with tests.

### Human Review
- Confirm the **1 s Starting delay** (added so the *Starting* state is visible; set `StartDelaySeconds` to 0 to remove it).
- Confirm the **per-spawner cap**, and whether weapons may hit an emerging slime that an orb flies through (§10).
- Assign the work to the four tickets.
- Choose the real spawn distances, interval, batch size and cap for the game's pacing (current values are starting points).

## 6. Step-by-Step Implementation

### Step 1 – Know when the map is ready
`ATileMapRenderer` builds its tiles, ground slabs and walls in `BeginPlay`, which can run *after* the player is spawned. `IsMapReady()` is false until `BuildMap` finishes. The existing spawn-position code reads only map data, which is why it never needed this.

### Step 2 – The run controller
`ARunController` holds the state, the start flag and the clock. `AdvanceRun(dt)` is the whole simulation: *Starting* waits for `StartRun` then `StartDelaySeconds`; *Active* adds time (never past the duration), broadcasts `OnTimeChanged` when the whole-second reading changes, and at zero makes the one `Active → BossPhase` transition and fires `OnBossPhaseStarted`. `TryTransition` is the only code that changes the state and allows only `Starting→Active`, `Active→BossPhase|Lost`, `BossPhase→Won|Lost`; so no state can be entered twice. `RequestWin` and `RequestLoss` are the hooks for the boss and the player's death. `Tick` calls `AdvanceRun` unless the world is paused. There is no score property: it was removed after the first pass once it was clear the character's level will track progress instead.

### Step 3 – Map and placement maths
`FSpawnArena` wraps one registered map and the renderer's transform. `WorldToCell` inverts the transform and rounds (cell `(X, Y)` is centred on local `(X, Y) × 100`). `IsFootprintPlayable(centre, radius)` tests the centre and eight rim points, so a capsule that sticks out past a map edge is rejected. `SampleRingOffset` picks a point in the ring with equal chance per area (it interpolates the *squared* radius).

### Step 4 – Emergence component
`Begin(visual)` records the visual's relative scale and location and its local bounds bottom, shows the flat first frame at once, and ticks. `ComputePose` returns scale `rest × lerp(FlattenedScale, 1, smoothstep(progress))` and a vertical offset that keeps `location.z + scale.z × bottomLocalZ` constant. `Finish` restores the exact rest pose and fires `OnFinished`. Zero duration or no visual finishes at once and touches nothing.

### Step 5 – Enemy behaviour while emerging
`AEnemyBase::BeginEmergence` sets `bIsEmerging` and starts the component with `GetEmergenceVisual()`. `Tick` returns before chasing or contact damage. `CanBeTargeted()` (alive, not being destroyed, not emerging) replaces the weapon's old "is living" test. Death clears the state and puts the visual back at its normal shape. `ASlimeEnemy` returns its `PlaceholderMesh`.

### Step 6 – The spawner, and its difficulty ramp
`SpawnBatch` removes dead or removed enemies from its list, then tries `GetCurrentBatchSize()` times, stopping at `GetCurrentMaxActiveEnemies()`. `FindSpawnLocation` samples the ring and runs the three checks per attempt. The capsule check uses a capsule padded by `ClearancePadding`, **made at least as tall as it is wide and raised so its bottom sits where the enemy's bottom will** (see §8.2). The spawned enemy gets the capsule-centre position at ground height + half-height + `GroundClearance`, faces the player, and `BeginEmergence()` is called. `Tick` accumulates game time only while spawning is allowed (the run is *Active*), spawns one batch per due interval (no catch-up bursts — a long frame still spawns only one batch, just a bigger one late in the run) and does nothing while paused.

`GetDifficultyAlpha()` is 0 at survival time 0 and 1 at `DifficultyRampSeconds`, straight-line interpolated unless `DifficultyCurve` is set (then the straight-line fraction is looked up on the curve and the result, also expected 0..1, is used instead — so a designer can make the ramp start slower, end in a rush, or stair-step, with no C++ change). Survival time comes from the resolved `ARunController`, or from `SetElapsedSecondsOverride` if a test or tool used it; with neither, difficulty stays at 0 (the "AtStart" values), which is also what a disconnected spawner (`bRequireActiveRun` off, no run placed) does forever. `GetCurrentSpawnInterval`, `GetCurrentBatchSize` and `GetCurrentMaxActiveEnemies` each lerp their own "AtStart"/"AtEnd" pair at that alpha (batch size and cap are rounded to the nearest whole enemy).

### Step 7 – Game mode and HUD
`AGrantMoneyGameMode::PrepareRun` uses a placed `ARunController` / `AEnemySpawner` or spawns defaults, then polls (0.1 s, up to 10 s, with an error log if it gives up) until the map is built and the player has a pawn, and calls `StartRun`. The HUD class is `ARunHUD`, and it is also set on the player controller in case the Blueprint game mode saved an older class. `ARunHUD::DrawHUD` draws four centred lines.

### Step 8 – Tests and whole-suite run
`python scripts/run_tests.py` (or `--filter GrantMoney.Run`). Result in §9.

## 6A. Designer Setup

**Change the run length.** Either place an `ARunController` in the level and edit it, or make a Blueprint child of `ARunController` and set it as `Run Controller Class` in `BP_GrantMoneyGameMode` (Class Defaults → Run). A controller placed in the level wins over the game mode's.

**Change the difficulty ramp.** Place an `AEnemySpawner` in the level (position does not matter) and edit the Difficulty category:
- `SpawnIntervalAtStart` / `AtEnd`, `BatchSizeAtStart` / `AtEnd`, `MaxActiveEnemiesAtStart` / `AtEnd` — the slow-and-small values early in the run and the fast-and-large values once the ramp is done.
- `DifficultyRampSeconds` — how much survival time it takes to go from "AtStart" to "AtEnd". Set it shorter than `RunDurationSeconds` to reach full difficulty before the boss, or equal to it to still be ramping at 0:00.
- `DifficultyCurve` (optional) — an asset of type Curve Float. Leave it empty for a straight-line ramp; set it to make the ramp non-linear (for example slow for the first half, then a rush). The curve's X axis is read as 0..1 (how far through `DifficultyRampSeconds` the run is) and its Y axis should also stay in 0..1.
- `MinSpawnDistance` / `MaxSpawnDistance`, or make a Blueprint child and set `Enemy Spawner Class` on the game mode. A placed spawner replaces the default one. You can place several spawners with different enemy classes; each ramps and caps on its own. To disable spawning completely, place none and set `Enemy Spawner Class` to None.

**Use a real slime mesh later.** On a Blueprint child of `ASlimeEnemy`, either (a) change the static mesh on `PlaceholderMesh`, or (b) hide `PlaceholderMesh` and add a new mesh component, then override `GetEmergenceVisual()` to return it (C++, one line). The animation reads the new mesh's scale and bounds by itself. For a skeletal mesh the origin should be at the feet. Set the collision capsule size on the Blueprint; the spawner reads it.

**Tune the rise.** Select an enemy → `Emergence` component → `Duration` and `Flattened Scale` (X, Y wider and Z flatter than 1 gives a puddle).

**Test pacing quickly.** Set `RunDurationSeconds` to 20 and the spawner's `DifficultyRampSeconds` to 5 to see the whole ramp, the cap, the replacement and the *Boss Phase* in under half a minute.

## 7. AI Prompts by Stage

| Stage | Prompt |
|---|---|
| Map | "Inspect GrantMoneyGameMode, PlayerSpawnResolver, MapArrays, TileMapRenderer, EnemyBase and SlimeEnemy. List what already exists for: starting after the map is ready, picking valid cells, ground height, spawning enemies, and what is missing." |
| Run controller | "Create one authoritative run controller with Starting, Active, BossPhase, Won and Lost states. Pause must freeze time. At zero remaining enter BossPhase exactly once. Expose events. Write tests that cannot pass if BossPhase is entered twice." |
| Spawner | "Reusable spawner with editable class, distance range, interval, batch size and cap. Validate map data, ground height and capsule clearance. Bounded retries, skip if none. Emerging enemies count toward the cap." |
| Emergence | "Animate the visual from flat to normal, keep the bottom near the ground, work with a replacement mesh, enable pursuit, contact damage and weapon targeting only afterwards." |
| Verification | "Verify spawning near map edges, the cap, replacement, pause and resume, emergence activation and the single BossPhase transition. Run the whole suite." |
| Revision | "So the score is unnecessary, the character's level will be like pseudo score once we have [it] implemented, how do I edit the spawning? It seems a little fast and shouldn't act off of something like a cap like 25, it should start spawning slowly and speed up as the level goes on" |

## 8. Common Mistakes

### 8.1 Zero-duration emergence flattened the mesh to scale 1
- **Symptom:** test `Emergence.EdgeCases`: "Visual untouched" failed; the visual ended at scale (1, 1, 1).
- **Cause:** with `Duration = 0` the component finished before it had recorded the visual's rest pose, then "restored" a default pose.
- **Fix:** nothing to animate means no pose to restore; the early path now only fires `OnFinished`.
- *Real failure during this work, caught by the new test.*

### 8.2 A padded capsule is never shorter than its radius
- **Symptom:** every spawn attempt was rejected; the test log showed the only overlap was the ground itself.
- **Cause:** the clearance check widened the slime capsule (radius 30 → 35) but kept its half-height at 30. A capsule cannot be shorter than its radius, so the shape became a sphere of radius 35 centred 32 cm up, which dips into the ground.
- **Fix:** give the check capsule a half-height of at least its radius and raise it so its bottom is where the enemy's bottom will be.
- *Real failure during this work; found by logging the overlapping component.*

### 8.3 A test enemy with no visual finishes emerging instantly
- **Symptom:** test `EnemyBase.EmergenceGatesPursuitAndDamage`: "Emerging after BeginEmergence" failed.
- **Cause:** the test subclass of `AEnemyBase` had no visual, and no visual means no animation time by design.
- **Fix:** the test enemy returns its character mesh as the visual.
- *Real failure during this work.*

### 8.4 A name in an anonymous test namespace
- **Symptom:** `error C2065: 'N': undeclared identifier` in `EnemySpawnPlacementTests.cpp`.
- **Cause:** the constant lived in `SpawnPlacementTest`; one use was outside it.
- **Fix:** qualify it. (Cheap to catch because the compile was run.)

### 8.5 Starting the run before the map exists
- **Risk, not hit:** the player is spawned before `ATileMapRenderer::BeginPlay`, so a run started in `HandleStartingNewPlayer` would begin with no ground slabs and the first spawn trace would miss. `IsMapReady()` and the polling in `CheckRunReady` are what prevent it.

### 8.6 Several test worlds alive at once
- **Hazard:** test scenes that create a second world while the first is still in scope. The tests use blocks so each world is destroyed first.

## 9. Verification

### Performed

| Test | Action | Result | Kind |
|---|---|---|---|
| Build | `python scripts/run_tests.py` builds `GrantMoneyEditor` | Succeeded | Compiled |
| Automation tests | Full `GrantMoney.*` suite, 2026-11-12 | **100 of 100 passed** (73 pre-existing, 27 new) | Automation test |
| Real level, headless | `UnrealEditor-Cmd GrantMoney.uproject /Game/Maps/PlayGame1 -game -nullrhi`, about 50 s | Log shows: map built (2896 tiles) → `run starting (duration 180 s, start delay 1.0 s)` → `Starting -> Active` → the ramp live: spawns of 1 every ~2.7-3.3 s at difficulty 2-15% (cap 7→9), growing to spawns of 2 by difficulty 17-37% (cap 10→15), exactly as `GetCurrentBatchSize` / `GetCurrentMaxActiveEnemies` predict for that much survival time | Headless run |

**New or changed tests** (rerun with `python scripts/run_tests.py`, Editor closed):

| Group | Tests |
|---|---|
| `Run.RunController` (new) | Defaults, StartDelay, Clock, BossPhaseOnce, WinAndLoss, PauseFreezesTime |
| `Run.SpawnPlacement` (new) | ArenaCells, FootprintAtEdges, RealMap (level 1), RingSampling |
| `Run.Emergence` (new) | PoseMath, AnimatesVisual, EdgeCases |
| `Run.EnemySpawner` (new) | Defaults, DifficultyRampsOverTime, RingAndCap, ReplacesDeadEnemies, SkipsWhenNoValidPosition, StaysOnPlayableCellsAtMapEdges, RejectsBlockedAndRaisedGround, FollowsRunStateAndPause, SlimeEmerges |
| `Map.TileMapRenderer` (new, real map) | IsMapReady, SpawnerOnRealMapEdge |
| `Combat.EnemyBase` | EmergenceGatesPursuitAndDamage, DeathDuringEmergence (new) |
| `Weapons.StarterWeapon` | SkipsEmergingEnemies (new) |

**What the tests prove, against the verification list:**

| Item | Test(s) | What it asserts |
|---|---|---|
| Spawning near map edges | `StaysOnPlayableCellsAtMapEdges`, `FootprintAtEdges`, `SpawnerOnRealMapEdge` | Players in two corners and on an edge: every spawned capsule's footprint is on playable cells, even with ground under the whole area; on the real map too |
| Active-enemy limit | `RingAndCap` | 3, 3, then 1 spawn at a cap of 7; a full spawner spawns 0; never above the cap |
| Removal and replacement | `ReplacesDeadEnemies` | A killed (destroyed) enemy, a killed-but-lingering corpse and a destroyed enemy each free a slot; the next batch refills exactly the gap |
| No valid position | `SkipsWhenNoValidPosition` | Nothing spawns, retries are bounded (15 attempts for 3 × 5), no crash without a player or map |
| Props, walls, raised ground | `RejectsBlockedAndRaisedGround` | Nothing spawns inside a pawn-blocking prop, on ground 60 cm above tile height, or with no ground |
| Pause and resume | `PauseFreezesTime`, `FollowsRunStateAndPause` | Real world ticks and direct `Tick` calls while paused change nothing; time continues from where it stopped |
| Emergence activation | `SlimeEmerges`, `EmergenceGatesPursuitAndDamage`, `SkipsEmergingEnemies` | Mesh flat at spawn, bottom 2 cm above ground at start, halfway and end; no walking, damage or targeting until finished; all three on after |
| Single BossPhase | `BossPhaseOnce` | One state change and one boss event, even after a long step past zero, more time and a second `StartRun`; clock frozen at 0:00; spawning stops |
| Spawning ramps up over time | `DifficultyRampsOverTime` | Difficulty 0 with no run/override; interval, batch size and cap each exactly at their "AtStart" value at 0, their "AtEnd" value past the ramp, and halfway between at the ramp's midpoint; a due batch is bigger late in the run than early; difficulty follows a real `ARunController`'s elapsed time once one is set |

If anything fails on a later run, record the real error under §8 before fixing it.

### Pending (developer, in the Editor)

For a quick look, set a Blueprint-child run controller (or a placed one) to `RunDurationSeconds` 30 and the spawner's `DifficultyRampSeconds` to 20, then reset.

| Test | Action | Expected Result |
|---|---|---|
| Starting and Active | Press Play | Top of screen: `3:00`, state `Starting` (about 1 s, white), then `Active` (green); the clock counts down |
| Spawn ramp | Watch the first minute, then the next | Slimes trickle in one at a time, slowly; over the next couple of minutes they come faster and in bigger bunches |
| Spawn rise | Watch a slime appear | It appears as a flat disc and swells into a sphere in under a second; it stays on the ground, never underneath it |
| Spawn distance | Look at where they appear | None next to the player; none outside the map or behind walls; some close to the map edge when you stand near it |
| Cap | Run away and let them accumulate for a while | The `Enemies x / y` line's `y` grows slowly as the run goes on, and the count stops there (no more appear) until `y` grows again |
| Replacement | Kill some | The count drops at once; later batches fill it back toward the current cap |
| Inactive while rising | Stand next to a rising slime | It does not move or damage you until it is full size; the orbs do not target it until then |
| Pause | Press Escape | Clock, spawning and rising stop; the state line says `(paused)`; they resume where they were after Resume |
| BossPhase | Wait for `0:00` | State `Boss Phase` (red), exactly once; no new slimes; existing ones keep chasing; clock stays `0:00` |
| Output Log | Filter `RunController` and `EnemySpawner` | `Starting -> Active`, one batch line per interval (shrinking interval, growing batch/cap as `difficulty %` rises), one `Active -> Boss Phase` |
| Map edge | Walk to a map corner | Slimes still appear, only on the map |
| Placed overrides | Place an `ARunController` / `AEnemySpawner` in the level and edit them | The level's values are used and no second default is created |
| Difficulty curve | Set `DifficultyCurve` to a simple eased curve asset | The ramp noticeably speeds up or slows down differently than the default straight line |

## 10. Known Limitations

- **Not looked at on screen.** HUD layout and the emergence animation are untested by eye (tests and a headless run only).
- **No boss.** `BossPhase` stops normal spawning and waits. Nothing spawns a boss, hooks `RequestWin`, or clears the remaining slimes (see §12).
- **The run does not end when the player dies.** `RequestLoss` exists but nothing calls it.
- **An orb that flies through an emerging slime can still hurt it.** Only automatic *targeting* skips emerging slimes. This matches "enable weapon targeting after emergence" but is a design call (§5).
- **The emerging capsule already blocks movement.** The player can bump an emerging slime. The capsule is placed at once so other spawns avoid it.
- **The cap is per spawner.** Several spawners add up.
- **The HUD enemy count shows only the first spawner.** It is a testing aid.
- **The slime settles about 2 cm** onto the ground when it starts to move (`GroundClearance`).
- **The ring is measured from the player**, ignoring walls between: a spot behind a wall that is on playable cells is valid.
- **`GroundTraceChannel` assumes the tile ground blocks Visibility.** If the ground's collision profile is changed to something that ignores it, no spawn will pass the ground check; change the property to match.
- **Time dilation** (slow motion) slows the clock and the spawner like everything else. That is intended.
- **The ramp's default numbers are starting points**, not tuned pacing. `SpawnIntervalAtStart/End`, `BatchSizeAtStart/End`, `MaxActiveEnemiesAtStart/End` and `DifficultyRampSeconds` all want playtesting.
- **The ramp never slows back down.** It is monotonic: survival time only grows (it is frozen, not reduced, outside `Active`), so difficulty cannot drop within a run. A later "calm wave" would need its own mechanism, not this ramp.

## 11. Extension Ideas

- A world subsystem exposing the run controller, if more systems need it.
- Several spawner profiles (a second enemy class at a different interval) with a shared global cap.
- Spawn from "off-screen" rings based on the camera's view instead of fixed distances.
- A real HUD widget bound to `OnStateChanged` / `OnTimeChanged`; this canvas HUD is the placeholder.
- Object pooling for enemies if counts grow.
- Ramp other things besides spawning off the same `GetDifficultyAlpha` (enemy speed or health, for example), or expose it from the run controller itself so several spawners and other systems share one alpha instead of each resolving its own run controller.
- Enemy variety: different `AEnemySpawner`s (or classes within one) phased in as difficulty rises, instead of only slimes for the whole run.

## 12. What Remains for Boss Integration

The seam is `ARunController`. Remaining work, in order:

1. **Boss spawn.** Bind to `OnBossPhaseStarted` (or `OnStateChanged` with `NewState == BossPhase`) and spawn the boss. Decide where (use `FSpawnArena` and the same three checks, or a fixed arena spot).
2. **What happens to the existing slimes.** Today they stay alive and keep chasing. Decide: leave, clear (`UGameplayStatics`/spawner API to remove them), or stop their drops.
3. **Win and loss.** Call `RequestWin()` from the boss's death (the boss can use `UCombatEventsSubsystem::OnEnemyKilled`) and `RequestLoss()` from the player's `UHealthComponent::OnDeath`. Both are single-use and ignore wrong states.
4. **Spawner during the boss.** Spawners stop in `BossPhase`. For minions, either give the boss its own spawner with `bRequireActiveRun` off, or add a state test to the spawner.
5. **Boss emergence / entrance.** `BeginEmergence` and `GetEmergenceVisual` already work for any `AEnemyBase`; a boss can reuse them.
6. **End-of-run UI.** Won / Lost screen and restart; the `ARunController` state and the player's level/XP (once implemented) are the data to show.
7. **Progress beyond survival time.** Once the character's level/XP system exists, decide whether the boss or anything else reads it (for example a stronger boss at a higher level). There is no `GetScore()` to change; survival time is read straight off `GetElapsedSeconds()` / `GetDifficultyAlpha()` today.
8. **Real HUD** replacing `ARunHUD`.

## 13. Source-Control Checkpoint

| Item | Value |
|---|---|
| Jira | SCRUM-31, SCRUM-30, SCRUM-38, SCRUM-42 (foundation) |
| Branch | Currently `main`. `CLAUDE.md` says one Jira issue → one branch → one PR; this milestone spans four, so decide whether to branch once (for example `SCRUM-31-spawning-emergence-run-timer`) or split the commit. |
| PR title | `SCRUM-31/30/38/42: Run controller, enemy spawner and slime emergence` |

Suggested commit message (after the PIE checks in §9):
```
SCRUM-31/30/38/42: Add run controller, enemy spawner and slime emergence

- ARunController: Starting/Active/BossPhase/Won/Lost, 180 s survival clock, events; pause freezes it
- AEnemySpawner: ring spawning around the player; interval, batch size and cap ramp from a slow, small
  start to a fast, large end over a difficulty curve driven by survival time, with bounded retries
- FSpawnArena: map-data, ground-trace and capsule-clearance validation (works on any registered map)
- UEnemyEmergenceComponent: flat-to-normal rise with the bottom held; enemies idle until risen
- AEnemyBase::CanBeTargeted; weapon skips emerging enemies
- ATileMapRenderer::IsMapReady; game mode starts the run when the map and player are ready
- ARunHUD: temporary clock / state / enemy count readout (no score: the character's level will track progress)
- 27 new Automation tests and milestone documentation
```
(The pre-existing change to `Game/Content/Maps/PlayGame1.umap` is not part of this work.)

---

# Part C – AI Workflow Log

**Tool:** Copilot (an AI assistant in VS Code), working on the repository on the developer's PC. This session **could** build and run the project: it ran `scripts/run_tests.py` and started the real level headless. It could not look at the screen.

| # | Purpose | Prompt (developer) | Result | Decision | Verification |
|---|---|---|---|---|---|
| 1 | Request | "So we are reaching a stage where we need to start getting a vertical slice out … **1. Spawning, emergence, and the level timer — SCRUM-31, SCRUM-30, SCRUM-38, foundation of SCRUM-42** … Build the foundation of a timed survivor run with automatic slime spawning. Inspect GrantMoneyGameMode, PlayerSpawnResolver, MapArrays, TileMapRenderer, EnemyBase, and SlimeEnemy …" (full text in the request) | Read the named classes plus the weapon, contact damage, health and pause code. Found: player start uses map data only (so it can run before the map is built); pause freezes the world; no run owner, spawner or emergence existed. | Accepted | Read from source |
| 2 | Implementation | (continued) | Wrote the run controller, placement maths, spawner, emergence component, HUD and game mode wiring, plus tests, in one pass. | Accepted | Compiled; tests below |
| 3 | First test run | (automatic) | 13 of 21 new tests passed. Failures: §8.1 (a real bug) and a spawner that never spawned. | Fixed | Re-run |
| 4 | Debugging | (automatic) | Temporary logging showed the *overlap* check was the one rejecting every spot, and that it overlapped only the ground (§8.2). Fixed the check; removed the logging. | Fixed | 21 of 21 |
| 5 | Whole suite | (automatic) | 98 of 99; one new test's helper enemy had no visual (§8.3). | Fixed | **99 of 99** |
| 6 | Smoke run | (automatic) | Started the real level headless; log read (§9). | — | Headless run |
| 7 | Documentation | "…having you make the same kind of documents as the ScriptDocumentation for them but I will have to record later and do the work now." | This document, in the SCRUM-53 format. | — | Checked against the code and the test and log output |
| 8 | Revision request | "So the score is unecessary, the characters level will be like pseudo score once we have the implemented, how do I edit the spawning? It seems a little fast and shouldn't act off of something like a cap like 25, it should start spawning slowly and speed up as the level goes on" | Removed `ScorePerSecond` / `GetScore()` from `ARunController` and the HUD. Replaced the spawner's fixed `SpawnInterval` / `BatchSize` / `MaxActiveEnemies` with `*AtStart` / `*AtEnd` pairs, `DifficultyRampSeconds` and an optional `DifficultyCurve`, all lerped by `GetDifficultyAlpha()` (survival time from the run controller, 0 at the start of the ramp, 1 at the end). Updated every affected test (set `*AtStart` = `*AtEnd` where a test wanted an exact, non-ramped count) and added one new ramp test. | Accepted | Compiled; 100 of 100 tests; a fresh headless run shows the ramp live (§9) |

**Where AI helped most:** finding that player start, map data and the renderer's build order were already separated, which shaped `IsMapReady()` and the polling; and writing tests alongside every class, which caught two real bugs.

**Where it needs a human:** looking at the HUD and the rise on screen; the ramp's actual numbers (how slow, how fast, how long to reach full difficulty — current values are a starting guess); the design calls in §5 and §10; assigning work to the four tickets; the boss.

---

# Part D – Video Voice-Over Script

## Opening
**SAY:** "A survivor game needs a clock and a stream of enemies. Today the level gets both: a three-minute run, and slimes that rise out of the ground around you until the clock runs out."

## Acceptance Criteria
**SAY:** "By the end: the run starts when the map and player are ready; there's one run controller with five states; pausing freezes everything; at zero it enters Boss Phase exactly once; slimes spawn only on valid ground, starting slow and small and ramping up to fast and large as the run goes on, and rise before they chase, hurt, or can be shot."

## Engineering Principle
**SAY:** "One owner for each fact. The run controller owns the clock. The spawner owns how many. The slime owns what it does while it rises. Everything else just reads."

## Step-by-Step Demonstration

### 1. Ask the AI to map the request to existing code
- **SAY:** "First I ask what already exists: where the player starts, how the map is read, what an enemy does."
- **SHOW:** the AI's answer naming `PlayerSpawnResolver`, `MapArrays`, `TileMapRenderer`. *(media 02)*

### 2. Wait for the map
- **SAY:** "The player spawns before the map is built, so the run waits. `IsMapReady` is false until the ground exists."
- **SHOW:** `TileMapRenderer.h` `IsMapReady` and `GrantMoneyGameMode.cpp` `CheckRunReady`. *(media 03)*

### 3. The run controller
- **SAY:** "One class, one table of allowed transitions. That's why Boss Phase can't happen twice."
- **SHOW:** `RunController.cpp` `AdvanceRun` and `TryTransition`. *(media 04)*

### 4. The spawner, and its ramp
- **SAY:** "A ring around the player. Three checks: on the map, on the ground, and room for the body. If nothing fits, skip. Never force it. And it isn't a fixed rate — it starts slow and small, and speeds up and grows the longer you survive."
- **SHOW:** `EnemySpawner.cpp` `TryPlaceAt` and `GetDifficultyAlpha`; the Details panel's Difficulty category. *(media 05, 06)*

### 5. Emergence
- **SAY:** "Only the picture animates. The body is already there. Scale about the centre would sink it, so we hold the bottom."
- **SHOW:** `EnemyEmergenceComponent.cpp` `ComputePose`. *(media 07)*

### 6. Tests
- **SAY:** "Twenty-seven new tests. Two of them caught real bugs before I ever pressed Play."
- **SHOW:** the test run. *(media 08)*

### 7. Play it
- **DO:** Play; show the clock and state. Watch slimes trickle in slowly, then faster and in bigger groups as time passes; show the cap growing; kill some and show refill; pause; let a short run hit Boss Phase. *(media 09 to 14)*

## Common Mistakes
**SAY:** "Two to watch: a capsule can't be shorter than it is wide, so widening it for a clearance check can sink it into the floor; and an animation with nothing to animate must not 'restore' a pose it never saved."

## Verification Checkpoint
**SAY:** "Tests green, then the PIE table. Only mark what you saw."

## Next Milestone
**SAY:** "Next: the boss. Everything it needs, the event, the win and loss calls, is already in the run controller."

---

# Media Capture List

Store under `Media/SCRUM-31-30-38-42/` (outside the repository). Only record what actually happens.

| # | File | Type | Show | Proves |
|---|---|---|---|---|
| 01 | `01-branch.png` | Screenshot | `git branch --show-current` | Branch (decide per §13) |
| 02 | `02-existing-systems-map.png` | Screenshot | AI answer mapping the request to `PlayerSpawnResolver`, `MapArrays`, `TileMapRenderer`, `EnemyBase` | Reuse before new code |
| 03 | `03-map-ready.png` | Screenshot | `IsMapReady` in `TileMapRenderer.h` and `CheckRunReady` in the game mode | The run waits for the map |
| 04 | `04-run-controller.png` | Screenshot | `RunController.cpp` `AdvanceRun`, `TryTransition` | One-time transitions |
| 05 | `05-spawner-details.png` | Screenshot | An `AEnemySpawner` in the level, Details panel's Difficulty category | Editable ramp settings |
| 06 | `06-placement-checks.png` | Screenshot | `EnemySpawner.cpp` `TryPlaceAt` (three checks) and `GetDifficultyAlpha` | Validation and the ramp |
| 07 | `07-emergence.mp4` | Clip | Close-up of a slime rising; bottom stays on the ground | Emergence look (**the unverified item**) |
| 08 | `08-tests-pass.png` | Screenshot | `run_tests.py` output or Automation window, `GrantMoney` all green (100/100) | Tests green |
| 09 | `09-hud-and-starting.mp4` | Clip | Press Play: `Starting` → `Active`, clock counting down | State and clock |
| 10 | `10-spawn-ramp.mp4` | Clip | Early minute: slimes trickle in slowly; a later minute: faster, bigger groups | Difficulty ramp (**the unverified item**) |
| 11 | `11-cap-and-replacement.mp4` | Clip | Count reaches the cap; kill some; it refills | Cap and replacement |
| 12 | `12-map-edge.mp4` | Clip | Stand at a corner; slimes only on the map | Edge handling |
| 13 | `13-pause.mp4` | Clip | Pause: clock and rising freeze; resume | Pause |
| 14 | `14-boss-phase.mp4` | Clip | Short run (30 s) reaches `Boss Phase` once; spawning stops | Single transition |
| 15 | `15-output-log.png` | Screenshot | Output Log filtered `RunController` / `EnemySpawner` | Transitions and batches |
