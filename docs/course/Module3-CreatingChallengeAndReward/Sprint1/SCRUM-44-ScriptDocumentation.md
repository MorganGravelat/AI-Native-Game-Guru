# SCRUM-44 – Player Death, Game Over Screen and Restart
## Module 3 Sprint 1 – Development and Recording Guide

This guide covers SCRUM-44: the loss side of the run. The player's health reaching zero now ends the run exactly once, pauses the game (stopping the player, the weapon, every enemy's pursuit and contact damage, spawning and pickup attraction, all through the same mechanism the existing pause menu already uses), and shows a Game Over screen with survival time and total XP, with Restart and Quit buttons. Restart reloads the current level, which is the simplest reliable way to reset every run-scoped actor and subsystem. A deterministic tie-break rule is defined for a win and a loss becoming true in the same tick.

- **Part A:** change report
- **Part B:** developer implementation guide
- **Part C:** AI workflow log
- **Part D:** video voice-over script
- **Media Capture List:** every screenshot and clip to record, with file names

**Status:** the code and tests are written. **The project builds, and the full `GrantMoney.*` Automation suite passes: 103 of 103, 3 of them new** (run by the AI session on this machine, 2026-11-12). **The developer created `WBP_GameOver` and verified the full loss flow in PIE** (2026-11-12), including a follow-up yes/no quiz on the remaining edge cases (Escape during Game Over, Quit, a second consecutive restart) — everything confirmed.

**What is and is not evidence in this document:**

| Kind | Meaning |
|---|---|
| Read from source | Taken from reading project or engine source, including the Unreal engine's own `LevelTick.cpp` (to confirm what pausing actually freezes). Not a test. |
| Compiled | UnrealBuildTool finished with `Result: Succeeded`. Says nothing about behaviour. |
| Automation test | A `GrantMoney.*` Automation test passed in a headless editor run, in a synthetic game world. |
| Headless run | The real project, game mode and level started with `-game -nullrhi` and the Output Log was read. Proves wiring and logic; shows nothing on screen, and only ran briefly to confirm the missing-widget case does not crash. |
| Developer reported | The developer played it in the Editor and said what they saw. |

At the time of writing, **all five kinds apply**, including "Developer reported" (see the quote above).

---

## Jira

| Key | Goal |
|---|---|
| SCRUM-44 | Complete the loss side of the run: connect the player's death to the run controller, stop gameplay on loss, show a Game Over screen with survival time, score (see below) and XP, restart the same level cleanly, and define the win/loss tie-break rule. |

**Note on "score":** the shared milestone brief for this ticket asks the Game Over screen to show "survival time, score, and total XP". Earlier the same day, in this same session, the score was deliberately removed from `ARunController` (`ScorePerSecond` / `GetScore()` deleted) because the character's level will be the run's progress readout once it exists. The Game Over screen therefore shows **survival time and total XP only**; there is no score field to display. This is a deliberate, documented deviation from the brief, not an oversight.

**Done when, and where each one is met:**

| Criterion | Met by | Evidence |
|---|---|---|
| Player's zero-health event ends the run, exactly once | `AGrantMoneyPlayerCharacter::HandleHealthDepleted` (bound to `Health->OnDeath` in `BeginPlay`) calls `ARunController::RequestLoss()`; `ARunController::TryTransition` never re-enters a state | Test `Run.Loss.PlayerDeathTransitionsRunToLost` |
| Continuous overlap damages at the configured cadence | Unchanged `UContactDamageComponent::TryApplyContactDamage` (existing logic); exercised end to end through a real `AEnemyBase`-derived attacker and the player's real `UHealthComponent` | Test `Run.Loss.SlimeContactKillsPlayerAtConfiguredCadence` (also existing `Combat.ContactDamageComponent.*` unit tests) |
| On loss: player movement, automatic firing, enemy pursuit/contact damage, spawning, further reward collection all stop | `UGameplayStatics::SetGamePaused` (the same call the pause menu already makes) — confirmed from the engine's own `LevelTick.cpp` that a paused world skips both actor `Tick()` (player movement, every enemy's pursuit/contact damage, pickups flying in) and `FTimerManager::Tick()` (the weapon's fire timer) entirely | Read from source (`UWorld::Tick`, `LevelTick.cpp:1608-1816`); existing tests `Run.RunController.PauseFreezesTime` and `Run.EnemySpawner.FollowsRunStateAndPause` already prove pause freezes the clock and spawning the same way |
| Camera and UI stay usable | Same `FInputModeGameAndUI` + visible cursor the pause menu already uses; the camera pawn's own Tick is what moves it, so it holds its last frame exactly like it does today during an ordinary pause | Read from source; shared mechanism with the already-working pause menu |
| Game Over screen: survival time, XP (score omitted, see above), Restart and Quit | `UGameOverWidget` (new), shown by `AMapCameraPlayerController::ShowGameOver`, following the exact conventions `UPauseMenuWidget` / `OpenPauseMenu` already use for cursor, focus and input mode | `// NO-TEST:` (pure UI glue, like `RunHUD.cpp`). Developer reported: confirmed. |
| Restart via the simplest reliable mechanism; every run-scoped state reset | `UGameOverWidget::HandleRestart` calls `UGameplayStatics::OpenLevel` on the current level name, which tears down and recreates the whole `UWorld` — every actor (player, enemies, projectiles, pickups) and every `UWorldSubsystem` (`UCombatEventsSubsystem`) starts fresh with no extra reset code needed | Read from source: nothing in this project stores run state outside the `UWorld` (no `UGameInstanceSubsystem` holds XP, stats or run state). Developer reported: confirmed. |
| Deterministic win/loss tie-break, states entered only once | `ARunController::RequestWin` refuses if the player (set via `SetPlayerHealthComponent`) already reports `IsDead()`; `UHealthComponent` marks itself dead **before** broadcasting `OnDeath`, so this is true regardless of whether the player's own death-handling has run yet that tick | Test `Run.RunController.WinRequiresAlivePlayer` |
| Verify death from slime contact; the Game Over screen is clickable; two consecutive restarts leave no stale objects or duplicated bindings | Slime-contact test above; `AddUniqueDynamic` used for every new binding (`RunController->OnStateChanged`, `Health->OnDeath`, the widget's own buttons); `OpenLevel` guarantees a fresh `UWorld` per restart, so there is no C++ object that could carry a stale binding across a restart | Automation test (contact) + read from source (bindings, world lifetime) + developer reported: confirmed, via PIE play and a follow-up yes/no quiz. |

**Related tickets:** SCRUM-33 (stats, health, death), SCRUM-32 (starter weapon), SCRUM-35 (pickups), SCRUM-53 (stat boosts), SCRUM-31/30/38/42 (run controller, spawner, emergence — this ticket builds directly on that milestone).

---

# Part A – Change Report

## Milestone Summary

When the player's health reaches zero, the run ends: the game pauses (freezing the player, every enemy, the weapon and spawning, by the same mechanism the pause menu already uses), and a Game Over screen appears showing how long the player survived and their XP, with Restart and Quit buttons. Restart reloads the level from scratch. A win and a loss that would both become true in the same tick always resolve to a loss. The score concept introduced earlier in this milestone's documents has been removed from the loss screen to match the project's decision that the character's level, not a score, tracks progress.

## Files Created

| File | Purpose |
|---|---|
| `Public/GameOverWidget.h`, `Private/GameOverWidget.cpp` | `UGameOverWidget`: survival time / XP text, Restart (reloads the level) and Quit buttons. `// NO-TEST:` (pure UI glue). |
| `Private/RunLossTests.cpp` | 2 tests, `GrantMoney.Run.Loss.*`: the player-death-to-Lost binding, and slime contact killing the player at the configured cadence. |
| `docs/course/Module3-CreatingChallengeAndReward/Sprint1/SCRUM-44-ScriptDocumentation.md` | This document. |

## Files Modified

| File | Change | Why |
|---|---|---|
| `Public/RunController.h`, `Private/RunController.cpp` | `static ARunController* Find(const UWorld*)`; `SetPlayerHealthComponent`; `RequestWin` refuses if that player is already dead. Updated class comment. | One shared lookup instead of three copies of the same `TActorIterator` loop; the deterministic win/loss tie-break rule. |
| `Private/EnemySpawner.cpp`, `Private/RunHUD.cpp` | `ResolveRunController` / `GetRunController` now call `ARunController::Find` instead of their own copy of the lookup. | Same lookup, one place; no behaviour change (existing tests for both still pass unchanged). |
| `Public/GrantMoneyPlayerCharacter.h`, `Private/GrantMoneyPlayerCharacter.cpp` | `HandleHealthDepleted` bound to `Health->OnDeath` in `BeginPlay`, calling `ARunController::Find(...)->RequestLoss()`. Dev console command `GrantMoney.DamagePlayer [Amount] [DelaySeconds]`. | Connects the player's death to the run. The console command is the quickest way to test the loss screen without waiting on real combat. |
| `Public/MapCameraPlayerController.h`, `Private/MapCameraPlayerController.cpp` | `GameOverWidgetClass` (resolves `/Game/UI/WBP_GameOver`); `ShowGameOver(SurvivalSeconds, Experience)`; `TogglePauseMenu` is a no-op once the Game Over screen is showing. | Reuses the pause menu's exact cursor/focus/input-mode conventions, per the brief. |
| `Public/GrantMoneyGameMode.h`, `Private/GrantMoneyGameMode.cpp` | `HandleRunStateChanged` bound to `RunController->OnStateChanged` (`AddUniqueDynamic`); on `Lost`, sets the player's health component on the run (for the tie-break) right before `StartRun()`, and calls `ShowGameOver` with elapsed survival time and XP. | The game mode is the existing place that already wires the run controller, the spawner and the HUD together; this is the same pattern for the loss screen. |

## Files Not Changed

- `UContactDamageComponent`, `UHealthComponent`, `UAttackHitComponent`, `UStarterWeaponComponent`, `AEnemyBase`, `APickupBase`, `UPickupComponent`, `UCombatEventsSubsystem`. The loss flow fits into them as they are; contact damage cadence, death detection and pause behaviour needed no changes.
- No `.uasset` / `.umap` was created or edited by this work. `WBP_GameOver` does **not** exist yet (§6A); `GameOverWidgetClass` silently stays unset until it is created, logging an error (not a crash) if `ShowGameOver` is reached first.

---

# Part B – Developer Implementation Guide

## 1. Objective

At the end:
- the player's health reaching zero pauses the game and ends the run as a loss, exactly once;
- pausing freezes the player, the weapon, every enemy and spawning, with the camera and UI still visible;
- a Game Over screen shows survival time and XP, with Restart (reloads the level) and Quit;
- a simultaneous win and loss always resolves to a loss;
- two consecutive restarts leave no stale actors or duplicated event bindings.

## 2. Why This System Exists

**Before this ticket**, `ARunController` could transition to `Lost`, but nothing ever called `RequestLoss()`; the player's `UHealthComponent` had no listener at all; and there was no Game Over UI.

**Chosen architecture:**

- **Reuse the pause menu's mechanism, not a parallel one.** `UGameplayStatics::SetGamePaused` already freezes everything a loss needs to freeze, because Unreal's own `UWorld::Tick` skips both actor ticking and `FTimerManager::Tick` while paused (confirmed by reading `LevelTick.cpp`, not assumed). This is why the "stop movement / firing / pursuit / contact damage / spawning / pickups" requirement needed no bespoke per-system "freeze" code: one call does it, the same way the existing pause menu already does it.
- **One owner decides whether a win or a loss actually happens.** `ARunController::RequestWin` / `RequestLoss` were already the only code that changes state. The tie-break rule for "both at once" belongs there too, not scattered across whoever calls them.
- **The tie-break checks a fact, not a race.** `RequestWin` refuses if the player's `UHealthComponent::IsDead()` is already true. Since `UHealthComponent::ApplyDamage` sets `bIsDead = true` **before** broadcasting `OnDeath`, this is reliable regardless of which of two same-tick events (a boss death, a player death) is processed first — there is no dependency on callback registration order.
- **The widget only owns its own buttons**, exactly like `UPauseMenuWidget`. The controller (`AMapCameraPlayerController`) owns creating it, pausing, and the cursor/input mode, exactly like `OpenPauseMenu` already does for the pause menu.
- **Restart reloads the level.** No manual "reset XP, reset enemies, reset pickups, reset boss state" code was written, because none is needed: a full `UGameplayStatics::OpenLevel` destroys the entire `UWorld`, and every piece of state this ticket's reset list mentions (health, XP, upgrades, permanent boosts, temporary effects, the timer, enemies, projectiles, pickups, boss state, and any `UWorldSubsystem`) lives on that `UWorld` or its actors. This project has no `UGameInstanceSubsystem` holding any of it, so nothing survives a reload to clean up by hand.

**Alternatives considered:**

| Alternative | Decision |
|---|---|
| A bespoke "freeze everything" pass (disable each component individually) | **Rejected.** `SetGamePaused` already does this, is already proven (the pause menu), and keeps the loss screen's logic to "pause, then show a widget", nothing more. |
| A separate `FTimerManager`-based "end of run" timeout to defer win/loss resolution by a frame, to settle simultaneous calls | **Rejected.** Would change the return-value contract of `RequestWin`/`RequestLoss` for every existing caller and test. Checking `IsDead()` directly is just as deterministic and needed no change to existing behaviour. |
| Keep "score" on the Game Over screen as the brief originally said | **Rejected, by the developer's own earlier instruction this session.** The screen shows survival time and XP only; see the Jira table note. |
| Reset run state by hand (clear enemies, reset stats, clear pickups, …) | **Rejected.** `OpenLevel` already does this for free and cannot miss anything added later, unlike a hand-written reset list. |
| A `UGameOverWidget` that also handles the Pause input itself | **Rejected.** `TogglePauseMenu` already exists on the controller; it now simply no-ops while the Game Over screen is up, which is simpler than teaching the widget about input actions. |

## 3. Prerequisites

- SCRUM-33, SCRUM-32, SCRUM-35, SCRUM-53 and the SCRUM-31/30/38/42 run milestone merged (this builds directly on `ARunController`, `AEnemySpawner`, `AEnemyBase` emergence).
- Unreal Engine 5.8, Editor **closed** when running `scripts/run_tests.py` (Live Coding breaks the build).
- `WBP_PauseMenu` already exists in `/Game/UI/` as the pattern to copy (§6A).

## 4. Architecture Overview

```
AGrantMoneyPlayerCharacter.Health->OnDeath (HealthComponent sets bIsDead = true FIRST, then broadcasts)
        └─► HandleHealthDepleted ─► ARunController::Find(World)->RequestLoss()
                                              │  (no-op, returns false, if the run isn't Active/BossPhase,
                                              │   or is already Won/Lost: TryTransition never re-enters)
                                              ▼
                                  ARunController: Active/BossPhase -> Lost   (OnStateChanged broadcasts)
                                              │
                                              ▼
                      AGrantMoneyGameMode::HandleRunStateChanged (AddUniqueDynamic, bound once in PrepareRun)
                                              │
                                              ▼
                 AMapCameraPlayerController::ShowGameOver(ElapsedSeconds, Experience)
                        ├─► ClosePauseMenu()                       (no pause menu on top of Game Over)
                        ├─► UGameOverWidget::SetResults + AddToViewport
                        └─► UGameplayStatics::SetGamePaused(true)  (same call OpenPauseMenu already makes)
                                   └─► UWorld::Tick skips actor Tick() and FTimerManager::Tick() entirely:
                                        player movement, weapon fire timer, every enemy's pursuit and
                                        contact damage, the spawner, pickups flying in — all frozen already

UGameOverWidget
    Restart ─► SetGamePaused(false); OpenLevel(current level name)   — fresh UWorld, fresh everything
    Quit    ─► UKismetSystemLibrary::QuitGame                        — identical to the pause menu's Quit

Simultaneous win/loss (future boss):
    ARunController::RequestWin() refuses if PlayerHealthComponent->IsDead() is already true
        (set by AGrantMoneyGameMode right before StartRun(), from the player pawn's own UHealthComponent)
```

## 5. AI-Assisted Planning

### AI Prompt
The shared Module 3 instructions (read `CLAUDE.md` / `AGENTS.md`, inspect the checkout, reuse existing systems, follow build/test requirements, maintain ScriptDocumentation.md per milestone), followed by the SCRUM-44 brief verbatim: complete the loss side of the run using the existing `HealthComponent` and `ContactDamageComponent`; connect the player's zero-health event to the run controller so it transitions to Lost exactly once; check contact-damage cadence; stop movement/firing/pursuit/contact damage/spawning/reward collection on loss while keeping camera and UI usable; a Game Over screen with survival time, score and XP, Restart and Quit, using pause-menu conventions; restart through the simplest reliable mechanism, resetting every listed piece of state including subsystems; define and document a deterministic win/loss tie-break rule; verify slime-contact death, the clickable screen, and two consecutive restarts.

### Expected AI Output
Inspection of the existing health/contact-damage/run-controller/pause-menu code, a design for the loss flow and the tie-break rule, the implementation, new tests, and this document.

### Human Review
- Confirm dropping "score" from the Game Over screen (per this session's earlier decision) is the right call rather than re-adding a score concept just for this screen.
- Confirm the win/loss tie-break rule ("death beats victory": `RequestWin` refuses once the player is dead) is the desired design, since there is no boss yet to exercise it for real.
- ~~Create `WBP_GameOver` in the Editor (§6A)~~ — **done**; the developer created it and confirmed the screen appears correctly.
- ~~Try the loss flow in PIE and report back what is seen.~~ — **done**; see §9.

## 6. Step-by-Step Implementation

### Step 1 – One shared "find the run controller" helper
`ARunController::Find(const UWorld*)` wraps the `TActorIterator<ARunController>` loop that `AEnemySpawner` and `ARunHUD` each already had their own copy of. Both were refactored to call it; neither's behaviour changed (their own tests still pass unchanged).

### Step 2 – Connect the player's death to the run
`AGrantMoneyPlayerCharacter::BeginPlay` binds `Health->OnDeath.AddUniqueDynamic(this, &AGrantMoneyPlayerCharacter::HandleHealthDepleted)`. The handler finds the run (if any) and calls `RequestLoss()`. Since `TryTransition` never re-enters a state, a second application of lethal damage (after death, which `UHealthComponent::ApplyDamage` already guards against doing anything) cannot cause a second transition either way.

### Step 3 – The deterministic win/loss tie-break
`ARunController::SetPlayerHealthComponent` records a weak pointer to the player's `UHealthComponent`. `AGrantMoneyGameMode::CheckRunReady` sets it right before calling `StartRun()` (the same place it already confirms the player pawn exists). `RequestWin` now refuses (returns `false`, no state change) if that health component already reports `IsDead()`. Because `UHealthComponent::ApplyDamage` sets `bIsDead = true` before broadcasting `OnDeath`, this check is correct no matter which of two same-tick events is handled first.

### Step 4 – Stopping gameplay on loss
No new "freeze" code. `AMapCameraPlayerController::ShowGameOver` calls `UGameplayStatics::SetGamePaused(this, true)`, exactly as `OpenPauseMenu` already does. Reading the engine's `LevelTick.cpp` confirms `UWorld::Tick` skips both actor ticking (so player movement, the camera's own tick, every enemy's `Tick` — which is where pursuit and contact damage both live — and `UPickupComponent`'s per-frame scan all stop) and `FTimerManager::Tick` (so `UStarterWeaponComponent`'s fire timer stops, and `AEnemySpawner::Tick`'s own `IsPaused()` guard stops it a second, redundant way) while the world is paused.

### Step 5 – The Game Over widget and controller wiring
`UGameOverWidget` mirrors `UPauseMenuWidget`: `BindWidget` names for two text blocks and two buttons, `NativeConstruct` binds the buttons with `AddUniqueDynamic`, `HandleQuit` is identical to the pause menu's, `HandleRestart` calls `UGameplayStatics::OpenLevel` on the current level's name. `AMapCameraPlayerController::ShowGameOver` creates it once, calls `SetResults`, adds it to the viewport, closes the pause menu first (so the two never stack), and sets the cursor/input mode the same way `OpenPauseMenu` does. `TogglePauseMenu` now first checks whether the Game Over screen is showing and no-ops if so.

### Step 6 – Game mode wiring
`AGrantMoneyGameMode::PrepareRun` binds `RunController->OnStateChanged.AddUniqueDynamic(this, &AGrantMoneyGameMode::HandleRunStateChanged)` once. On `Lost`, it finds the local `AMapCameraPlayerController` and the player's `AGrantMoneyPlayerCharacter`, and calls `ShowGameOver(RunController->GetElapsedSeconds(), PlayerCharacter->GetExperience())`. Every other transition (including `Won`, which has no screen yet since there is no boss) is ignored.

### Step 7 – A dev console command for testing without combat
`GrantMoney.DamagePlayer [Amount=1000] [DelaySeconds=0]`, next to the existing `GrantMoney.DamageEnemies`. With a delay it schedules the hit via a one-shot timer, which is useful for lining a test hit up with a specific moment (for example, well after the run's 1-second Starting delay) instead of hitting whatever state happens to be current the instant the command is typed.

### Step 8 – Tests and whole-suite run
`python scripts/run_tests.py` (or `--filter GrantMoney.Run`). Result in §9.

## 6A. Designer / Developer Setup: Creating WBP_GameOver

**Status: done.** The developer created this asset and confirmed the screen appears correctly in PIE. The steps below are kept as a record of how it was built, and as a reference if the widget ever needs to be recreated or extended.

This cannot be done from a terminal or an AI session (it is a binary `.uasset`). In the Unreal Editor:

1. **Content Browser → `Content/UI/`** (same folder as the existing `WBP_PauseMenu`).
2. **Right-click → User Interface → Widget Blueprint.** Name it `WBP_GameOver`.
3. **Open it. In the Graph settings / Class Settings, set the Parent Class to `GameOverWidget`** (search for it; it appears because `UGameOverWidget` is `GRANTMONEY_API` and `UCLASS()`).
4. **Build the layout** (a Canvas Panel is simplest, matching `WBP_PauseMenu`'s likely structure):
   - A `Text` widget named exactly **`SurvivalTimeText`**.
   - A `Text` widget named exactly **`ExperienceText`**.
   - A `Button` named exactly **`RestartButton`**, with a child `Text` reading "Restart".
   - A `Button` named exactly **`QuitButton`**, with a child `Text` reading "Quit".
   - Names must match exactly (`BindWidget` is case- and name-sensitive); a mismatch compiles the widget but leaves that pointer null at runtime, and the corresponding text/button silently does nothing.
5. **Compile and save.**
6. **Set it on the controller.** Select `BP_MapCameraPlayerController` (or whichever Blueprint/class currently fills the game mode's Player Controller Class) → Class Defaults → **Game Over → Game Over Widget Class → `WBP_GameOver`**. (The C++ constructor already tries to auto-resolve `/Game/UI/WBP_GameOver` the same way it does for `WBP_PauseMenu`, so once the asset exists at that exact path this step may already be filled in — check it rather than assume.)

## 7. AI Prompts by Stage

| Stage | Prompt |
|---|---|
| Inspection | "Inspect how slimes currently apply contact damage and how the player receives it, and how the existing pause menu controls cursor, focus and input mode." |
| Death → Loss | "Connect the player's zero-health event to the run controller so it transitions to Lost exactly once." |
| Stop gameplay | "On loss, stop player movement, automatic firing, enemy pursuit/contact damage, spawning, and further reward collection. Keep the camera and UI usable." |
| Game Over screen | "Create a simple Game Over screen displaying survival time and total XP (not score — see the Jira note), with Restart and Quit buttons, using the existing controller and pause-menu conventions." |
| Restart | "Restart the same demo level through the simplest reliable existing mechanism. Reset health, XP, enemies, projectiles, pickups, boss state and any subsystem state." |
| Tie-break | "Define a deterministic rule for simultaneous victory and player death, and document it. Terminal run states must only be entered once." |
| Verification | "Verify death from slime contact at the configured cadence, and that restarting twice in a row leaves no stale objects or duplicated event bindings." |

## 8. Common Mistakes

### 8.1 `TWeakObjectPtr<UHealthComponent>` assignment failed to compile from a forward declaration
- **Symptom:** `error C2676`-style messages from `WeakObjectPtrTemplates.h`: "Types pointed to are unrelated; conversion requires reinterpret_cast…", pointing at `PlayerHealthComponent = InPlayerHealth;` inside an inline header function.
- **Cause:** `RunController.h` only forward-declares `class UHealthComponent;`. Assigning a raw pointer into a `TWeakObjectPtr<T>` needs the compiler to see that `T` derives from `UObject` at the point of assignment, which an incomplete forward declaration cannot provide.
- **Fix:** move the assignment out of the header and into `RunController.cpp`, which already includes `HealthComponent.h`. The header keeps only the declaration.
- *Real failure during this work, caught by the first build.*

### 8.2 A headless smoke-test's injected console command fired before the run left `Starting`
- **Symptom:** `GrantMoney.DamagePlayer 1000` passed via `-ExecCmds` at process launch applied the damage (confirmed in the log) about 65 ms **before** `ARunController::StartRun()` was even called, so `RequestLoss()` correctly no-op'd (Starting cannot go directly to Lost) instead of demonstrating the Active→Lost→Game-Over path.
- **Cause:** `-ExecCmds` runs at a fixed, very early point during startup; it is not synchronized with the game mode's own map-ready polling.
- **Fix:** added an optional `DelaySeconds` argument to `GrantMoney.DamagePlayer` so a later, precisely-timed hit can be scheduled instead of applied immediately. **Not re-verified headlessly afterward** — the developer offered to test this directly in the Editor instead, which is both faster and conclusive, so the remaining confirmation is left to PIE rather than further headless engineering.
- *Real issue during this work; resolved by stopping the automated-smoke-test approach rather than over-engineering around a test-harness timing quirk.*

## 9. Verification

### Performed

| Test | Action | Result | Kind |
|---|---|---|---|
| Build | `python scripts/run_tests.py` builds `GrantMoneyEditor` | Succeeded | Compiled |
| Automation tests | Full `GrantMoney.*` suite, 2026-11-12 | **103 of 103 passed** (100 pre-existing, 3 new) | Automation test |
| Missing-widget safety | Brief headless launch of the real level (`-game -nullrhi`) | `WBP_GameOver` not found logs an engine warning/error (`Failed to find /Game/UI/WBP_GameOver.WBP_GameOver_C`); no crash; the level continued loading and starting the run normally | Headless run |
| Full loss flow: both death paths, readouts, Restart, Escape no-op, Quit, a second consecutive restart | Developer played in PIE, then a follow-up yes/no quiz on the edge cases | All confirmed working | Developer reported |

**New tests** (rerun with `python scripts/run_tests.py --filter GrantMoney.Run`):

| Group | Tests |
|---|---|
| `Run.RunController` | `WinRequiresAlivePlayer` (new) |
| `Run.Loss` (new file) | `PlayerDeathTransitionsRunToLost`, `SlimeContactKillsPlayerAtConfiguredCadence` |

**What the tests prove, against the ticket's Verify list:**

| Item | Test(s) | What it asserts |
|---|---|---|
| Death from slime contact | `SlimeContactKillsPlayerAtConfiguredCadence` | A real `AEnemyBase`-derived attacker in constant contact range deals exactly one hit (10 damage) per 1-second window, never once per sub-step, over 10 windows (using 0.25 s sub-steps, exact in binary floating point, so there is no timing flakiness); health reaches exactly zero; `OnDeath` fires once; the bound `ARunController` reaches `Lost` exactly once; further contact afterward changes nothing |
| Player death → Lost, exactly once | `PlayerDeathTransitionsRunToLost` | Non-lethal damage changes nothing; lethal damage transitions the run to `Lost` exactly once (through the real `AGrantMoneyPlayerCharacter::HandleHealthDepleted` binding, not a test double); further damage after death causes no second transition |
| Deterministic win/loss tie-break | `WinRequiresAlivePlayer` | With no player set, `RequestWin` behaves exactly as before (the pre-existing `WinAndLoss` test is unaffected); with a live player set, win succeeds; with a **dead** player set (killed before `RequestWin` is even called, modelling the worst-case ordering), `RequestWin` is refused and the state stays `BossPhase`; `RequestLoss` still succeeds afterward; clearing the player restores the original behaviour |
| Stop movement / firing / pursuit / contact damage / spawning / pickups | Not a new test: `Run.RunController.PauseFreezesTime` and `Run.EnemySpawner.FollowsRunStateAndPause` (pre-existing) already prove the pause mechanism this ticket reuses freezes the clock and spawning; the engine source read in §2/§4 explains why the same mechanism also freezes actor Tick (movement, enemy pursuit/contact damage, pickups) and `FTimerManager` (the weapon) | — |
| Clickable Game Over screen; two consecutive restarts, no stale objects or duplicated bindings | Slime-contact test above; `AddUniqueDynamic` used for every new binding (`RunController->OnStateChanged`, `Health->OnDeath`, the widget's own buttons); `OpenLevel` guarantees a fresh `UWorld` per restart, so there is no C++ object that could carry a stale binding across a restart | Developer reported: confirmed, including a second consecutive restart |

If anything fails on a later run, record the real error under §8 before fixing it.

### Pending

None — all items were confirmed by the developer.

## 10. Known Limitations

- **No boss, still.** The win/loss tie-break rule (`RequestWin` refuses while the player is dead) is implemented and tested, but nothing in the project currently calls `RequestWin()` at all.
- **`Won` has no screen.** Only `Lost` is wired to `ShowGameOver`; a future boss ticket will decide what the Won screen looks like and whether it reuses `UGameOverWidget` or a different one.
- **The loss screen is tied to the first local player only** (`UGameplayStatics::GetPlayerController(this, 0)` / `GetPlayerPawn(this, 0)`), consistent with the rest of this single-player project.
- **A Game Over caused by something other than contact damage** (for example a future hazard) is already covered: `HandleHealthDepleted` reacts to `OnDeath` regardless of damage source.

## 11. Extension Ideas

- A fade-to-black or a short delay before the Game Over screen appears, instead of an instant cut.
- A "Best Survival Time" / "Best XP" readout once there is somewhere to persist it between runs (a `UGameInstanceSubsystem` or a save game) — deliberately out of scope today, consistent with "no state should survive a restart" for the current run.
- Once a level-up system exists, show the player's level on the Game Over screen (the "pseudo score" the developer described).
- A distinct Won screen once the boss exists, reusing or branching from `UGameOverWidget`.

## 12. Source-Control Checkpoint

| Item | Value |
|---|---|
| Jira | SCRUM-44 |
| Branch | Currently `main`. Per `CLAUDE.md`, one Jira issue → one branch → one PR; consider `SCRUM-44-player-death-game-over-restart`. |
| PR title | `SCRUM-44: Player death, Game Over screen and restart` |

Suggested commit message (after the Editor checks in §9 and creating WBP_GameOver):
```
SCRUM-44: Add player death, Game Over screen and restart

- AGrantMoneyPlayerCharacter: Health->OnDeath ends the run (ARunController::RequestLoss)
- ARunController: Find() helper (also used by EnemySpawner/RunHUD); RequestWin refuses a dead player
  (deterministic win/loss tie-break: death beats victory, regardless of event order)
- AMapCameraPlayerController: ShowGameOver reuses the pause menu's pause/cursor/input-mode conventions
- UGameOverWidget: survival time, XP, Restart (reloads the level) and Quit
- AGrantMoneyGameMode: binds RunController::OnStateChanged, shows the Game Over screen on Lost
- GrantMoney.DamagePlayer dev console command, with an optional delay
- 3 new Automation tests and SCRUM-44 documentation
- Developer created Content/UI/WBP_GameOver and confirmed the loss flow, death, the readouts and restart in PIE
```

---

# Part C – AI Workflow Log

**Tool:** Copilot (an AI assistant in VS Code), working on the repository on the developer's PC. This session could build and run the project (`scripts/run_tests.py`) and briefly start the real level headless. It could not look at the screen or create the Blueprint widget asset.

| # | Purpose | Prompt (developer) | Result | Decision | Verification |
|---|---|---|---|---|---|
| 1 | Request | The shared Module 3 instructions, plus the SCRUM-44 brief verbatim ("Complete the loss side of the run using the existing HealthComponent and ContactDamageComponent…"). | Read `HealthComponent`, `ContactDamageComponent`, `AEnemyBase`, `ARunController`, `AEnemySpawner`, `PauseMenuWidget`, `MapCameraPlayerController`, `GrantMoneyPlayerCharacter`, `GrantMoneyGameMode`, `StatsComponent`/`GrantMoneyStatTypes` (confirmed no level/score system exists yet besides the `Experience` stat) and the engine's `LevelTick.cpp` (confirmed what pausing actually freezes). | Accepted | Read from source |
| 2 | Implementation | (continued) | Wrote the `ARunController::Find` refactor, the death→loss binding, the tie-break rule, `UGameOverWidget`, the controller and game-mode wiring, the dev console command, and tests. | Accepted | Compiled; tests below |
| 3 | First build | (automatic) | A forward-declared `UHealthComponent` could not be assigned into a `TWeakObjectPtr` inline in the header (§8.1). | Fixed | Rebuilt |
| 4 | Test run | (automatic) | 25 of 25 `Run.*` tests, then 103 of 103 whole-suite. | Accepted | **103 of 103** |
| 5 | Headless smoke attempt | (automatic) | Confirmed the missing `WBP_GameOver` fails gracefully (no crash); the injected test kill landed in `Starting`, not `Active` (§8.2). Added a `DelaySeconds` option to the dev console command to make a future attempt easy to time correctly. | Partially verified | Headless run (safety only) |
| 6 | Stop automating the smoke test | "You know I can test things out for you, no need to waste tokens on figuring out ways to do testing I can do for you" | Stopped the lingering headless process, rebuilt once more, confirmed the automated suite still passes, and left the remaining on-screen verification to the developer. | Accepted | Compiled; 103 of 103 |
| 7 | PIE verification + process refinement | Developer played the loss flow in PIE and reported it working; asked a short yes/no quiz on the remaining edge cases (Escape, Quit, a second consecutive restart), all confirmed. Developer then asked that this quiz method (not a detailed evidence write-up) be the standard way to close out verification going forward. | Updated this document to report the confirmation without transcribing it; added the convention to `CLAUDE.md`'s Course Documentation Workflow section. | Accepted | Developer reported |
| 8 | Documentation | (implicit, per the shared Module 3 instructions) | This document. | — | Checked against the code, the test output, and the developer's report |

**Where AI helped most:** reading the engine's own `LevelTick.cpp` before writing any "freeze gameplay" code, which showed that the existing pause menu's mechanism already does everything this ticket needed — no new per-system stop logic was necessary.

**Where it needs a human:** creating `WBP_GameOver` in the Editor; everything that needs eyes on the screen (the Game Over screen's look, clickability, the camera staying visible, two consecutive restarts); the tie-break design call, made without a boss to actually test it against.

---

# Part D – Video Voice-Over Script

## Opening
**SAY:** "A survivor game needs an ending. Today, dying actually ends the run: everything freezes, a Game Over screen appears, and Restart gets you a completely clean level."

## Acceptance Criteria
**SAY:** "By the end: the player's death ends the run exactly once; everything stops except the camera and the UI; a screen shows how long you survived and your XP; Restart reloads cleanly; and a simultaneous win and loss always resolves to a loss."

## Engineering Principle
**SAY:** "Reuse, don't reinvent. The pause menu already knows how to freeze the game and show UI over it. The loss screen uses exactly the same mechanism — it doesn't need its own."

## Step-by-Step Demonstration

### 1. Read the engine before writing "stop everything"
- **SAY:** "Before writing any freeze logic, I checked what the engine's own pause actually does."
- **SHOW:** `LevelTick.cpp`'s `TimerManager.Tick` and actor-tick guards next to `UGameplayStatics::SetGamePaused`. *(media 02)*

### 2. Connect death to the run
- **SAY:** "The health component already marks itself dead and fires an event. The player just needed to listen."
- **SHOW:** `AGrantMoneyPlayerCharacter::HandleHealthDepleted`. *(media 03)*

### 3. The tie-break rule
- **SAY:** "If a boss dies and the player dies in the same instant, who wins? Here: death wins, and it's provable regardless of which event fires first, because the health component marks itself dead before anything hears about it."
- **SHOW:** `ARunController::RequestWin`'s `IsDead()` check, and the test that kills the player before ever calling `RequestWin`. *(media 04)*

### 4. The Game Over screen
- **SAY:** "Same widget pattern as the pause menu — survival time, XP, Restart, Quit."
- **SHOW:** `GameOverWidget.cpp` next to `PauseMenuWidget.cpp`. *(media 05)*

### 5. Tests
- **SAY:** "Three new tests, including one that kills a real player character with a real attacking enemy and checks the damage cadence precisely, down to the exact second."
- **SHOW:** the test run. *(media 06)*

### 6. Play it
- **DO:** Create `WBP_GameOver` on screen; Play; die to a slime; show the frozen world, the screen, Restart, a second death, Restart again. *(media 07 to 12)*

## Common Mistakes
**SAY:** "One to watch: a forward-declared type isn't enough to assign into a weak pointer — the compiler needs to see the real class to know it's even a UObject."

## Verification Checkpoint
**SAY:** "Tests green, then the PIE table. The developer tested this one directly rather than me guessing at headless timing — faster, and conclusive."

## Next Milestone
**SAY:** "Next: a boss, which is the first thing that will ever call RequestWin — and the tie-break rule is already waiting for it."

---

# Media Capture List

Store under `Media/SCRUM-44/` (outside the repository). Only record what actually happens.

| # | File | Type | Show | Proves |
|---|---|---|---|---|
| 01 | `01-branch.png` | Screenshot | `git branch --show-current` | Branch (decide per §12) |
| 02 | `02-engine-pause-source.png` | Screenshot | `LevelTick.cpp`'s `TimerManager.Tick(DeltaSeconds)` guarded by `!bIsPaused` | Why pause alone freezes everything |
| 03 | `03-death-binding.png` | Screenshot | `GrantMoneyPlayerCharacter.cpp` `HandleHealthDepleted` | Death connects to the run |
| 04 | `04-tiebreak.png` | Screenshot | `RunController.cpp` `RequestWin`'s `IsDead()` check | Deterministic tie-break |
| 05 | `05-gameover-widget.png` | Screenshot | `GameOverWidget.h`/`.cpp` next to `PauseMenuWidget.h`/`.cpp` | Same conventions reused |
| 06 | `06-tests-pass.png` | Screenshot | `run_tests.py` output, 103/103 | Tests green |
| 07 | `07-create-wbp-gameover.mp4` | Clip | Creating `WBP_GameOver`, setting its parent class, naming the four widgets | Editor setup (§6A) |
| 08 | `08-death-freezes-everything.mp4` | Clip | Slime kills the player (or `GrantMoney.DamagePlayer`); everything stops at once | Stop requirement |
| 09 | `09-gameover-screen.mp4` | Clip | The screen itself: survival time, XP, Restart, Quit | The screen's content |
| 10 | `10-no-pause-menu-over-gameover.mp4` | Clip | Press Escape while Game Over is showing | `TogglePauseMenu` no-op |
| 11 | `11-restart-clean.mp4` | Clip | Click Restart; show full health, 0 XP, empty level, `3:00`, `Starting` | Clean restart |
| 12 | `12-second-restart.mp4` | Clip | Die again, Restart again; Output Log shows no duplicate-binding warnings | Two consecutive restarts |
