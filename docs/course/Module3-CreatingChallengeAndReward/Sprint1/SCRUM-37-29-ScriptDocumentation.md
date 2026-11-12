# SCRUM-37 / SCRUM-29 – Escalating Pressure and Usable Terrain Traversal
## Module 3 Sprint 1 – Development and Recording Guide (combined milestone)

This guide covers two related tickets in one pass: **SCRUM-37** (making the three-minute survival phase
progressively harder) and **SCRUM-29** (enemies climbing over ordinary obstacles and each other while
pursuing, instead of getting stuck). Kept intentionally light: most of SCRUM-37's required mechanism
already existed from the SCRUM-31/30/38/42 milestone; this pass adds the one missing piece (a default
ease-in shape) and tunes/documents the rest. SCRUM-29 is almost entirely configuration of Unreal's own
`UCharacterMovementComponent`, plus one real engine default that would have silently blocked it.

- **Part A:** change report
- **Part B:** developer implementation guide
- **Part C:** AI workflow log
- **Part D:** video voice-over script
- **Media Capture List:** every screenshot and clip to record, with file names

**Status:** the code and tests are written. **`GrantMoney.Run.EnemySpawner` (10/10) and
`GrantMoney.Combat.EnemyBase` (7/7) pass**, including the new tests for this work (run by the AI session
on this machine, 2026-11-12). The full suite was not re-run this pass (nothing outside these two areas
was touched; see the developer's note in Part C). **Nothing in this milestone has been tried on the real
demo map yet** — the developer will verify spawn pacing and actual climbing/stacking there directly (see
§9, Pending). Values below are marked **(estimate)** where they have not been observed in a real PIE run.

**What is and is not evidence in this document:**

| Kind | Meaning |
|---|---|
| Read from source | Taken from reading project or **engine** source (this pass reads Unreal's own `Character.cpp`/`CharacterMovementComponent.cpp` directly — see §8). Not a test. |
| Compiled | UnrealBuildTool finished with `Result: Succeeded`. Says nothing about behaviour. |
| Automation test | A `GrantMoney.*` Automation test passed in a headless editor run, in a synthetic game world. |
| Developer reported | The developer played it in the Editor and said what they saw. |

At the time of writing, the first three kinds apply; "Developer reported" is pending (§9).

---

## Jira

| Key | Goal |
|---|---|
| SCRUM-37 | Make the three-minute survival phase progressively harder: editable stages/curve for spawn interval, batch size and active-enemy cap; begin gently, ramp through the middle, busier at the end; keep the minimum safe spawn distance; configure the existing spawner through its interface with no duplicate timers, bursts or cap breaches; freeze while paused; stop escalating in BossPhase. |
| SCRUM-29 | Complete direct pursuit across ordinary terrain and simple obstacles via constrained stepping/sliding upward (climb over obstacles and stack on other enemies, Megabonk-style, sliding to the side instead when that is shorter); expose the traversal limits; prevent uncontrolled upward launches or escaping the playable boundary. |

**Done when, and where each one is met:**

| Criterion | Met by | Evidence |
|---|---|---|
| Editable stages/curve for interval, batch size, cap | `AEnemySpawner`'s existing `*AtStart`/`*AtEnd` pairs plus `DifficultyRampSeconds`/`DifficultyCurve` (already built, SCRUM-31/30/38/42 milestone) | Pre-existing `EnemySpawner.DifficultyRampsOverTime` |
| Begin gently, ramp through the middle, busier at the end | **New:** `DifficultyCurveExponent` (default 1.6) eases `GetDifficultyAlpha()` in by default, with no curve asset needed | `EnemySpawner.DifficultyEasesInByDefault` (new) |
| Minimum safe spawn distance maintained | Unchanged: `MinSpawnDistance`/`MaxSpawnDistance` are independent of difficulty and untouched by this pass | Pre-existing spawn-ring tests |
| Configured through the spawner's interface; no duplicate timers, bursts or cap breaches | Unchanged, already correct: a single `SpawnTimer` float, reset (not accumulated) when spawning is disallowed, one batch per due interval even after a long frame (`AdvanceSpawning`) | Read from source; pre-existing tests exercise this path |
| Freeze while paused; stop escalating in BossPhase | Unchanged, already correct: `AdvanceSpawning` only runs when the world is not paused, and `ARunController::GetElapsedSeconds()` (what difficulty follows) is itself frozen outside the `Active` state | Read from source (`ARunController::GetElapsedSeconds` doc comment; `AEnemySpawner::Tick`) |
| Pursuit across ordinary terrain/obstacles via constrained stepping/sliding | `UCharacterMovementComponent`'s own step-up and slope-walking, tuned via two new exposed properties (`MaxStepHeight`, `MaxWalkableFloorAngle`) | `EnemyBase.TraversalLimitsApplyToMovement` (new) |
| Climbing over/stacking on other enemies (Megabonk-style) | **New, and the actual fix this milestone needed:** `GetCapsuleComponent()->CanCharacterStepUpOn` turned back on — `ACharacter`'s own constructor turns it off by default (see §8) | Same test; real stacking depth is a PIE check (§9, Pending) |
| Traversal limits exposed; no uncontrolled upward launches; cannot escape the boundary | `MaxStepHeight`/`MaxWalkableFloorAngle` are editable `UPROPERTY`s; no custom vertical velocity is ever added (pursuit is 100% `AddMovementInput`, horizontal only — the engine's own bounded step-up algorithm does the lifting), and `MaxStepHeight` is kept well under the boundary wall's height by default | Read from source; the height relationship is documented in both files |
| Sliding to the side when shorter | Unchanged engine behaviour: `UCharacterMovementComponent` already slides along a blocking surface it cannot step up or over | Read from source (no new code — nothing to test) |

**Related tickets:** SCRUM-31/30/38/42 (the spawner and run controller this builds on), SCRUM-36 (leveling,
unrelated but sharing the same spawner/run-controller foundation).

---

# Part A – Change Report

## Milestone Summary

Two independent, small changes. SCRUM-37: the spawner's difficulty ramp (built previously) now eases in
by default — noticeably gentler than a straight line through the first half of the ramp, reaching full
intensity only as `DifficultyRampSeconds` closes in — without needing a curve asset. SCRUM-29: enemies
can now climb over ordinary obstacles and stack on top of each other while pursuing, using Unreal's own
character movement stepping rather than any custom physics, tuned to climb a full slime but never the
map's boundary wall, with one real engine default (`CanCharacterStepUpOn`) found and corrected along the
way.

## Files Modified

| File | Change |
|---|---|
| `Public/EnemySpawner.h`/`.cpp` | Added `DifficultyCurveExponent` (default 1.6): when `DifficultyCurve` is not set, `GetDifficultyAlpha()` raises the straight-line ramp fraction to this power instead of using it directly. 1.0 is the old straight-line behaviour. |
| `Public/EnemyBase.h`/`.cpp` | Added `MaxStepHeight` (90 cm) and `MaxWalkableFloorAngle` (58°), applied to `GetCharacterMovement()` every tick in `MoveTowardTarget` (same "tune while playing" pattern as `MoveSpeed`). Constructor now sets `GetCapsuleComponent()->CanCharacterStepUpOn = ECB_Yes` (see §8). |
| `Private/EnemySpawnerTests.cpp` | Existing `DifficultyRampsOverTime` test now pins `DifficultyCurveExponent = 1.0f` (it tests the raw Lerp math). Added `DifficultyEasesInByDefault` (new). |
| `Private/EnemyBaseTests.cpp` | Added `TraversalLimitsApplyToMovement` (new). |
| `docs/course/Module3-CreatingChallengeAndReward/Sprint1/SCRUM-37-29-ScriptDocumentation.md` | This document. |

No new files, no Editor-side setup, no new assets. Both tickets are pure C++ configuration.

---

# Part B – Developer Implementation Guide

## 1. Prerequisites

- Unreal Engine 5.8, Editor **closed** when running `scripts/run_tests.py` (Live Coding breaks the build).

## 2. Architecture Overview

```
SCRUM-37 (unchanged plumbing, new shape):
ARunController::GetElapsedSeconds() [frozen while paused or outside Active]
        │
        ▼
AEnemySpawner::GetDifficultyAlpha()
        Linear = clamp(Elapsed / DifficultyRampSeconds, 0, 1)
        DifficultyCurve set?  → DifficultyCurve->GetFloatValue(Linear)
        else                  → Linear ^ DifficultyCurveExponent   (NEW: default 1.6, eases in)
        │
        ▼
GetCurrentSpawnInterval / GetCurrentBatchSize / GetCurrentMaxActiveEnemies
        (Lerp AtStart..AtEnd by the alpha above — unchanged)

SCRUM-29:
AEnemyBase::Tick → MoveTowardTarget → AddMovementInput(flat direction toward target)
        │
        ▼
UCharacterMovementComponent (built-in, not custom):
   blocked by a low obstruction / another enemy's capsule, within MaxStepHeight
        → steps up onto it automatically (same mechanism as a player walking up stairs)
   blocked by something too tall/steep
        → slides along it instead (automatic; this is why "shorter path to the side" needs no new code)
   WalkableFloorAngle (MaxWalkableFloorAngle) → stays put near the edge of a rounded capsule-top perch
        instead of sliding straight back off
   GetCapsuleComponent()->CanCharacterStepUpOn = ECB_Yes → lets this stepping-up target OTHER ENEMIES'
        capsules at all (off by default on every ACharacter — see §8)
```

## 3. AI-Assisted Planning

### AI Prompt
The shared Module 3 instructions, followed by the SCRUM-37 and SCRUM-29 brief verbatim: progressive
difficulty for the survival phase via editable stages/curve on the existing spawner, maintaining minimum
spawn distance, no duplicate timers/bursts/cap breaches, frozen while paused and in BossPhase; and direct
pursuit across terrain/obstacles via constrained stepping/sliding, exposed traversal limits, no
uncontrolled launches or boundary escapes, with an explicit description of Megabonk-style stacking
(enemies climbing over each other toward the player when crowded, sliding to the side only if shorter).

### Expected AI Output
Inspection of the existing spawner/run-controller/enemy-movement code before writing anything, since
SCRUM-37's core mechanism turned out to already exist; the actual new work, tests, and this document.

### Human Review
- Confirm the default ease-in shape (`DifficultyCurveExponent = 1.6`) *feels* right once the run is
  actually played — this is an estimate, not an observed value (§9).
- Confirm `MaxStepHeight = 90 cm` reliably lets enemies climb over each other on the real map, including
  at its edges, and does not let anything reach the boundary wall. Also an estimate (§9).
- No Editor setup to review — nothing here needs an asset or a Blueprint change.

## 4. Step-by-Step Implementation

### Step 1 – Inspect before building (SCRUM-37)
Reading `AEnemySpawner` first showed `SpawnIntervalAtStart/AtEnd`, `BatchSizeAtStart/AtEnd`,
`MaxActiveEnemiesAtStart/AtEnd`, `DifficultyRampSeconds` and an optional `DifficultyCurve` already fully
implemented and tested (from the SCRUM-31/30/38/42 milestone, when the developer asked for the spawner to
"start spawning slowly and speed up as the level goes on" instead of a fixed rate). The single-timer,
no-burst, freeze-on-pause and freeze-in-BossPhase requirements were all already correct by construction
(see the Done-when table above) — nothing to change there.

### Step 2 – The one real gap: a default shape (SCRUM-37)
The only genuinely missing piece was the "begin gently" part: with no curve asset, the ramp was a plain
straight line, which does not read as "gentle" at the very start. `DifficultyCurveExponent` (default 1.6,
only used when `DifficultyCurve` is unset) raises the straight-line fraction to a power: at the ramp's
halfway point, alpha is about 0.33 instead of 0.5 — noticeably gentler through the middle, catching up
to full intensity only near the end. An exponent of 1 keeps the exact old straight-line behaviour.

### Step 3 – Inspect before building (SCRUM-29)
Reading `AEnemyBase` showed pursuit is already plain `ACharacter`/`AddMovementInput`, horizontal only —
no custom vertical movement anywhere. Reading `ATileMapRenderer` showed the generated map is flat (every
playable cell at the same tile height, with a solid boundary wall, 300 cm tall by default); there is no
literal slope/stair geometry today, so "ordinary terrain changes" in practice means obstructions: static
props/walls, and (per the ticket's explicit Megabonk description) other enemies' capsules.

### Step 4 – Reuse the engine's own stepping, do not hand-roll physics (SCRUM-29)
`UCharacterMovementComponent` already has exactly the bounded "step up or slide" behaviour the ticket
describes (step up within `MaxStepHeight`, walk a slope within `WalkableFloorAngle`, slide along anything
steeper/taller — all built in, all already safe against "uncontrolled launches" since no vertical velocity
is ever directly added, only a bounded sweep-tested step). Two new properties on `AEnemyBase` expose and
tune this per the ticket's "expose the traversal limits" requirement, applied every tick the same way
`MoveSpeed` already is, so they can be tuned live:
- `MaxStepHeight` (90 cm): a slime's own capsule is 60 cm tall (`ASlimeEnemy`: 30 cm radius, 30 cm
  half-height); 90 cm clears one slime with margin, so climbing one enemy is itself another valid "step"
  for the one behind it — stacking several is just that same step repeating, not special-cased.
- `MaxWalkableFloorAngle` (58°): a little above the engine's own default (~44.7°, see §8), so standing
  slightly off-centre on a stacked enemy's rounded top does not immediately slide back off, while staying
  far short of a near-vertical wall.
Both are kept comfortably under the boundary wall's height (300 cm) and well short of a 90° wall slope,
which is how "cannot escape the boundary" and "no uncontrolled launches" are satisfied — through the
existing, already-safe engine mechanism, not a new one.

### Step 5 – A real engine default that would have silently defeated this (SCRUM-29)
See §8. Fixed with one line in the constructor.

### Step 6 – Tests
`python scripts/run_tests.py --filter GrantMoney.Run.EnemySpawner` and
`--filter GrantMoney.Combat.EnemyBase --skip-build`. Results in §9.

## 5. AI Prompts by Stage

| Stage | Prompt |
|---|---|
| Inspection | "Inspect the existing enemy movement for SCRUM-29" and the existing spawner/run timer for SCRUM-37, before writing anything. |
| Difficulty shape | "Begin gently, increase pressure during the middle of the run, and make the final survival section noticeably busier." |
| Traversal | "Complete direct pursuit across ordinary terrain changes and simple obstacles using constrained stepping/sliding upward... climb over any terrain that has collision and even climb over other slimes if they are all grouped up, sliding over each other and toward the player, creating a stack... unless the path to them is shorter just to slide to the side." |
| Safety | "Expose the relevant traversal limits and prevent uncontrolled upward launches or escaping the playable boundary." |
| Verification | "I will Verify this on the actual demo map with several slimes, including at its edges and ordinary obstacles. Document initial tuning values and distinguish observed performance from estimates." |

## 6. Common Mistakes

### 6.1 `ACharacter`'s own constructor turns off stacking by default
- **Symptom:** none visible in code — this would have compiled and run fine, just quietly never let one
  enemy step onto another, no matter how large `MaxStepHeight` was set.
- **Cause:** confirmed by reading the engine's own source
  (`Engine/Private/Character.cpp`, `ACharacter::ACharacter()`):
  `CapsuleComponent->CanCharacterStepUpOn = ECB_No;` — every `ACharacter`'s capsule refuses to be a
  step-up target for another character by default (a sensible default for players, so one cannot casually
  stand on another's head; exactly the opposite of what an enemy swarm needs here).
- **Fix:** `GetCapsuleComponent()->CanCharacterStepUpOn = ECB_Yes;` in `AEnemyBase`'s constructor.
- *Real engine behaviour found by reading engine source before assuming the feature would "just work"
  once `MaxStepHeight` was large enough — not caught by a test first, since nothing was broken to catch;
  caught by checking the actual mechanism instead of only the exposed property.*

### 6.2 A pre-existing test needed to be told about the new default
- **Symptom:** `FEnemySpawnerDifficultyRampTest` ("Halfway difficulty") would have started failing —
  it explicitly checks `GetDifficultyAlpha() == 0.5` at the ramp's midpoint, which is only true for a
  straight line, not the new default eased curve.
- **Fix:** that test now pins `DifficultyCurveExponent = 1.0f` first, since it is specifically testing the
  raw interpolation math, not the new default shape; a separate new test
  (`DifficultyEasesInByDefault`) checks the eased default on its own.
- *A deliberate, understood test update, not a silent regression — the test was right before and is still
  right now, just about a narrower thing than "the spawner's default behaviour."*

## 7. Verification

### Performed

| Test | Action | Result | Kind |
|---|---|---|---|
| Build | `python scripts/run_tests.py --filter GrantMoney.Run.EnemySpawner` builds `GrantMoneyEditor` | Succeeded | Compiled |
| `EnemySpawner` suite | `GrantMoney.Run.EnemySpawner.*` | **10 of 10 passed** (1 new, 1 updated) | Automation test |
| `EnemyBase` suite | `GrantMoney.Combat.EnemyBase.*` (`--skip-build`) | **7 of 7 passed** (1 new) | Automation test |

Per the developer's direction this pass, the full `GrantMoney.*` suite was **not** re-run, since nothing
outside `EnemySpawner`/`EnemyBase` was touched; the developer will re-verify end to end on their own if
anything looks off.

**What the tests prove:**

| Item | Test | What it asserts |
|---|---|---|
| Default ramp eases in, still reaches exactly 1 | `EnemySpawner.DifficultyEasesInByDefault` | Alpha is below 0.25 a quarter of the way through the ramp and below 0.5 at the halfway point (both estimates of "gentle," not tuned against real play yet); reaches exactly 1.0 at the end regardless; exponent 1 reproduces the exact old straight line |
| Raw ramp math (`*AtStart`/`*AtEnd`, cap, batch, interval) | `EnemySpawner.DifficultyRampsOverTime` (pre-existing, now pinned to exponent 1) | Unchanged from before this pass |
| Traversal limits reach the real movement component; stacking is possible at all | `EnemyBase.TraversalLimitsApplyToMovement` | Non-default `MaxStepHeight`/`MaxWalkableFloorAngle` values reach `GetCharacterMovement()` after a tick; `GetCapsuleComponent()->CanCharacterStepUpOn == ECB_Yes` (the engine default is `No` — see §6.1) |

### Pending (estimates, not yet observed)

| Item | Why |
|---|---|
| Does the default ease-in (exponent 1.6) actually *feel* like "begin gently, ramp through the middle, busier at the end" during a real run? | Needs real play. The math is verified; the feel is an estimate. |
| Does `MaxStepHeight = 90 cm` reliably climb one slime, and does a crowd reliably stack several, on the real demo map? | Needs the real map, real collision, and several real slimes — exactly what the developer plans to check, including at the map's edges and against ordinary obstacles. |
| Does `MaxWalkableFloorAngle = 58°` keep an enemy standing on a stacked capsule's rounded top without obviously sliding off? | Same — a real-geometry, real-gravity check. |
| Does an enemy ever visibly launch upward, or get anywhere near the boundary wall? | Expected "no" by construction (no vertical velocity is ever added directly; the wall is over 3× `MaxStepHeight`), but not yet watched on screen. |

If anything looks wrong once tested, record the real observation here (replacing the estimate) before
retuning the numbers.

## 8. Known Limitations

- **All traversal numbers in this document are estimates** picked from reading the map/enemy dimensions
  (slime capsule height, boundary wall height), not from watching the real behaviour yet. Expect to
  retune `MaxStepHeight`/`MaxWalkableFloorAngle` after the developer's PIE pass.
- **The map has no literal slopes or stairs today** (`ATileMapRenderer` generates a flat grid), so
  "ordinary terrain changes" in this pass means obstructions and other enemies, not elevation. The engine
  mechanism used here (step-up + slope-walking) already handles real slopes too, if a future map adds any
  — nothing here would need to change.
- **`DifficultyCurveExponent` only applies when `DifficultyCurve` is not set.** A designer who sets a
  curve asset gets exactly what that curve says, same as before this pass.
- **No change to `MaxSpawnDistance`/`MinSpawnDistance`, batch/cap/interval values themselves** — only the
  *shape* of the ramp between them changed.

## 9. Extension Ideas

- A `UCurveFloat`-authored difficulty shape once a designer wants a hand-placed stair-step instead of a
  smooth ease, using the existing `DifficultyCurve` override — no new code needed.
- Per-enemy-type traversal tuning once a second enemy type exists (right now both new properties live on
  the shared `AEnemyBase`, with one default for every type).
- If a future map adds real elevation, revisit `GroundHeightTolerance`-style spawn validation together
  with traversal, since spawning and walking would then both need to agree on what "ground" means.

## 10. Source-Control Checkpoint

| Item | Value |
|---|---|
| Jira | SCRUM-37, SCRUM-29 |
| Branch | Currently `main`. Per `CLAUDE.md`, one Jira issue → one branch → one PR; consider `SCRUM-37-29-escalating-pressure-and-terrain-traversal`. |
| PR title | `SCRUM-37/29: Escalating pressure and usable terrain traversal` |

Suggested commit message (after the developer's PIE checks in §9):
```
SCRUM-37/29: Ease the difficulty ramp in by default; let enemies climb and stack

- AEnemySpawner: DifficultyCurveExponent (default 1.6) eases the ramp in when no DifficultyCurve asset
  is set, so a run begins gently and is noticeably busier by the end; exponent 1 is the old straight line
- AEnemyBase: MaxStepHeight/MaxWalkableFloorAngle expose and tune the engine's own step-up/slope-walking
  for climbing obstacles and stacking on other enemies; CanCharacterStepUpOn turned back on (ACharacter's
  constructor disables it by default)
- 2 new Automation tests, 1 existing test updated to pin the old straight-line behaviour explicitly
- SCRUM-37-29 documentation, with tuning values marked as estimates pending a real PIE pass
```

---

# Part C – AI Workflow Log

**Tool:** Copilot (an AI assistant in VS Code), working on the repository on the developer's PC. This
session built and ran only the two test suites this work touched, per the developer's direction to run
targeted tests itself and leave broader/real-map verification to the developer.

| # | Purpose | Prompt (developer) | Result | Decision | Verification |
|---|---|---|---|---|---|
| 1 | Request, with explicit testing scope | The SCRUM-37/SCRUM-29 brief verbatim, plus: "as long as the test is for the code itself you can run them but... only run tests on the logic you created on the last run... go light on extra work... if something goes wrong you can run the whole test suite but that will be verified on my end." | Read `AEnemySpawner` (found the difficulty ramp already fully implemented), `ARunController`, `AEnemyBase`, `ASlimeEnemy`, `ATileMapRenderer`, and the engine's own `Character.cpp`/`CharacterMovementComponent.cpp`. | Accepted | Read from source |
| 2 | Implementation | (continued) | Added `DifficultyCurveExponent`; added `MaxStepHeight`/`MaxWalkableFloorAngle`; found and fixed `CanCharacterStepUpOn` defaulting to No; updated one existing test, added two new ones. | Accepted | Compiled; tests below |
| 3 | Test run (targeted only) | (automatic, per the stated scope) | `GrantMoney.Run.EnemySpawner`: 10/10. `GrantMoney.Combat.EnemyBase` (`--skip-build`): 7/7. | Accepted | **10/10, 7/7** |
| 4 | Documentation | (implicit, per the shared Module 3 instructions) | This document, explicitly separating observed automation results from estimated real-world tuning values. | — | Checked against the code and the test output |

**Where AI helped most:** reading the engine's own `Character.cpp` before assuming a bigger
`MaxStepHeight` alone would be enough — it would not have been, silently, without also reversing
`CanCharacterStepUpOn`.

**Where it needs a human:** everything about how the ramp and the climbing/stacking actually *feel* on
the real demo map — explicitly out of scope for this session per the developer's instruction, and
explicitly the developer's own plan to verify directly.

---

# Part D – Video Voice-Over Script

## Opening
**SAY:** "Two small, related upgrades today: the run gets harder in a way that actually feels like it
starts gentle, and enemies stop getting stuck on each other — they climb."

## Acceptance Criteria
**SAY:** "By the end: the difficulty ramp eases in by default instead of rising in a straight line, and
enemies can step over obstacles and stack on top of each other to reach you, sliding around instead only
when that's actually shorter."

## Engineering Principle
**SAY:** "Check what already exists before building anything. The entire difficulty ramp was already
there from an earlier milestone — this pass added exactly one missing shape. And the climbing isn't new
physics at all: it's Unreal's own character stepping, turned on and tuned."

## Step-by-Step Demonstration

### 1. The ramp already existed
- **SAY:** "Before writing anything, I found the spawner already ramps spawn interval, batch size and
  the enemy cap from slow-and-small to fast-and-large. The only gap was the shape of that ramp."
- **SHOW:** `AEnemySpawner`'s `*AtStart`/`*AtEnd` properties. *(media 02)*

### 2. One new exponent
- **SAY:** "Raising the ramp's progress to a power instead of using it directly means the run is only
  about a third of the way to full intensity at its halfway point — genuinely gentle, not just labeled
  that way."
- **SHOW:** `GetDifficultyAlpha()`'s new branch. *(media 03)*

### 3. The engine default that would have quietly broken stacking
- **SAY:** "This one's from reading the engine itself: every Unreal character refuses to be stepped on by
  another character, by default. Without reversing that, no amount of step-height tuning would ever have
  let one enemy climb another."
- **SHOW:** `Character.cpp`'s `CanCharacterStepUpOn = ECB_No` next to the one-line fix. *(media 04)*

### 4. Tests
- **SAY:** "Targeted tests only this time, by design — ten for the spawner, seven for enemy movement, all
  green."
- **SHOW:** the test run. *(media 05)*

### 5. Play it
- **DO:** On the real demo map: watch the run's pace over its first and last minute; group several
  slimes against an obstacle or each other and watch them climb/stack; check a slime cannot climb the
  boundary wall. *(media 06 to 09)*

## Common Mistakes
**SAY:** "Watch for engine defaults that look unrelated to your feature until you read the actual engine
source — `CanCharacterStepUpOn` is exactly that kind of silent blocker."

## Verification Checkpoint
**SAY:** "Automation tests confirm the code does what it says. Whether the ramp *feels* gentle and the
stacking *looks* right is a real-map call — that's next."

## Next Milestone
**SAY:** "With pacing and movement both in place, the vertical slice is close to feeling like a real
survivor game end to end."

---

# Media Capture List

Store under `Media/SCRUM-37-29/` (outside the repository). Only record what actually happens.

| # | File | Type | Show | Proves |
|---|---|---|---|---|
| 01 | `01-branch.png` | Screenshot | `git branch --show-current` | Branch (decide per §10) |
| 02 | `02-existing-ramp.png` | Screenshot | `EnemySpawner.h`'s `*AtStart`/`*AtEnd` properties | The ramp mechanism already existed |
| 03 | `03-exponent.png` | Screenshot | `GetDifficultyAlpha()`'s new branch | The one new piece |
| 04 | `04-engine-default.png` | Screenshot | `Character.cpp`'s `CanCharacterStepUpOn = ECB_No` next to `EnemyBase.cpp`'s fix | The real engine gotcha found and fixed |
| 05 | `05-tests-pass.png` | Screenshot | `run_tests.py` output, 10/10 and 7/7 | Tests green |
| 06 | `06-ramp-start-vs-end.mp4` | Clip | The first minute vs. the last minute of a run, side by side or back to back | The ramp's actual feel |
| 07 | `07-climb-one-obstacle.mp4` | Clip | A slime climbing over a static obstacle/prop | Ordinary-obstacle traversal |
| 08 | `08-stack-on-slimes.mp4` | Clip | Several slimes crowding and climbing/stacking on each other toward the player | The core SCRUM-29 ask |
| 09 | `09-boundary-holds.mp4` | Clip | A crowd of slimes against the boundary wall; none climbs over | Boundary safety |
