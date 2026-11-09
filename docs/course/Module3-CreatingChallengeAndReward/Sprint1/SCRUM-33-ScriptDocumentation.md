# SCRUM-33 – Stat-Driven Health, Damage, Death and Hit Rules
## Module 3 Sprint 1 – Development and Recording Guide

This guide covers SCRUM-33 (migrated from GMS-134): the combat contracts that connect the player, enemies, and the weapons coming in SCRUM-32. It has four parts:

- **Part A:** change report
- **Part B:** developer implementation guide
- **Part C:** AI workflow log
- **Part D:** video voice-over script

**Status:** all four steps are implemented. The editor target compiles and **24/24 `GrantMoney.*` Automation tests pass** in a headless editor run: 21 new, plus 3 existing EnvironmentPlacer tests. **The manual PIE checks in [section B9](#9-verification) have not been performed yet**, and nothing in this document marks them as passed. Screenshots are suggestions only; none have been captured yet.

**What is and is not evidence in this document:**

| Kind | Meaning |
|---|---|
| Read from source | Taken from reading project or engine source. Not a test. |
| Compiled | UnrealBuildTool finished with `Result: Succeeded`. Says nothing about behaviour. |
| Automation test | A `GrantMoney.*` Automation test passed in a headless editor run, in a synthetic empty world (no real map, no Game Mode). |
| Developer reported | The developer played it in the Editor and said what they saw. |

---

## Jira

| Key | Goal |
|---|---|
| SCRUM-33 | Establish the minimal reusable combat contracts connecting player weapons, enemies, and later bosses. |

**Done when (from Jira), and where each one is met:**

| Criterion | Met by | Evidence |
|---|---|---|
| Enemies can receive damage and die. | Step 3: `AEnemyBase` + `UHealthComponent` | Automation tests; PIE pending |
| Player can receive enemy/contact damage through a reusable interface/component approach. | Step 2: `UHealthComponent` on the engine damage pipeline + `UContactDamageComponent` | Automation tests; PIE pending |
| Death produces clean lifecycle events/hooks for XP, score, VFX, and audio. | Step 3: `UCombatEventsSubsystem::OnEnemyKilled`, `UHealthComponent::OnDeath`, virtual `AEnemyBase::HandleDeath` | Automation tests |
| Damage is not tightly coupled to the slime or magic-orb implementation. | All damage goes through `UGameplayStatics::ApplyDamage`. No combat class references the slime or the orb. | Read from source |
| Duplicate hits from the same projectile respect penetration/hit rules. | Step 4: `UAttackHitComponent` | Automation tests. Hooking it to the real projectile is SCRUM-32. |

**Extra developer requirement:** health and damage sit on a modular **stats** system. Later modifiers (health buff, damage buff, SCRUM-53 stat-boost drops) then change stats without touching combat code.

**Related tickets:**
- SCRUM-32: weapon framework and magic orb. Consumes `UAttackHitComponent` and the `Damage`/`Penetration` stats.
- SCRUM-53: enemy-dropped permanent stat boosts. Consumes `UStatsComponent` and `OnEnemyKilled`.
- SCRUM-54: unit-test enforcement. Its testing rules were followed here.

---

# Part A – Change Report

## Milestone Summary

The player and every enemy now have a **stats component**: base values plus removable add/multiply modifiers. They also have a **health component**, whose max health is a stat and which receives Unreal's standard damage. Enemies damage the player on contact, at a rate set by a stat. Enemies die cleanly and announce the kill through a world-wide event. A reusable **attack-hit component** enforces "each target once" and penetration for the upcoming projectile.

## Files Created

| File | Purpose |
|---|---|
| `Game/Source/GrantMoney/Public/GrantMoneyStatTypes.h` | `EGrantMoneyStat` (MaxHealth, ContactDamage, AttackInterval, Damage, Penetration), `EGrantMoneyStatModifierOp`, `FGrantMoneyStatModifier`, `FGrantMoneyStatModifierHandle`. |
| `Game/Source/GrantMoney/Public/StatsComponent.h`, `Private/StatsComponent.cpp` | Base stats + modifiers. `GetStat`, `HasStat`, `SetBaseStat`, `AddModifier`, `RemoveModifier`, `OnStatChanged`. |
| `Game/Source/GrantMoney/Public/HealthComponent.h`, `Private/HealthComponent.cpp` | Current health. Max health from the MaxHealth stat. Receives engine damage via `OnTakeAnyDamage`. `ApplyDamage`, `Heal`, `OnHealthChanged`, `OnDeath`. |
| `Game/Source/GrantMoney/Public/ContactDamageComponent.h`, `Private/ContactDamageComponent.cpp` | Touch damage measured edge to edge on the ground plane. Damage and interval come from the ContactDamage / AttackInterval stats. |
| `Game/Source/GrantMoney/Public/CombatEventsSubsystem.h`, `Private/CombatEventsSubsystem.cpp` | World subsystem with `OnEnemyKilled(Enemy, Killer)`. |
| `Game/Source/GrantMoney/Public/AttackHitComponent.h`, `Private/AttackHitComponent.cpp` | Hit rules for one attack: once per target, `1 + Penetration` targets, ignores self/owner/instigator/non-living. Damage and penetration come from stats. |
| `Game/Source/GrantMoney/Private/Tests/CombatTestListener.h` | Test helper `UObject` that records delegate broadcasts. Dynamic delegates can only bind to `UFUNCTION`s. |
| `Game/Source/GrantMoney/Private/StatsComponentTests.cpp` | 4 tests, `GrantMoney.Combat.StatsComponent.*` |
| `Game/Source/GrantMoney/Private/HealthComponentTests.cpp` | 5 tests, `GrantMoney.Combat.HealthComponent.*` |
| `Game/Source/GrantMoney/Private/ContactDamageComponentTests.cpp` | 4 tests, `GrantMoney.Combat.ContactDamageComponent.*` |
| `Game/Source/GrantMoney/Private/CombatEventsSubsystemTests.cpp` | 1 test, `GrantMoney.Combat.CombatEventsSubsystem.*` |
| `Game/Source/GrantMoney/Private/EnemyBaseTests.cpp` | 3 tests, `GrantMoney.Combat.EnemyBase.*` |
| `Game/Source/GrantMoney/Private/AttackHitComponentTests.cpp` | 4 tests, `GrantMoney.Combat.AttackHitComponent.*` |
| `docs/course/Module3-CreatingChallengeAndReward/Sprint1/SCRUM-33-ScriptDocumentation.md` | This document. |

## Files Modified

| File | Change |
|---|---|
| `Game/Source/GrantMoney/Public/GrantMoneyPlayerCharacter.h`, `Private/GrantMoneyPlayerCharacter.cpp` | Player owns `Stats` (MaxHealth 100) and `Health`. |
| `Game/Source/GrantMoney/Public/EnemyBase.h`, `Private/EnemyBase.cpp` | Enemies own `Stats` (MaxHealth 30, ContactDamage 10, AttackInterval 1), `Health`, and `ContactDamage`. Tick tries contact damage. Death handling (`HandleDeath`, `DeathCleanupDelay`). Dev console command `GrantMoney.DamageEnemies`. |
| `Game/Source/GrantMoney/Public/SlimeEnemy.h` | Class comment only: combat now comes from `AEnemyBase`. |

## Files Not Changed

- `ASlimeEnemy` behaviour (`SlimeEnemy.cpp`): the slime inherits all combat from `AEnemyBase` unchanged.
- `AGrantMoneyGameMode`, map/TileGen code, `MapArrays`, config files, `.uproject`, `Build.cs`. No new module dependencies were needed.
- No Blueprint or other `.uasset`/`.umap` assets.

---

# Part B – Developer Implementation Guide

## 1. Objective

At the end, any actor can be given:
- **stats** that buffs and upgrades modify;
- **health** that any damage source can reduce through Unreal's standard damage call;
- **clean death events**.

Enemies hurt the player by touch, at a rate set per enemy type. Enemies die and announce it once to the whole game. Any future attack can use one component to guarantee "each target once, up to N targets".

## 2. Why This System Exists

**Before this ticket**, the player and the slime could move and chase, but nothing had health. The classes said "health, attacks and death are added in later tickets". Three upcoming tickets need the same contracts:
- the weapon/magic orb (SCRUM-32);
- stat-boost drops (SCRUM-53);
- the enemy spawner (SCRUM-31).

If each invented its own health or damage code, they would be coupled to each other's classes.

**Chosen architecture:**

- **Components, not base-class code.** Health, stats, contact damage and hit rules are `UActorComponent`s. The player, enemies, future bosses and even breakable props can use them without sharing a base class.
- **Unreal's damage pipeline as the one damage entry point.** Damage dealers call `UGameplayStatics::ApplyDamage(Target, ...)`. The health component listens to its owner's `OnTakeAnyDamage`, so dealers never know the target's class.
- **Values come from stats.** MaxHealth, ContactDamage, AttackInterval, Damage and Penetration are stats with base values plus modifiers. A health buff or a +1 penetration upgrade is just `AddModifier`.
- **Events, not side effects.** The health component only tracks health and fires `OnHealthChanged`/`OnDeath`. XP, score, VFX and audio are listeners.

**Alternatives considered:**

| Alternative | Decision |
|---|---|
| Unreal's Gameplay Ability System (GAS) for attributes and modifiers | **Rejected for now** by the developer. More modules, boilerplate and setup than this game needs, and harder to teach. The custom component can be swapped out later because everything goes through `GetStat`. |
| An `IDamageable` C++ interface | **Not needed.** `AActor::TakeDamage`/`OnTakeAnyDamage` already is that interface, and it works with engine tools. |
| Overlap/hit events for contact damage | **Rejected.** Enemies stop at `StopDistance` before their capsule touches the player's (see [Common Mistakes](#8-common-mistakes)). |
| Player invulnerability frames instead of a per-enemy hit interval | **Deferred.** Per-enemy intervals make swarms scale in danger, which suits a survivor game. I-frames can be added to the health component later if playtesting wants them. |
| Binding XP/score to each enemy's `OnDeath` | **Rejected.** Listeners would have to track every spawned enemy. A world subsystem gives one place to subscribe. |

## 3. Prerequisites

- Unreal Engine 5.8 C++ project that builds (`GrantMoneyEditor` Development Editor Win64).
- A player character class and at least one enemy class. Here: `AGrantMoneyPlayerCharacter`, and `AEnemyBase` from SCRUM-27 with `ASlimeEnemy` from SCRUM-28.
- A Game Mode in the play level. Characters (`APawn`) refuse engine damage when there is none.
- Optional, for running tests from the command line: SCRUM-54's `scripts/run_tests.py`. Otherwise use Session Frontend → Automation in the Editor.

## 4. Architecture Overview

```
                 Modifiers (buffs, upgrades, difficulty)
                              │ AddModifier / RemoveModifier
                              ▼
                     ┌──────────────────┐
                     │  UStatsComponent │  MaxHealth, ContactDamage, AttackInterval, Damage, Penetration
                     └──────────────────┘
                 GetStat │          │ OnStatChanged(MaxHealth)
                         ▼          ▼
  Damage dealers                              Damage receiver (any actor)
  ──────────────                              ───────────────────────────
  UContactDamageComponent (enemy touch)       AActor::TakeDamage
  UAttackHitComponent (projectile, SCRUM-32)        │ OnTakeAnyDamage
  GrantMoney.DamageEnemies (dev command)            ▼
        │                                     UHealthComponent ── OnHealthChanged ──► health bar (future)
        └── UGameplayStatics::ApplyDamage ──►       │
                                                    └── OnDeath ──► AEnemyBase::HandleDeath
                                                                       │ stop tick/collision/movement
                                                                       ├──► UCombatEventsSubsystem::OnEnemyKilled
                                                                       │        └──► XP, score, drops, audio (future)
                                                                       └──► Destroy() / SetLifeSpan(DeathCleanupDelay)
```

**Responsibility boundaries:**

| Class | Owns | Does not do |
|---|---|---|
| `UStatsComponent` | Numbers and modifiers | Gameplay |
| `UHealthComponent` | Current health and death state | Destroying actors, effects, rewards |
| `UContactDamageComponent` | When a touch counts and how often | Choosing a target (the enemy does that) |
| `UAttackHitComponent` | Who an attack may hit and how many | Detecting overlaps or moving the projectile (the projectile does that) |
| `AEnemyBase` | What an enemy does when it dies | — |
| `UCombatEventsSubsystem` | Telling the rest of the game | — |

## 5. AI-Assisted Planning

### AI Prompt

> Refresh yourself on current documentation and the current status and features of the project. I want to implement ticket SCRUM-33, establishing reusable health and damage systems for combat. Some notes about this ticket and the systems: The health and damage systems should be tied to a modular "stats" system so that it can be easily modified by modifiers in the future (i.e., a health buff, damage buff, etc.). I want to implement this step by step, first starting with the player's health system.

This was the actual opening prompt. It worked because `CLAUDE.md` already told the AI to inspect only relevant files, preserve existing systems, keep gameplay in C++, and never claim untested results. In a project without such a file, add those rules to the prompt (see Prompt A below).

### Expected AI Output

- A summary of the ticket's acceptance criteria and of related tickets that will consume the system.
- A list of files to create and change for the first step only, before any code.
- Questions on decisions that are genuinely yours. Here the AI asked two: custom stats vs GAS, and branch naming.
- Code plus automated tests for one step at a time, with real compile/test output.

### Human Review

Before accepting each step, check:
- **Components and the engine call:** does damage go through `UGameplayStatics::ApplyDamage`, or did the AI add a direct call to a specific class? A direct call couples the systems.
- **Stats:** is every tunable value a stat, or a hard-coded member?
- **Edge cases:** how are missing stats handled? A stat with no base value reads as 0.
- **Spatial assumptions:** do the distance and collision assumptions match your actual enemy settings? Read `StopDistance` and the capsule radii.
- **Evidence:** are claimed test results backed by real tool output?

## 6. Step-by-Step Implementation

The work was done in four steps. Each ended with a compile and a full `GrantMoney.*` test run.

### Step 1 – Stats and player health

1. **(Git Bash)** Create the branch from an up-to-date `main`:
   ```bash
   git checkout main && git pull && git checkout -b SCRUM-33-health-damage
   ```
2. **(IDE)** Create `GrantMoneyStatTypes.h`:
   - `UENUM EGrantMoneyStat`, starting with `MaxHealth`. Add new stats only at the end, so saved values keep their meaning.
   - `EGrantMoneyStatModifierOp` (`Additive`, `Multiplicative`).
   - The modifier struct, and a handle struct (`Id`, invalid by default).

   *Why:* an enum is type-safe, shows as a dropdown in the editor, and needs no extra module.
3. **(IDE)** Create `UStatsComponent`:
   - Formula: `Final = (Base + Σ Additive) × (1 + Σ Multiplicative)`, clamped at 0. Multiplicative values are fractions that **add**: two +10% buffs give +20%, not +21%.
   - `AddModifier` returns a handle for `RemoveModifier`. Temporary buffs keep it; permanent upgrades can ignore it.
   - `OnStatChanged` fires only when the final value actually changes.
   ```cpp
   float UStatsComponent::GetStat(EGrantMoneyStat Stat) const
   {
       float Additive = 0.0f;
       float Multiplicative = 0.0f;
       for (const FGrantMoneyStatModifier& Modifier : Modifiers)
       {
           if (Modifier.Stat != Stat) { continue; }
           (Modifier.Op == EGrantMoneyStatModifierOp::Additive ? Additive : Multiplicative) += Modifier.Value;
       }
       return FMath::Max(0.0f, (GetBaseStat(Stat) + Additive) * (1.0f + Multiplicative));
   }
   ```
4. **(IDE)** Create `UHealthComponent`:
   - In `BeginPlay`, find the sibling `UStatsComponent`, bind to its `OnStatChanged`, and start at full health.
   - `GetMaxHealth()` reads the MaxHealth stat live. With no stats component it uses `FallbackMaxHealth` (100).
   - `ApplyDamage`/`Heal` ignore non-positive amounts, clamp to `0..Max`, and return the amount actually applied. `OnDeath` fires exactly once, and damage or healing after death is ignored.
   - When MaxHealth goes **up**, current health rises by the same amount, so an upgrade never leaves the player "less full". When it goes **down**, current health is only clamped.
5. **(IDE)** In `AGrantMoneyPlayerCharacter`'s constructor:
   ```cpp
   Stats = CreateDefaultSubobject<UStatsComponent>(TEXT("Stats"));
   Stats->SetBaseStat(EGrantMoneyStat::MaxHealth, DefaultMaxHealth); // 100
   Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
   ```
6. **(IDE)** Write tests (house style: `EnvironmentPlacerTests.cpp`, SCRUM-54 rules). Two things to know:
   - The stats tests need no world: `NewObject<UStatsComponent>()`.
   - The health tests need `BeginPlay`. Use the engine's `FTestWorldWrapper` (`Tests/AutomationCommon.h`): `CreateTestWorld(EWorldType::Game)`, `BeginPlayInTestWorld()`, spawn an `AActor`, then `NewObject` + `RegisterComponent()` the stats component **before** the health component. Registering a component on an actor that has begun play runs its `BeginPlay`.
   - Dynamic delegates can only bind to `UFUNCTION`s, so a tiny `UCombatTestListener` `UCLASS` records broadcasts.
7. **(Terminal)** Build and test. *Expect:* `Result: Succeeded`, then all `GrantMoney.*` tests passing (11 at this point).

### Step 2 – Player takes contact damage from enemies

1. **(IDE)** In `UHealthComponent::BeginPlay`, bind `GetOwner()->OnTakeAnyDamage` to a `UFUNCTION` that calls `ApplyDamage(Damage, DamageCauser)`. Unbind in `EndPlay`.

   *Why:* every engine damage source now reaches health, and dealers never name the health component.
2. **(Read source)** Before choosing how to detect "touching", read the enemy's spacing:
   - Slime `StopDistance` = 80 cm (centre to centre).
   - Combined capsule radii = 64 cm (slime 30 + player 34).

   The capsules never touch, so overlap and hit events cannot fire.
3. **(IDE)** Add stats `ContactDamage` and `AttackInterval` (seconds between hits, lower is faster). Add `UStatsComponent::HasStat` so a stat with no base value can fall back to a default instead of reading 0.
4. **(IDE)** Create `UContactDamageComponent`:
   - `IsInReach`: on the ground plane, `distance − ownerRadius − targetRadius ≤ Reach` (40 cm), using `GetSimpleCollisionRadius()` so it works for any size.
   - `TryApplyContactDamage(Target)`: checks reach and the attack interval against world time, then calls `UGameplayStatics::ApplyDamage(Target, GetDamage(), ...)`.
   - Damage and interval come from the stats, with `FallbackDamage`/`FallbackInterval` when a stat is missing.
5. **(IDE)** In `AEnemyBase`'s constructor:
   - Add `Stats` with ContactDamage 10 and AttackInterval 1.
   - Add a `ContactDamage` component.
   - At the end of `Tick`: `ContactDamage->TryApplyContactDamage(Target);`.
6. **(IDE)** Tests:
   - Reach, including that height is ignored.
   - The interval, advanced with `FTestWorldWrapper::TickTestWorld`, which advances world time.
   - Damage and interval taken from stats, including modifiers.
   - `UGameplayStatics::ApplyDamage` reaching the health component.

   The tests use plain `AActor`s (see [Common Mistakes](#8-common-mistakes): Pawns and the Game Mode).
7. **(Terminal)** Build and test. *Expect:* 16 tests passing.

### Step 3 – Enemies have health and die cleanly

1. **(IDE)** Create `UCombatEventsSubsystem : UWorldSubsystem` with a dynamic multicast `OnEnemyKilled(AEnemyBase* Enemy, AActor* Killer)` and `NotifyEnemyKilled`. A world subsystem exists automatically in every world. Listeners use `GetWorld()->GetSubsystem<UCombatEventsSubsystem>()`.
2. **(IDE)** In `AEnemyBase`:
   - Add a `Health` component, with base MaxHealth 30 (a placeholder).
   - In `BeginPlay`, bind `Health->OnDeath` to a `UFUNCTION` that calls a **virtual** `HandleDeath(Killer)`.
3. **(IDE)** `HandleDeath`:
   ```cpp
   SetActorTickEnabled(false);              // no more chasing or contact hits
   SetActorEnableCollision(false);          // stop blocking the player and other enemies
   GetCharacterMovement()->DisableMovement();
   CombatEvents->NotifyEnemyKilled(this, Killer);   // before destroying: listeners can still read the enemy
   if (DeathCleanupDelay > 0.0f) { SetLifeSpan(DeathCleanupDelay); } else { Destroy(); }
   ```
4. **(IDE)** Add a dev-only console command, `GrantMoney.DamageEnemies [Amount]`, wrapped in `#if !UE_BUILD_SHIPPING`. It calls `ApplyDamage` on every `AEnemyBase`. Collect the enemies into an array first, because killing them during an actor iterator would destroy them mid-iteration.
5. **(IDE)** Tests:
   - Enemy defaults.
   - Lethal damage reports one kill, with the right enemy and killer, **while the enemy is still alive**, and then the enemy is being destroyed.
   - With a cleanup delay: stopped, no collision, life span set, and no second kill.
6. **(Terminal)** Build and test. *Expect:* 20 tests passing (this step needed two fixes first; see Common Mistakes).

### Step 4 – Hit and penetration rules for attacks

1. **(IDE)** Add stats `Damage` and `Penetration` (extra targets after the first; rounded down).
2. **(IDE)** Create `UAttackHitComponent` with `TryHit(Target)` returning `EGrantMoneyHitResult`. It checks in this order:
   - **NotATarget:** null, being destroyed, the attack itself, its owner (weapon), its instigator (player), or no **living** `UHealthComponent`. None of these use up penetration.
   - **AlreadyHit:** the target is in the `TSet<TWeakObjectPtr<const AActor>>`.
   - **Exhausted:** `1 + Penetration` hits are already used.
   - Otherwise it **records the target first**, then calls `ApplyDamage`, broadcasts `OnTargetHit`, and fires `OnHitsExhausted` once when the last hit is used.

   `ResetHits()` is for pooled or returning attacks.
3. **(IDE)** Tests:
   - Once per target.
   - Penetration 0 and 1, plus reset.
   - Every not-a-target case.
   - Damage and penetration from stats, including modifiers and rounding.
4. **(Terminal)** Build and test. *Expect:* 24 tests passing.
5. **Hook-up contract for SCRUM-32** (also in the `AttackHitComponent.h` class comment):
   ```cpp
   // Projectile constructor
   AttackHit = CreateDefaultSubobject<UAttackHitComponent>(TEXT("AttackHit"));
   // Projectile BeginPlay
   Collision->OnComponentBeginOverlap.AddDynamic(this, &AMagicOrbProjectile::HandleOverlap);   // calls AttackHit->TryHit(OtherActor)
   AttackHit->OnHitsExhausted.AddDynamic(this, &AMagicOrbProjectile::HandleHitsExhausted);     // calls Destroy()
   // Weapon, when firing: Owner = weapon, Instigator = player pawn; copy damage/penetration from weapon stats
   FActorSpawnParameters Params;
   Params.Owner = this;
   Params.Instigator = PlayerPawn;
   ```
   This snippet is **not compiled**. `AMagicOrbProjectile` belongs to SCRUM-32. Two points to agree with the SCRUM-32 owner:
   - Does the weapon copy its stats into the projectile's `FallbackDamage`/`FallbackPenetration`, or does the projectile get its own `UStatsComponent`?
   - The projectile collision must generate overlaps with Pawns.

## 7. AI Prompts by Stage

Reusable versions of the prompts, for a different game.

### Prompt A – Inspect and plan

> Read the ticket below and the headers of my player and enemy classes. Do not read generated folders or binary assets. List which existing classes the combat system must connect to, which other tickets will consume it, and the smallest first step. Propose files to create and change for that step only, and ask me about any decision that is mine (e.g. Gameplay Ability System vs a custom component). Do not write code yet.

### Prompt B – Stats and health

> Implement a `UActorComponent` for stats: an enum of stats, base values, and additive and multiplicative modifiers that can be removed by handle. Final = (Base + ΣAdd) × (1 + ΣMult), never below 0. Add a change event that fires only on real changes. Then a health component that reads max health from that stat and fires health-changed and death events, with death firing exactly once. Add Automation tests (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`, `FTestWorldWrapper` for anything that needs BeginPlay). Compile and run the tests; report the real output.

### Prompt C – Damage delivery

> Make the health component receive damage from `UGameplayStatics::ApplyDamage` via the owner's `OnTakeAnyDamage`, so damage dealers never reference it. Then add enemy contact damage. First read the enemy's stop distance and capsule sizes and tell me whether the capsules ever touch, before choosing overlap events or a distance check. Damage and attack interval must be stats with fallbacks when a stat is missing.

### Prompt D – Death and lifecycle hooks

> Give enemies health. On death, stop the enemy (tick, collision, movement), broadcast a kill event that XP/score/VFX/audio can subscribe to once per world, then destroy it, with an optional delay. Report the kill before destroying. Add a dev-only console command to damage all enemies, so I can test in PIE before weapons exist.

### Prompt E – Hit rules for a system that does not exist yet

> Projectiles are built in another ticket. Build a component that a projectile adds and calls from its overlap event: each target is damaged once, up to 1 + Penetration targets, never the attack's owner or instigator, never actors without living health. Put a hook-up contract in the header comment so the other developer can wire it without reading the implementation.

### Prompt F – Debug a failing test

> This Automation test fails with `<exact message>`. Before changing code, decide whether the code or the test is wrong, and explain why. Change only the side that is wrong.

## 8. Common Mistakes

These all happened, or were caught, during this ticket.

### 8.1 Contact detection with overlap events never fires
- **Symptom:** an overlap- or hit-based contact damage would never trigger, because enemies stop just short of the player.
- **Likely cause:** the enemy's `StopDistance` (slime: 80 cm centre to centre) is larger than the combined capsule radii (64 cm).
- **Fix:** measure the gap between the two actors' edges on the ground plane: `distance − radiusA − radiusB ≤ Reach`.
- **Lesson:** read the numbers of the systems you are connecting before choosing a mechanism. *Found while reading source during Step 2; never shipped.*

### 8.2 Characters ignore damage in an automated test world
- **Symptom:** `UGameplayStatics::ApplyDamage` on a Character or Pawn does nothing in a test world.
- **Likely cause:** `APawn::ShouldTakeDamage` returns false when the world has no Game Mode, and `FTestWorldWrapper` worlds have none.
- **Fix:** test the engine damage path on plain `AActor`s, and call `UHealthComponent::ApplyDamage` directly for enemy tests. Check the Character path in PIE (`GrantMoney.DamageEnemies`).
- **Lesson:** know what your test harness does not simulate. *Found while reading engine source; it shaped the tests.*

### 8.3 A missing stat reads as 0
- **Symptom:** an enemy with a stats component but no AttackInterval value would hit every frame.
- **Likely cause:** `GetStat` returns 0 for a stat with no base value.
- **Fix:** `UStatsComponent::HasStat`. Consumers use their own fallback when `HasStat` is false.
- **Lesson:** decide what "no data" means explicitly. *Caught while designing the AttackInterval change.*

### 8.4 Float rounding at exactly zero
- **Symptom:** a test that reduced max health to 0 with −0.2 and −0.8 multiplicative modifiers could leave a tiny positive value, so the actor would not die.
- **Fix:** use −1.0, so the total is below −100% and clamps to exactly 0.
- **Lesson:** don't build tests on float sums landing exactly on a boundary. *Caught before the first run.*

### 8.5 Build error: forward declaration and `TWeakObjectPtr`
- **Symptom:** `cannot convert argument 2 from 'AEnemyBase *' to 'const UObject *'… Types pointed to are unrelated` in `CombatTestListener.h`.
- **Likely cause:** the header only forward-declared `class AEnemyBase;`. Assigning to `TWeakObjectPtr<AEnemyBase>` needs the full class, so the compiler knows it derives from `UObject`.
- **Fix:** `#include "EnemyBase.h"`.
- **Lesson:** a forward declaration covers raw pointers, but not conversions that need the class hierarchy. *Real build failure in Step 3.*

### 8.6 Test failure: weak pointer to a destroyed actor
- **Symptom:** `GrantMoney.Combat.EnemyBase.Death` failed with "Expected 'The kill reports this enemy' to be true" (19/20).
- **Likely cause:** the code was right; the test was wrong. The listener stored the enemy in a `TWeakObjectPtr`, and the enemy is destroyed right after the event, so `Get()` returns null.
- **Fix:** compare with `Get(true)` (*even if garbage*). Also record `IsValid(Enemy) && !Enemy->IsActorBeingDestroyed()` at broadcast time, which turns "reported before destroyed" into a real assertion.
- **Lesson:** when a test fails, decide which side is wrong before changing anything. *Real test failure in Step 3.*

### 8.7 `SetLifeSpan(0)` does not destroy
- **Symptom:** setting the cleanup delay to 0 and calling `SetLifeSpan(0)` would leave dead enemies in the level forever.
- **Likely cause:** in Unreal, a life span of 0 means "no life span".
- **Fix:** `if (Delay > 0) SetLifeSpan(Delay); else Destroy();`.
- **Lesson:** check what the engine treats as a sentinel value. *Caught while writing Step 3.*

### 8.8 Comparing `AActor*` with `APawn*`
- **Symptom:** comparing `Target == Owner->GetInstigator()` fails to compile without the full `APawn` definition.
- **Fix:** `#include "GameFramework/Pawn.h"`. *Caught before compiling Step 4.*

## 9. Verification

### Performed

| Test | Action | Result | Kind |
|---|---|---|---|
| Build | `GrantMoneyEditor` via SCRUM-54's `run_tests.py`. It was run from a scratch copy, because SCRUM-54 is not merged yet. | Succeeded (after the 8.5 fix) | Compiled |
| Step 1 | `GrantMoney.*` tests | 11/11 passed | Automation test |
| Step 2 | `GrantMoney.*` tests | 15/15, then 16/16 after the AttackInterval change | Automation test |
| Step 3 | `GrantMoney.*` tests | 19/20 (8.6), then **20/20** after the test fix | Automation test |
| Step 4 | `GrantMoney.*` tests | **24/24 passed** | Automation test |

**The 21 new tests:**

| Test group | Tests |
|---|---|
| `StatsComponent` | BaseValue, ModifierMath, RemoveModifier, ChangeEvent |
| `HealthComponent` | StartsFull, DamageAndHeal, Death, MaxHealthModifier, EngineDamage |
| `ContactDamageComponent` | Reach, Interval, UsesStat, IntervalFromStat |
| `CombatEventsSubsystem` | EnemyKilled |
| `EnemyBase` | CombatDefaults, Death, DelayedCleanup |
| `AttackHitComponent` | OncePerTarget, Penetration, NotATarget, UsesStats |

**To rerun:**
- In the Editor: Session Frontend → Automation, filter `GrantMoney.Combat`, run.
- From the command line, once SCRUM-54 is merged: `python scripts/run_tests.py --filter GrantMoney.Combat`.

### Pending (developer, in the Editor)

| Test | Action | Expected Result |
|---|---|---|
| Player components | Select the player Blueprint/character, Details panel | `Stats` (Base Stats: Max Health 100) and `Health` components |
| Player starts full | PIE, select the player, Health component | Current Health 100 |
| Contact damage | PIE, let one slime reach the player, stand still | Current Health drops by 10 about once per second per touching slime |
| Player death | Keep one slime touching for ~10 s | Output Log: `HealthComponent: <player> died (damage causer: <slime>)`. Nothing else happens yet. |
| Enemy damage | PIE console: `GrantMoney.DamageEnemies 10` | Log `applied 10.0 damage to N enemies`; a selected slime shows Current Health 20 |
| Enemy death | `GrantMoney.DamageEnemies 20` | Every slime disappears; one `died` log line each |
| Delayed cleanup | Set Death Cleanup Delay = 2 on a placed slime, repeat | That slime stops and stops blocking, then disappears 2 s later |
| Hit rules | — | No in-game check until SCRUM-32's projectile exists |
| Packaged build | — | Not required for this ticket; not performed |

Results: _you fill in_ (only with what was actually observed).

## 10. Known Limitations

- **Player death has no consequence yet.** There is no game-over screen, input still works, and enemies keep pushing (their damage is ignored).
- No health bar or HUD, and no hit feedback (flash, sound, knockback).
- No death VFX, sound or animation. Enemies vanish; the hooks exist (`HandleDeath`, `OnEnemyKilled`).
- Nothing listens to `OnEnemyKilled` yet (XP and score are later tickets).
- `Killer` is the damage causer (e.g. the projectile), not the player who fired it.
- `UAttackHitComponent` is not used by any projectile yet (SCRUM-32).
- Placeholder balance: player MaxHealth 100; enemy MaxHealth 30, ContactDamage 10, AttackInterval 1 s.
- Damage type is always the default `UDamageType` (no elemental categories).
- Not replicated (the game is single-player).
- `UCombatTestListener` is a `UCLASS`, so it is compiled into every build of the game module, though it is only used by tests.

## 11. Extension Ideas

- **Any genre with hit points.** The same stats + health + engine-damage split works for a tower-defence tower, a destructible crate, or a boss with phases (bind `OnHealthChanged` and check `GetHealthFraction()`).
- **Roguelike upgrades.** Every upgrade is an `AddModifier` on a stat. Keep the handle for timed buffs, ignore it for permanent ones.
- **Melee and area attacks.** Give a sword swing or an explosion pulse a `UAttackHitComponent` and call `ResetHits()` at the start of each swing or pulse.
- **Difficulty scaling.** A spawner adds a +X% MaxHealth / ContactDamage modifier to each enemy it spawns as time passes.
- **Damage types.** Pass a custom `UDamageType` subclass through `ApplyDamage` for resistances, read in the health component.

## 12. Source-Control Checkpoint

| Item | Value |
|---|---|
| Jira | SCRUM-33 (migrated from GMS-134) |
| Branch | `SCRUM-33-health-damage` |
| PR title | `SCRUM-33: Stat-driven health, damage, death and hit rules` |

**Staging (only milestone files):**
```bash
git add Game/Source/GrantMoney/Public/GrantMoneyStatTypes.h \
        Game/Source/GrantMoney/Public/StatsComponent.h Game/Source/GrantMoney/Private/StatsComponent.cpp \
        Game/Source/GrantMoney/Public/HealthComponent.h Game/Source/GrantMoney/Private/HealthComponent.cpp \
        Game/Source/GrantMoney/Public/ContactDamageComponent.h Game/Source/GrantMoney/Private/ContactDamageComponent.cpp \
        Game/Source/GrantMoney/Public/CombatEventsSubsystem.h Game/Source/GrantMoney/Private/CombatEventsSubsystem.cpp \
        Game/Source/GrantMoney/Public/AttackHitComponent.h Game/Source/GrantMoney/Private/AttackHitComponent.cpp \
        Game/Source/GrantMoney/Private/Tests/CombatTestListener.h \
        Game/Source/GrantMoney/Private/*ComponentTests.cpp \
        Game/Source/GrantMoney/Private/CombatEventsSubsystemTests.cpp Game/Source/GrantMoney/Private/EnemyBaseTests.cpp \
        Game/Source/GrantMoney/Public/GrantMoneyPlayerCharacter.h Game/Source/GrantMoney/Private/GrantMoneyPlayerCharacter.cpp \
        Game/Source/GrantMoney/Public/EnemyBase.h Game/Source/GrantMoney/Private/EnemyBase.cpp \
        Game/Source/GrantMoney/Public/SlimeEnemy.h \
        docs/course/Module3-CreatingChallengeAndReward/Sprint1/SCRUM-33-ScriptDocumentation.md
```

**Checklist:**
- [x] Project compiles (`GrantMoneyEditor`)
- [x] Automation tests pass (24/24)
- [ ] Feature checked in Unreal (PIE checks above)
- [ ] Teammate review
- [x] No generated Unreal folders, binaries or credentials staged
- [ ] Screenshots captured

---

# Part C – AI Workflow Log

**Tool:** Claude Code (Claude Opus 5.5) in the repository, with Jira access. It ran builds and tests itself through SCRUM-54's `run_tests.py`.

| # | Purpose | Prompt (developer) | Result | Decision | Verification |
|---|---|---|---|---|---|
| 1 | Context and plan | Opening prompt (section 5) | Read SCRUM-33 and related tickets (32, 53, 54), the player/enemy headers and the SCRUM-54 testing rules. Proposed Step 1 files. Asked: GAS vs custom, and branch name. | **Modified.** The developer chose the custom component and `SCRUM-33-health-damage`. | — |
| 2 | Step 1: stats + player health | (continued from 1) | Stats, health and player wiring, plus 8 tests. Self-corrected the float-boundary test (8.4) before running. | **Accepted** | Compiled; 11/11 tests |
| 3 | Step 2: contact damage | "Don't commit, move to implement Step 2" | Found that capsules never touch (8.1). Chose engine damage + a distance-based contact component, with a per-enemy interval instead of player i-frames. Noted the Pawn/Game Mode limitation in tests (8.2). | **Accepted** | Compiled; 15/15 tests |
| 4 | Attack rate as a stat | "Each enemy should have a hit rate stat for how fast they attack the player… Set it to a default of 1 second." | The first Step 2 version had a plain `Interval` property, which was not tied to stats. Moved it into an `AttackInterval` stat. The AI named it an interval because "rate" would mean hits per second, and added `HasStat` (8.3). | **Modified** (developer correction of the AI's design) | Compiled; 16/16 tests |
| 5 | Step 3: enemy death | "Implement Step 3" | World subsystem for kill events, virtual `HandleDeath`, cleanup delay, dev console command. Build failed (8.5); one test failed (8.6). Both were fixed, and the fix strengthened the test. | **Accepted after fixes** | Failed build → fixed; 19/20 → 20/20 |
| 6 | Step 4: hit rules | "Sure, implement that so that it's easy to hook once SCRUM-32's projectile actually gets made" | `UAttackHitComponent` with a hook-up contract in the header. Caught the `APawn` include (8.8) before compiling. | **Accepted** | Compiled; 24/24 tests |
| 7 | Documentation | "Finish up the ticket documentation file and then commit this in a new PR to main." | This document, restructured from the running log following `docs/ai/DocumentationScript.md`. | — | Checked against the repository diff |

**Where AI helped most:**
- Reading engine source to find harness limits (`APawn::ShouldTakeDamage`, `FTestWorldWrapper`, `SetLifeSpan(0)`).
- Writing the tests alongside the code.

**Where it needed a human:**
- The architectural choice (GAS vs custom).
- Noticing that the attack rate belonged in the stats system (row 4).

---

# Part D – Video Voice-Over Script

## Opening

**SAY:** "So far our player can run around the map and slimes chase them, but nobody can get hurt. Today we give the game its combat contracts: health, damage, death, and the rules for what a hit means. The important idea isn't the slime or our survivor game. Every value lives in a stats system, and every damage source goes through one engine call. That way weapons, enemies and upgrades never need to know about each other."

## Acceptance Criteria

**SAY:** "At the end of this part:
- the player has health that comes from a stat;
- slimes hurt the player by touching them, at a rate we can tune per enemy;
- enemies die and announce it once to the whole game;
- a projectile can be guaranteed to hit each enemy once, up to its penetration limit;
- and all of it is covered by automated tests."

## Engineering Principle

**SAY:** "Our principle today: **numbers live in stats, damage goes through one door.** If a health buff, a damage upgrade and a difficulty curve are all just modifiers on a stat, we never write special cases for them. And if every attack calls the same engine damage function, the thing being hit doesn't care what hit it."

## Prerequisites

**SAY:** "You need:
- a C++ Unreal project that builds;
- a player character and an enemy that walks toward the player;
- a Game Mode in your level.

In GrantMoney that's the player character, `AEnemyBase`, and the slime from the previous two tickets."

## Step-by-Step Demonstration

### 1. Plan with AI before writing code

- **SAY:** "First I give the AI the ticket, and tell it to read only my player and enemy headers and propose the first step. It doesn't write code yet."
- **SHOW:** The Jira ticket, then the AI's file plan and its two questions.
- **DO:** Answer "custom component" and choose the branch name.
- **AI PROMPT:** Prompt A (section B7).
- **VERIFY:** The plan only covers step one, and names real files.

> **Here is an engineering discussion.** Unreal has a Gameplay Ability System that already does attributes and modifiers. It's tempting to reach for it. But it brings modules, boilerplate and setup that a small survivor game doesn't need yet. Because every system reads stats through one `GetStat` function, we could swap the implementation later without touching the health or damage code.

### 2. Stats and player health

- **SAY:** "A stat is a base value plus modifiers. Add the flat bonuses, then multiply by one plus the percentage bonuses. Health doesn't store its own maximum. It asks the stats component, so a max-health buff just works."
- **SHOW:** `UStatsComponent::GetStat`, `UHealthComponent::ApplyDamage`, and the player constructor.
- **DO:** Build. Run Session Frontend → Automation → `GrantMoney.Combat`.
- **AI PROMPT:** Prompt B.
- **VERIFY:** The build succeeds and the stats and health tests are green. *(S1-2)*

### 3. Damage through the engine, and contact damage

- **SAY:** "The health component listens to the engine's `OnTakeAnyDamage`. From now on, anything that calls `ApplyDamage` can hurt anything with health."
- **SAY:** "Before choosing how slimes touch the player, I asked the AI to check the numbers. The slime stops 80 centimetres away, but the two capsules together are only 64 wide. They never touch, so overlap events would never fire. We measure the gap between their edges instead."
- **SHOW:** `StopDistance` in `SlimeEnemy.cpp`, the capsule radii, then `UContactDamageComponent::IsInReach`.
- **DO:** Add the `ContactDamage` and `AttackInterval` stats on `AEnemyBase`. Build and test.
- **AI PROMPT:** Prompt C.
- **VERIFY:** The tests are green. In PIE, let a slime reach the player and watch Current Health drop by 10 once per second. *(S2-2)*

> **Here is an engineering discussion.** We gave each enemy its own attack interval instead of making the player briefly invulnerable after a hit. With invulnerability, ten slimes hurt as much as one. With per-enemy intervals, a swarm is actually dangerous, which is what a survivor game needs.

### 4. Enemy death and the kill event

- **SAY:** "When an enemy dies it stops moving, stops colliding, and tells the whole game through one world subsystem. Then it's removed. XP, score and sound effects subscribe once, not to every enemy."
- **SHOW:** `AEnemyBase::HandleDeath` and `UCombatEventsSubsystem`.
- **DO:** Build. In PIE, open the console and run `GrantMoney.DamageEnemies 10`, then `GrantMoney.DamageEnemies 20`.
- **AI PROMPT:** Prompt D.
- **VERIFY:** The slimes disappear, with one `died` line each in the Output Log. *(S3-2)*

### 5. Hit rules for a projectile that doesn't exist yet

- **SAY:** "The magic orb is another teammate's ticket. Rather than wait, we build the hit rules as a component the orb will add: each enemy once, one plus penetration enemies in total, never the player who fired it. The header comment tells the other developer the four lines they need."
- **SHOW:** The `AttackHitComponent.h` class comment and `TryHit`.
- **DO:** Build and test.
- **AI PROMPT:** Prompt E.
- **VERIFY:** The `AttackHitComponent` tests are green. *(S4-2)*

## Common Mistakes

**SAY:** "Here are some common mistakes, and all of these actually came up.

- **Characters ignore damage in a test world without a Game Mode.** That's why our engine-damage tests use plain actors.
- **A stat nobody set reads as zero.** An attack interval of zero means hitting every frame, so missing stats fall back to a default.
- **In Unreal, a life span of zero means 'live forever', not 'die now'.**
- **A forward declaration isn't enough to assign to a weak pointer.** Our build failed until we included the enemy header.
- **One test failed because the test was wrong, not the game.** It looked at the enemy through a weak pointer after the enemy was destroyed. We fixed the test, and made it check that the kill is announced before the enemy disappears."

## Verification Checkpoint

**SAY:** "Now let's verify our result."

**SHOW:**
- The build result.
- Automation: 24 of 24 `GrantMoney` tests green.
- In PIE:
  - the player's health dropping while a slime touches them;
  - the death line in the Output Log;
  - the console command killing every slime.

*(Record the PIE parts only once they've actually been performed.)*

## AI Review

**SAY:** "Let's review what AI actually did here.
- It read the ticket, the related tickets and our headers, and proposed one step at a time.
- It wrote the code and the tests, and ran both.
- It found engine behaviour we'd have tripped over: Pawns, the Game Mode, and life span zero.
- It caused one build failure and one wrong test, and both are in our mistakes list.

The human decisions:
- the custom stats system over GAS;
- and the attack rate belonging in stats, which the AI's first version missed."

## Engineering Pause

**SAY:** "Here is an engineering pause. Consider this question: if our projectile had to know it was hitting a slime in order to damage it, would we have built a combat system, or just a slime-shooting feature?

The point of `ApplyDamage` and the health component is that the projectile never knows. A boss, a crate or a second player could be hit tomorrow without changing the orb."

## Completion Criteria

**SAY:** "This milestone is complete when:
- the player and enemies have stat-driven health;
- slimes damage the player at a tunable rate;
- enemies die once, announce it, and are removed;
- the hit-rule component passes its tests and is ready for the weapon ticket;
- and the automated tests and the in-editor checks have been run."

## Next Milestone

**SAY:** "Next, the weapon ticket gives the player an automatic magic orb. It plugs straight into the attack-hit component. Its damage and penetration are stats, so upgrades and the stat-boost drops can make it stronger without touching any of today's code."

---

## Suggested Screenshots / Clips

None have been captured yet. Capture them while doing the PIE checks, and store them outside Git or under `images/` next to this file, per the delivery guide.

| # | Content |
|---|---|
| S1-1 | Player Details panel: `Stats` (Base Stats → Max Health 100) and `Health` components |
| S1-2 | Automation window: `GrantMoney.Combat` all green |
| S2-1 | Slime Details panel: Base Stats with Contact Damage 10 and Attack Interval 1 |
| S2-2 | Clip: slime touching the player, Current Health dropping |
| S2-3 | Output Log: player death line |
| S3-1 | Slime Details panel: `Health`, Max Health 30, Death Cleanup Delay |
| S3-2 | Clip: `GrantMoney.DamageEnemies` twice, slimes disappearing, death log lines |
| S4-1 | `AttackHitComponent.h` class comment (the hook-up contract) |
| S4-2 | Automation window: `GrantMoney.Combat.AttackHitComponent` all green |
