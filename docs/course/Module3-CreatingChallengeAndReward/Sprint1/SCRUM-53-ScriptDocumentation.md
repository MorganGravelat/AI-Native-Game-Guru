# SCRUM-53 – Rare Permanent Stat Boost Drops
## Module 3 Sprint 1 – Development and Recording Guide

This guide covers SCRUM-53: enemies can now drop rare **permanent stat boosts**. A boost uses the pickup system from SCRUM-35 unchanged. It flies to the player, is consumed the moment it arrives (there is no inventory), and adds one permanent modifier to one of the player's stats: **Max Health, Damage, Attack Interval (attack speed), Penetration** or **Contact Damage**. Each boost can show a 2D icon that always faces the camera, and can play a sound and a Niagara effect when it is picked up. Every number is editable in the Details panel and has a code default.

- **Part A:** change report
- **Part B:** developer implementation guide, including [B6A, the designer setup for icons, sounds and drop chances](#6a-designer-setup-icons-sounds-and-drop-chances)
- **Part C:** AI workflow log
- **Part D:** video voice-over script
- **Media Capture List:** every screenshot and clip to record, with file names

**Status:** the code, tests and this document are written. **It has not been compiled and no Automation test has been run yet**: the AI session that wrote it had no Unreal Engine available (see Part C). Run `python scripts/run_tests.py` with the Editor closed before anything else. The manual PIE checks in [B9](#9-verification) have not been performed. No media has been captured.

**What is and is not evidence in this document:**

| Kind | Meaning |
|---|---|
| Read from source | Taken from reading project or engine source. Not a test. |
| Compiled | UnrealBuildTool finished with `Result: Succeeded`. Says nothing about behaviour. |
| Automation test | A `GrantMoney.*` Automation test passed in a headless editor run, in a synthetic empty world. |
| Developer reported | The developer played it in the Editor and said what they saw. |

At the time of writing, **only "Read from source" applies**.

---

## Jira

| Key | Goal |
|---|---|
| SCRUM-53 | Enemies drop rare permanent stat boosts (max health, contact damage, damage, attack interval, penetration). Instant consumption, flat or percent modifiers, permanent duration, sound and visual feedback, 2D camera-facing icons, and every value (amount, drop chance) editable in the Details panel or in code. |

**Done when, and where each one is met:**

| Criterion | Met by | Evidence |
|---|---|---|
| Reusable, modular C++ stat boost that hooks into the existing drop and pickup pipeline | `AStatBoostPickup : APickupBase`, overriding only `OnCollected` | Read from source; tests written |
| Instant consumption, no inventory | `APickupBase::TryCollect` runs `OnCollected` once and destroys the pickup | Test `StatBoost.AppliesOnceOnArrival` (not yet run) |
| Stat type enum | Existing `EGrantMoneyStat` (no new enum) | Read from source |
| Flat vs percent | Existing `EGrantMoneyStatModifierOp` (`Additive` / `Multiplicative`), exposed as **Modifier Type** | Test `StatBoost.FlatAndPercentStack` (not yet run) |
| Permanent duration | The modifier handle is never kept, so nothing removes it during the run | Read from source |
| Health boost raises only max health | New `UHealthComponent::bHealOnMaxHealthGain`, set to false on the player | Tests `StatBoost.MaxHealthRaisesOnlyTheMaximum`, `HealthComponent.MaxHealthGainWithoutHeal` (not yet run) |
| Penetration N lets an orb hit N + 1 enemies | Existing `UAttackHitComponent` rule; the weapon now passes the boosted value to each orb | Test `StarterWeapon.PenetrationTwoHitsThreeSlimes` (not yet run) |
| Damage / attack speed boosts change the shots | `UStarterWeaponComponent::GetEffectiveDamage` / `GetEffectiveFireInterval` | Test `StarterWeapon.OwnerStatBoostsApply` (not yet run) |
| Sound on pickup | `AStatBoostPickup::PickupSound` (+ volume) | Read from source; PIE pending |
| 2D image that always faces the player | `AStatBoostPickup::Icon` on a `UMaterialBillboardComponent` | Test `StatBoost.IconReplacesSphere` (not yet run); look is PIE pending |
| Rare, editable drop chance | Four entries in `ASlimeEnemy::BonusPickupDrops` at `DefaultStatBoostDropChance` (1% each) | Test `EnemyDrops.SlimeDropsRareStatBoosts` (not yet run) |
| Unit tests per `CLAUDE.md` | `StatBoostPickupTests.cpp` plus updated tests for each changed class | Written; not yet run |

**Related tickets:** SCRUM-33 (stats, health, death), SCRUM-32 (starter weapon, magic orb), SCRUM-35 (pickup system and `BonusPickupDrops`).

---

# Part A – Change Report

## Milestone Summary

Slimes now have a small chance to drop one of four permanent stat boosts next to their guaranteed XP orb. A boost flies to the player like an XP orb and permanently changes one stat. To make those changes visible, the starter weapon now reads the player's Damage, AttackInterval and Penetration stats (before this ticket it ignored them; see SCRUM-35 §8.4), and the player's max health can rise without healing.

## Files Created

| File | Purpose |
|---|---|
| `Game/Source/GrantMoney/Public/StatBoostPickup.h`, `Private/StatBoostPickup.cpp` | `AStatBoostPickup` (Stat, Modifier Type, Amount, Icon, IconSize, IconMaterial, GlowColor, PickupSound, PickupSoundVolume, PickupEffect; `ApplyBoost`, `IsShowingIcon`). Ready-made subclasses: `AMaxHealthBoostPickup` (+10 flat), `ADamageBoostPickup` (+2 flat), `AAttackSpeedBoostPickup` (-5% interval), `APenetrationBoostPickup` (+1), `AContactDamageBoostPickup` (+2 flat). |
| `Game/Source/GrantMoney/Private/StatBoostPickupTests.cpp` | 6 tests, `GrantMoney.Pickups.StatBoost.*` |
| `docs/course/Module3-CreatingChallengeAndReward/Sprint1/SCRUM-53-ScriptDocumentation.md` | This document. |

## Files Modified

| File | Change | Why |
|---|---|---|
| `Public/StatsComponent.h`, `Private/StatsComponent.cpp` | New `GetStatFromBase(Stat, BaseValue)`. `GetStat` now calls it with the stored base. | Lets the weapon keep its own editable base values and still apply the player's boost modifiers. |
| `Public/HealthComponent.h`, `Private/HealthComponent.cpp` | New `bHealOnMaxHealthGain` (default **true**, so enemies and the existing test keep today's behaviour). | Requirement: a health boost raises only max health. |
| `Private/GrantMoneyPlayerCharacter.cpp` | `Health->bHealOnMaxHealthGain = false`. | Same. |
| `Public/StarterWeaponComponent.h`, `Private/StarterWeaponComponent.cpp` | New `Penetration` property and `GetEffectiveDamage`, `GetEffectiveFireInterval`, `GetEffectivePenetration`. Shots and the fire timer use them. | Damage, attack speed and penetration boosts must change the shots. |
| `Public/MagicOrbProjectile.h`, `Private/MagicOrbProjectile.cpp` | `Launch(..., int32 Penetration = 0)` sets `AttackHit->FallbackPenetration`. | Each orb carries the weapon's penetration at the moment it is fired. |
| `Public/SlimeEnemy.h`, `Private/SlimeEnemy.cpp` | `DefaultStatBoostDropChance = 0.01f`; four `BonusPickupDrops` entries. | Rare drops with a code default, editable per entry in the Details panel. |
| `GrantMoney.Build.cs` | Private dependency on `Niagara`. | Optional pickup effect. The Niagara plugin is enabled by default in UE 5. |
| `Private/StatsComponentTests.cpp`, `HealthComponentTests.cpp`, `StarterWeaponComponentTests.cpp`, `EnemyDropTests.cpp` | New tests (see B9). `EnemyDrops.SlimeDropsExactlyOneOrb` now clears the slime's bonus drops first. | `CLAUDE.md`: changed logic gets tests. The orb-count test would otherwise fail about 4% of the time now that slimes have rare bonus drops. |

## Files Not Changed

- `APickupBase`, `UPickupComponent`, `AXpOrbPickup`, `FEnemyPickupDrop`, `AEnemyBase` (drop spawning), `UAttackHitComponent`, `UContactDamageComponent`, `GrantMoneyStatTypes.h`. The ticket fits into them as they are.
- No `.uasset`/`.umap` was created or edited. Config, `.uproject` and map/TileGen code are untouched.

---

# Part B – Developer Implementation Guide

## 1. Objective

At the end:
- a slime sometimes drops a coloured boost (1% per boost type, so about 4% of kills drop at least one);
- picking it up permanently raises a stat, with an optional sound, effect and 2D icon;
- damage, attack-speed and penetration boosts visibly change the starter weapon's orbs;
- a max-health boost raises the ceiling and leaves current health alone;
- designers tune every amount and chance in the Details panel without touching code.

## 2. Why This System Exists

**Before this ticket**, the pickup system could deliver any effect, and `UStatsComponent` could hold permanent modifiers, but nothing dropped them and the weapon ignored the Damage, AttackInterval and Penetration stats.

**Chosen architecture:**

- **A boost is a pickup.** It derives from `APickupBase` and overrides `OnCollected`. Detection, flight, once-only collection and removal are reused.
- **A boost is a stat modifier.** Stat type and flat/percent are the existing `EGrantMoneyStat` and `EGrantMoneyStatModifierOp`. "Permanent" means the handle is never stored, so it is never removed.
- **Readers react on their own.** Health, the weapon and contact damage read stats when they need them, so a boost needs no reference to the player class.
- **Drops are data on the enemy.** The existing `BonusPickupDrops` list with its independent `DropChance` per entry.
- **The weapon owns its base numbers, the player owns the boosts.** `GetStatFromBase` applies the player's modifiers to the weapon's own `Damage` / `FireInterval` / `Penetration`, so editing the weapon's base values in the Details panel still works.

**Alternatives considered:**

| Alternative | Decision |
|---|---|
| A new `UStatBoostComponent` / receiver interface | **Rejected.** `UStatsComponent` already is that component. |
| A new stat enum or modifier struct for boosts | **Rejected.** `EGrantMoneyStat` and `EGrantMoneyStatModifierOp` already cover all five stats and both modifier types. |
| Put Damage/AttackInterval/Penetration base values on the player's Stats | **Rejected.** That creates two "Damage" settings (weapon and player) where only one works. |
| `UBillboardComponent` for the icon (takes a texture directly) | **Rejected.** It is an editor sprite: it only draws when the BillboardSprites show flag is on, which is off in game and fixed off in shipping. `UMaterialBillboardComponent` renders in game. |
| `UPaperSpriteComponent` | **Rejected for now.** Needs a Sprite asset per image and code to face the camera. A material billboard takes the texture as-is and always faces the camera. |
| A data asset per boost | **Deferred.** Blueprint subclasses of the ready-made classes give the same Details-panel editing with no new asset type. |

## 3. Prerequisites

- SCRUM-33, SCRUM-32 and SCRUM-35 merged.
- Unreal Engine 5.8, Editor **closed** when running `scripts/run_tests.py` (Live Coding breaks the build).
- Paper2D and Niagara plugins enabled (both are on by default) if you use icons or effects.

## 4. Architecture Overview

```
Slime dies ─► AEnemyBase::SpawnPickupDrops ─► rolls each BonusPickupDrops entry (1% each by default)
                                                  │
                                                  ▼
                                        AStatBoostPickup (e.g. ADamageBoostPickup)
                                                  │  UPickupComponent finds it, APickupBase flies it in
                                                  ▼
                                        OnCollected ─► ApplyBoost ─► Player UStatsComponent::AddModifier (permanent)
                                                  │                         │ OnStatChanged / read on demand
                                                  │                         ├─► UHealthComponent: MaxHealth (no heal on the player)
                                                  │                         └─► UStarterWeaponComponent: Damage, AttackInterval, Penetration
                                                  └─► PlayPickupFeedback: PickupSound (2D), PickupEffect (Niagara on the player)
```

**Stat maths** (unchanged from SCRUM-33): `final = (base + Σ flat) × (1 + Σ percent)`, never below 0. Percent boosts on the same stat add before being applied.

| Stat | Reader | Base | Default boost | Notes |
|---|---|---|---|---|
| MaxHealth | `UHealthComponent` | Player Stats, 100 | +10 flat | Player does not heal on gain. |
| Damage | Starter weapon | Weapon `Damage`, 10 | +2 flat | |
| AttackInterval | Starter weapon | Weapon `FireInterval`, 1 s | -5% | Lower is faster. Clamped at 0.05 s. |
| Penetration | Starter weapon → orb | Weapon `Penetration`, 0 | +1 | Rounded down. 0 = 1 enemy, 2 = up to 3. |
| ContactDamage | `UContactDamageComponent` | — | +2 flat | **The player has no contact damage component**, so this boost does nothing yet and is not in the slime's drop list. |

## 5. AI-Assisted Planning

### AI Prompt
The developer's request (Part C, row 1), plus answers to the AI's clarifying questions: stats are in the stats component; headers in `Game/Source/GrantMoney/Public`; single-player; UE 5.8, no sounds yet but each boost should be able to play one; 2D images facing the player; drop chance and amounts editable in the Details panel or code; health boosts raise only max health; penetration N hits N + 1 enemies; permanent, rare; reuse what exists; follow `CLAUDE.md`.

### Expected AI Output
A list of the existing classes that already cover part of the ticket, the gaps, and the smallest set of new files.

### Human Review
- Confirm that contact damage on the player is a later decision (see §10).
- Choose the drop rate (1% per boost type is a placeholder).
- Supply icon textures and sounds.

## 6. Step-by-Step Implementation

### Step 1 – Make the weapon read the player's stats
`UStatsComponent::GetStatFromBase` runs the stat formula on a base the caller supplies. `UStarterWeaponComponent` gets `GetEffectiveDamage`, `GetEffectiveFireInterval` (clamped at 0.05 s) and `GetEffectivePenetration` (rounded down), and uses them in `FireAt` and `ScheduleNextShot`. `AMagicOrbProjectile::Launch` takes the penetration and puts it on the orb's `UAttackHitComponent`. If the owner's Stats has a *base* value for one of those stats, that base replaces the weapon's, which is the rule the other combat components already follow.

### Step 2 – Health boosts raise only the maximum
`UHealthComponent::bHealOnMaxHealthGain` (default true). The player sets it to false in its constructor.

### Step 3 – The stat boost pickup
`AStatBoostPickup::OnCollected` calls `ApplyBoost` (one `AddModifier`, handle discarded) and then `PlayPickupFeedback` (sound, Niagara). In `BeginPlay`, if `Icon` is set, it loads `IconMaterial` (soft reference, Paper2D's `MaskedUnlitSpriteMaterial` by default), makes a dynamic instance with `Icon` in its `SpriteTexture` parameter, adds it to the camera-facing billboard at `IconSize`, and hides the sphere. Without an icon it tints a sphere with `GlowColor`. Five ready-made subclasses set the defaults per stat.

### Step 4 – Rare drops
`ASlimeEnemy` adds four `FEnemyPickupDrop` entries (not contact damage) at `DefaultStatBoostDropChance`.

### Step 5 – Tests and whole-suite run
`python scripts/run_tests.py` (or `--filter GrantMoney.Pickups.StatBoost`). **Not yet run.**

## 6A. Designer Setup: Icons, Sounds and Drop Chances

1. **Import the art.** Drag your PNG (with transparency) into `Content/Pickups/StatBoosts/`. In the texture, set Compression to `UserInterface2D` and Texture Group to `UI` so it stays crisp.
2. **Make a Blueprint per boost.** Content Browser → right-click → Blueprint Class → search `DamageBoostPickup` (or another ready-made class) → name it `BP_DamageBoost`.
3. **Set its values** in Class Defaults → Details:
   - **Pickup | Stat Boost:** `Stat`, `Modifier Type` (Additive = flat, Multiplicative = percent as a fraction: 0.1 = +10%), `Amount` (negative for Attack Interval).
   - **Pickup | Visual:** `Icon` = your texture, `Icon Size` (cm, default 60).
   - **Pickup | Feedback:** `Pickup Sound`, `Pickup Sound Volume`, optional `Pickup Effect` (Niagara system).
4. **Point the slime at it.** Open the slime Blueprint (or select a placed slime) → Enemy | Drops → `Bonus Pickup Drops` → set the entry's `Pickup Class` to `BP_DamageBoost` and its `Drop Chance` (0.01 = 1%). Add or remove entries freely; each rolls on its own.
5. **Change the code defaults** in `SlimeEnemy.h` (`DefaultStatBoostDropChance`) and the subclass constructors in `StatBoostPickup.cpp` (amounts and colours).

**If the icon shows as a grey/checkered square or not at all:** make your own material: New Material → Shading Model `Unlit`, Blend Mode `Masked`, add a `TextureSampleParameter2D` named `SpriteTexture`, RGB → Emissive Color, A → Opacity Mask. Assign it to the Blueprint's `Icon Material`. (Paper2D's material is the default but has not been checked on screen; see §8.4.)

## 7. AI Prompts by Stage

- **Prompt A – Inspect and plan:** "Read CLAUDE.md, then the headers for the stats, health, pickup, drop, weapon and orb classes. List what already exists for each requirement and what is missing. No code yet."
- **Prompt B – Wire the readers:** "Make the starter weapon apply the owner's Damage, AttackInterval and Penetration modifiers on top of its own values, and pass penetration to each orb. Keep the weapon's properties editable as the base."
- **Prompt C – The pickup:** "Add a stat boost pickup derived from APickupBase with stat, flat/percent and amount, an optional camera-facing 2D icon, a sound and a Niagara effect, plus one ready-made subclass per stat."
- **Prompt D – Tests:** "Write Automation tests in the house style for every new and changed behaviour."

## 8. Common Mistakes

### 8.1 A boost that changes a stat nothing reads
- **Symptom:** the Damage stat goes up but the orbs hit just as hard.
- **Cause:** before this ticket the weapon copied its own `Damage` into each orb. (Flagged in SCRUM-35 §8.4.)
- **Fix:** Step 1. **Contact damage is still in this state on the player** (§10).
- *Found while reading the source before writing code.*

### 8.2 The existing health rule contradicted the ticket
- **Symptom (avoided):** a max-health boost would also heal the player.
- **Cause:** `UHealthComponent` heals by the amount gained, by design (SCRUM-33), and a test covers it.
- **Fix:** a per-component flag rather than changing the shared rule, so the existing test and enemies keep their behaviour.
- *Found while reading the source.*

### 8.3 Unity-build name clash
- **Symptom (avoided):** `PlaceholderScale` redefinition when Unreal merges `.cpp` files in a unity build.
- **Cause:** the first draft of `StatBoostPickup.cpp` used the same anonymous-namespace constant name as `SlimeEnemy.cpp`.
- **Fix:** renamed to `StatBoostPlaceholderScale` (and the helper to `StatBoostStatName`).
- *Caught in the AI's own diff review, before any build.*

### 8.4 Icon material not confirmed on screen
- **Risk:** the default icon material (`/Paper2D/MaskedUnlitSpriteMaterial`, texture parameter `SpriteTexture`) comes from knowledge of Paper2D, not from opening the asset. If the name is wrong, `SetTextureParameterValue` silently does nothing and the icon will not appear. An engine material may also need the "used with" flags for a billboard.
- **Fix:** see the end of 6A. Both `Icon Material` and `Icon Texture Parameter Name` are editable.
- *Open item until PIE (media 12).*

### 8.5 A flaky test from a random drop
- **Symptom (avoided):** `EnemyDrops.SlimeDropsExactlyOneOrb` counts every pickup, and slimes now have rare random drops.
- **Fix:** the test clears `BonusPickupDrops` first. Rare drops are tested separately with chances forced to 1.

## 9. Verification

### Performed
None in Unreal. The code was written and reviewed by reading only.

### To run first (developer)
`python scripts/run_tests.py` with the Editor closed. New or changed tests:

| Group | Tests |
|---|---|
| `Pickups.StatBoost` (new file) | ReadyMadeDefaults, AppliesOnceOnArrival, FlatAndPercentStack, MaxHealthRaisesOnlyTheMaximum, NoStatsOrZeroAmountChangesNothing, IconReplacesSphere |
| `Combat.StatsComponent` | StatFromBase |
| `Combat.HealthComponent` | MaxHealthGainWithoutHeal |
| `Weapons.StarterWeapon` | OwnerStatBoostsApply, PenetrationTwoHitsThreeSlimes |
| `Pickups.EnemyDrops` | SlimeDropsRareStatBoosts (new); SlimeDropsExactlyOneOrb (updated) |

If anything fails, record the real error under §8 before fixing it.

### Pending (developer, in the Editor)

To see boosts quickly, set every slime `Bonus Pickup Drops` chance to 1.0 temporarily, then reset to 0.01.

| Test | Action | Expected Result |
|---|---|---|
| Slime defaults | Select a slime → Enemy\|Drops | Four entries (MaxHealth, Damage, AttackSpeed, Penetration boost), 0.01 each |
| Placeholder look | Kill a slime with chances at 1.0 | Four coloured spheres (red, orange, yellow, blue) and a green XP orb |
| Max health | Collect the red one; select player → Health | Max health 110; current health unchanged |
| Damage | Collect orange, shoot a slime | Slime loses 12 per orb (30 → 18) |
| Attack speed | Collect yellow several times | Shots visibly more frequent |
| Penetration | Collect blue twice, line up 4 slimes | One orb damages 3, the 4th untouched |
| Icon | Make `BP_DamageBoost` with an Icon (6A) and use it | The image floats at the drop spot and faces the camera from every angle; no sphere |
| Sound / effect | Set a sound and a Niagara system | Played once on pickup; the effect follows the player |
| Output Log | Filter `StatBoostPickup` | `... gained +2.00 Damage (permanent).` once per pickup |
| Rarity | Reset to 0.01, play a normal wave | Boosts are rare (about 1 in 25 kills drops one) |

## 10. Known Limitations

- **Not compiled or tested yet** (see Status).
- **Contact damage boosts do nothing on the player.** The player has no `UContactDamageComponent`. `AContactDamageBoostPickup` exists but is not in the slime's list. Deciding whether the player deals contact damage is a design call.
- **Attack speed and contact damage share the AttackInterval stat** on any actor that has both a weapon and a contact damage component.
- **Boosts last for the current run only.** They are runtime modifiers on the player; nothing saves them across levels or sessions.
- **No HUD** shows the boosted stats.
- The icon look and the default icon material are unverified (8.4).
- The pickup sound is 2D (same volume anywhere), which suits a single-player camera view.

## 11. Extension Ideas

- A temporary boost: keep the modifier handle and remove it with a timer on the collector (SCRUM-35 §8.5).
- A shared drop table data asset so new enemy types reuse the slime's list.
- A HUD panel bound to `UStatsComponent::OnStatChanged`.
- Saving boosts in a `USaveGame` if they should survive between levels.

## 12. Source-Control Checkpoint

| Item | Value |
|---|---|
| Jira | SCRUM-53 |
| Branch | `SCRUM-53-Create-enemy-Drop-stat-boosts` (current) |
| PR title | `SCRUM-53: Rare permanent stat boost drops` |

Suggested commit message (after tests pass):
```
SCRUM-53: Add rare permanent stat boost drops

- AStatBoostPickup + ready-made MaxHealth/Damage/AttackSpeed/Penetration/ContactDamage boosts
- Starter weapon applies the owner's Damage, AttackInterval and Penetration modifiers
- Magic orb penetration set per shot; UStatsComponent::GetStatFromBase
- Player max-health boosts no longer heal (UHealthComponent::bHealOnMaxHealthGain)
- Slimes drop each boost at 1%; Niagara dependency for pickup effects
- Tests and SCRUM-53 course documentation
```

---

# Part C – AI Workflow Log

**Tool:** Claude, working on the repository through a folder linked from the developer's PC. It could read and write files but **could not run Unreal, UnrealBuildTool or `run_tests.py`**, so nothing was compiled or tested.

| # | Purpose | Prompt (developer) | Result | Decision | Verification |
|---|---|---|---|---|---|
| 1 | Request | "Act as a Principal Unreal Engine C++ Developer. I need to extend my existing Item Drop system to support droppable Stat Boosts … Please ask any clarifying questions … before generating the full implementation." | Asked about where stats live, the pickup pipeline, multiplayer, engine/FX, stat maths and persistence. | Accepted | — |
| 2 | Answers | Stats in a stats component; headers in Public; single-player; UE 5.8; sound per boost; 2D facing icons; editable chances and amounts; max-health only; penetration rule; permanent and rare; "Utilize what is already there … Follow claude.md". | Read `CLAUDE.md` and only the relevant headers. Found stats, modifiers, pickups and drops already cover most of it; found the weapon ignored stats and health healed on gain. | Accepted | Read from source |
| 3 | Implementation | (continued) | Files in Part A. Caught 8.3 in its own diff review. | Pending developer review | Not compiled |
| 4 | Documentation | (continued) | This document. | — | Checked against the diff |

**Where AI helped most:** mapping requirements to existing classes before writing anything, which kept the change to one new class plus small edits.

**Where it needs a human:** compiling and running the tests, checking the icon material on screen, choosing drop rates and art, and deciding on player contact damage.

---

# Part D – Video Voice-Over Script

## Opening
**SAY:** "Slimes already drop XP. Today they sometimes drop something better: a permanent upgrade. More health, harder hits, faster shots, or orbs that punch through enemies. And we'll do it by writing almost nothing new."

## Acceptance Criteria
**SAY:** "By the end: a rare boost drops, flies to us, and is used up instantly; the stat changes for the rest of the run; the weapon really fires harder, faster or through more enemies; a health boost raises the maximum without healing; and every number, including the drop chance, is in the Details panel."

## Engineering Principle
**SAY:** "Before writing a class, find the one that already does the job. The pickup system, the stats and the drop list already exist. We only connect them."

## Step-by-Step Demonstration

### 1. Ask the AI to map the ticket to existing code
- **SAY:** "First I ask the AI what already exists for each requirement."
- **SHOW:** the requirement-to-class table (§4). *(media 02)*
- **VERIFY:** every class named is real.

> **Engineering discussion.** The AI found that the weapon never read the Damage stat. A boost would have changed a number nothing uses. Always check that something reads the value you are changing.

### 2. Make the weapon read the player's stats
- **SHOW:** `GetEffectiveDamage`, `GetEffectiveFireInterval`, `GetEffectivePenetration`. *(media 03)*
- **SAY:** "The weapon keeps its own base numbers. The player's boosts are applied on top."

### 3. The boost pickup
- **SHOW:** `AStatBoostPickup::OnCollected` and the ready-made subclasses. *(media 04)*
- **SAY:** "One function: add a modifier and never remove it. That's what 'permanent' means here."

### 4. Rare drops and tests
- **SHOW:** the slime's `Bonus Pickup Drops` and the test run. *(media 05, 06)*

### 5. Play it
- **DO:** chances at 1.0 for the demo; collect each boost; show the stat and the behaviour change. *(media 07 to 11)*
- **DO:** add an icon and a sound through a Blueprint. *(media 12, 13)*
- **DO:** reset chances to 1%.

## Common Mistakes
**SAY:** "Three to watch for: a boost on a stat nothing reads; a shared rule like 'heal on max health gain' that you need to switch off, not rewrite; and tests that count random drops."

## Verification Checkpoint
**SAY:** "Tests green, then the PIE table in the guide. Only mark what you saw."

## Next Milestone
**SAY:** "Next, a HUD for these stats, or deciding whether the player deals contact damage so the fifth boost has something to do."

---

# Media Capture List

Store under `Media/SCRUM-53/` (outside the repository). Only record what actually happens.

| # | File | Type | Show | Proves |
|---|---|---|---|---|
| 01 | `01-branch.png` | Screenshot | `git branch --show-current` → `SCRUM-53-Create-enemy-Drop-stat-boosts` | Ticket branch |
| 02 | `02-existing-systems-map.png` | Screenshot | AI answer mapping requirements to `UStatsComponent`, `APickupBase`, `BonusPickupDrops`, `UAttackHitComponent` | Reuse before new code |
| 03 | `03-weapon-effective-stats.png` | Screenshot | `StarterWeaponComponent.cpp` `GetEffective*` and `FireAt` | Boosts change the shots |
| 04 | `04-stat-boost-pickup.png` | Screenshot | `StatBoostPickup.cpp` `ApplyBoost`, `OnCollected`, subclasses | The whole effect is one modifier |
| 05 | `05-slime-bonus-drops.png` | Screenshot | Slime Details → `Bonus Pickup Drops`, four entries at 0.01 | Rare, editable chances |
| 06 | `06-tests-pass.png` | Screenshot | `run_tests.py` / Automation with `GrantMoney.Pickups.StatBoost` expanded | Tests green (**only after they really pass**) |
| 07 | `07-boost-drop.mp4` | Clip | Chances 1.0, kill a slime: four coloured spheres + XP orb | Drops work |
| 08 | `08-max-health-no-heal.mp4` | Clip | Hurt player, collect red; Health shows max 110, current unchanged | Max-only rule |
| 09 | `09-damage-boost.mp4` | Clip | Before/after hit on a slime (10 → 12) | Damage boost |
| 10 | `10-attack-speed.mp4` | Clip | Fire rate before/after several yellow boosts | Attack interval boost |
| 11 | `11-penetration.mp4` | Clip | Two blue boosts, 4 slimes in a line, one orb hits 3 | Penetration rule |
| 12 | `12-icon-billboard.mp4` | Clip | `BP_DamageBoost` with an icon; orbit the camera | Faces the camera (**confirms 8.4**) |
| 13 | `13-sound-and-effect.mp4` | Clip | Pickup with sound and Niagara effect | Feedback hooks |
| 14 | `14-output-log.png` | Screenshot | Output Log filtered `StatBoostPickup` | One application per pickup |
