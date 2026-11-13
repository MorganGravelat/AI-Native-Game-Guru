# SCRUM-45 – Readable Gameplay HUD
## Module 3 Sprint 1 – Development and Recording Guide

This guide covers SCRUM-45: the placeholder canvas readouts are replaced by **one gameplay HUD** that shows
health, the survival clock, score, level, XP, buffs, the upgrades taken and the boss fight, and that
cooperates with the pause menu, level-up choice, Game Over and Victory screens instead of fighting them for
input. The HUD is built entirely in C++ (UMG widgets created in code), so there is no Blueprint asset to make.

The same Copilot session then went on to finish SCRUM-39 (reward totems). That work is documented in the
updated `SCRUM-39-ScriptDocumentation.md`; this document only covers the parts of it that changed the HUD
(the buff panel's data source, permanent buffs, and the buff toast).

- **Part A:** change report
- **Part B:** developer implementation guide
- **Part C:** AI workflow log
- **Part D:** video voice-over script
- **Media Capture List:** every screenshot and clip to record, with file names

**Status:** code and tests are committed on branch `SCRUM-45-HUD` (commits `ada819a` "Hud Template for
vertical slice" and `e6b32ac` "Finished hud and tweaks on Totems"). **The first HUD build passed 24
targeted tests** (`GrantMoney.UI` 10/10, plus the buff component and upgrade history tests). After the
SCRUM-39 changes that touched the HUD, **the full suite passed 192/192**. The developer play-tested the HUD
through one yes/no quiz round at 1920×1080 and in a small window and **confirmed every item**. The buff toast
was added after that round and has not been confirmed on its own (§9, Pending).

| Kind | Meaning |
|---|---|
| Read from source | Taken from reading the project's committed source or git history. Not a test. |
| Compiled | UnrealBuildTool finished with `Result: Succeeded`. Says nothing about behaviour. |
| Automation test | A `GrantMoney.*` Automation test passed in a headless editor run, in a synthetic game world. |
| Developer reported | The developer played it in the Editor and answered the AI's yes/no quiz. |

---

## Jira

| Key | Goal |
|---|---|
| SCRUM-45 | Consolidate the placeholder UI into one readable gameplay HUD: player current/max health, remaining survival time, score, level and XP progress; in BossPhase the phase label and boss health; temporary buff status and remaining time when available. Read values from the existing components and run controller, using their change events and refreshing initial values on bind. Handle player replacement and restart without duplicate widgets or stale subscriptions. Make the level-up choices, pause menu, Game Over and Victory screens cooperate on input focus and visibility; a finished run must not reopen upgrade selection or resume combat through the pause menu. Basic panels, bars, text and buttons with readable contrast. Verify at 1920×1080 and a smaller window, including pause, upgrade selection, death, victory and restart. |

**Done when, and where each one is met:**

| Criterion | Met by | Evidence |
|---|---|---|
| Health current/max, survival time, score, level, XP | `UGameplayHudWidget`: level, red HP bar "cur / max", blue XP bar (top-left); clock (red in the last 10 s) and phase (top-centre); score (top-right) | `UI.GameplayHud.BindingShowsCurrentValuesAtOnce`, `EventsUpdateHealthXpClockAndScore`; developer reported |
| Score | **No score existed.** Developer chose a mix of XP and time: `FRunHudFormat::ComputeScore` = XP × 10 + seconds survived × 5 | `UI.RunHudFormat.ScoreMixesXpAndTime` |
| BossPhase label and boss health | Phase line turns red; "BOSS ARRIVED", spawn diagnostic and a bottom-centre boss health bar (moved over from the old canvas HUD) | Developer reported |
| Buff status and remaining time | BUFFS panel: timed buffs in green with a countdown, permanent totem buffs in gold with only a name; a toast under the clock names the buff and the stat it changed | `UI.GameplayHud.ListsBuffsWithTimeAndUpgradesTaken`, `UI.RunHudFormat.BuffsUpgradesAndPhase`; first round developer reported, toast pending |
| Change events + initial refresh on bind | `BindRun` / `BindPlayer` subscribe to health, stats, level, upgrade, effect and run events, then call the refresh functions at once | `BindingShowsCurrentValuesAtOnce` |
| Player replacement and restart, no duplicates or stale subscriptions | `ARunHUD` owns exactly one widget for its lifetime; the widget unbinds the old player/run before binding a new one, and re-resolves sources when they are gone | `UI.GameplayHud.ReplacedPlayerAndRunLeaveNoStaleSubscriptions`; developer reported one HUD after restart |
| Screens cooperate on focus and visibility | HUD on viewport layer 0 and ignores the mouse; pause menu 50, level-up 60, Game Over/Victory 70 | Developer reported |
| Finished run: no upgrade choice, no resume through pause | `FRunUiPolicy` rules, asked by `AMapCameraPlayerController` before it pauses, unpauses or changes input; pause menu Resume goes through `ResumeFromPauseMenu()` | `UI.RunUiPolicy.*` (3 tests); developer reported |
| Readable contrast; 1920×1080 and a small window | Panels, bars and text built in C++ | Developer reported (both sizes) |

**Related tickets:** SCRUM-39 (reward totems and the effect system the buff panel reads), SCRUM-42/43 (boss
UI), SCRUM-44 (Game Over), SCRUM-36 (levels, XP and upgrades), SCRUM-33 (health).

---

# Part A – Change Report

## Milestone Summary

The old `ARunHUD` drew text straight onto the canvas. It is now a thin owner that creates one
`UGameplayHudWidget` and removes it when it ends. The widget builds its own layout in C++, follows the
player, run controller and boss encounter through their events, and refreshes everything when it binds, so
nothing is blank for a frame. A small set of pure rules (`FRunUiPolicy`) decides which screen may pause,
unpause or take input, so a finished run can't be resumed or interrupted by a level-up. The HUD also lists
the upgrades taken and, after the SCRUM-39 work, the totem buffs and a short toast when one is gained.

## Files Created

| File | Purpose |
|---|---|
| `Public/GameplayHudWidget.h` / `Private/GameplayHudWidget.cpp` | The HUD widget: layout built in code, bind/unbind, refresh functions, event handlers, toast. |
| `Public/RunHudFormat.h` / `Private/RunHudFormat.cpp` | Pure text and number rules: score formula, health/XP/level/buff/upgrade/phase text, stat names and magnitudes for the toast. |
| `Public/RunUiPolicy.h` / `Private/RunUiPolicy.cpp` | Pure screen rules: what is terminal, when the pause menu or a level-up may open, when Resume may unpause. |
| `Private/GameplayHudWidgetTests.cpp` | 4 widget tests (bind, events, buffs/upgrades, rebinding). |
| `Private/RunHudFormatTests.cpp` | 3 formatting tests. |
| `Private/RunUiPolicyTests.cpp` | 3 policy tests. |

`TemporaryBuffComponent` (+ tests) was created in the first commit and **deleted** in the second, when the
SCRUM-39 work showed the project already had `UTemporaryEffectComponent` (§6.1).

## Files Modified

| File | Change |
|---|---|
| `Public/RunHUD.h` / `Private/RunHUD.cpp` | Canvas drawing removed; now creates and removes one `UGameplayHudWidget` (`GameplayHudClass`). |
| `Public/MapCameraPlayerController.h` / `.cpp` | Asks `FRunUiPolicy` before pausing/unpausing; `ResumeFromPauseMenu()`, `IsRunFinished()`, `bResultShown`; z-orders for pause (50), level-up (60) and results (70); a late level-up is refused once the run is finished; fixed the level-up screen keeping a stale reference after the player is replaced. |
| `Private/PauseMenuWidget.cpp` | Resume calls the controller's `ResumeFromPauseMenu()` instead of unpausing itself. |
| `Public/UpgradeComponent.h` / `.cpp` | `GetPickedUpgrades()` (pick order with stack counts), `FGrantMoneyPickedUpgrade`, `OnUpgradeChosen` event. |
| `Private/UpgradeComponentTests.cpp`, `Private/Tests/ProgressionTestListener.h` | New `PickedUpgradesListInPickOrderWithStacks` test and listener support. |
| `Public/RunController.h` | Comment only: the score lives in the HUD, derived from XP and elapsed time. |
| `Public/GrantMoneyPlayerCharacter.h` / `.cpp` | First commit added the buff component; second commit removed it again. |
| `Private/BossDebugCommands.cpp` | New `GrantMoney.GiveXp <n>` and `GrantMoney.GiveBuff <seconds>`; later `GrantMoney.GiveFrenzy` and `GrantMoney.GiveTotemBuff` (SCRUM-39). Compiled out of shipping builds. |

---

# Part B – Developer Implementation Guide

## 1. Prerequisites

- Unreal Engine 5.8, Editor **closed** when running `scripts/run_tests.py` (it holds the build output; the
  AI asked the developer to close it several times in this session).
- The run controller, health, stats, level, upgrade and boss encounter systems from earlier tickets.

## 2. Architecture Overview

```
AGrantMoneyGameMode → HUD class = ARunHUD
ARunHUD::BeginPlay → CreateWidget<UGameplayHudWidget> → AddToViewport(0)   (one per HUD, removed in EndPlay)

UGameplayHudWidget
   EnsureLayout()  builds panels, bars and text once, in C++
   ResolveSources() finds the player pawn, ARunController, ABossEncounter, AEnemySpawner (re-checks when gone)
   BindPlayer(Player)  → UHealthComponent, UStatsComponent, ULevelComponent, UUpgradeComponent,
                         UTemporaryEffectComponent events      (unbinds the previous player first)
   BindRun(Run)        → OnTimeChanged, OnStateChanged        (unbinds the previous run first)
   each bind → RefreshAll()  so values show at once
   text and numbers via FRunHudFormat (pure, tested)

AMapCameraPlayerController  (asks FRunUiPolicy first)
   layer 50 pause menu   – not over a level-up, not after the run is finished
   layer 60 level-up     – refused once the run is finished
   layer 70 Game Over / Victory – sets bResultShown; only Restart / Quit leave it
   UPauseMenuWidget Resume → ResumeFromPauseMenu() → refused if finished or a level-up is pending
```

## 3. AI-Assisted Planning

### AI Prompt
The developer's testing and verification rules, "claude will handle documentation externally... only
worry about functionality", and the SCRUM-45 brief verbatim.

### Expected AI Output
Inspection of the existing UI (`RunHUD`, `GameOverWidget`, `PauseMenuWidget`, `LevelUpWidget`), the player
controller, game mode, run controller and the components the HUD must read; then questions for anything
the brief asks for that doesn't exist yet.

### Human Review
The AI found two things the brief asks for that didn't exist, and asked before building:
- **Score:** nothing in the code kept a score (comments said level was the progress readout). The developer
  chose a mix of XP and time. The AI picked the weights (XP × 10 + seconds × 5) and said so; they are two
  constants in `RunHudFormat.h`.
- **Buff durations:** stat modifiers had no durations. The AI built a small timed-buff component. This turned
  out to duplicate SCRUM-39's existing effect system and was removed later (§6.1).

## 4. Step-by-Step Implementation

### Step 1 – Inspect before building
The AI listed the source tree, read `CLAUDE.md` and `Useful.md`, then the four existing UI classes, the
controller, game mode, run controller, `CombatEventsSubsystem`, `StatsComponent`, `HealthComponent`,
`LevelComponent`, `UpgradeComponent`, the pickup classes and the existing tests and console commands.

### Step 2 – Upgrade history
`UUpgradeComponent::GetPickedUpgrades()` and `OnUpgradeChosen`, so the HUD can list what was taken, with ×N
for repeats and "(max)" at a stack limit.

### Step 3 – Pure helpers first
`FRunHudFormat` (score and every piece of HUD text) and `FRunUiPolicy` (screen rules) have no widgets and no
world, so each rule is tested headlessly and only the layout is left to the eye.

### Step 4 – Screen cooperation in the controller
The controller asks `FRunUiPolicy` before pausing, unpausing or changing the input mode, gives each screen
its layer, and remembers once a result screen has been shown. The pause menu's Resume now goes through the
controller.

### Step 5 – Checking the real UMG API
Before writing the widget, the AI read the installed engine's `ProgressBar.h`, `TextBlock.h`, `Border.h`,
`CanvasPanelSlot.h` and `UserWidget.cpp` (`Initialize`) to use the exact setters instead of guessing.

### Step 6 – The widget
`UGameplayHudWidget` builds its layout in `EnsureLayout()`, binds and refreshes as above, and is set to
ignore the mouse. `ARunHUD` was rewritten to own one instance of it.

### Step 7 – Tests
`python scripts/run_tests.py --filter GrantMoney.UI` (first build failed, see §6.3), then
`--skip-build` runs for the buff component and `GrantMoney.Progression.UpgradeComponent`.

### Step 8 – Developer quiz
11 yes/no questions plus a notes field, at 1920×1080 and in a small window. The developer was asked to
restart the Editor first, because a component had been added to the player.

### Step 9 – Follow-ups from the SCRUM-39 work (second commit)
- The buff panel now reads `UTemporaryEffectComponent` (`OnEffectApplied` / `OnEffectEnded`).
- **Toast:** when a buff is gained, a gold line under the clock names it and its stat change, for example
  "Frenzy  Attack Speed +30%", for 3 seconds. Attack interval is shown as "Attack Speed" with the sign
  flipped, so a buff always reads as a positive number.
- **Permanent buffs:** `FormatBuffLine(..., bPermanent)` shows just the name, and the panel draws
  permanent buffs in gold and timed ones in green with a countdown.

## 5. AI Prompts by Stage

| Stage | Prompt |
|---|---|
| Scope | "Lets keep going, claude will handle documentation externally for you now... only worry about functionality." |
| Request | "Consolidate the placeholder UI into one readable gameplay HUD..." (full brief) |
| Score and buffs | The AI's question: score source (kills, XP, or skip) and buffs (build a timed-buff API, or hide the panel). |
| Totem feedback | "Plus something in the hud to maybe pop up and say what stat was buffed" (asked during SCRUM-39). |
| Permanent buffs | "Show it on the side bar like it does now but instead of counting it down it just stays there as a permanent random buff." |

## 6. Common Mistakes

### 6.1 Building a buff system that already existed
- **Symptom:** none at first. The HUD's timed-buff component worked and was tested (part of the 24 passing tests).
- **Cause:** when the AI searched for `Buff|Temporary|Duration` for SCRUM-45, it concluded that "stat modifiers have no durations" and that a Temporary Buff pickup existed only as a code comment. SCRUM-39's `UTemporaryEffect` / `UTemporaryEffectComponent` (merged in PR #37 just before) was missed.
- **Fix:** found during SCRUM-39. `TemporaryBuffComponent` and its tests were deleted, and the HUD and test helpers were moved over to the effect system.
- **Lesson:** after a merge, search for the concept as well as the words. Here, "effect" would have found the existing system.

### 6.2 Run the full suite only for a reason
- The AI ran the full `GrantMoney` suite three times during the SCRUM-39 rounds (188, 188, 192), saying the
  round "touched several shared files". `CLAUDE.md` asks for targeted runs, with the full suite only when a
  targeted run fails. Note the reason whenever the wider run is used.

### 6.3 First UI build failed
- `run_tests.py --filter GrantMoney.UI` exited with code 2 on the first build. The AI made four edits and the
  rebuild passed 10/10. The compiler message isn't in the transcript.

### 6.4 Tool slips
- Several edits failed with "No match found" or "Tool 'str_replace' does not exist" and were redone with a
  fresh read of the file. None of them changed the outcome.

## 6A. Designer Setup

- **Nothing to create in the Editor.** `ARunHUD` creates the widget; there is no Blueprint asset.
- **Restyle:** `ARunHUD::GameplayHudClass` can point at a Blueprint child of `UGameplayHudWidget`.
- **Score weights:** `FRunHudFormat::ScorePerXp` (10) and `ScorePerSurvivalSecond` (5).
- **Toast time:** `UGameplayHudWidget::ToastVisibleSeconds` (3 s).
- **Testing commands:** `GrantMoney.GiveXp <n>`, `GrantMoney.GiveBuff <seconds>` (timed +50% move speed, debug
  only), `GrantMoney.GiveTotemBuff [Swiftness|Might|Frenzy|Piercing]`, `GrantMoney.GiveFrenzy`,
  `GrantMoney.PlayerHealth <n>`, `GrantMoney.SkipToBoss`, `GrantMoney.BossHealth <n>`.

## 7. AI Workflow Notes

Manual verification used the yes/no quiz method required by `CLAUDE.md`. Individual answers are not
transcribed here.

## 8. Known Limitations

- **Score is a guess at "a mix of XP and time".** The weights are the AI's and haven't been tuned.
- **The enemy count readout from the old HUD was kept** in the top-centre column. The brief didn't ask for it.
- **Layout is code, not a designer asset.** Moving things means editing `EnsureLayout()` or making a Blueprint child.
- **Nothing in play grants a timed buff now.** Totems give permanent buffs (SCRUM-39), so the green countdown
  rows only appear through `GrantMoney.GiveBuff` until a timed source (the planned rare enemy drop) exists.

## 9. Verification

### Performed

| Test | Action | Result | Kind |
|---|---|---|---|
| UI build | `run_tests.py --filter GrantMoney.UI` | Failed once (exit code 2), then succeeded | Compiled |
| UI suite | `GrantMoney.UI` | **10/10** | Automation test |
| Buff component and upgrade history | `--skip-build` runs | Passed; 24 targeted tests in total | Automation test |
| After the SCRUM-39 changes | `GrantMoney.UI` (with Effects and Rewards) | Passed | Automation test |
| Full suite (SCRUM-39 rounds) | `--filter GrantMoney` | 188/188, 188/188, then **192/192** | Automation test |
| PIE quiz | 1920×1080 and a small window | **Every item confirmed:** layout readable at both sizes; GiveXp raises XP, level and score, and score grows with time; timed buff row counts down, disappears and refreshes instead of duplicating; upgrade list with ×N; level-up above the HUD, freezes the game and Esc doesn't open pause; pause and Resume; boss phase, BOSS ARRIVED and the boss bar; Game Over with no pause and no level-up over it; Victory with no resume; Restart gives one fresh HUD and pause works again | Developer reported |
| Permanent buffs in the panel (SCRUM-39 round) | Quiz | Confirmed: gold row, name only, no countdown; stays through pause and level-up, gone after restart; `GiveBuff` still green with a countdown | Developer reported |

**What the tests cover:**

| Area | Tests |
|---|---|
| Widget | `UI.GameplayHud.BindingShowsCurrentValuesAtOnce`, `EventsUpdateHealthXpClockAndScore`, `ListsBuffsWithTimeAndUpgradesTaken` (includes permanent buffs and toast text), `ReplacedPlayerAndRunLeaveNoStaleSubscriptions` |
| Formatting | `UI.RunHudFormat.ScoreMixesXpAndTime`, `NumbersRead`, `BuffsUpgradesAndPhase` |
| Screen rules | `UI.RunUiPolicy.OnlyWonAndLostAreTerminal`, `FinishedRunAllowsNoPauseMenuNoUpgradesNoResume`, `RunningGameKeepsNormalFlow` |
| Upgrade history | `Progression.UpgradeComponent.PickedUpgradesListInPickOrderWithStacks` |

### Pending (developer, in the Editor)

| Item | Why |
|---|---|
| The buff toast appears under the clock, names the stat change, and fades after a few seconds | Asked in the first SCRUM-39 quiz, but that round's answers aren't in the transcript, and the later round didn't ask again. |
| HUD at a small window size after the SCRUM-39 changes (gold rows, toast) | The size check was done before those changes. |

## 10. Extension Ideas

- Tune the score weights, or add kills to the score through `UCombatEventsSubsystem`.
- A designer-editable Blueprint child for layout and fonts.
- Icons for buffs and upgrades instead of text rows.
- A timed buff source (rare enemy drop) to use the green countdown rows in real play.

## 11. Source-Control Checkpoint

| Item | Value |
|---|---|
| Jira | SCRUM-45 (with SCRUM-39 follow-ups) |
| Branch | `SCRUM-45-HUD` |
| Commits | `ada819a` Hud Template for vertical slice; `c7bc516` merge of `main`; `e6b32ac` Finished hud and tweaks on Totems |
| PR | Not opened yet at the time of writing |

The AI did not commit; the developer committed.

---

# Part C – AI Workflow Log

**Tool:** Copilot (an AI assistant in VS Code), working on the repository on the developer's PC.

| # | Purpose | Prompt (developer) | Result | Decision | Verification |
|---|---|---|---|---|---|
| 1 | Scope and rules | "claude will handle documentation externally"; testing philosophy | Saved a memory note not to write docs | Accepted | — |
| 2 | Request | SCRUM-45 brief | Read the UI classes, controller, game mode, run controller, components, tests | Accepted | Read from source |
| 3 | Missing pieces | AI asked: score source, buff durations | Developer chose XP-and-time score and a timed-buff API | Developer decided | — |
| 4 | Implementation | (continued) | Upgrade history, buff component, `FRunHudFormat`, `FRunUiPolicy`, controller rules, widget, `ARunHUD` rewrite, tests; UMG API read from engine headers | Accepted | Compiled after one failed build |
| 5 | Targeted tests | (automatic) | UI 10/10; 24 targeted tests in total | Accepted | Automation test |
| 6 | PIE quiz | 11 yes/no items at two sizes | All confirmed | Accepted | Developer reported |
| 7 | SCRUM-39 (same chat) | Totems, readable bar, "something in the hud to maybe pop up and say what stat was buffed" | Found the existing effect system; deleted the duplicate buff component; HUD moved to effects; toast added | Accepted | UI, Effects, Rewards passed; full 188/188 |
| 8 | Permanent buffs | "Instead of counting it down it just stays there as a permanent random buff" | `FormatBuffLine` permanent mode; gold rows | Accepted | Full 192/192; developer reported |

**Where AI helped most:** finding the gaps (no score, no buff durations) before building and asking about
them; splitting the HUD's rules into pure code that can be tested without looking at the screen.

**Where it needed a human:** the score definition, everything about readability at both window sizes, and
catching (indirectly, through SCRUM-39) that the buff system already existed.

---

# Part D – Video Voice-Over Script

## Opening
**SAY:** "Our HUD was debug text on the canvas. Today it becomes one readable HUD that also plays nicely
with every menu in the game."

## Acceptance Criteria
**SAY:** "Health, clock, score, level, XP, buffs and the boss, all live; and once a run is over, nothing
can reopen an upgrade choice or resume combat."

## Engineering Principle
**SAY:** "Keep the rules out of the widget. The score, the text and who may pause are plain functions with
tests. The widget just shows them."

## Step-by-Step Demonstration

### 1. Ask before inventing
- **SAY:** "The ticket asks for a score and buff timers. Neither existed, so the AI asked instead of
  guessing."
- **SHOW:** the quiz question and `ComputeScore`. *(media 02)*

### 2. The rules
- **SAY:** "These few functions decide which screen may pause, unpause or take input."
- **SHOW:** `RunUiPolicy.h`. *(media 03)*

### 3. Bind, then refresh
- **SAY:** "When the HUD binds to a player or run, it unsubscribes from the old one and fills every value
  straight away. Restart a dozen times and there's still exactly one HUD."
- **SHOW:** `BindPlayer`. *(media 04)*

### 4. Tests
- **SHOW:** `run_tests.py --filter GrantMoney.UI`. *(media 05)*

### 5. Play it
- **DO:** at 1080p and a small window: GiveXp, level-up, pause, boss, death, victory, restart.
  *(media 06 to 10)*

### 6. Buffs
- **SAY:** "Totem buffs are permanent now, so they sit in gold. A timed buff counts down in green, and a
  toast tells you what you just got."
- **SHOW:** `GrantMoney.GiveTotemBuff Frenzy`, then `GrantMoney.GiveBuff 15`. *(media 11)*

## Common Mistakes
**SAY:** "Search for the idea, not just the word. We built a buff system the project already had, because
the search looked for 'buff' and the existing one was called 'effect'."

## Verification Checkpoint
**SAY:** "Tests cover the rules and the bindings. Readability at both sizes was checked by eye."

## Next Milestone
**SAY:** "With the HUD in place, the vertical slice reads like a game from first spawn to Victory."

---

# Media Capture List

Store under `Media/SCRUM-45/` (outside the repository). Only record what actually happens.

| # | File | Type | Show | Proves |
|---|---|---|---|---|
| 01 | `01-branch.png` | Screenshot | `git branch --show-current` on `SCRUM-45-HUD` | Branch |
| 02 | `02-score.png` | Screenshot | `FRunHudFormat::ComputeScore` and its constants | Score definition |
| 03 | `03-ui-policy.png` | Screenshot | `RunUiPolicy.h` | Screen rules |
| 04 | `04-bind-refresh.png` | Screenshot | `BindPlayer` / `BindRun` | Initial refresh, no stale subscriptions |
| 05 | `05-tests-pass.png` | Screenshot | `run_tests.py --filter GrantMoney.UI` | Tests green |
| 06 | `06-hud-1080p.png` | Screenshot | Full HUD at 1920×1080 | Layout and contrast |
| 07 | `07-hud-small.png` | Screenshot | Same HUD in a small window | Nothing cut off |
| 08 | `08-levelup-pause.mp4` | Clip | Level-up over the HUD; Esc does nothing; pause and Resume afterwards | Screen cooperation |
| 09 | `09-boss-ui.mp4` | Clip | SkipToBoss: red phase, BOSS ARRIVED, boss bar draining | Boss readouts |
| 10 | `10-results-restart.mp4` | Clip | Death → Game Over (Esc ignored) → Restart; Victory → Restart; one HUD each time | Terminal runs, no duplicates |
| 11 | `11-buffs-toast.mp4` | Clip | Totem buff (gold row + toast) and a timed buff (green countdown) | Buff panel and toast |
