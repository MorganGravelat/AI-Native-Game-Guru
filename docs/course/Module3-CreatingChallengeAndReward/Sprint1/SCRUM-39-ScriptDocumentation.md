# SCRUM-39 – Temporary Environmental Reward Totems
## Module 3 Sprint 1 – Development and Recording Guide

This guide covers SCRUM-39: **reward totems** appear at random, safe places on the map when the level loads. While the player stands near one, a bar above it fills. When the bar is full, the player gets a **temporary buff** (by default +50% move speed for 8 seconds) and glows while it lasts. The buff runs through a new reusable **temporary effect contract**, so new reward types can be added without touching the player, its movement or the camera.

- **Part A:** change report
- **Part B:** developer implementation guide
- **Part C:** AI workflow log
- **Part D:** video voice-over script
- **Media Capture List:** screenshots and clips to record

**Status:** code, Automation tests and this document are written. **The developer built the project and ran the Automation tests: 145/145 passed** after one build fix (B8.4), and **150/150 passed** after the custom-look and random-buff changes (developer run, report `Game/Saved/Automation/ClaudeReport/index.json`, 2026-11-12). The AI session had no Unreal Engine, so the build and test run were done by the developer. The in-Editor checks in [B9](#9-verification) have not been recorded yet. No media has been captured.

| Kind of evidence | Meaning |
|---|---|
| Read from source | Taken from reading the code. Not a test. |
| Compiled | UnrealBuildTool finished with `Result: Succeeded`. |
| Automation test | A `GrantMoney.*` test passed in a headless editor run, in a synthetic world. |
| Developer reported | The developer played it in the Editor and said what they saw. |

At the time of writing, **"Read from source" and "Developer reported" (build; 150/150 Automation tests in the latest run) apply.** No PIE results yet.

---

## Jira

| Key | Goal |
|---|---|
| SCRUM-39 | Create one proof-of-concept world reward, such as a totem that temporarily improves the player. |

| Criterion | Met by | Evidence |
|---|---|---|
| Player can activate the totem: a bar fills while the player stands near it, and a full bar activates the reward | `ARewardTotem::AdvanceCharge`, bar = `BarPivot`/`BarFill` | Tests `Totem.ChargesOnlyWhilePlayerIsNear`, `Totem.FullBarGivesTemporaryBuff` (passed, developer reported) |
| Clearly visible temporary buff through a reusable buff/effect contract | `UTemporaryEffect` + `UTemporaryEffectComponent` (aura light and glowing ring at the feet; the move speed change is visible itself) | Test `TemporaryEffectComponent.AuraShowsWhileActive` (passed, developer reported); PIE check pending |
| Duration and magnitude configurable | `UTemporaryEffect::Duration`, `UStatModifierEffect::Magnitude`, edited inline in the totem's **Effects** list | Test `TemporaryEffectComponent.DurationAndMagnitudeAreConfigurable` (passed, developer reported) |
| Expires cleanly; cannot permanently corrupt base stats | The buff is one `UStatsComponent` modifier removed by its handle; base values are never written. All running effects end when the component or owner goes away | Tests `ExpiryNeverCorruptsBaseStats`, `DestroyedComponentEndsEverything` (passed, developer reported) |
| New temporary reward types without changing movement/camera | New `UTemporaryEffect` subclass; the effect component is added to the player on demand | Tests `NewEffectTypesNeedNoChanges`, `Totem.NewRewardTypesNeedNoChanges` (passed, developer reported) |
| (Added during the task) Totems populate the level at load, at random positions with no collision issues | `ARewardTotemSpawner`, spawned by `AGrantMoneyGameMode` | Tests `TotemSpawner.*` (passed, developer reported); PIE check pending |

---

# Part A – Change Report

## Milestone Summary
A reusable timed-effect system, a chargeable reward totem that uses it, and a spawner that scatters totems on safe ground when the map is built.

## Files Created
| File | Purpose |
|---|---|
| `Public/TemporaryEffect.h`, `Private/TemporaryEffect.cpp` | `UTemporaryEffect` (the contract) and `UStatModifierEffect` (ready-made stat buff) |
| `Public/TemporaryEffectComponent.h`, `Private/TemporaryEffectComponent.cpp` | Runs effects: timers, stacking, aura, clean-up |
| `Public/RewardTotem.h`, `Private/RewardTotem.cpp` | The totem: range disc, charge bar, activation, single use or cooldown |
| `Public/RewardTotemSpawner.h`, `Private/RewardTotemSpawner.cpp` | Random, collision-safe placement at level start |
| `Private/TemporaryEffectTests.cpp`, `Private/TemporaryEffectComponentTests.cpp`, `Private/RewardTotemTests.cpp`, `Private/RewardTotemSpawnerTests.cpp` | Automation tests |
| `Private/Tests/TemporaryEffectTestScene.h`, `Private/Tests/TemporaryEffectTestTypes.h` | Test helpers (a test world, and a stat-free "counting" effect) |

## Files Modified
| File | Change |
|---|---|
| `Public/GrantMoneyGameMode.h`, `Private/GrantMoneyGameMode.cpp` | New `RewardTotemSpawnerClass`; `PrepareRun` spawns a default `ARewardTotemSpawner` unless the level has one (same pattern as `AEnemySpawner`) |

## Files Not Changed
`AGrantMoneyPlayerCharacter`, its movement, `AMapCameraPawn`, `UStatsComponent`, and the pickup system. The player already maps the `MoveSpeed` stat to `MaxWalkSpeed`, so the speed buff needs no movement code.

---

# Part B – Developer Implementation Guide

## 1. Objective
Give the player a reason to move around the map: totems that reward standing near them with a short, strong, clearly visible buff.

## 2. Why This System Exists
SCRUM-53 added *permanent* stat boosts. Temporary rewards need two more things: something that removes the boost on time, and a guarantee that removing it restores the exact earlier value. Writing that logic once (the effect component) means each new reward is only "what it does" and "how to undo it".

## 3. Prerequisites
SCRUM-53's `UStatsComponent` modifiers with handles, the `MoveSpeed` stat (SCRUM-36/37), and the TileGen map/`FSpawnArena` used by the enemy spawner.

## 4. Architecture Overview
```text
ARewardTotemSpawner --(map built)--> spawns ARewardTotem x N on safe ground
ARewardTotem --(player in range, bar full)--> UTemporaryEffectComponent::FindOrAdd(player)
UTemporaryEffectComponent --duplicates--> UTemporaryEffect template --Apply--> target
   ticks Duration down, shows aura, Remove() on expiry / destroy
UStatModifierEffect --> UStatsComponent::AddModifier / RemoveModifier(handle)
AGrantMoneyPlayerCharacter (unchanged) --> MoveSpeed stat -> MaxWalkSpeed
```

## 5. AI-Assisted Planning
### AI Prompt
The Jira ticket text, pasted as-is (see Part C).
### Expected AI Output
A plan built on existing code, not new parallel systems.
### Human Review
The developer added a requirement mid-task: totems must populate the level at load, at random positions with no collision issues. The AI then reused the enemy spawner's `FSpawnArena` checks instead of writing new map logic.

## 6. Step-by-Step Implementation
### Step 1 – The effect contract
`UTemporaryEffect` is an `EditInlineNew` UObject with `Duration`, `StackKey`, aura settings and two overridable events, `ApplyEffect` and `RemoveEffect`. `Apply`/`Remove` guard against double application and double removal.
### Step 2 – The stat buff
`UStatModifierEffect` adds one modifier and keeps its handle; removal uses only that handle, so base values and other modifiers are never touched.
### Step 3 – The effect runner
`UTemporaryEffectComponent` duplicates the template, applies the copy, counts down, shows an aura (point light and glowing ring), and removes everything on expiry, on `EndPlay` and on destroy. It is added to the player on demand, so the player class is unchanged. It pauses with the game.
### Step 4 – The totem
`ARewardTotem` checks for the closest `ActivatorClass` pawn (default: the player character) within `ActivationRadius`, fills or drains the bar, and on a full bar applies every effect in `Effects`. The totem is not used up if nothing could be applied.
### Step 5 – Random, safe placement
`ARewardTotemSpawner` waits for the map, then tries shuffled playable cells: footprint on playable cells, ground trace at tile height, no pawn-blocking overlap in a box above the ground, distance from the player and from other totems.
### Step 6 – Game mode
`PrepareRun` adds a default spawner unless the level has one.

## 6A. Designer Setup (no Blueprints)
The project keeps gameplay in C++ (`CLAUDE.md`), so there are two ways to tune totems, neither using Blueprints:

**A. Change the C++ defaults** (every level). In `ARewardTotem::ARewardTotem()` (`RewardTotem.cpp`), edit the `SpeedBoost` lines (`Stat`, `Magnitude`, `Duration`), or set `CustomMesh`/`CustomMaterial` with a `ConstructorHelpers::FObjectFinder` the same way the pillar mesh is loaded. Spawner defaults (`TotemCount`, spacing) are in `RewardTotemSpawner.h`.

**B. Per level, in the Details panel.** Drag a **RewardTotemSpawner** (a C++ actor, not a Blueprint) from Place Actors into the level. The game mode then uses it instead of spawning its own. On it:
- **Totems → Look:** `Totem Mesh`, `Totem Material`, `Totem Mesh Scale`. The mesh's bottom is put on the ground. A custom mesh keeps its own materials (no glow); the range disc and bar still glow.
- **Totems → Reward → Effects Override:** add entries (e.g. *Stat Modifier Effect*) and set **Duration** (seconds), **Stat**, **Modifier Type** and **Magnitude** (0.5 = +50%). Empty = the C++ default (+50% move speed, 8 s).
- **Totems → Reward → Random Effect Pool:** each spawned totem gets one buff picked at random from this list (default: Swiftness +50% move speed, Might +50% damage, Frenzy 30% shorter attack interval, Piercing +2 penetration, all 8 s). With **Vary Buffs Across Totems** on, buffs are dealt like cards, so no buff repeats in a level until all have been used. **Color Totem By Buff** tints each totem with its buff's colour. List an entry twice to make it more common. Effects Override, when set, replaces the random pick.
- **Totems / Placement:** `Totem Count`, `Footprint Radius` (raise it for a wider mesh), `Min Spacing`, `Min Distance From Player`, `Random Seed`.

Note: option B saves the spawner into the level (`.umap`), which is a binary asset; coordinate with the team before committing map changes.

## 7. AI Prompts by Stage
See Part C.

## 8. Common Mistakes
### 8.1 Editing the base stat for a temporary buff
**Symptom:** speed stays wrong after an upgrade taken during the buff. **Cause:** saving and restoring a base value. **Fix:** use a modifier and remove it by handle. **Lesson:** temporary changes are layers, not overwrites.
### 8.2 Applying the template itself
**Symptom:** the second totem does nothing, or a buff never ends. **Cause:** the shared template object holds the "applied" state. **Fix:** the component duplicates the template for every activation.
### 8.3 Spawning on the map edge or inside props
**Cause:** picking any valid cell. **Fix:** footprint check, ground trace and a clearance overlap, the same checks the enemy spawner uses.

### 8.4 Test helper named like an engine class (real failure, first build)
**Symptom:** `error C2872: 'FScene': ambiguous symbol` in `RewardTotemSpawnerTests.cpp`, followed by knock-on errors (`C2079 uses undefined class`, `C2737 const object must be initialized`). **Cause:** the AI named a test struct `FScene` inside a namespace and used `using namespace`; Unreal already has a global renderer class `FScene` (declared as a friend in `Engine/Level.h`). **Fix:** renamed it `FSpawnScene`. **Lesson:** avoid generic `F`-names that the engine may already use; prefer qualified names over `using namespace` in test files.

### 8.5 Suggesting a Blueprint in a C++-only project
**Symptom:** the AI's first answer for "change the totem's assets" was a Blueprint child class. **Cause:** Blueprint subclassing is the usual Unreal answer; the AI did not apply the project rule in `CLAUDE.md`. **Fix:** expose the look and rewards as C++ properties, editable on a placed C++ spawner. **Lesson:** check project rules before suggesting an editor workflow.

*(Add further failures here as they happen.)*

## 9. Verification
### Performed
- Code reviewed by reading (no compiler available to the AI session).
- Build of `GrantMoneyEditor` and `python scripts\run_tests.py`: **145/145 passed** (developer reported, 2026-11-12).
- After the custom-look and random-buff changes: **150/150 passed**, including all 26 SCRUM-39 tests (developer's run; read from the exported Automation report).
### Pending (developer)
1. `python scripts\run_tests.py` (builds `GrantMoneyEditor`, runs `GrantMoney.*`). Record the result line.
2. PIE on PlayGame1: totems visible at load, none inside walls/props or off the map edge, none on top of the player.
3. Stand near a totem: bar appears and fills; step away: it drains.
4. Full bar: player is visibly faster and glows; after 8 s both end; speed is back to normal.
5. Spent totem goes dark and cannot be used again.

## 10. Known Limitations
- Environment props placed **without collision** (`bCollision` off in `MapEnvironmentPopulator`) are not detected, so a totem can overlap one visually.
- The bar is built from engine cubes (no UMG); it turns to face the camera by yaw only.
- No HUD timer for the buff yet (`OnEffectApplied`/`OnEffectEnded` are ready for one).
- `AGrantMoneyGameMode`'s new default class has no dedicated test.

## 11. Extension Ideas
Damage or attack-speed totems (just another `UStatModifierEffect` entry), heal-over-time effects, debuff totems, a HUD buff bar.

## 12. Source-Control Checkpoint
Branch `SCRUM-39-reward-totems`. Stage only the Source and docs files listed in Part A. Do not stage the unrelated `.uasset`/`.umap` changes already in the working tree.

---

# Part C – AI Workflow Log
1. **Prompt:** the SCRUM-39 ticket text, including the course capture requirement.
2. **AI inspection:** read `CLAUDE.md`, `StatsComponent`, `GrantMoneyStatTypes`, `StatBoostPickup` (+ tests), `PickupBase`, `GrantMoneyPlayerCharacter.h`, test helpers. Found that `MoveSpeed` already drives `MaxWalkSpeed`, so no movement change was needed.
3. **Mid-task correction by the developer:** "These totems would populate in the level when the level is loaded. It must be random location where there will be no issues with collisions." The AI then read `EnemySpawnPlacement`, `EnemySpawner`, `GrantMoneyGameMode` and reused `FSpawnArena`.
4. **First build (developer):** failed with `C2872 'FScene': ambiguous symbol` in `RewardTotemSpawnerTests.cpp` (see B8.4). The AI renamed the struct to `FSpawnScene`. All other new files compiled in that run.
5. **Second build/test (developer):** build succeeded, **145/145 Automation tests passed**.
6. **First PIE (developer reported):** 3 pillars spawn in different places each run; charging works; the range disc disappears once the buff is collected. The pillars looked white rather than green. The developer asked to swap the totem's assets and set the buff duration in the Details panel; the AI first suggested a Blueprint child and added `bTintTotemMesh` (the glow used to overwrite custom materials).
   **Correction by the developer:** the project uses C++ only, not Blueprints (`CLAUDE.md`: no Blueprint gameplay logic). The AI replaced the Blueprint route with C++ properties: `CustomMesh`, `CustomMaterial`, `CustomMeshScale` on `ARewardTotem`, and `TotemMesh`, `TotemMaterial`, `TotemMeshScale`, `EffectsOverride` on `ARewardTotemSpawner` (editable on a spawner placed in the level). Tests: `Totem.CustomMeshAndMaterial`, `TotemSpawner.LookAndRewardOverrides`.
7. **Developer request:** "What if I want the buffs per level to be random? ... I want each totem that spawns to be a random buff." The AI added `RandomEffectPool` (4 default C++ buffs), deck-style dealing (`bVaryBuffsAcrossTotems`), per-buff totem colour (`bColorTotemByBuff`), and tests `TotemSpawner.DefaultRandomPool`, `RandomBuffsPerLevel`, `FixedOverrideBeatsRandomPool`. It checked that the weapon reads Damage, AttackInterval and Penetration from stats on every shot, so the new buffs need no weapon changes.
8. **Third test run (developer):** 150/150 passed, including the new random-buff and custom-look tests.
9. **Manual verification:** to be done with the quiz-style yes/no checks (see `CLAUDE.md`); record only whether everything was confirmed.

---

# Part D – Video Voice-Over Script
## Opening
"In this video we add reward totems: stand next to one, fill its bar, and get a short speed buff."
## Acceptance Criteria
Read the criteria table from the Jira section.
## Engineering Principle
"A temporary buff is a layer on top of the stats, never a change to them. Remove the layer and you are exactly where you were."
## Step-by-Step Demonstration
1. Show `UTemporaryEffect` and `UStatModifierEffect`: apply adds a modifier, remove deletes it by handle.
2. Show the effect component: copy, timer, aura, clean-up.
3. Show the totem in PIE: bar fills, buff applies, glow, expiry.
4. Show the spawner: restart PIE twice to show different layouts; point out that none are in walls.
5. Run the tests.
## Common Mistakes
Part B section 8.
## Verification Checkpoint
Part B section 9.

---

# Media Capture List
| # | File name | What |
|---|---|---|
| 1 | `SCRUM-39-01-totems-at-load.png` | Overview of the map with totems placed |
| 2 | `SCRUM-39-02-bar-filling.mp4` | Player walks into range; bar fills |
| 3 | `SCRUM-39-03-buff-active.mp4` | Speed and glow while buffed, then expiry |
| 4 | `SCRUM-39-04-spent-totem.png` | Dark, spent totem |
| 5 | `SCRUM-39-05-two-layouts.mp4` | Two PIE runs, different random layouts |
| 6 | `SCRUM-39-06-tests.png` | `run_tests.py` result |
| 7 | `SCRUM-39-07-effects-details.png` | Totem Blueprint Details: Effects list edited inline |

Keep raw media out of Git.
