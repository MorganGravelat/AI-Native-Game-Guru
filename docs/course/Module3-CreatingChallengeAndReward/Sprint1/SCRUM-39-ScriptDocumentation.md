# SCRUM-39 – Environmental Reward Totems
## Module 3 Sprint 1 – Development and Recording Guide

This guide covers SCRUM-39: **reward totems** appear at random, safe places on the map when the level loads. While the player stands near one, a bar above it fills. When the bar is full, the player gets a buff. The buff runs through a new reusable **temporary effect contract**, so new reward types can be added without touching the player, its movement or the camera.

The work happened in **two passes**:
- **Pass 1** (branch `SCRUM-39-reward-totems`, merged as PR #37) built the effect system, the totem and the spawner. Totems gave a **temporary** buff (by default +50% move speed for 8 seconds).
- **Pass 2** (in the same Copilot chat as SCRUM-45, branch `SCRUM-45-HUD`, commit `e6b32ac`) completed the ticket: attack-speed reward checks, pause freezing, a documented refresh rule, HUD connection, and a readable charge bar. The developer then changed totems to give **small, permanent** buffs (by default +10% move speed) shown in the HUD without a countdown. The timed path is kept and tested for a future rare enemy drop. See [Pass 2](#pass-2--completing-scrum-39-permanent-totem-buffs) at the end of Part B.

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

**Pass 2 status:** the AI session ran the tests itself this time. The final run passed **192/192** (full suite), with `GrantMoney.Effects` 12/12 and `GrantMoney.Rewards` 21/21. The developer confirmed in PIE (yes/no quiz) that the bar reads as red on white, permanent buffs show in gold with no countdown, last through pause and level-up, and clear on restart, the smaller magnitudes feel reasonable, and timed buffs still work. Several other pass 2 checks are still pending (see [Pass 2 verification](#p26-verification)).

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

**Pass 2 ticket text (completing SCRUM-39):** one placeholder world totem that grants a temporary attack-speed buff, reusing the StatsComponent modifier handles and the existing pickup/effect code. A visible totem at a valid reachable location; on entering its radius apply the buff once, show feedback, and consume the totem for the run. Expose magnitude and duration. Track the modifier on the player so it survives the totem's removal and is removed precisely on expiry; permanent upgrades and other modifiers keep composing. Repeated activation refreshes duration by a documented rule. Freeze duration during pause and upgrade selection, clear on run reset, connect active state and remaining time to the HUD. Keep the effect definition reusable for future temporary reward pickups.

| Pass 2 criterion | Met by | Evidence |
|---|---|---|
| Attack-speed buff, reusing modifier handles and effects | Already in pass 1: **Frenzy** in the random pool (`AttackInterval`, Multiplicative) through `UStatModifierEffect` | `Totem.TimedAttackSpeedBuffCombinesWithPermanentUpgradeThenExpires` (passed) |
| Composes with permanent attack-speed upgrades | Separate modifier handles in `UStatsComponent`; removal by handle leaves the upgrade | Same test, plus `Totem.PermanentFrenzyRewardCombinesWithPermanentUpgradeAndNeverExpires` (passed); PIE pending |
| Survives totem removal, removed precisely | Modifier is held by `UTemporaryEffectComponent` on the player, not by the totem | Pass 1 tests (`ExpiryNeverCorruptsBaseStats`) |
| Documented refresh rule | Same `StackKey`, timed: remaining time becomes `max(remaining, Duration)` (restarts, never shortens). Same `StackKey`, permanent: no-op. Different keys stack. | `StackKeyRefreshesInsteadOfStacking`, `PermanentStackKeyRepeatIsANoOp` (passed) |
| Freeze in pause and upgrade selection | Effect timers and totem charge only advance when the world isn't paused (level-up also pauses) | `TemporaryEffectComponent.PauseFreezesTheDuration`, `Totem.PauseFreezesCharge` (passed); PIE pending |
| Clear on run reset | Restart reloads the level; the player and its effect component are destroyed | Developer reported (permanent buffs gone after restart) |
| Active state and remaining time in the HUD | SCRUM-45 HUD BUFFS panel reads `OnEffectApplied`/`OnEffectEnded`; timed rows green with a countdown, permanent rows gold; a toast names the stat change | `UI.GameplayHud.ListsBuffsWithTimeAndUpgradesTaken` (passed); developer reported the rows |
| (Developer change) Small, permanent totem buffs | `UTemporaryEffect::bPermanent`; totem defaults and pool at +10% / −10% / +1 | `PermanentEffectNeverExpiresAndNeedsNoTick`, `Totem.FullBarGivesAPermanentBuffThatNeverExpires` (passed); developer reported |
| (Developer request) Readable charge bar | Red fill on a white background, on an opaque material | Developer reported |

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

## Files Changed in Pass 2
| File | Change |
|---|---|
| `Public/TemporaryEffect.h` | New `bPermanent` flag (`Duration` is greyed out when it is set). |
| `Public/TemporaryEffectComponent.h`, `Private/TemporaryEffectComponent.cpp` | Permanent effects are never counted down, report −1 remaining, don't keep the component ticking, and a repeat is a no-op. Aura ring tint uses the material's real `Color` parameter. |
| `Public/RewardTotem.h`, `Private/RewardTotem.cpp` | Default reward is permanent +10% move speed. Glow tint uses `Color` instead of `EmissiveColor`. New `BarMaterial` (`BasicShapeMaterial`, opaque) and `MakeBarSegment`; `BarColor` red, new editable `BarBackgroundColor` white. |
| `Private/RewardTotemSpawner.cpp` | Random pool is small and permanent: Swiftness +10% move speed, Might +10% damage, Frenzy 10% shorter attack interval, Piercing +1 penetration. |
| `Private/TemporaryEffectComponentTests.cpp` | New `PauseFreezesTheDuration`, `PermanentEffectNeverExpiresAndNeedsNoTick`, `PermanentStackKeyRepeatIsANoOp`. |
| `Private/RewardTotemTests.cpp`, `Private/RewardTotemSpawnerTests.cpp`, `Private/Tests/TemporaryEffectTestScene.h` | New `PauseFreezesCharge`, the two attack-speed composition tests, `FullBarGivesAPermanentBuffThatNeverExpires`; the old full-bar test now explicitly tests the timed path; magnitudes updated. |
| `Private/BossDebugCommands.cpp` | `GrantMoney.GiveFrenzy` (permanent −10% attack interval) and `GrantMoney.GiveTotemBuff [Swiftness\|Might\|Frenzy\|Piercing]`. |
| HUD files | See `SCRUM-45-ScriptDocumentation.md`. |

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
- **Totems → Reward → Effects Override:** add entries (e.g. *Stat Modifier Effect*) and set **Permanent** or a **Duration** (seconds), **Stat**, **Modifier Type** and **Magnitude** (0.1 = +10%). Empty = the C++ default (permanent +10% move speed; pass 1 was +50% for 8 s).
- **Totems → Reward → Random Effect Pool:** each spawned totem gets one buff picked at random from this list (default after pass 2: Swiftness +10% move speed, Might +10% damage, Frenzy 10% shorter attack interval, Piercing +1 penetration, all permanent). With **Vary Buffs Across Totems** on, buffs are dealt like cards, so no buff repeats in a level until all have been used. **Color Totem By Buff** tints each totem with its buff's colour. List an entry twice to make it more common. Effects Override, when set, replaces the random pick.
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

### 8.6 Wrong material parameter name (pass 2, real bug)
**Symptom:** totem pillars, the range disc, the bar and the buff aura never showed their tint. **Cause:** the code set `EmissiveColor`, but the engine's `EmissiveMeshMaterial` and `BasicShapeMaterial` expose their colour as `Color`. A dynamic material instance silently ignores an unknown parameter. **Fix:** use `Color` everywhere in the totem and aura code. Found by a throwaway headless diagnostic test (`ZZDiagTest`, deleted afterwards) that listed the material's real parameters. **Lesson:** check a material's parameter names before setting them.

### 8.7 An additive material can't make a solid bar (pass 2, real bug)
**Symptom:** after the 8.6 fix the bar was still a white blob. **Cause:** `EmissiveMeshMaterial` is **Additive** blend: it adds light on top of whatever is behind it and never occludes, so the two bar segments washed out together. **Fix:** the bar's two meshes now use the opaque `BasicShapeMaterial` (red fill on white); the pillar and disc keep the glow. Found with a second diagnostic test (`ZZDiagTest2`, deleted). **Lesson:** a colour bug can be a blend-mode bug. Confirm the data is right before changing more code.

### 8.8 Building a second buff system
The SCRUM-45 HUD work built a `UTemporaryBuffComponent` without noticing this ticket's `UTemporaryEffectComponent`. Pass 2 found it, deleted the duplicate, and moved the HUD to the effect system (see `SCRUM-45-ScriptDocumentation.md` §6.1).

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

### Pass 2 verification
See [Pass 2 verification](#p26-verification) at the end of Part B.

## 10. Known Limitations
- Environment props placed **without collision** (`bCollision` off in `MapEnvironmentPopulator`) are not detected, so a totem can overlap one visually.
- The bar is built from engine cubes (no UMG); it turns to face the camera by yaw only.
- ~~No HUD timer for the buff yet.~~ Done in pass 2 through the SCRUM-45 HUD.
- After pass 2, totems give permanent buffs, so nothing in normal play uses the timed path. The developer has reserved it for a future rare enemy drop.
- `bPermanent` effects are removed only by an explicit call or by destroying the player (a restart). There is no "end of level" clean-up beyond that.
- `AGrantMoneyGameMode`'s new default class has no dedicated test.

## 11. Extension Ideas
Damage or attack-speed totems (just another `UStatModifierEffect` entry), heal-over-time effects, debuff totems, a HUD buff bar.

## 12. Source-Control Checkpoint
Pass 1: branch `SCRUM-39-reward-totems`, merged as PR #37. Pass 2: committed on `SCRUM-45-HUD` together with the HUD (`e6b32ac` "Finished hud and tweaks on Totems"), so it will arrive in `main` with the SCRUM-45 PR.

## Pass 2 – Completing SCRUM-39, permanent totem buffs

### P2.1 Inspect what exists
The developer combined the SCRUM-39 completion brief with two requests: make the totem's "pure white" loading bar readable (a red fill over the white background) and add a HUD pop-up naming the buffed stat. The AI searched for `Totem`, read the totem, spawner and effect files and their tests, and found that the attack-speed reward (**Frenzy**), modifier handles, single use and pause handling already existed from pass 1. The real gaps were the HUD connection, explicit pause and composition tests, and the bar.

### P2.2 Fix the bar (two causes)
1. Wrong parameter name (§8.6). The developer reported the bar was still white after this fix.
2. Additive blend (§8.7). Switched the bar to an opaque material.
Both causes were found with throwaway headless diagnostic tests that only printed material data and were deleted afterwards. They read data and didn't simulate gameplay, which stays within the testing rules. The developer was asked to close the Editor for each rebuild.

### P2.3 Connect the HUD
The duplicate `UTemporaryBuffComponent` from SCRUM-45 was deleted, and the HUD's BUFFS panel and buff toast now read `UTemporaryEffectComponent`. Details are in `SCRUM-45-ScriptDocumentation.md`.

### P2.4 New tests for the ticket's rules
Pause freezes effect durations and totem charge. A timed attack-speed buff composes with a permanent Attack Speed upgrade and expires back to the upgrade's level, not to base.

### P2.5 Developer change: small, permanent buffs
"Make the reward totems give smaller buffs but make the permanent... instead of counting it down it just stays there as a permanent random buff." The AI added `bPermanent` to the effect contract rather than a new system, cut the magnitudes to +10% / +10% / −10% / +1, made a repeat of the same permanent buff a no-op, and drew permanent buffs in gold without a countdown. The timed path is unchanged and still covered by tests. In the final quiz's notes, the developer said the timed system is for a future rare enemy drop.

<a id="p26-verification"></a>
### P2.6 Verification

| Test | Result | Kind |
|---|---|---|
| `GrantMoney.UI`, `GrantMoney.Effects`, `GrantMoney.Rewards` after the HUD connection | Passed | Automation test |
| Full suite after the first bar fix | 188/188 | Automation test |
| `GrantMoney.Rewards`, then full suite, after the opaque bar | Passed; 188/188 | Automation test |
| After permanent buffs: `GrantMoney.Effects` / `GrantMoney.Rewards` / `GrantMoney.UI` / full suite | 12/12, 21/21, passed, **192/192** | Automation test |
| Final PIE quiz | Confirmed: red fill on a white bar; permanent buffs as gold rows with no countdown that never expire; they last through pause and level-up and are gone after restart; the smaller magnitudes feel reasonable; `GrantMoney.GiveBuff` still shows a green countdown and expires | Developer reported |

The AI ran the full suite three times in this pass because it said the changes touched several shared files. `CLAUDE.md` asks for targeted runs first.

**Pending (developer, in PIE).** These were asked in an earlier quiz round whose answers aren't in the transcript (apart from the bar still being white), and weren't asked again:
- With a permanent Attack Speed upgrade, Frenzy visibly speeds attacks up further.
- Standing in a totem's radius while paused or choosing a level-up freezes its charge bar.
- The totem is clearly visible and reachable, shows feedback when used, and is spent afterwards.
- The buff toast appears, names the stat, and fades.
- The pillar and range disc still look right after the bar material change. This was asked in a quiz that failed to send.

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

**Pass 2 (same Copilot chat as SCRUM-45; the AI ran the tests itself):**

10. **Prompt:** the SCRUM-39 completion brief, plus "see if you can alter it to have a red loading bar over the white background. Plus something in the hud to maybe pop up and say what stat was buffed."
11. **AI inspection:** found the existing totem and effect system and the Frenzy reward, then deleted its own duplicate buff component from SCRUM-45.
12. **Diagnostic test 1** (Editor had to be closed): the colour parameter is `Color`, not `EmissiveColor`. Fixed in the totem and the aura. HUD connected, toast added, pause and composition tests added. Full suite 188/188.
13. **Quiz:** the bar was still white. The other answers aren't in the transcript.
14. **Diagnostic test 2:** the glow material is Additive. The bar was moved to an opaque material. Full suite 188/188. A follow-up quiz failed to send.
15. **Developer change:** small, permanent totem buffs. The AI added `bPermanent`, smaller magnitudes and gold HUD rows, plus `GrantMoney.GiveTotemBuff`. The first build failed (exit code 2) while the Editor was open; after the developer closed it, Effects 12/12, Rewards 21/21, UI passed, full 192/192.
16. **Final quiz:** all confirmed. Developer note: keep the timed system for a future rare enemy drop.

---

# Part D – Video Voice-Over Script
## Opening
"In this video we add reward totems: stand next to one, fill its bar, and get a buff. First as a short timed boost, then, after play-testing, as a small permanent one that keeps adding up over the run."
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
6. Pass 2: show the bar before and after (white blob, then red on white) and explain the two causes: the wrong parameter name and the Additive blend.
7. Pass 2: take a totem, show the gold BUFFS row and the toast, then `GrantMoney.GiveBuff 15` for a green countdown row next to it.
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
| 7 | `SCRUM-39-07-effects-details.png` | Placed RewardTotemSpawner's Details panel: Effects Override / Random Effect Pool edited inline (C++ actor, not a Blueprint) |
| 8 | `SCRUM-39-08-bar-before-after.png` | Charge bar as a white blob (pass 1) and as red on white (pass 2) |
| 9 | `SCRUM-39-09-material-diag.png` | Diagnostic test output listing the material's real parameters and its Additive blend mode |
| 10 | `SCRUM-39-10-permanent-buff-hud.mp4` | Totem used, gold BUFFS row and toast; then a timed `GiveBuff` row counting down |
| 11 | `SCRUM-39-11-frenzy-with-upgrade.mp4` | Attack Speed upgrade, then Frenzy: attacks speed up further (pending check) |

Keep raw media out of Git.
