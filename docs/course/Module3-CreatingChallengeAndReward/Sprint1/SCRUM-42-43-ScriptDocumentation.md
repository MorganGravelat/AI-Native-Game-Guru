# SCRUM-42 / SCRUM-43 – Boss Arrival and Victory
## Module 3 Sprint 1 – Development and Recording Guide (combined milestone)

This guide covers the rest of **SCRUM-42** (the boss phase that the run controller already entered but
never filled) and **SCRUM-43** (a single prototype slime boss, its win condition and a Victory screen).
Together they close the single-level run: survive the timer → the boss arrives → kill it and win, or die
and lose → restart from either result.

- **Part A:** change report
- **Part B:** developer implementation guide
- **Part C:** AI workflow log
- **Part D:** video voice-over script
- **Media Capture List:** every screenshot and clip to record, with file names

**Status:** the code and tests are written and committed on branch `SCRUM-42-43-Boss-Victory`
(commit `caa540d`, merged to `main` as PR #36). **All 23 `GrantMoney.Boss` Automation tests pass** on
the final build, and the 5 `GrantMoney.Combat.ContactDamageComponent` tests pass. `GrantMoney.Run` (26)
and `GrantMoney.Combat.EnemyBase` (7) passed after the shared-code changes earlier in the session but were
not re-run after the later rounds. The full suite was not run. The developer play-tested in PIE over
several rounds of yes/no quiz questions; most of the fight was confirmed, but **the Victory heading and
Restart-from-Victory were not confirmed in the session** (see §9, Pending).

**What is and is not evidence in this document:**

| Kind | Meaning |
|---|---|
| Read from source | Taken from reading the project's committed source or git history. Not a test. |
| Compiled | UnrealBuildTool finished with `Result: Succeeded`. Says nothing about behaviour. |
| Automation test | A `GrantMoney.*` Automation test passed in a headless editor run, in a synthetic game world. |
| Developer reported | The developer played it in the Editor and answered the AI's yes/no quiz, or said so in chat. |

---

## Jira

| Key | Goal |
|---|---|
| SCRUM-42 (remainder) | When the run enters BossPhase, spawn exactly one boss at a validated safe location using the spawner's location validation. Keep existing normal slimes alive and stop new normal spawns as the initial, configurable policy. Handle spawn failure with a bounded retry and a visible diagnostic so the run cannot sit silently in an empty boss phase. Prevent late spawn callbacks or death events from changing a finished run. |
| SCRUM-43 | One prototype slime boss that reuses the shared enemy health, damage, targeting, death and movement systems, with its own configurable class and a recognizable placeholder look. One distinct behaviour: a clearly telegraphed lunge with editable windup, speed, duration and cooldown, contact damage at a controlled cadence, and movement bounds so the lunge stays in the playable area. Show a Boss Arrived message and boss health bar. Boss death moves the run to Won once and opens a Victory screen using the same result/restart flow as Game Over. |

**Done when, and where each one is met:**

| Criterion | Met by | Evidence |
|---|---|---|
| Reuse shared health, damage, targeting, death, movement | `ABossSlimeEnemy` derives from `ASlimeEnemy`/`AEnemyBase`; only a `GetDesiredMoveSpeed()` virtual hook was added to `AEnemyBase` so the lunge can override speed without copying pursuit code | `Boss.Slime.DefaultsAreIndependentAndBigger`; developer reported targeting/damage |
| Independently configurable class, recognizable placeholder | Own class with `BossMaxHealth` (3000), contact damage 20, attack interval 1.5 s, mesh scaled 1.8×, emissive purple body | `Boss.Slime.DefaultsAreIndependentAndBigger`; developer reported it glows (see §8 on colour) |
| Telegraphed lunge, editable windup/speed/duration/cooldown | `FBossLungeState`/`FBossLungeSettings` (pure state machine: Idle → Windup → Lunging → Cooldown) exposed as `Lunge*` properties on the boss; telegraph = stops, tints toward red, swells, red line on the ground | `Boss.Lunge.PhasesAndCooldown`, `Boss.Lunge.TimingIsEditable`, `Boss.Slime.LungeTelegraphChargeCooldown`; developer reported |
| Contact damage at a controlled cadence | Reuses `UContactDamageComponent` with the boss's `AttackInterval` stat (1.5 s); new `bLogHits` writes each landed hit to the Output Log so the cadence can be read | `Combat.ContactDamageComponent.LogHitsIsOptional`; developer reported the hit log |
| Lunge stays inside the playable area | `FBossLungeMath::ComputeSafeDistance`/`ComputeSafeDuration` shorten each dash against the map's `FSpawnArena` plus the boss's radius and `LungeBoundsPadding` | `Boss.Lunge.StaysInsidePlayableArea`, `Boss.Slime.LungeStaysInsidePlayableArea`; developer reported |
| Exactly one boss at a validated safe location | `ABossEncounter` calls the new `AEnemySpawner::FindSpawnLocationForClass` (same arena, clearance and ground checks, sized for the boss's larger capsule) | `Boss.Encounter.SpawnsExactlyOneBossAtValidLocation`; developer reported |
| Keep slimes, stop new spawns (configurable) | `ARunController::bAllowNormalSpawnsInBossPhase` (default false) folded into `IsNormalSpawningAllowed()` | `Boss.Encounter.KeepsSlimesAndStopsNormalSpawns`; developer reported |
| Boss Arrived message and boss health bar | `ARunHUD::DrawBossUi()` (canvas-drawn like the rest of the HUD, so no widget asset was needed) | Developer reported |
| Boss death → Won once → Victory screen, same flow as Game Over | `ABossEncounter` calls `RequestWin()` once; `AGrantMoneyGameMode` shows `ShowVictory`, which shares `ShowRunResult` with `ShowGameOver` | `Boss.Encounter.BossDeathWinsExactlyOnce`; **Victory heading and Restart pending** (§9) |
| Bounded retry with a visible diagnostic | `MaxSpawnAttempts` (5) every `SpawnRetryIntervalSeconds` (1 s); after that the encounter is `Failed` and the HUD shows `GetSpawnDiagnostic()` | `Boss.Encounter.SpawnFailureRetriesBoundedWithDiagnostic`, `Boss.Encounter.RetrySucceedsAfterFailure` |
| Late spawns / deaths cannot change a finished run | Encounter goes `Cancelled` when the run ends without a boss kill and ignores later callbacks; `RequestWin`/`RequestLoss` were already single-use | `Boss.Encounter.LateSpawnCannotChangeTerminalRun`, `Boss.Encounter.PlayerDeathDuringFightEndsRunAsLost` |

**Related tickets:** SCRUM-31/30/38/42 (run controller, spawner and its "What Remains for Boss
Integration" list, which this milestone works through), SCRUM-44 (Game Over screen, reused here for
Victory), SCRUM-33 (health/damage), SCRUM-32 (weapon auto-targeting).

---

# Part A – Change Report

## Milestone Summary

The run now has an ending. When the survival timer hits zero, a boss encounter places one large,
glowing purple slime at a validated spot near the player, announces it, and shows its health bar. The
boss chases like any slime, but when in range it winds up, shows where it will go, and charges in a
combo of dashes that re-aim at the player after each one and lead a moving player. Every dash is cut
short at the map's edge. Killing it wins the run and opens the Game Over screen retitled as Victory;
dying during the fight still loses. A few console commands make the fight quick to reach and tune
during a demo.

## Files Created

| File | Purpose |
|---|---|
| `Public/BossLunge.h` / `Private/BossLunge.cpp` | `EBossLungePhase`, `FBossLungeSettings`, `FBossLungeState` (the pure lunge state machine, including combo dashes) and `FBossLungeMath` (safe distance/duration inside the arena, and the intercept/lead direction). No actor code, so it is tested directly. |
| `Public/BossSlimeEnemy.h` / `Private/BossSlimeEnemy.cpp` | `ABossSlimeEnemy`: boss stats, look, telegraph (tint, swell, ground line), lunge driving, lead strength, and a debug max-health override. |
| `Public/BossEncounter.h` / `Private/BossEncounter.cpp` | `ABossEncounter`: binds to the run, spawns one boss in BossPhase with bounded retry, reports the diagnostic, wins on boss death, cancels on a finished run. States: Waiting, Spawning, Fighting, Defeated, Failed, Cancelled. |
| `Private/BossDebugCommands.cpp` | Console commands `GrantMoney.SkipToBoss`, `GrantMoney.BossHealth <n>`, `GrantMoney.PlayerHealth <n>`. |
| `Private/BossLungeTests.cpp` | 6 tests for the pure lunge logic. |
| `Private/BossSlimeEnemyTests.cpp` | 9 tests for the boss actor. |
| `Private/BossEncounterTests.cpp` | 8 tests for the encounter and run-state guards. |
| `Private/Tests/BossTestTypes.h` | Shared test helpers, including a boss test double with a deterministic fake target velocity. |

## Files Modified

| File | Change |
|---|---|
| `Public/EnemyBase.h` / `.cpp` | New `virtual float GetDesiredMoveSpeed() const` so a subclass can override speed (the lunge returns its charge speed). |
| `Public/EnemySpawner.h` / `.cpp` | New `FindSpawnLocationForClass(Class, MinDistance, MaxDistance, OutCentre)`: the existing validated search for any enemy class and distance ring. Does not check run state or the enemy cap. |
| `Public/RunController.h` | New `bAllowNormalSpawnsInBossPhase` (default false), used by `IsNormalSpawningAllowed()`. Doc comments updated: BossPhase now waits on `ABossEncounter` and the player's death. |
| `Public/ContactDamageComponent.h` / `.cpp` | New `bLogHits` (default false; the boss turns it on). |
| `Private/ContactDamageComponentTests.cpp` | New `LogHitsIsOptional` test. |
| `Public/GameOverWidget.h` / `.cpp` | `SetResults` takes an optional title. Optional `ResultTitleText` (`BindWidgetOptional`); without it, the widget retitles whichever TextBlock reads "Game Over", and warns if there is none. |
| `Public/MapCameraPlayerController.h` / `.cpp` | `ShowVictory`, sharing a new `ShowRunResult` with `ShowGameOver`; optional `VictoryWidgetClass`. |
| `Public/GrantMoneyGameMode.h` / `.cpp` | `BossEncounterClass` (defaults to `ABossEncounter`; spawned only if the level has none; null disables the boss). Shows Victory on `Won`. |
| `Public/RunHUD.h` / `.cpp` | `DrawBossUi()`: "BOSS ARRIVED", boss health bar, spawn-failure diagnostic, plus a player HP readout. |
| `Content/Maps/PlayGame1.umap` | Changed in the same commit (binary; not inspected). |
| `CLAUDE.md` | New "Verification Scope (mandatory)" section, writing the testing philosophy used in this session into the project rules. |

---

# Part B – Developer Implementation Guide

## 1. Prerequisites

- Unreal Engine 5.8, Editor **closed** when running `scripts/run_tests.py` (Live Coding breaks the build).
- The SCRUM-31/30/38/42 run controller and spawner, SCRUM-33 health/damage, SCRUM-44 Game Over screen.

## 2. Architecture Overview

```
ARunController  Active ──timer 0──▶ BossPhase ──RequestWin──▶ Won
                                        │      └─RequestLoss─▶ Lost  (player death)
                                        │
          IsNormalSpawningAllowed(): Active, or BossPhase && bAllowNormalSpawnsInBossPhase (default off)
                                        │
                                        ▼
ABossEncounter (created by AGrantMoneyGameMode unless the level has one)
   Waiting ─BossPhase─▶ Spawning ──AEnemySpawner::FindSpawnLocationForClass ok──▶ Fighting
                           │  fail → retry every 1 s, up to 5 attempts → Failed (HUD diagnostic)
   Fighting ─boss OnDeath─▶ Defeated → RequestWin() (once)
   run ends any other way ─▶ Cancelled (later spawns / deaths ignored)

ABossSlimeEnemy : ASlimeEnemy : AEnemyBase   (shared health, stats, targeting, death, pursuit)
   Tick → FBossLungeState:  Idle ─in range─▶ Windup (tint red, swell, ground line)
                                            ▶ Lunging (GetDesiredMoveSpeed = LungeSpeed,
                                                       distance cut by FBossLungeMath::ComputeSafeDistance)
                                            ▶ next combo dash (short windup, re-aim + lead) … ▶ Cooldown ▶ Idle
   Aim = Lerp(straight at player, FBossLungeMath::ComputeInterceptDirection, LungeLeadStrength)
   UContactDamageComponent (AttackInterval 1.5 s, bLogHits on)

AGrantMoneyGameMode on Won  → AMapCameraPlayerController::ShowVictory  ┐ ShowRunResult
                    on Lost → AMapCameraPlayerController::ShowGameOver ┘ (same widget, Restart/Quit)
ARunHUD::DrawBossUi → "BOSS ARRIVED", boss health bar, spawn diagnostic, player HP
```

## 3. AI-Assisted Planning

### AI Prompt
The developer's testing and verification rules (AI runs only targeted headless Automation tests for what
it changed; all PIE, visual and Blueprint work is the developer's, confirmed through short yes/no
quizzes), then: "continue the modules... do not run tests for past modules unless something goes wrong",
followed by the SCRUM-43 / SCRUM-42 brief verbatim.

### Expected AI Output
Inspection of the run controller, enemy base, slime, spawner, game mode, Game Over widget, HUD,
player controller, contact damage and existing tests before writing anything; then pure lunge logic
first, the boss, the encounter, integration, tests, and a quiz for the developer.

### Human Review
- Play the fight in PIE and answer each quiz round (done over three rounds plus two tuning checks).
- Decide what stays as a later ticket: the glow hiding the tint, and live tuning through a settings menu.
- Tune the combo: the committed defaults are `LungeComboCount = 6` and `LungeComboWindupSeconds = 0.25`,
  where the AI had set 3 and 0.5 (read from source; the AI's last message refers to "your windup tweak").

## 4. Step-by-Step Implementation

### Step 1 – Inspect before building
The AI read `CLAUDE.md`, then `RunController`, `EnemyBase`, `SlimeEnemy`, `EnemySpawner`,
`EnemySpawnPlacement`, `GrantMoneyGameMode`, `GameOverWidget`, `RunHUD`, `MapCameraPlayerController`,
`HealthComponent`, `EnemyEmergenceComponent`, `ContactDamageComponent` and the existing test helpers
(`RunTestListener.h`, `CombatTestListener.h`). This settled what could be reused instead of rebuilt: the
run's single-use `RequestWin`/`RequestLoss`, the spawner's placement checks, emergence, contact damage
and the SCRUM-44 Game Over flow.

### Step 2 – Pure lunge logic first
`BossLunge.h/.cpp` holds the state machine and the maths with no actor in it, so windup, duration,
cooldown and the bounds clamp can be tested with plain numbers.

### Step 3 – A speed hook instead of copied pursuit
One virtual, `AEnemyBase::GetDesiredMoveSpeed()`, lets the boss return its charge speed while the shared
pursuit code keeps doing the moving.

### Step 4 – The boss
`ABossSlimeEnemy`: bigger (1.8×), more health (3000), harder hits (20 every 1.5 s), purple emissive body,
and the telegraph (tint toward `TelegraphColor`, swell by 15 %, red ground line along the planned dash).

### Step 5 – Reuse the spawner's validation for a bigger capsule
`AEnemySpawner::FindSpawnLocationForClass` exposes the same arena, clearance and ground checks for any
class and distance ring, so the boss's larger capsule is validated with the same rules as a slime.

### Step 6 – The spawn policy
`bAllowNormalSpawnsInBossPhase` (default off) on the run controller: existing slimes stay, new ones stop.

### Step 7 – The encounter
`ABossEncounter` owns the boss's lifecycle: spawn with bounded retry, diagnostic on failure, win once on
the boss's death, and `Cancelled` if the run ends first so a late spawn or death cannot reopen it.

### Step 8 – HUD and Victory
Boss UI is drawn on the canvas HUD. Victory reuses the Game Over widget and flow with a "Victory!" title.

### Step 9 – Tests (round 1)
`python scripts/run_tests.py --filter GrantMoney.Boss`: 17/17. Because shared code changed, also
`--skip-build --filter GrantMoney.Run` (26) and `--filter GrantMoney.Combat.EnemyBase` (7): both passed.

### Step 10 – Developer quiz round 1, then fixes
After the first PIE round the AI added: the multi-dash combo that re-aims after each dash (existing tests
pinned to 1 dash, new tests for the combo, retargeting and early abort), the console commands, a player
HP line on the HUD, and the debug health override. The developer's round-1 answers are not in the
transcript; the next round's questions show colour and the Victory heading were the open issues.

### Step 11 – Colour and the Victory heading
- **Colour:** the boss's tint was not showing (round 2 asked whether a "no material with a Color
  parameter" warning appeared). The fix loads the engine's `EmissiveMeshMaterial` and drives its
  `EmissiveColor` parameter through a dynamic material instance, after checking how
  `TileMapRenderer.cpp` already loads materials.
- **Heading:** an optional `ResultTitleText` was added for the "Victory!" title. Round 3 showed
  `WBP_GameOver` has no TextBlock by that name, so the widget now falls back to retitling the TextBlock
  that reads "Game Over" (§6.3).
- `bLogHits` was added so the hit cadence could be read in the Output Log. Contact damage tests 5/5;
  boss tests 21/21 after the round-3 fix.

### Step 12 – Lead aim
At the developer's request, each dash leads a moving player: `FBossLungeMath::ComputeInterceptDirection`
solves the standard intercept quadratic for where the player will be when the dash launches (after the
windup), falling back to straight aim if the player is still or can't be caught. First built as an on/off
`bLungeLeadsTarget`, then replaced with `LungeLeadStrength` (0 = straight, 1 = full intercept, default 0.6)
after the developer's quiz answers. Boss tests 23/23.

## 5. AI Prompts by Stage

| Stage | Prompt |
|---|---|
| Rules | "The coding AI only runs headless Unreal Automation tests... only for the logic it actually wrote or changed... never does headless PIE runs... Creating or editing Blueprint widgets is always the developer's step." |
| Request | "SCRUM-43 and the remainder of SCRUM-42 Boss arrival and Victory. Complete the single-level run with one prototype slime boss..." (full brief) |
| Combo length | "The current lunge attacks work for now, would I be able to up the max dashes in the code? Or is that set" |
| Lead aim | "Is there a way you can make it so the dashes lead a bit when the player is moving? Like they try and intercept based off the player speed and direction?" |
| Wrap-up | "We will iterate more after I present tomorrow... a basic boss battle mechanic to show off in our Vertical Slice." |

## 6. Common Mistakes

### 6.1 The boss's colour didn't show
- **Symptom:** in the first PIE round the boss did not show its purple body colour.
- **Likely cause:** the colour was being set on a material parameter the placeholder mesh's material
  did not have (the round-2 quiz asked about exactly that warning; the answer isn't in the transcript).
- **Fix:** use the engine's `EmissiveMeshMaterial` and set `EmissiveColor` on a dynamic instance; log a
  warning naming the missing parameter if it ever happens again.
- *Along the way the AI read the bytes of an engine `.uasset` (`BasicShapeMaterial`) as text to look for
  parameter names. `CLAUDE.md` says never to interpret binary Unreal assets as text; the working fix came
  from reading existing C++ (`TileMapRenderer.cpp`) instead.*

### 6.2 The glow overpowers the colour
- **Symptom:** the developer reported the boss glows, but the glow overpowers the body colour, so the red
  windup tint is hard to see.
- **Status:** deliberately deferred by the developer ("don't worry about it right now"). See §8.

### 6.3 The Victory title had nowhere to go
- **Symptom:** an Output Log warning that `WBP_GameOver` has no TextBlock named `ResultTitleText`.
- **Cause:** the widget asset was built for SCRUM-44 with a fixed "Game Over" heading.
- **Fix in code:** fall back to retitling the TextBlock that reads "Game Over". If that doesn't work, the
  developer's Editor step is to rename the heading TextBlock in `WBP_GameOver` to `ResultTitleText` with
  **Is Variable** ticked. Not confirmed in PIE (§9).

### 6.4 A shadowed local variable broke the build
- **Symptom:** the lead-aim build failed (exit code 2) on a local that shadowed a member.
- **Fix:** renamed the local. Caught by the build in `run_tests.py` before any test ran.

### 6.5 Changing a property's type means changing its tests
- Replacing `bLungeLeadsTarget` with `LungeLeadStrength` needed the tests updated to the blend-based
  property (`Boss.Slime.LungeLeadStrengthBlendsTowardFullLead`).

## 6A. Designer Setup

- **Defaults with no setup:** the game mode creates an `ABossEncounter` with an `ABossSlimeEnemy`.
- **Per-level tuning:** place a `BossEncounter` (or a Blueprint child) and point **Boss Class** at a
  Blueprint child of `ABossSlimeEnemy`; set `BossMaxHealth`, `Lunge*`, `LungeLeadStrength` there.
- **Keep spawning during the fight:** tick `bAllowNormalSpawnsInBossPhase` on the run controller.
- **Separate Victory widget:** set `VictoryWidgetClass` to a child of `UGameOverWidget` with the same
  widget names; otherwise `WBP_GameOver` is reused.
- **Demo/testing:** `GrantMoney.SkipToBoss`, `GrantMoney.BossHealth <n>`, `GrantMoney.PlayerHealth <n>`,
  and the existing `GrantMoney.DamagePlayer`.

Current boss defaults (read from source):

| Property | Default |
|---|---|
| `BossMaxHealth` | 3000 |
| Contact damage / attack interval | 20 / 1.5 s |
| `LungeWindupSeconds` | 1.0 |
| `LungeSpeed` | 1400 cm/s |
| `LungeDurationSeconds` | 0.6 |
| `LungeCooldownSeconds` | 4.0 |
| `LungeComboCount` | 6 (developer-tuned; AI set 3) |
| `LungeComboWindupSeconds` | 0.25 (developer-tuned; AI set 0.5) |
| `LungeTriggerRange` / `MinLungeDistance` | 700 / 150 cm |
| `LungeBoundsPadding` | 10 cm |
| `LungeLeadStrength` | 0.6 |
| Encounter spawn ring | 700–1200 cm |
| `MaxSpawnAttempts` / `SpawnRetryIntervalSeconds` | 5 / 1 s |
| `ArrivalMessageSeconds` | 4 |

## 7. AI Workflow Notes

The AI asked for manual verification through short yes/no quiz batches rather than free-form questions,
as `CLAUDE.md` requires. Individual answers are not transcribed here.

## 8. Known Limitations

- **The glow hides the windup tint.** Telegraphing currently relies more on the stop, the swell and the
  red ground line than on the colour change.
- **The ground line is a debug draw** (`DrawDebugLine`), fine for a prototype but not a shipping visual.
- **No live tuning.** Health, combo and lead changes need a Blueprint child plus a placed encounter, or a
  C++ change. The developer plans a settings menu later.
- **One boss type, one level.** The encounter spawns exactly one boss class.
- **The Victory heading depends on the fallback** until `WBP_GameOver` gets a `ResultTitleText`.
- **`GrantMoney.Run` and `GrantMoney.Combat.EnemyBase` were not re-run** after the later rounds.

## 9. Verification

### Performed

| Test | Action | Result | Kind |
|---|---|---|---|
| Build | `run_tests.py --filter GrantMoney.Boss` builds `GrantMoneyEditor` | Succeeded (after the §6.4 fix) | Compiled |
| Boss suite, round 1 | `GrantMoney.Boss` | 17/17 | Automation test |
| Shared code | `GrantMoney.Run` / `GrantMoney.Combat.EnemyBase` (`--skip-build`) | 26/26, 7/7 (round 1 only) | Automation test |
| Boss suite, after round 3 | `GrantMoney.Boss` | 21/21 | Automation test |
| Contact damage | `GrantMoney.Combat.ContactDamageComponent` | 5/5 | Automation test |
| Boss suite, final | `GrantMoney.Boss` | **23/23** | Automation test |
| PIE play-test | Yes/no quiz rounds | Confirmed: boss arrival, weapon targeting and damage, the dash combo, dashes stopping at the map edge, no new normal spawns with existing slimes alive, `SkipToBoss`, the boss hit log cadence, Game Over and Restart after a boss-fight death, max health working, lead aim at 0.6 feeling good | Developer reported |

**What the tests cover:**

| Area | Tests |
|---|---|
| Lunge logic | `Boss.Lunge.PhasesAndCooldown`, `TimingIsEditable`, `DurationLimitShortensCharge`, `StaysInsidePlayableArea`, `ComboDashesBackToBack`, `InterceptLeadsAMovingTarget` |
| Boss actor | `Boss.Slime.DefaultsAreIndependentAndBigger`, `LungeTelegraphChargeCooldown`, `LungeStaysInsidePlayableArea`, `NoLungeWithoutRoomOrMapOrRange`, `DeathCancelsLunge`, `ComboRetargetsAfterEachDash`, `ComboEndsEarlyWithoutAimOrRoom`, `DebugMaxHealthOverride`, `LungeLeadStrengthBlendsTowardFullLead` |
| Encounter | `Boss.Encounter.Defaults`, `SpawnsExactlyOneBossAtValidLocation`, `KeepsSlimesAndStopsNormalSpawns`, `BossDeathWinsExactlyOnce`, `PlayerDeathDuringFightEndsRunAsLost`, `SpawnFailureRetriesBoundedWithDiagnostic`, `RetrySucceedsAfterFailure`, `LateSpawnCannotChangeTerminalRun` |
| Contact damage | `Combat.ContactDamageComponent.LogHitsIsOptional` (new) plus the 4 existing tests |

### Pending (developer, in the Editor)

| Item | Why |
|---|---|
| Killing the boss shows a "Victory!" heading, exactly once | The retitle fallback was added after the last confirmed round; not confirmed in the transcript. |
| Restart from the Victory screen gives a fresh run (timer back, no boss, normal spawns resume) | Not confirmed in the transcript. |
| The boss visibly turns red during windups | Developer reported the glow overpowers the colour and deferred it. |
| A Blueprint boss set through a placed encounter's **Boss Class** gets its configured health | The developer said max health works but did not describe testing the Blueprint route. |
| The spawn-failure diagnostic on screen | Covered by automation only; never forced in PIE. |

## 10. Extension Ideas

- Settings menu for live boss tuning (health, combo, lead strength), which the developer already flagged.
- Replace the debug ground line with a decal or Niagara telegraph; tone down the emissive so the tint reads.
- A real Victory widget via `VictoryWidgetClass`.
- Boss minions using a dedicated spawner, or `bAllowNormalSpawnsInBossPhase`.
- Scale the boss to the player's level once there is reason to.

## 11. Source-Control Checkpoint

| Item | Value |
|---|---|
| Jira | SCRUM-42, SCRUM-43 |
| Branch | `SCRUM-42-43-Boss-Victory` |
| Commit | `caa540d` "Boss and Victory screen added, lots of tweaks for boss mechanic" (29 files, +2876 / −32) |
| PR | #36, merged to `main` (`c3b4711`) |

The AI did not commit; the developer committed and merged after the session.

---

# Part C – AI Workflow Log

**Tool:** Copilot (an AI assistant in VS Code), working on the repository on the developer's PC. It ran
only the boss tests and the tests for the shared code it touched, and handed all PIE checks to the
developer as yes/no quizzes.

| # | Purpose | Prompt (developer) | Result | Decision | Verification |
|---|---|---|---|---|---|
| 1 | Rules and request | Testing philosophy + SCRUM-43/42 brief | Read the run, enemy, spawner, game mode, UI, contact damage and test-helper code | Accepted | Read from source |
| 2 | Implementation | (continued) | Lunge logic, boss, speed hook, spawner search, spawn policy, encounter, HUD, Victory flow, 3 test files + helper | Accepted | Compiled |
| 3 | Targeted tests | (automatic) | Boss 17/17; Run 26/26; EnemyBase 7/7 | Accepted | Automation test |
| 4 | PIE quiz, round 1 | Developer answered 10 yes/no questions | Led to the combo, debug commands, HP readout, health override | Accepted | Developer reported |
| 5 | PIE quiz, round 2 | Developer answered 9 questions | First attempt failed (quiz schema sent as a string), re-sent correctly. Led to the material fix, an optional `ResultTitleText` heading, `bLogHits` | Accepted | Boss tests passed; ContactDamage 5/5 |
| 6 | PIE quiz, round 3 | Developer answered 7 questions | `WBP_GameOver` had no `ResultTitleText`, so the widget got a fallback that retitles the "Game Over" TextBlock | Accepted | Boss 21/21 |
| 7 | Combo length | "Would I be able to up the max dashes?" | Explained `LungeComboCount` is editable (C++ default or Blueprint) | Developer tuned it later | Read from source |
| 8 | Lead aim | "Make the dashes lead... intercept based off the player speed and direction" | `ComputeInterceptDirection` + `bLungeLeadsTarget`; build failed on a shadowed variable, fixed | Accepted | Boss 23/23 |
| 9 | Lead tuning | Quiz on fairness | Replaced the toggle with `LungeLeadStrength` (0.6) | Accepted | Boss 23/23; developer reported it lands well |
| 10 | Wrap-up | "We will iterate more after I present tomorrow" | — | Stop here for the vertical slice | — |

**Where AI helped most:** turning one large brief into small testable pieces (pure lunge maths, an
encounter state machine) that reuse the run controller's single-use win/loss and the spawner's
validation, and the intercept maths for lead aim.

**Where it needed a human:** everything visual (colour, telegraph readability, how fair the lead feels),
the combo tuning, and the Victory screen's look. The AI's attempt to read a binary engine asset as text
was against project rules and was not what solved the colour problem.

---

# Part D – Video Voice-Over Script

## Opening
**SAY:** "Until now, when the timer hit zero the run just... sat there. Today it gets an ending: a boss,
a win, and a way back to the start."

## Acceptance Criteria
**SAY:** "By the end: one boss arrives at a safe spot when the timer runs out, it has a readable lunge
attack that never leaves the map, killing it shows Victory, dying shows Game Over, and both restart."

## Engineering Principle
**SAY:** "Reuse what already works. The boss is a slime with a few overrides; the spawn spot uses the
spawner's own checks; Victory is the Game Over screen with a different title."

## Step-by-Step Demonstration

### 1. Inspect first
- **SAY:** "Before writing anything, the AI read the run controller, the spawner and the Game Over flow, so
  we knew the win and loss calls already existed and were single-use."
- **SHOW:** `ARunController::RequestWin` / `IsNormalSpawningAllowed`. *(media 02)*

### 2. Pure logic first
- **SAY:** "The lunge is a tiny state machine with no actor in it: windup, lunge, cooldown. That makes it
  testable with plain numbers, including clamping every dash to the map."
- **SHOW:** `BossLunge.h`. *(media 03)*

### 3. The encounter
- **SAY:** "One actor owns the boss's life: spawn once, retry a few times if there's no safe spot and say
  so on screen, win once on death, and ignore anything that arrives after the run is over."
- **SHOW:** `EBossEncounterState`. *(media 04)*

### 4. Tests
- **SAY:** "Twenty-three boss tests, run on their own, all green."
- **SHOW:** the test output. *(media 05)*

### 5. Play it
- **DO:** `GrantMoney.SkipToBoss`; show the arrival, health bar, telegraph and combo, a dash stopping at the
  wall, then a win and a loss, each followed by Restart. *(media 06 to 10)*

### 6. Lead aim
- **SAY:** "Then we made it smarter: each dash aims where you're going to be, blended sixty percent of the
  way so you can still juke it."
- **SHOW:** a dash leading a running player. *(media 11)*

## Common Mistakes
**SAY:** "Check what your materials actually expose before setting a colour on them, and don't let a
widget's fixed heading quietly decide what your Victory screen says."

## Verification Checkpoint
**SAY:** "Tests prove the rules: one boss, one win, no late surprises. Whether it looks and feels right is
checked by playing it."

## Next Milestone
**SAY:** "That's a complete vertical slice, from first spawn to Victory. Next is tuning it live and making
it look the part."

---

# Media Capture List

Store under `Media/SCRUM-42-43/` (outside the repository). Only record what actually happens.

| # | File | Type | Show | Proves |
|---|---|---|---|---|
| 01 | `01-branch.png` | Screenshot | `git branch --show-current` on `SCRUM-42-43-Boss-Victory` | Branch |
| 02 | `02-run-controller.png` | Screenshot | `IsNormalSpawningAllowed` with `bAllowNormalSpawnsInBossPhase` | Spawn policy |
| 03 | `03-lunge-state.png` | Screenshot | `BossLunge.h` phases and settings | Pure, editable lunge |
| 04 | `04-encounter-states.png` | Screenshot | `EBossEncounterState` and retry settings | Bounded retry, terminal guards |
| 05 | `05-tests-pass.png` | Screenshot | `run_tests.py --filter GrantMoney.Boss`, 23/23 | Tests green |
| 06 | `06-boss-arrives.mp4` | Clip | Timer hits 0 (or `SkipToBoss`): emergence, "BOSS ARRIVED", health bar | Arrival |
| 07 | `07-telegraph-combo.mp4` | Clip | Windup (stop, swell, red line) then the dash combo | Telegraphed lunge |
| 08 | `08-dash-at-edge.mp4` | Clip | A dash toward the boundary stopping short | Bounds |
| 09 | `09-victory-restart.mp4` | Clip | Boss dies → Victory screen → Restart → fresh run | Win flow (pending confirmation) |
| 10 | `10-death-restart.mp4` | Clip | Player dies mid-fight → Game Over → Restart | Loss flow |
| 11 | `11-lead-aim.mp4` | Clip | Running sideways during the windup; the dash leads | Lead aim |
