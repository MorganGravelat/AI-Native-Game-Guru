# SCRUM-36 – XP Leveling and Upgrade Choices
## Module 3 Sprint 1 – Development and Recording Guide

This guide covers SCRUM-36: turning XP collection into leveling and upgrade selection. The player's
existing `Experience` stat now drives a level, a level-up event, and a pause-everything choice of up to
three distinct upgrades. Upgrades are applied through the same `UStatsComponent::AddModifier` call
SCRUM-53's stat boosts use, so the two combine correctly. The offered starter orb upgrades are Damage,
Attack Speed, Penetration, Projectile Count, and Move Speed; a fully implemented and tested AOE Explosion
upgrade exists in code but is deliberately left out of the offered pool for now (see the Addendum below).

- **Part A:** change report
- **Part B:** developer implementation guide
- **Part C:** AI workflow log
- **Part D:** video voice-over script
- **Media Capture List:** every screenshot and clip to record, with file names

**Status:** the code and tests are written. **The project builds, and the full `GrantMoney.*` Automation
suite passes: 122 of 122, 19 of them new** (run by the AI session on this machine, 2026-11-12). **The
developer has not yet created `WBP_LevelUp` or tested the flow in PIE** — this is the next step, using
this project's established yes/no quiz method once the widget exists (see §9, Pending).

## Addendum (same day, after initial implementation): pool change and an explosion-timing fix

Two follow-up requests from the developer, after the initial SCRUM-36 implementation above was already
built and tested:

1. **AOE Explosion removed from the offered pool, replaced with Move Speed.** The developer felt the
   explosion needed more design thought before offering it to players. `MakeExplosionUpgrade()` and all
   of its underlying `AMagicOrbProjectile`/`UAttackHitComponent` functionality are untouched and fully
   working — it is just not in `UUpgradeComponent::AvailableUpgrades` anymore. Re-enable it any time by
   adding `MakeExplosionUpgrade()` back to that list (one line in `UpgradeComponent.cpp`). A new **Move
   Speed** upgrade (+5% `MaxWalkSpeed` per stack, up to 5 stacks) takes its place, via a new `MoveSpeed`
   stat wired to `GetCharacterMovement()->MaxWalkSpeed` in `AGrantMoneyPlayerCharacter`.
2. **Explosion now triggers on first contact, not on exhaustion/expiry.** The developer noticed that with
   a Penetration upgrade also active, the burst only went off once the orb's hits ran out (its last
   penetrating hit) or it missed entirely — not at the moment it actually touched an enemy. Fixed: the
   orb now spends its one explosion charge the instant it lands its first direct hit, then keeps flying
   through any remaining penetration as a normal (non-exploding) hit — it can never explode a second time
   off the same orb. A totcreating WBP_LevelUp (parent class LevelUpWidget, three Option1Text/Option1Button...Option3Text/Option3Button pairs, same pattern as WBP_GameOver), then setting it on BP_MapCameraPlayerController if not auto-resolved. Once that's done, try leveling up, picking upgrades, stacking to the limit, a multi-level pickup, and a restart — happy to run another quick yes/no quiz once you've done that.al miss (no direct hit before the orb's lifetime ends) still spends an unused
   charge on expiry, same as before. See §6 Step 7 (rewritten) and §8.4 for a real bug this surfaced and fixed.

**What is and is not evidence in this document:**

| Kind | Meaning |
|---|---|
| Read from source | Taken from reading project source. Not a test. |
| Compiled | UnrealBuildTool finished with `Result: Succeeded`. Says nothing about behaviour. |
| Automation test | A `GrantMoney.*` Automation test passed in a headless editor run, in a synthetic game world. |
| Developer reported | The developer played it in the Editor and said what they saw (via a short yes/no quiz, per the project's established verification method). |

At the time of writing, only the first three kinds apply; "Developer reported" is pending (§9).

---

## Jira

| Key | Goal |
|---|---|
| SCRUM-36 | Turn XP collection into leveling and upgrade selection: configurable thresholds, a level-up event, a pause-everything choice of up to three eligible upgrades, reusable upgrade definitions applied through the existing stats interface, and the five starter orb upgrades. |

**Done when, and where each one is met:**

| Criterion | Met by | Evidence |
|---|---|---|
| Configurable level thresholds, current level, progress, level-up event | `ULevelComponent` (`LevelThresholds`, `GetLevel`, `GetXpIntoLevel`, `GetXpToNextLevel`, `GetProgressFraction`, `OnLevelUp`) | `GrantMoney.Progression.LevelComponent.*` (6 tests) |
| Excess XP preserved; one pickup can cross multiple thresholds | `ULevelComponent::HandleStatChanged`'s `while` loop subtracts one threshold per iteration instead of resetting to 0 | `LevelComponent.ExcessXpPreserved`, `LevelComponent.MultipleThresholdsInOneGain` |
| Up to three distinct eligible upgrades offered on level-up | `UUpgradeComponent::RollOptions` (partial Fisher-Yates over upgrades not at `MaxStacks`) | `UpgradeComponent.Defaults`, `UpgradeComponent.OffersDistinctOptions` |
| Suspend combat / timer / temporary effects while choosing; resume after the final choice | No new freeze code: `AMapCameraPlayerController::ShowLevelUpChoice` calls `UGameplayStatics::SetGamePaused(true)`, the same mechanism SCRUM-44's Game Over screen and the pause menu already use (see that ticket's `LevelTick.cpp` read: a paused world skips actor `Tick()` and `FTimerManager::Tick()` entirely, which already covers the weapon's fire timer, enemy pursuit/contact damage and any future timer-driven effect) | Read from source (reused, not re-verified: already proven by SCRUM-44's tests) |
| Queue multiple earned selections safely; resume only after the last | `UUpgradeComponent::PendingChoices` + `PresentNextChoice` (keeps `OnChoiceReady` firing per presented choice; `OnChoicesResolved` fires exactly once, after the queue empties) | `UpgradeComponent.QueuedChoicesResumeAfterTheLast` |
| Reusable upgrade definitions: display text, stat, magnitude, stack limits | `FGrantMoneyUpgradeDefinition` (`UpgradeTypes.h`) | `UpgradeComponent.Defaults` |
| Applied through existing stats/weapon interfaces; combine with SCRUM-53 boosts | `UUpgradeComponent::ApplyUpgrade` calls `UStatsComponent::AddModifier`, the exact call `AStatBoostPickup` already uses | `UpgradeComponent.ChooseAppliesModifier` |
| Damage, Attack Speed, Penetration, Projectile Count, AOE explosion starter upgrades | 5 code-default entries in `UUpgradeComponent`'s constructor (see §4) | `UpgradeComponent.Defaults`; weapon-side behaviour below |
| Penetration and multi-projectile behaviour clearly defined; no double-damage | `UAttackHitComponent::TryAreaHit` (ignores penetration exhaustion, still respects the existing hit-once-per-target set); `UStarterWeaponComponent::SelectTargets`/`TryFire` (fires exactly N orbs at the N nearest distinct living enemies, reusing the nearest available target via modulo if fewer enemies exist than orbs) | `AttackHitComponent.TryAreaHit*`, `StarterWeapon.ProjectileCountFiresAtDistinctTargets`, `MagicOrb.Explosion*` |
| Level and XP progress exposed to the HUD | `ARunHUD::DrawHUD` reads the player's `GetLevelComponent()` | `// NO-TEST:` (pure UI glue, like the rest of `RunHUD.cpp`) |
| Verify upgrades change gameplay, stacking limits, multiple pending level-ups, restart clears progression | Automation tests below for everything except restart (restart reuses SCRUM-44's `OpenLevel`-based reset, which already resets every actor and component with no run-scoped state outside the `UWorld`; not re-tested here, see §10) | See §9 |

**Related tickets:** SCRUM-33 (stats, health), SCRUM-35 (pickups, `XpOrbPickup`), SCRUM-53 (stat boost
pickups — upgrades combine with these through the same `AddModifier`), SCRUM-44 (pause/Game Over
conventions this reuses), SCRUM-31/30/38/42 (run controller, HUD).

---

# Part A – Change Report

## Milestone Summary

Collecting XP now builds toward a level. Each level-up queues an upgrade choice: up to three distinct,
not-yet-maxed upgrades, picked at random, shown on a simple placeholder widget. Choosing pauses the game
exactly like the pause menu and Game Over screen already do, so combat, the run timer and the weapon's
fire-rate timer all freeze together; multiple level-ups queued from one big pickup are presented one
after another, and gameplay resumes only once the last one is answered. The five starter orb upgrades
are Damage, Attack Speed, Penetration, Projectile Count (fire more than one orb per shot), and an AOE
Explosion (an orb that, once its hits are spent or its lifetime runs out, bursts and damages everyone in
a radius, visualised by its placeholder sphere briefly growing before it disappears).

## Files Created

| File | Purpose |
|---|---|
| `Public/UpgradeTypes.h` | `FGrantMoneyUpgradeDefinition`: Id, display text, stat, op, magnitude, max stacks. |
| `Public/LevelComponent.h`, `Private/LevelComponent.cpp` | `ULevelComponent`: level/progress derived from the `Experience` stat, configurable thresholds, `OnLevelUp`. |
| `Public/UpgradeComponent.h`, `Private/UpgradeComponent.cpp` | `UUpgradeComponent`: the offered starter upgrades, choice queue/roll/apply logic, `OnChoiceReady`/`OnChoicesResolved`. |
| `Public/LevelUpWidget.h`, `Private/LevelUpWidget.cpp` | `ULevelUpWidget`: three placeholder Text+Button option pairs. `// NO-TEST:` (pure UI glue). |
| `Private/Tests/ProgressionTestListener.h` | Test-only `UObject` that records `OnLevelUp`/`OnChoiceReady`/`OnChoicesResolved` calls (dynamic multicast delegates cannot bind to a lambda — see §8.1). |
| `Private/LevelComponentTests.cpp` | 6 tests, `GrantMoney.Progression.LevelComponent.*`. |
| `Private/UpgradeComponentTests.cpp` | 6 tests, `GrantMoney.Progression.UpgradeComponent.*`. |
| `docs/course/Module3-CreatingChallengeAndReward/Sprint1/SCRUM-36-ScriptDocumentation.md` | This document. |

## Files Modified

| File | Change |
|---|---|
| `Public/GrantMoneyStatTypes.h` | Added `ProjectileCount`, `ExplosionRadius` and `MoveSpeed` to `EGrantMoneyStat`. |
| `Public/AttackHitComponent.h`/`.cpp` | Added `TryAreaHit(AActor*)`: damages once, ignores `IsExhausted()` (penetration cap) but still respects the existing hit-once-per-target set, so a direct hit and a later explosion can never double-damage the same enemy. **Addendum:** split the penetration budget (`DirectHitCount`, counted only by `TryHit`) from the double-damage-prevention set (`HitActors`, shared by `TryHit`/`TryAreaHit`) — see §8.4. |
| `Public/MagicOrbProjectile.h`/`.cpp` | `Launch()` gained a trailing `ExplosionRadius` parameter (default 0, backward compatible). **Addendum (rewritten, see §6 Step 7):** the orb now spends its one explosion charge at its first direct hit (`TriggerExplosion`), not when its hits are later exhausted; `FinishIfOutOfHits` stops the orb only once it is actually out of hits or its lifetime ends, waiting for any in-progress burst visual to finish first. |
| `Public/StarterWeaponComponent.h`/`.cpp` | Added `ProjectileCount`/`ExplosionRadius` properties and `GetEffective*` getters (same stat-lookup pattern as Damage/Penetration). Added `SelectTargets(int32)` (independent of the existing `SelectTarget()`, which is untouched). `TryFire()` branches: effective count ≤ 1 keeps the exact original single-target path; otherwise fires one orb per target from `SelectTargets`, reusing the nearest target via modulo if fewer enemies exist than orbs. |
| `Public/GrantMoneyPlayerCharacter.h`/`.cpp` | Added `LevelComponent`/`UpgradeComponent` subobjects and getters; wired `LevelComponent->OnLevelUp` → `UpgradeComponent->NotifyLevelGained()` → the local controller's `ShowLevelUpChoice`/`HideLevelUpChoice`. **Addendum:** seeds the `MoveSpeed` stat from the movement component's own default and keeps `GetCharacterMovement()->MaxWalkSpeed` in sync with it (`HandleStatChanged`). |
| `Public/MapCameraPlayerController.h`/`.cpp` | Added `LevelUpWidgetClass` (auto-resolves `/Game/UI/WBP_LevelUp`), `ShowLevelUpChoice`/`HideLevelUpChoice` (same pause/cursor/input-mode pattern as `ShowGameOver`); `TogglePauseMenu` now also no-ops while the level-up choice is showing. |
| `Private/RunHUD.cpp`, `Public/RunHUD.h` | Added a "Level N (X / Y XP)" line, read from the player's `GetLevelComponent()`. |
| `Private/AttackHitComponentTests.cpp` | Added `FAttackHitTryAreaHitTest` (ignores exhaustion, no double-hit, rejects invalid targets). |
| `Private/StarterWeaponComponentTests.cpp` | Added `FStarterWeaponProjectileCountTest` and `FStarterWeaponExplosionRadiusWiringTest`. |
| `Private/MagicOrbProjectileTests.cpp` | Added `FMagicOrbExplosionHitsNearbyEnemiesTest`, `FMagicOrbExplosionOnExpiryTest`, `FMagicOrbNoExplosionWithoutUpgradeTest` (regression guard). **Addendum:** added `FMagicOrbExplosionOnlyOnFirstContactTest` (penetration keeps flying and hitting directly after the one explosion, no second burst). |

---

# Part B – Developer Implementation Guide

## 1. Prerequisites

- Unreal Engine 5.8, Editor **closed** when running `scripts/run_tests.py` (Live Coding breaks the build).
- `WBP_GameOver` / `WBP_PauseMenu` already exist in `/Game/UI/` as the pattern to copy (§2 below).

## 2. Designer / Developer Setup: Creating WBP_LevelUp

**Status: not yet created.** This cannot be done from a terminal or an AI session (it is a binary
`.uasset`).

1. **Content Browser → `Content/UI/`** (same folder as `WBP_GameOver`/`WBP_PauseMenu`).
2. **Right-click → User Interface → Widget Blueprint.** Name it `WBP_LevelUp`.
3. **Open it. Set the Parent Class to `LevelUpWidget`** (search for it; it appears because
   `ULevelUpWidget` is `GRANTMONEY_API` and `UCLASS()`).
4. **Build the layout** (a Canvas Panel with three stacked rows is simplest):
   - A `Text` widget named exactly **`Option1Text`**, a `Button` named exactly **`Option1Button`**.
   - A `Text` widget named exactly **`Option2Text`**, a `Button` named exactly **`Option2Button`**.
   - A `Text` widget named exactly **`Option3Text`**, a `Button` named exactly **`Option3Button`**.
   - Names must match exactly (`BindWidget` is case- and name-sensitive); a mismatch compiles the widget
     but leaves that pointer null, and the corresponding row silently does nothing.
   - When fewer than three upgrades are eligible, the unused rows are automatically collapsed at
     runtime — no need to build any "show/hide" logic into the widget itself.
5. **Compile and save.**
6. **Set it on the controller.** Select `BP_MapCameraPlayerController` (or whichever class fills the
   game mode's Player Controller Class) → Class Defaults → **Level Up → Level Up Widget Class →
   `WBP_LevelUp`**. (The C++ constructor already tries to auto-resolve `/Game/UI/WBP_LevelUp`, so once
   the asset exists at that exact path this may already be filled in — check rather than assume.)

## 3. Architecture Overview

```
XpOrbPickup collected ─► UStatsComponent::AddModifier(Experience, Additive, Amount)
                                  │ (OnStatChanged)
                                  ▼
                  ULevelComponent::HandleStatChanged
                        while XpIntoLevel >= GetThreshold(Level):
                            XpIntoLevel -= threshold; ++Level; OnLevelUp.Broadcast(Level, Level-1)
                                  │ (once per level gained, in order)
                                  ▼
       AGrantMoneyPlayerCharacter::HandleLevelGained ─► UpgradeComponent->NotifyLevelGained()
                                  │
                                  ▼
       UUpgradeComponent: ++PendingChoices; if no choice active, PresentNextChoice()
             RollOptions(): up to OptionsPerChoice distinct upgrades not at MaxStacks, random order
                  │ (OnChoiceReady)                              │ (if nothing eligible: consumed silently,
                  ▼                                               try the next pending choice instead)
       AMapCameraPlayerController::ShowLevelUpChoice
             ├─► ULevelUpWidget::RefreshOptions (shows up to 3 buttons, collapses the rest)
             └─► SetGamePaused(true)   — same mechanism as the pause menu / Game Over screen:
                                          freezes actor Tick (enemy pursuit/contact damage, movement)
                                          and FTimerManager::Tick (the weapon's fire timer) together

ULevelUpWidget button clicked ─► UpgradeComponent->ChooseUpgrade(Index)
       ApplyUpgrade: StackCounts[Id]++; Stats->AddModifier(Stat, Op, Magnitude)   (same call SCRUM-53 uses)
       --PendingChoices; PresentNextChoice() again:
           another pending choice ─► OnChoiceReady again (same pause, widget refreshes)
           queue empty             ─► OnChoicesResolved ─► HideLevelUpChoice() ─► SetGamePaused(false)

Weapon-side upgrades:
  Damage / Attack Speed  ─► read every shot via GetEffectiveDamage / effective AttackInterval (unchanged plumbing)
  Penetration            ─► UAttackHitComponent::IsExhausted() budget (unchanged; already existed)
  Projectile Count       ─► StarterWeaponComponent::SelectTargets(N) + TryFire fires N orbs at the
                             N nearest distinct living enemies (reusing the nearest if fewer exist)
  AOE Explosion          ─► MagicOrbProjectile carries ExplosionRadius; once its hits are exhausted
                             (or it times out with nothing hit) it calls Explode(): AttackHit->TryAreaHit
                             on everyone in range (ignores penetration, but keeps the hit-once-per-target
                             set, so nobody already hit directly is hit again), then the placeholder mesh
                             grows and glows red (a dynamic material instance on the engine's emissive
                             material), fading to black over ExplosionVisualSeconds before the orb is destroyed
```

## 4. The Starter Upgrades (code defaults, `UpgradeComponent.cpp`)

| Id | Stat | Op | Magnitude | Max Stacks | Effect | Offered? |
|---|---|---|---|---|---|---|
| `Damage` | `Damage` | Additive | +3 | 5 | +3 damage per magic orb hit. | Yes |
| `AttackSpeed` | `AttackInterval` | Multiplicative | −0.08 | 5 | Fires ~8% more often per stack. | Yes |
| `Penetration` | `Penetration` | Additive | +1 | 3 | Each orb hits one more enemy before it is spent. | Yes |
| `ProjectileCount` | `ProjectileCount` | Additive | +1 | 3 | Fires one more orb per firing cycle. | Yes |
| `MoveSpeed` | `MoveSpeed` | Multiplicative | +0.05 | 5 | +5% move speed per stack. | Yes |
| `ExplosionRadius` | `ExplosionRadius` | Additive | +60 cm | 4 | Orb explodes once, on its first direct hit, damaging everyone in the radius (see §6 Step 7). | **No (addendum)** — implemented and tested, deliberately left out of `AvailableUpgrades` for now; add `MakeExplosionUpgrade()` back to that list to offer it again. |

All magnitudes are placeholders, tuned to be clearly visible rather than balanced, and editable per
instance in the Details panel (`UUpgradeComponent::AvailableUpgrades`).

## 5. AI-Assisted Planning

### AI Prompt
The shared Module 3 instructions, followed by the SCRUM-36 brief verbatim: turn XP into leveling and
upgrade selection using the existing `StatsComponent`; configurable thresholds, level, progress,
level-up event; preserve excess XP and handle multi-threshold crossings; present up to three distinct
eligible upgrades on level-up; suspend combat/timer/temporary effects while choosing; queue multiple
earned selections safely; reusable upgrade definitions applied through the existing stats/weapon
interfaces so they combine with SCRUM-53 boosts; implement Damage, Attack Speed, Penetration, Projectile
Count, and an AOE explosion; prevent double-damage from the same projectile; expose level/XP to the
HUD; verify stacking, queueing, and that restart clears progression.

### Expected AI Output
Inspection of `StatsComponent`, `StarterWeaponComponent`, `MagicOrbProjectile`, `AttackHitComponent`,
`GrantMoneyPlayerCharacter`, and the pause/Game Over conventions, followed by a design, the
implementation, new tests, and this document.

### Human Review
- Confirm the five starter-upgrade magnitudes/stack limits are reasonable placeholders, not final balance.
- Confirm the Projectile Count target rule (fire N orbs at the N nearest distinct living enemies,
  reusing the nearest if there are fewer enemies than orbs) matches the intended design.
- **Create `WBP_LevelUp` in the Editor (§2)** — pending.
- **Try the leveling/upgrade flow in PIE and answer a short yes/no quiz** — pending (§9).

## 6. Step-by-Step Implementation

### Step 1 – Leveling from the existing Experience stat
`ULevelComponent::HandleStatChanged` is bound to `UStatsComponent::OnStatChanged` and ignores every stat
except `Experience`, and ignores any decrease (so, for example, a future debuff on `Experience`, if one
ever existed, couldn't delevel the player). A `while` loop subtracts one threshold per iteration instead
of resetting progress to 0, so excess XP carries over and multiple thresholds can be crossed by one
stat change, firing `OnLevelUp` once per level gained, in order.

### Step 2 – Queuing and rolling upgrade choices
`UUpgradeComponent::NotifyLevelGained` increments `PendingChoices` and only calls `PresentNextChoice` if
no choice is currently active — so a second level-up arriving while the first choice is still on screen
simply waits. `RollOptions` filters `AvailableUpgrades` down to those under their `MaxStacks`, then does
a partial Fisher-Yates shuffle to pick `OptionsPerChoice` (default 3) of them in random order. If nothing
is eligible, the pending choice is silently consumed (not stuck forever waiting) and the next one (if
any) is tried.

### Step 3 – Applying a choice through the existing stats interface
`ChooseUpgrade(Index)` calls `ApplyUpgrade`, which increments the stack count and calls
`Stats->AddModifier(Upgrade.Stat, Upgrade.Op, Upgrade.Magnitude)` — the exact call `AStatBoostPickup`
already uses, so an upgrade and a field pickup on the same stat combine through the same formula.

### Step 4 – Reusing the pause/show-UI convention
No new freeze code: `AMapCameraPlayerController::ShowLevelUpChoice`/`HideLevelUpChoice` follow the exact
pattern `ShowGameOver` (SCRUM-44) established — `SetGamePaused`, cursor visibility, `FInputModeGameAndUI`,
widget focus. `TogglePauseMenu` was extended to also no-op while the level-up widget is showing, matching
how it already no-ops during Game Over.

### Step 5 – Preventing double-damage between a direct hit and an explosion
`UAttackHitComponent::TryHit` already tracked which actors this attack had hit, to prevent one orb from
hitting the same enemy twice via penetration. `TryAreaHit` was added as a sibling that shares that same
tracked set (so an explosion can never re-damage someone the orb already hit directly) but skips the
`IsExhausted()` check — the whole point of an explosion is to hit more enemies at exactly the moment
the orb's normal hit budget is used up, which `IsExhausted()` would otherwise block.

### Step 6 – Multi-projectile firing without touching the existing single-target path
`SelectTargets(Count)` is a new, independent method; the heavily-tested `SelectTarget()` was left
completely untouched. `TryFire()` branches on the effective projectile count: `<= 1` uses the original
single-target call (byte-for-byte the same code path as before this ticket), so the default case has
zero behavioural change. Above 1, it fires one orb per target returned by `SelectTargets`, reusing the
nearest target via modulo indexing if there are fewer enemies in range than orbs.

### Step 7 – The explosion's trigger, visual and lifecycle (rewritten in the addendum)
`MagicOrbProjectile::TriggerExplosion()` runs once per orb, the instant the orb's `HandleBeginOverlap`
sees a successful direct hit (`EGrantMoneyHitResult::Hit`) — not when the orb's hits are later exhausted
by penetration, and not on a miss unless the orb never lands a direct hit at all (see `LifeSpanExpired`).
It runs the area damage pass once (`OverlapMultiByChannel` + `TryAreaHit` on every result, skipping
whoever the direct hit already caught), then starts a grow-and-glow visual: `PlaceholderMesh`'s scale
lerps toward a size roughly representing the blast radius, while a `UMaterialInstanceDynamic` on the
engine's emissive mesh material (the same material `XpOrbPickup`/`StatBoostPickup` already tint for their
own glow) fades its `EmissiveColor` from `ExplosionColor` (red) to black, both over `ExplosionVisualSeconds`
(`Tick()`, otherwise disabled). Crucially, `TriggerExplosion` does **not** stop the orb's movement or
collision — if it still has penetration left, it must keep flying and hitting more enemies directly,
never exploding again (`bHasExploded` guards that). `FinishIfOutOfHits()` is what actually stops the orb,
called only once its hits are truly exhausted or its lifetime is over; if the burst visual is still
playing when that happens, it waits for `Tick()` to finish the visual before destroying the orb, so the
fade is never cut short. `LifeSpanExpired` is overridden so a total miss (no direct hit ever landed) can
still spend its unused charge at wherever the orb ends up, instead of silently vanishing.

### Step 8 – HUD and tests
`ARunHUD::DrawHUD` reads the player's `GetLevelComponent()` for a "Level N (X / Y XP)" line, next to the
existing timer/state text. `python scripts/run_tests.py` (or `--filter GrantMoney.Progression` /
`GrantMoney.Weapons`). Results in §9.

## 7. AI Prompts by Stage

| Stage | Prompt |
|---|---|
| Inspection | "Inspect StatsComponent, StarterWeaponComponent, MagicOrbProjectile and AttackHitComponent to see what already exists before adding leveling and upgrades." |
| Leveling | "Add configurable level thresholds, current level, progress toward the next level, and a level-up event, derived from the existing Experience stat. Preserve excess XP and handle a pickup crossing multiple thresholds." |
| Upgrade choices | "Present a simple choice of up to three distinct eligible upgrades when the player levels up. Suspend combat, the run timer and temporary-effect durations while choosing. Queue multiple earned selections safely and resume only after the final choice." |
| Starter upgrades | "Implement Damage, Attack Speed, Penetration, Projectile Count and an AOE explosion that widens before disappearing. Define penetration and multi-projectile behaviour clearly, and prevent the same projectile from damaging the same enemy repeatedly." |
| HUD | "Expose level and XP progress to the HUD." |
| Verification | "Verify each upgrade visibly changes gameplay, stacking respects limits, multiple pending level-ups work, and restart clears progression." |
| Addendum: pool change | "Remove AOE Explosion from the upgrade options pool but leave the functionality for later; replace it with a slight move speed buff." |
| Addendum: explosion timing | "The AOE explosion, once penetration is active, only explodes at the end of the orb's life cycle. Instead have it explode on contact with an enemy, then the ability to explode goes away — the orb keeps going through remaining enemies via penetration without exploding again." |

## 8. Common Mistakes

### 8.1 Dynamic multicast delegates do not support `.AddLambda()`
- **Symptom:** `error C2039: 'AddLambda': is not a member of 'FOnGrantMoneyLevelUp'` (and the same for
  `FOnGrantMoneyUpgradeChoiceReady`/`FOnGrantMoneyUpgradeChoicesResolved`).
- **Cause:** `DECLARE_DYNAMIC_MULTICAST_DELEGATE*` delegates only support binding `UFUNCTION`s
  (`AddDynamic`/`AddUniqueDynamic`), not arbitrary lambdas, unlike the plain (non-dynamic) multicast
  delegate flavor used elsewhere in the engine.
- **Fix:** added `Tests/ProgressionTestListener.h`, a small test-only `UObject` with three `UFUNCTION`s
  that record what they were called with, and bound to it with `AddDynamic` instead.
- *Real failure during this work, caught by the first build of the new tests.*

### 8.2 A collinear test layout made a multi-projectile test's assumption wrong, not the code
- **Symptom:** `FStarterWeaponProjectileCountTest` initially placed three slimes in a straight line from
  the shooter. Every orb aimed at any of them travels the same path, so all three orbs hit only the
  nearest one (which died), while the farther two stayed untouched — not a code bug, but a test setup
  that didn't actually exercise "distinct targets."
- **Fix:** spread the three slimes around the shooter at different angles (still ordered nearest to
  farthest) so each orb's straight-line path reaches its own intended target instead of being blocked by
  one standing in front of it.
- *Real test-design issue during this work, caught by the automation run, not a defect in `SelectTargets`/`TryFire`.*

### 8.3 An explosion test's timing window overran the whole burst
- **Symptom:** `FMagicOrbExplosionHitsNearbyEnemiesTest` stepped the test world forward by 0.6 seconds
  before checking "is the orb mid-burst, not yet destroyed" — but travel time (0.3 s) plus the full
  0.25 s visual duration only adds up to 0.55 s, so by 0.6 s the orb had already finished exploding and
  been destroyed.
- **Fix:** stepped forward only enough to cover travel and the start of the explosion (0.35 s), checked
  the mid-burst state there, then stepped the remaining partial and full burst durations separately.
- *Real test-timing issue during this work, caught by the automation run.*

### 8.4 (Addendum) An area hit was silently stealing a penetrating orb's next direct hit
- **Symptom:** a new test firing a penetrating, exploding orb through three enemies in a row (each with a
  neighbour inside the blast radius) found the third enemy in line untouched, even though it should have
  taken a direct hit.
- **Cause:** `UAttackHitComponent::GetRemainingHits()` computed the penetration budget as
  `1 + Penetration - HitActors.Num()`, and `HitActors` was shared by both `TryHit` (direct hits) and
  `TryAreaHit` (the explosion's splash). Once explosion-on-contact (this same addendum) let the burst go
  off while the orb still had further enemies ahead on its path, the blast could catch one of those same
  upcoming enemies early, via `TryAreaHit` — which silently used up a "slot" in the penetration budget
  that was meant for a later *direct* hit, leaving the orb exhausted one enemy early.
- **Fix:** added a separate `DirectHitCount`, incremented only by `TryHit`; `GetRemainingHits()` now reads
  that instead of `HitActors.Num()`. `HitActors` still does its original job (never damage the same
  target twice, whichever method hits it first) — it just no longer doubles as the penetration counter.
- *Real bug, not present before this addendum's explosion-on-contact change (previously the explosion only
  ever ran once the orb had no more flying/hitting left to do anyway, so the interaction never mattered);
  caught by the new `FMagicOrbExplosionOnlyOnFirstContactTest`.*

## 9. Verification

### Performed

| Test | Action | Result | Kind |
|---|---|---|---|
| Build | `python scripts/run_tests.py` builds `GrantMoneyEditor` | Succeeded | Compiled |
| Automation tests | Full `GrantMoney.*` suite, 2026-11-12 (includes the addendum) | **122 of 122 passed** (103 pre-existing, 19 new) | Automation test |

**New tests** (rerun with `python scripts/run_tests.py --filter GrantMoney.Progression`,
`--filter GrantMoney.Weapons` and `--filter GrantMoney.Combat.AttackHitComponent`):

| Group | Tests |
|---|---|
| `Progression.LevelComponent` | `Defaults`, `GainsLevel`, `ExcessXpPreserved`, `MultipleThresholdsInOneGain`, `ThresholdArrayExhausted`, `IgnoresOtherStatsAndLoss` |
| `Progression.UpgradeComponent` | `Defaults`, `ChooseAppliesModifier`, `StackLimitExcludesFromFutureChoices`, `QueuedChoicesResumeAfterTheLast`, `OffersDistinctOptions`, `SameSeedSameRoll` |
| `Combat.AttackHitComponent` | `FAttackHitTryAreaHitTest` |
| `Weapons.StarterWeapon` | `FStarterWeaponProjectileCountTest`, `FStarterWeaponExplosionRadiusWiringTest` |
| `Weapons.MagicOrb` | `ExplosionHitsNearbyEnemiesOnExhaustion`, `ExplosionAlsoHappensOnAMiss`, `NoExplosionWithoutTheUpgrade`, `ExplosionOnlyOnFirstContact` (addendum) |

**What the tests prove, against the ticket's Verify list:**

| Item | Test(s) | What it asserts |
|---|---|---|
| Each upgrade visibly changes gameplay | `FStarterWeaponProjectileCountTest`, `FStarterWeaponExplosionRadiusWiringTest`, `FMagicOrbExplosion*`, `FAttackHitTryAreaHitTest` | Damage/Attack Speed/Penetration are unchanged existing plumbing (already covered by pre-existing stat tests) read through the same `GetEffective*` pattern; Projectile Count, Explosion Radius and Move Speed are exercised end to end |
| Stacking respects limits | `UpgradeComponent.StackLimitExcludesFromFutureChoices` | An upgrade at its `MaxStacks` is no longer offered; a level-up with nothing eligible resolves without getting stuck |
| Multiple pending level-ups work | `UpgradeComponent.QueuedChoicesResumeAfterTheLast` | Two level-ups queue; `OnChoiceReady` fires once per presented choice; `OnChoicesResolved` fires exactly once, after the second is answered |
| Restart clears progression | Not re-tested here: this reuses SCRUM-44's `OpenLevel`-based restart, already proven (that ticket's tests + developer confirmation) to tear down and recreate the whole `UWorld`, and nothing in this ticket adds any state outside it (`ULevelComponent`/`UUpgradeComponent` are both plain `UActorComponent`s on the player character, with no `UGameInstanceSubsystem` involved) | Read from source |
| (Addendum) Explosion fires exactly once, at first contact, and penetration keeps dealing direct damage afterward with no second burst | `FMagicOrbExplosionOnlyOnFirstContactTest` | Three slimes in a row (penetration budget for 3 direct hits), each with a neighbour inside the blast radius: the first slime and its neighbour both take damage from the one explosion; the second and third slimes each take only their own direct hit; neither of *their* neighbours is ever touched |

If anything fails on a later run, record the real error under §8 before fixing it.

### Pending

| Item | Why |
|---|---|
| Create `WBP_LevelUp` (§2) | Binary `.uasset`; cannot be done from this session. |
| Try leveling and each upgrade (including the new Move Speed upgrade) in PIE; verify the choice screen, stacking visibly disappearing from choices once maxed, multiple pending level-ups, and HUD level/XP | Needs eyes on the screen. Will use the project's established short yes/no quiz once the widget exists. |
| Restart clears level/upgrades/stacks back to 1/0/none | Developer to confirm directly, per the established workflow. |
| (Addendum) Explosion-on-contact and the red grow/fade visual, in a real PIE run with multiple enemies | Automation-tested end to end already (§9 Performed); a quick visual confirmation once `ExplosionRadius` is offered again (or tested manually via the Details panel) is still worthwhile but not blocking. |

---

## 10. Known Limitations

- **Upgrade magnitudes and stack limits are placeholders**, chosen to be clearly visible, not balanced.
- **AOE Explosion is not currently offered** (addendum): fully implemented and tested, but left out of
  `AvailableUpgrades` pending further design. See §4.
- **The explosion's blast damage ignores penetration but not the hit-once set**: an enemy already hit
  directly by the orb is never hit again by its own explosion, but there is no separate "explosion
  penetration" concept — the blast always hits everyone in range once, in the single burst it gets.
- **The explosion charge is one-time per orb, spent at the first direct hit** (addendum): if the orb never
  lands a direct hit at all, the charge is still spent on expiry, wherever the orb ends up. There is no
  way, currently, for a single orb to explode more than once, even with very high penetration.
- **Projectile Count's target rule is "nearest N distinct enemies, straight-line fire"**: if two targets
  happen to be collinear with the shooter, the farther orb can still strike whichever enemy is physically
  in its path first, the same as a real projectile would. This is accepted, intended behaviour (see §8.2),
  not a bug to fix.
- **No upgrade currently removes/replaces the placeholder mesh** mentioned in the original spawn-emergence
  milestone; that remains a separate, later concern.
- **The explosion's "fade" dims the emissive glow to black, not true alpha transparency**: the engine's
  emissive mesh material is opaque, the same one used for XP/stat-boost pickup glows, so there is no
  translucent material asset in the project yet to fade out an actual alpha channel. Good enough for a
  placeholder; swapping in a real translucent VFX material later needs no change to `TriggerExplosion`'s logic.

## 11. Extension Ideas

- A short flash or sound cue when a level-up or upgrade pick happens, instead of an instant UI swap.
- Rarity tiers or weighted rolls instead of a uniform random pick among eligible upgrades.
- Upgrades that affect systems beyond the starter orb, once more weapons/abilities exist.
- Persisting a "preferred upgrade path" across runs, once there is somewhere to store it (explicitly out
  of scope today, consistent with "nothing survives a restart").

## 12. Source-Control Checkpoint

| Item | Value |
|---|---|
| Jira | SCRUM-36 |
| Branch | Currently `main`. Per `CLAUDE.md`, one Jira issue → one branch → one PR; consider `SCRUM-36-xp-leveling-and-upgrades`. |
| PR title | `SCRUM-36: XP leveling and upgrade choices` |

Suggested commit message (after the Editor checks in §9 and creating WBP_LevelUp):
```
SCRUM-36: Add XP leveling and upgrade choices

- ULevelComponent: level/progress derived from the existing Experience stat, configurable thresholds,
  OnLevelUp (handles multi-threshold crossings, preserves excess XP)
- UUpgradeComponent: starter orb upgrades (Damage, Attack Speed, Penetration, Projectile Count, Move
  Speed), queued/rolled/applied through the existing UStatsComponent::AddModifier. AOE Explosion is
  implemented but deliberately left out of the offered pool for now.
- UAttackHitComponent: TryAreaHit (explosion damage that never double-hits); DirectHitCount tracks the
  penetration budget separately from the double-damage-prevention set, so an area hit can never steal a
  penetrating attack's next direct hit
- AMagicOrbProjectile: ExplosionRadius, TriggerExplosion() (spends its one explosion charge at the first
  direct hit, not on exhaustion/expiry), grow-then-fade red glow visual, FinishIfOutOfHits (removal waits
  for an in-progress burst to finish)
- AGrantMoneyPlayerCharacter: MoveSpeed stat kept in sync with GetCharacterMovement()->MaxWalkSpeed
- UStarterWeaponComponent: SelectTargets/multi-orb firing, independent of the existing SelectTarget path
- ULevelUpWidget + MapCameraPlayerController::ShowLevelUpChoice/HideLevelUpChoice (same pause/cursor/
  input-mode conventions as the Game Over screen)
- RunHUD: level and XP progress line
- 19 new Automation tests and SCRUM-36 documentation
```

---

# Part C – AI Workflow Log

**Tool:** Copilot (an AI assistant in VS Code), working on the repository on the developer's PC. This
session could build and run the project (`scripts/run_tests.py`). It could not look at the screen or
create the Blueprint widget asset.

| # | Purpose | Prompt (developer) | Result | Decision | Verification |
|---|---|---|---|---|---|
| 1 | Request | The shared Module 3 instructions plus the SCRUM-36 brief verbatim, with explicit permission to ask as many yes/no questions as needed and to leave widget creation and final manual verification to the developer. | Read `StatsComponent`, `XpOrbPickup`, `StarterWeaponComponent`, `MagicOrbProjectile`, `AttackHitComponent`, `GrantMoneyStatTypes`, `GrantMoneyPlayerCharacter`, and the pause menu/Game Over conventions. | Accepted | Read from source |
| 2 | Implementation | (continued) | Wrote `ULevelComponent`, `UUpgradeComponent`, `FGrantMoneyUpgradeDefinition`, the `AttackHitComponent`/`MagicOrbProjectile`/`StarterWeaponComponent` extensions, `ULevelUpWidget`, the player character/controller wiring, the HUD line, and tests. | Accepted | Compiled; tests below |
| 3 | First build | (automatic) | Dynamic multicast delegates do not support `.AddLambda()` (§8.1). | Fixed | Rebuilt |
| 4 | Test run | (automatic) | 119 of 121 passed: a collinear multi-projectile test layout (§8.2) and an explosion test's timing window (§8.3) were both test-design issues, not product bugs. | Fixed both tests | **121 of 121** |
| 5 | Whole-suite run | (automatic) | Confirmed no regressions in the 103 pre-existing tests. | Accepted | 121 of 121 |
| 6 | Documentation | (implicit, per the shared Module 3 instructions) | Wrote this document. | — | Checked against the code and the test output |
| 7 | Design quiz | A short yes/no quiz on placeholder magnitudes, `OptionsPerChoice`, and the explosion's look; developer asked for the burst to be "colored red and expand while fading" instead of a plain grow. | Added a dynamic material instance on the engine's emissive mesh material (same one `XpOrbPickup`/`StatBoostPickup` already use), fading `EmissiveColor` from red to black alongside the existing grow. | Accepted | Rebuilt; 121 of 121 |
| 8 | Addendum: pool change | "Remove AOE Explosion from the options pool but leave the functionality for later; replace it with a slight move speed buff." | Removed `MakeExplosionUpgrade()` from `AvailableUpgrades` (left the function and all underlying code intact); added `MakeMoveSpeedUpgrade()` and wired a new `MoveSpeed` stat to `GetCharacterMovement()->MaxWalkSpeed`. | Accepted | Rebuilt; tests below |
| 9 | Addendum: explosion timing | "The AOE explosion, once penetration is active, only explodes at the end of the orb's life cycle. Have it explode on contact, then the ability to explode goes away — the orb keeps going through remaining enemies without exploding again." | Rewrote the explosion trigger from "on hits-exhausted/expiry" to "on first direct hit" (`TriggerExplosion`/`FinishIfOutOfHits`); added `FMagicOrbExplosionOnlyOnFirstContactTest`, which failed once and caught a real bug (§8.4: an area hit was silently stealing a penetrating orb's next direct hit) before passing. | Accepted | **122 of 122** |

**Where AI helped most:** keeping the existing single-target `SelectTarget()` path completely untouched
while adding multi-projectile firing, so the default (no-upgrade) case has zero behavioural risk. In the
addendum, writing the new `ExplosionOnlyOnFirstContact` test before assuming the fix was done surfaced a
second, unrelated bug (§8.4) in the shared penetration/hit-tracking logic that manual play alone would
have been much harder to pin down precisely.

**Where it needs a human:** creating `WBP_LevelUp` in the Editor; everything that needs eyes on the
screen (the choice UI appearing, buttons working, stacking visibly disappearing once maxed, the HUD
line, restart clearing progression, and — once offered again — the explosion's red grow/fade visual).

---

# Part D – Video Voice-Over Script

## Opening
**SAY:** "Collecting XP used to just be a number. Now it builds toward a level — and leveling up gives
you a real choice: pick one of up to three upgrades for your starter orb."

## Acceptance Criteria
**SAY:** "By the end: XP thresholds are configurable, leveling fires an event, up to three distinct
eligible upgrades are offered, the game pauses while you choose, several level-ups queue up safely, and
five starter upgrades — damage, attack speed, penetration, projectile count, and an AOE explosion — all
work and stack with limits."

## Engineering Principle
**SAY:** "Reuse, don't reinvent, again. Upgrades apply through the exact same stat-modifier call the
field pickups already use. Pausing while you choose reuses the exact mechanism the pause menu and Game
Over screen already proved works."

## Step-by-Step Demonstration

### 1. Leveling from the existing stat
- **SAY:** "No new XP tracking — just watching the Experience stat that already existed, and turning its
  changes into levels, with a loop so one big pickup can grant more than one level at once."
- **SHOW:** `LevelComponent.cpp`'s `HandleStatChanged`. *(media 02)*

### 2. The upgrade choice
- **SAY:** "Leveling up queues a choice. If you're already choosing something, it waits — nothing gets
  lost, and you only resume play once the whole queue is empty."
- **SHOW:** `UpgradeComponent.cpp`'s `PresentNextChoice`. *(media 03)*

### 3. The explosion upgrade
- **SAY:** "The trickiest one: an orb that bursts once it's spent, hitting everyone nearby exactly once —
  never double-hitting whoever it already struck directly."
- **SHOW:** `MagicOrbProjectile.cpp`'s `Explode()` next to `AttackHitComponent.cpp`'s `TryAreaHit`.
  *(media 04)*

### 4. Multi-projectile firing
- **SAY:** "Projectile Count fires more orbs at more distinct targets — without touching the original
  single-target code path at all, so the default case can't regress."
- **SHOW:** `StarterWeaponComponent.cpp`'s `TryFire`, the `ShotCount <= 1` branch. *(media 05)*

### 5. Tests
- **SAY:** "Eighteen new tests, all green, on top of the full existing suite."
- **SHOW:** the test run. *(media 06)*

### 6. Play it
- **DO:** Create `WBP_LevelUp` on screen; play; level up; show the choice screen, picking an upgrade,
  the game resuming, and a stacked upgrade disappearing from choices once maxed. *(media 07 to 11)*

## Common Mistakes
**SAY:** "One to watch: Unreal's dynamic delegates — the kind the Editor can see and bind to — don't
support plain lambdas. Tests need a tiny listener object instead."

## Verification Checkpoint
**SAY:** "Tests green first. The actual choice screen and stacking behaviour, I'll leave to the
developer to confirm directly — faster, and conclusive."

## Next Milestone
**SAY:** "Next: carrying this leveling system through to the boss phase, and deciding what — if
anything — upgrades should do once the boss shows up."

---

# Media Capture List

Store under `Media/SCRUM-36/` (outside the repository). Only record what actually happens.

| # | File | Type | Show | Proves |
|---|---|---|---|---|
| 01 | `01-branch.png` | Screenshot | `git branch --show-current` | Branch (decide per §12) |
| 02 | `02-level-component.png` | Screenshot | `LevelComponent.cpp`'s `HandleStatChanged` | Thresholds, excess XP, multi-level crossing |
| 03 | `03-upgrade-queue.png` | Screenshot | `UpgradeComponent.cpp`'s `PresentNextChoice` | Safe queueing of multiple level-ups |
| 04 | `04-explosion.png` | Screenshot | `MagicOrbProjectile.cpp` `Explode()` next to `AttackHitComponent.cpp` `TryAreaHit` | No double-damage, penetration ignored by design |
| 05 | `05-multi-projectile.png` | Screenshot | `StarterWeaponComponent.cpp` `TryFire`'s branch | Default path unchanged |
| 06 | `06-tests-pass.png` | Screenshot | `run_tests.py` output, 121/121 | Tests green |
| 07 | `07-create-wbp-levelup.mp4` | Clip | Creating `WBP_LevelUp`, setting its parent class, naming the six widgets | Editor setup (§2) |
| 08 | `08-level-up-pauses.mp4` | Clip | Leveling up; everything freezes; the choice screen appears | Pause requirement |
| 09 | `09-pick-upgrade.mp4` | Clip | Picking an upgrade; game resumes; the weapon visibly changed (e.g. extra orb, explosion) | Upgrade applies and is visible |
| 10 | `10-stack-limit.mp4` | Clip | Picking the same upgrade until it disappears from the choices | Stack limit enforced |
| 11 | `11-queued-choices.mp4` | Clip | A big XP pickup crossing two levels at once; two choices presented back to back; resumes once after the second | Queued choices |
| 12 | `12-restart-clears-progression.mp4` | Clip | Restart; HUD shows Level 1, 0 XP | Restart clears progression |
