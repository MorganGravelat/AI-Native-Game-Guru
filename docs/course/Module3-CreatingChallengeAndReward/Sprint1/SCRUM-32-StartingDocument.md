# SCRUM-32 – Starter Weapon: Auto-Targeting Magic Orb
## Module 3 Sprint 1 – Starting Document (for a future AI recording this task)

This is the hand-off for whoever records and writes up SCRUM-32 later. It says what was built, in what order, what was **actually** verified, and exactly what to record. Same idea as `GMS-127-128-StartingDocument.md` and `SCRUM-33-ScriptDocumentation.md` in this folder, but shorter.

**Status: implemented, compiled, unit-tested and exercised in the real game headless. NOT yet played by a person in the Editor window.** No media has been captured. Nothing is committed or pushed.

| Evidence kind | Meaning |
|---|---|
| Compiled | `GrantMoneyEditor Win64 Development` built with `Result: Succeeded`. |
| Automation test | `python scripts/run_tests.py` ran the `GrantMoney.*` tests in a synthetic world. |
| Headless real-game run | The real `PlayGame1` level, real game mode and real player, run with `-game -nullrhi` and a temporary script. No window, no person. The script was **deleted**, so it cannot be rerun and left no file. Only the AI's chat transcript records its output. |
| Person in the Editor | Someone pressed Play and watched. **Not done yet.** Every "Expected" in the capture list below is a prediction until this is done. |

---

## Jira ticket (verbatim prompt)

```text
Implement SCRUM-32 in Grant Money Survivor. Give the player a starter weapon that automatically targets the nearest living slime within range and fires a magic orb at it. Targeting should work in any direction, regardless of where the player or camera is facing.

Inspect the repository first. Find the active player class, the slime class, the existing enemy health and damage APIs, and any death or despawn handling. Reuse those systems and follow the project’s Unreal C++ conventions.

1. Give the player the starter weapon when a run begins. On each firing interval, select the nearest living slime within a configurable range. If none is in range, skip the shot and check again on the next interval. The player should never need to press an attack button or turn toward the slime.
2. Use Unreal’s basic sphere mesh for the orb. Aim toward the selected slime’s collision center at the moment of firing, then let the orb travel in a straight line. Expose damage, fire interval, targeting range, and projectile speed as editable defaults. Give each projectile a finite lifetime and have it ignore the player who fired it.
3. Apply damage through the existing enemy health system when the orb hits a living slime, then remove the orb. Apply damage at most once per projectile. The slime actually hit takes the damage, even if it was a different slime from the one originally targeted.
4. Keep death handling in the enemy’s existing death flow. When health reaches zero, run death handling once and despawn the slime. Capture its world transform before removal so a later drop system can spawn items where it died. Leave item drops for a future ticket.
5. Keep firing and target selection in the weapon so future weapons can use different targeting rules. Clean up the firing timer when the weapon or player is removed.

Verify in Play In Editor that the weapon targets a slime behind the player, picks the closer of two living slimes, skips firing when both are out of range, and picks a new target after the first dies. Confirm that a nonlethal hit reduces health, a lethal hit despawns the slime once, and a missed orb expires.

Summarize the files changed, any editor setup required, the default weapon settings, where drops can hook into the death flow, and which checks you actually ran.
```

Not done on purpose: item drops, weapon upgrades, a second weapon, UI, player death.

---

## What already existed (reused, not rebuilt)

All from SCRUM-33 (`80cd97d`), read from source:

| System | Used for |
|---|---|
| `AEnemyBase` / `ASlimeEnemy` | The things shot. The weapon targets `AEnemyBase`, so slimes qualify and future enemies come free. |
| `UHealthComponent` | Health, `IsDead()`, and the `OnDeath` event. Takes damage from Unreal's `ApplyDamage`. |
| `UAttackHitComponent` | The orb's hit rules: once per target, never the shooter, only living things with health. Its header comment (`AttackHitComponent.h` 44–51) literally describes how to hook a projectile to it. |
| `AEnemyBase::HandleDeath` + `UCombatEventsSubsystem::OnEnemyKilled` | The one death flow. Not duplicated. |
| `AGrantMoneyPlayerCharacter` | Active player class. Spawned by `AGrantMoneyGameMode`, possessed by `AMapCameraPlayerController`. Spawn point in `PlayGame1`: **X=3200 Y=3100**, facing +X (from the game mode's log line and the headless run). |
| `scripts/run_tests.py`, `CLAUDE.md` Testing Rules | Every new logic `.cpp` needs a `<ClassName>Tests.cpp`. A Stop hook enforces it. |

---

## Design decisions

| Decision | Why |
|---|---|
| Weapon is a component (`UStarterWeaponComponent`) on the player, driven by a **timer**, not Tick | Nothing needs a per-frame update. Timer is cleared in `EndPlay`, which runs when the weapon or the player is removed. |
| `SelectTarget` and `FireAt` are `virtual` | Brief item 5: a future weapon overrides targeting or firing without touching the rest. |
| Range = **ground-plane** distance, owner centre to enemy centre | A raised slime is not "farther". Camera and player facing are never read, so direction never matters. |
| "Living" = health component exists, not dead, not being destroyed | A slime that is dead but waiting out `DeathCleanupDelay` must not be chosen. |
| Aim point = `GetActorBounds(true, …)` centre | The collision centre of the capsule, not the feet or origin. Computed once, at firing. |
| Orb = new actor `AMagicOrbProjectile` with `UProjectileMovementComponent` (no gravity, no bounce, no homing) | Straight line from the aim point. |
| Orb overlaps **Pawn only** and ignores everything else | Walls and ground do not stop it. It only needs to meet slimes. |
| Hit rules come from `UAttackHitComponent` with penetration 0 | Reuse. One hit per target, never the shooter (via Owner/Instigator), then `OnHitsExhausted` destroys the orb. |
| Orb damage is applied by `UGameplayStatics::ApplyDamage` (inside `UAttackHitComponent`) | "Through the existing enemy health system." The orb never touches `UHealthComponent` directly. |
| Orb lifetime via `SetLifeSpan` | A missed orb removes itself. |
| Death transform: `AEnemyBase::DeathTransform`, set as the first line of `HandleDeath`, read with `GetDeathTransform()` | Captured before the enemy stops or is destroyed, and readable inside `OnEnemyKilled`. |
| Damage comes from the weapon's `Damage` property, **not** the player's `Damage` stat | Brief says "editable defaults". Wiring it to `UStatsComponent` is a later upgrade ticket. |

### Default weapon settings

On the player's **StarterWeapon** component. All read when a shot fires, so they can be changed during Play.

| Setting | Default |
|---|---|
| Damage | 10 (a slime has 30 health: 3 hits) |
| Fire Interval | 1.0 s |
| Target Range | 800 cm |
| Projectile Speed | 1000 cm/s |
| Projectile Lifetime | 2.0 s (so an unblocked orb reaches 2000 cm) |

---

## Files

| File | Change |
|---|---|
| `Game/Source/GrantMoney/Public/StarterWeaponComponent.h` + `Private/StarterWeaponComponent.cpp` | **New.** The weapon. |
| `Game/Source/GrantMoney/Public/MagicOrbProjectile.h` + `Private/MagicOrbProjectile.cpp` | **New.** The orb. |
| `Game/Source/GrantMoney/Public/GrantMoneyPlayerCharacter.h` / `Private/…Character.cpp` | Adds the `StarterWeapon` subobject and `GetStarterWeapon()`. |
| `Game/Source/GrantMoney/Public/EnemyBase.h` / `Private/EnemyBase.cpp` | Adds `DeathTransform` and `GetDeathTransform()`. Death behaviour unchanged otherwise. |
| `Private/StarterWeaponComponentTests.cpp`, `Private/MagicOrbProjectileTests.cpp`, `Private/Tests/WeaponTestScene.h` | **New.** 15 tests plus a shared test scene (a header, so no extra `.cpp`). |
| `Private/EnemyBaseTests.cpp`, `Private/Tests/CombatTestListener.h` | One new test (`DeathTransform`), one new helper field. |

No Editor setup, no new assets, no config, no `Build.cs` change. The weapon is native, so it exists on the player and on any Blueprint subclass of it.

### Where drops hook in later

```text
UCombatEventsSubsystem::OnEnemyKilled(Enemy, Killer)   // fires before the enemy is destroyed
    └─ Enemy->GetDeathTransform()                       // spawn items here
```

---

## What was done, in order (for a future AI)

1. Created branch `SCRUM-32-starter-weapon` from `main`. Two uncommitted files came along: `PlayGame1.umap` and `GMS-127-128-StartingDocument.md` (see "Source control state").
2. Read `CLAUDE.md` (the Testing Rules are mandatory), the player, slime, health, hit-rule, stats and combat-event code, `EnemyBaseTests.cpp`, `AttackHitComponentTests.cpp` and `CombatTestListener.h`.
3. Checked engine source for one fact the tests depend on: `APawn::ShouldTakeDamage` refuses damage in a world with no game mode (`Pawn.cpp` ~567), and `FTestWorldWrapper::BeginPlayInTestWorld` creates one when the world has a game instance. This is why the weapon tests can damage slimes through `ApplyDamage` and the SCRUM-33 enemy tests damage the health component directly.
4. Wrote the orb, then the weapon, then the player and `EnemyBase` edits, then the tests.
5. Ran `python scripts/run_tests.py`. First run: one compile error (a test used `UAttackHitComponent` without including its header). Fixed. Second run: **40/40 passed** (24 existing + 16 new).
6. **Mutation check.** Broke three behaviours on purpose (orb ignores overlaps; dead enemies count as living; range ignored). 10 of 15 weapon tests failed, as they should. All three were reverted and a search for the markers found nothing.
7. **Headless real-game run.** Built a temporary console command that waited for the real player in `PlayGame1`, froze staged slimes, and checked each scenario from the ticket. First run: 8 pass / 3 fail. All three fails were the test script, not the game: the level already had 5 placed slimes firing at once, and orbs left over from the previous stage were counted in the next. After fixing the script: **11/11**. Script and log deleted.
8. Final run of the full suite on the final code: 40/40. Reviewed the diff.

### Mistakes and surprises worth showing in the video

- A missing `#include` in a test was the only build failure.
- The first real-game run "failed" three checks. The cause was the level's five pre-placed slimes and leftover orbs, not the weapon. Lesson: a failing check can be the checker.
- The mutation check proves the tests can fail. Without it, "40/40" says little.
- `PlayGame1.umap` was rewritten at 12:33 AM during the session. The AI did not save it (no Unreal process was running afterwards, and the AI's runs used different log files), but could not prove who did. Treat the map as unreviewed.

### Evidence from the headless real-game run (chat transcript only)

| Check | Result |
|---|---|
| Player possessed, owns weapon, settings 10 / 1.0 / 800 / 1000 / 2.0 | pass |
| No slimes in the level: nothing fires | pass |
| Slime directly behind the player (player facing +X) is hit: health 20 | pass |
| Closer of two is hit (20), farther untouched (30) | pass |
| Both beyond 800 cm for 3.5 s: no orb, no damage | pass |
| Near slime (10 health) dies and despawns; far slime becomes the target and is hit | pass |
| Missed orb did no damage and expired after 2.00 s | pass |

---

## Verification still owed (a person in the Editor)

Do these while recording. They are the capture list below. Write down what you actually see; do not copy "Expected".

| # | Action | Expected |
|---|---|---|
| V1 | Play, press nothing, slime within 8 m | Orbs fire by themselves, about one per second. |
| V2 | Slime behind the player | Orb flies backward and hits it. Player never turns. |
| V3 | Two slimes, one near, one far | Near one is shot until it dies, then the far one. |
| V4 | Both beyond 8 m | No orbs, no `StarterWeapon:` log lines. |
| V5 | First slime dies | Next shot goes to a new target. |
| V6 | Non-lethal hit | Slime's CurrentHealth drops by 10 (30 → 20). |
| V7 | Lethal hit | Slime disappears once. One `died` log line. |
| V8 | Missed orb | Orb vanishes mid-air after its lifetime. Nothing is hurt. |

---

## Staging for recordings (read first)

1. **The level currently has 5 slimes** placed in the working-tree `PlayGame1.umap`, and they chase and hurt the player (SCRUM-33). For controlled shots, delete or move them in the Editor and **do not save the map**. Plan to `git restore Game/Content/Maps/PlayGame1.umap` afterwards, after closing the Editor.
2. **Hold staged slimes still:** select each placed slime, Details → set **Move Speed = 0**. Then they should stay where you put them and cannot hurt you. *(Not tried: the headless run froze slimes in code instead. If a slime still creeps, set its Details → Character Movement → Max Walk Speed to 0 as well, or just keep the player moving away.)*
3. **Where to place them.** The player spawns at X=3200 Y=3100 and faces +X. Ground-level slime Z is about 32. Confirm the spawn from the Output Log line `GrantMoneyGameMode: safe start on level 1 at cell … X=… Y=…` before relying on these numbers. Set them in Details → Transform → Location:

| Scene | Slime A | Slime B |
|---|---|---|
| Behind the player | 2800, 3100, 32 | – |
| Nearest of two | 3200, 3400, 32 (300 cm right) | 3200, 2500, 32 (600 cm left) |
| Out of range | 4200, 3100, 32 (1000 cm ahead) | 3200, 1600, 32 (1500 cm left) |

4. **Make the orb easier to see.** It is a 20 cm grey sphere moving at 1000 cm/s. Use the console command `slomo 0.3` (and `slomo 1` to restore), and zoom the camera in with the mouse wheel.
5. **Console and logs.** `show collision` toggles capsules. In **Window → Output Log**, filter on `StarterWeapon` to see `fired an orb at SlimeEnemy_N`, and on `HealthComponent` to see `SlimeEnemy_N died (damage causer: MagicOrbProjectile_N)`.
6. **Reading health live.** In Play press **F8** to eject, click the slime in the **World Outliner**, then in Details open its **Health** component and read **Current Health** (a read-only field).
7. **Editing weapon values live.** Same way, select `GrantMoneyPlayerCharacter_0` in the Outliner, find the **StarterWeapon** component, category **Weapon**.

---

## Media to record

Folder: `Media/SCRUM32/` in the shared media folder. **Not in Git.** Clips 10–30 s. Keep failures. Nothing below has been captured yet. Paths are relative to the repo root `C:\Dev\AI-Native-Game-Guru`; line numbers are 1-based, as shown in VS Code, for the files on this branch.

### A. Setup and the reused design (screenshots, no Play needed)

| File | What to show | Where / how |
|---|---|---|
| `01-branch-created.png` | The branch name | Terminal in the repo root: `git branch --show-current` → `SCRUM-32-starter-weapon`. |
| `02-existing-systems-reused.png` | The comment that tells you how to hook a projectile to the hit rules | `Game/Source/GrantMoney/Public/AttackHitComponent.h`, lines **44–51**. |
| `03-weapon-header.png` | The class, the two overridable functions, the five editable settings | `Game/Source/GrantMoney/Public/StarterWeaponComponent.h`, lines **25–64**. |
| `04-select-target.png` | Range, living check, ground-plane distance, nearest wins | `Game/Source/GrantMoney/Private/StarterWeaponComponent.cpp`, lines **69–101** (the distance line is 92). Optional second shot, the helper above it, lines **18–22**. |
| `05-fire-at.png` | Aim at the collision centre, deferred spawn, set up the orb, then finish | Same file, lines **103–137** (aim at 115, spawn at 125). |
| `06-timer-and-cleanup.png` | The timer schedule and the cleanup | Same file, lines **40–67** (`ClearTimer` is at 45). |
| `07-orb-collision-and-flight.png` | Pawn-only overlap, no gravity or bounce | `Game/Source/GrantMoney/Private/MagicOrbProjectile.cpp`, lines **36–42** and **59–67**. Or one shot of **27–72**. |
| `08-orb-hit-and-removal.png` | Hit goes through the hit rules, then the orb is destroyed; plus the lifetime | Same file, lines **90–107**. |
| `09-death-transform.png` | The transform saved before the enemy stops; the kill event after it | `Game/Source/GrantMoney/Private/EnemyBase.cpp`, lines **95–122** (capture at 98, event at 110). Optional: `Public/EnemyBase.h`, lines **53–56**. |
| `10-player-owns-weapon.png` | One added line makes the player armed | `Game/Source/GrantMoney/Private/GrantMoneyPlayerCharacter.cpp`, lines **48–49**. Optional: `Public/GrantMoneyPlayerCharacter.h`, line **53**. |

### B. Build and tests

| File | What to show | Where / how |
|---|---|---|
| `11-compile-success.png` | `Result: Succeeded` | Close the Editor first. Terminal: `python scripts/run_tests.py` (builds, then tests). Or the Build.bat command used in `GMS-127-128-StartingDocument.md`. |
| `12-all-tests-pass.png` | `40/40 tests passed (filter 'GrantMoney').` | Same command, last lines. |
| `13-weapon-tests-pass.png` | `15/15 tests passed (filter 'GrantMoney.Weapons')` | `python scripts/run_tests.py --skip-build --filter GrantMoney.Weapons`. |
| `14-mutation-check.png` *(optional, good course material)* | Tests catching a deliberate bug | **Commit first**, because the new files are untracked and cannot be restored otherwise. In `MagicOrbProjectile.cpp` put `//` before line **101** (`AttackHit->TryHit(OtherActor);`), run `python scripts/run_tests.py --filter GrantMoney.Weapons`, screenshot the failing list (about `5/15`), then undo with `git restore Game/Source/GrantMoney/Private/MagicOrbProjectile.cpp` and rebuild. |

### C. In the Editor (Play in `PlayGame1`)

Stage with the section above. Record the screen with the Output Log visible.

| File | What to show | Expected (a prediction until you watch it) |
|---|---|---|
| `15-details-weapon-settings.png` | The five settings | Play, F8, select `GrantMoneyPlayerCharacter_0`, StarterWeapon → Weapon: 10 / 1.0 / 800 / 1000 / 2.0. |
| `16-staged-slimes.png` | The staging layout and a slime's **Move Speed = 0** | Editor viewport before Play, with a slime selected and its Details visible. |
| `17-auto-fire-no-input.mp4` | The weapon fires with **no input** | Slime 5–7 m away, Move Speed 0. Start Play, take your hands off the keyboard and mouse, say so in the narration. Orbs fire about once a second. |
| `18-target-behind-player.mp4` | V2 | Slime A "behind". Player faces away. Orb flies backward and hits. Slow with `slomo 0.3`. |
| `19-nearest-of-two.mp4` | V3, V5 | "Nearest of two". Near slime is shot first. When it dies the far one is next. Show the `StarterWeapon:` log lines naming each slime. |
| `20-out-of-range-skip.mp4` | V4 | "Out of range" pair. No orbs for several seconds, and no `fired` lines. Then walk toward one: orbs start once it is under 8 m. |
| `21-nonlethal-then-lethal.mp4` | V6, V7 | One slime, eject and select it, read **Current Health**: 30 → 20 → 10 → gone. Show the single `died (damage causer: MagicOrbProjectile_…)` line. |
| `22-missed-orb-expires.mp4` | V8 | Slime at 700 cm. In Details set **Projectile Lifetime = 0.3**. The orb should vanish mid-air before reaching it, slime health unchanged. Put it back to 2.0. |
| `23-live-tuning.mp4` | The editable defaults really are editable | During Play change **Fire Interval** (try 0.3) and **Damage** (try 30). Fire rate and kill speed change immediately. Restore afterwards. |
| `24-output-log-evidence.png` | The log story | Output Log filtered on `StarterWeapon` and then `HealthComponent`, showing `fired an orb at…` and `died (damage causer: MagicOrbProjectile_…)`. |
| `25-full-demo.mp4` | The payoff, 20–30 s | Several live slimes chasing (leave their Move Speed alone), player running and the orbs killing them. Do this last. Slimes do hurt the player (10 per second each) and nothing handles player death yet, so keep moving. |

### D. Closing

| File | What to show | Where / how |
|---|---|---|
| `26-no-unrelated-files.png` | What the branch touches | Terminal: `git status`. The only non-source change is `PlayGame1.umap`, which should be restored first. |
| `27-final-verification.png` | The filled-in V1–V8 table beside a clean Output Log | The table above, with your real results. |

### Suggested order

1. `01`–`10` code screenshots first (no Play needed).
2. `11`–`13` terminal runs. Close the Editor before building.
3. Open the Editor. `15`, `16`, then the clips `17`–`23` in order (each depends on staging).
4. `24`, then `25` last.
5. Restore the map, then `26`, `27`.

---

## Source control state (read before committing)

- Branch: `SCRUM-32-starter-weapon`. `CLAUDE.md` says branch names follow `SCRUM-XXX-description`. This one does.
- **Uncommitted and unrelated to SCRUM-32:** `Game/Content/Maps/PlayGame1.umap` (an Editor save; do not commit unless you want the placed slimes) and `docs/…/GMS-127-128-StartingDocument.md` (a capture-checklist edit from an earlier task that rode along when the branch was cut). Commit the latter separately or move it.
- Never commit or push unless told to.

Suggested commit message:

```text
SCRUM-32: Add auto-targeting starter weapon and magic orb

- UStarterWeaponComponent fires a magic orb at the nearest living enemy
  within range on a timer, in any direction; targeting and firing are
  virtual so other weapons can change them
- AMagicOrbProjectile flies straight, ignores its shooter, damages
  through UAttackHitComponent / ApplyDamage once, and expires on a lifetime
- Player starts every run with the weapon
- AEnemyBase records its death transform before removal for future drops
- 16 new automation tests

No drops, upgrades, UI or stat-driven damage yet.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>
```

---

## Known limitations

- **Not played by a person yet.** See the table at the top.
- Orbs pass through walls and props (pawn-only overlap).
- Weapon damage is a plain property, not the player's `Damage` stat.
- Two orbs arriving on the same slime in the same frame kill it once (tested). Whether a slime can take more than one orb in flight beyond that was not examined separately.
- Slimes hurt the player, and nothing handles player death (SCRUM-33).
- Slimes bump into each other when crowded (GMS-127/128 finding).
- Weapon fires only while the player exists; it does nothing at 0 enemies, by design.

## Voice-over outline

> **The goal.** "A survivor-game player doesn't aim. The weapon finds a target by itself, in any direction."
> **Reuse.** "SCRUM-33 already gave us health, damage and a death event, and a hit-rule component that was written with this projectile in mind." (`02`)
> **The weapon.** "One question and one action: who is nearest, and fire at them. Each is its own function, so another weapon can change either." (`03`–`05`)
> **The orb.** "A sphere, a straight line, a lifetime. It never decides who is hurt; the hit rules do." (`07`, `08`)
> **Proof.** "No input, behind us, nearest of two, out of range, a new target after a kill." (`17`–`21`)
> **The miss.** "Shorten the lifetime and the orb simply disappears." (`22`)
> **Where drops will go.** "We save where it died before it is gone." (`09`)
> **Honesty.** "Tests catch deliberate bugs (`14`). We also say what we have not watched."
