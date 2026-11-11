# SCRUM-35 – Reusable Pickups and Guaranteed XP Orb Drops
## Module 3 Sprint 1 – Development and Recording Guide

This guide covers SCRUM-35: a reusable pickup system, with a guaranteed XP orb as its first pickup type. When a slime dies it drops a small glowing green orb. When the player gets close, the orb flies to them and adds XP on arrival. It has four parts, plus a recipe and a media list:

- **Part A:** change report
- **Part B:** developer implementation guide, including [section B6A, the recipe for adding a rare power-up](#6a-recipe-add-a-new-pickup-and-make-it-a-rare-drop)
- **Part C:** AI workflow log
- **Part D:** video voice-over script
- **Media Capture List:** every screenshot and clip to record, with file names

**Status:** all four steps are implemented. The editor target compiles and **62/62 `GrantMoney.*` Automation tests pass** in a headless editor run: 22 new, plus 40 existing. **The manual PIE checks in [section B9](#9-verification) have not been performed yet**, and nothing in this document marks them as passed. No media has been captured yet.

**What is and is not evidence in this document:**

| Kind | Meaning |
|---|---|
| Read from source | Taken from reading project or engine source. Not a test. |
| Compiled | UnrealBuildTool finished with `Result: Succeeded`. Says nothing about behaviour. |
| Automation test | A `GrantMoney.*` Automation test passed in a headless editor run, in a synthetic empty world (no real map, no Game Mode, no rendering). |
| Developer reported | The developer played it in the Editor and said what they saw. |

---

## Jira

| Key | Goal |
|---|---|
| SCRUM-35 | Add a reusable pickup system. Make guaranteed XP orbs the first pickup type, dropped by slimes. XP is a running total for this milestone. |

**Done when (from the ticket), and where each one is met:**

| Criterion | Met by | Evidence |
|---|---|---|
| The player has a reusable pickup component with an editable radius that detects any supported pickup type. | `UPickupComponent` (`PickupRadius`), scanning for `APickupBase` | Automation tests; PIE pending |
| Detection, attraction and collection are in shared code. Each pickup type has a collection hook. | `APickupBase::BeginAttraction`, `Tick`, `TryCollect`, virtual `OnCollected` | Automation tests |
| A future power-up can apply a temporary buff through the same hook without changing the player's pickup component. | `ATestBuffPickup` (test class) applies a stat modifier through `OnCollected`. See [B6A](#6a-recipe-add-a-new-pickup-and-make-it-a-rare-drop). | Automation test |
| Collection happens once per pickup, even if overlap callbacks repeat. The pickup is removed after its effect. | `bCollected` flag in `TryCollect`, then `Destroy()` | Automation test |
| One XP orb spawns at the enemy's death location through the existing death flow, before the enemy despawns. Slimes drop it at 100%. | `AEnemyBase::SpawnPickupDrops`, called from `HandleDeath`. `bDropsXpOrb = true` in `ASlimeEnemy`. | Automation tests; PIE pending |
| Other pickup classes drop with their own independent chances, without lowering the XP drop. | `AEnemyBase::BonusPickupDrops` (`FEnemyPickupDrop`) | Automation tests |
| Simultaneous lethal hits cannot create duplicate drops. | `UHealthComponent` fires `OnDeath` once, and drops happen only inside that handler. | Automation test |
| The orb waits outside the radius, locks on inside it, and follows turns. It keeps chasing after the player leaves the radius. | `APickupBase::Tick` aims at the collector's current position every frame | Automation tests; PIE pending |
| Speed rises from an editable base to an editable maximum above the player's top speed. It is frame-rate independent and collects on arrival even if it would overshoot. | `BaseSpeed`, `Acceleration`, `MaxSpeed`, `CollectionDistance` | Automation tests |
| XP is stored on the player with the existing stats approach. Each orb has an editable XP value. | `EGrantMoneyStat::Experience` on `UStatsComponent`; `AXpOrbPickup::XpValue` | Automation tests; PIE pending |
| Leveling, upgrade choice, UI and real buffs stay out of this ticket. | Not implemented | Read from source |

**Related tickets:**
- SCRUM-33: stats, health and the death flow (`HandleDeath`, `OnEnemyKilled`). Drops hook into that flow.
- SCRUM-32: the weapon that kills slimes. Needed to see drops in normal play.
- SCRUM-53: enemy-dropped permanent stat boosts. They can be a new pickup type (see B6A).
- A leveling ticket: it will read the `Experience` stat.

---

# Part A – Change Report

## Milestone Summary

The player now has a **pickup component** with an editable radius. Pickups share one base class that handles locking onto the player, flying in with acceleration, and collecting exactly once. The **XP orb** is the first pickup type: it adds its XP value to a new **Experience stat** on arrival. Every enemy can be set to drop one guaranteed XP orb when it dies (slimes do) plus any number of **bonus pickups**, each with its own chance.

## Files Created

| File | Purpose |
|---|---|
| `Game/Source/GrantMoney/Public/PickupBase.h`, `Private/PickupBase.cpp` | `APickupBase` (abstract). Query-only sphere collision, visual mesh, `BeginAttraction`, `Tick` pursuit, `TryCollect`, virtual `OnCollected`. Tuning: `BaseSpeed`, `Acceleration`, `MaxSpeed`, `CollectionDistance`. |
| `Game/Source/GrantMoney/Public/PickupComponent.h`, `Private/PickupComponent.cpp` | `UPickupComponent`. Every frame, scans `PickupRadius` around its owner and calls `BeginAttraction` on each `APickupBase` found. |
| `Game/Source/GrantMoney/Public/XpOrbPickup.h`, `Private/XpOrbPickup.cpp` | `AXpOrbPickup`. Placeholder green sphere. `XpValue`. `OnCollected` adds to the collector's `Experience` base stat. |
| `Game/Source/GrantMoney/Public/EnemyPickupDrop.h` | `FEnemyPickupDrop`: a pickup class and a `DropChance` (0–1), with `ShouldDrop(Roll)`. Header only. |
| `Game/Source/GrantMoney/Private/Tests/PickupTestTypes.h` | Test helper `ATestBuffPickup`: a stand-in power-up that adds a stat modifier through `OnCollected` and counts collections. |
| `Game/Source/GrantMoney/Private/Tests/PickupTestScene.h` | Test helper: an empty game world with a frozen stand-in player (Stats + `UPickupComponent`) and spawn and step helpers. |
| `Game/Source/GrantMoney/Private/PickupBaseTests.cpp` | 7 tests, `GrantMoney.Pickups.PickupBase.*` |
| `Game/Source/GrantMoney/Private/PickupComponentTests.cpp` | 5 tests, `GrantMoney.Pickups.PickupComponent.*` |
| `Game/Source/GrantMoney/Private/XpOrbPickupTests.cpp` | 4 tests, `GrantMoney.Pickups.XpOrb.*` |
| `Game/Source/GrantMoney/Private/EnemyDropTests.cpp` | 6 tests, `GrantMoney.Pickups.EnemyDrops.*` |
| `docs/course/Module3-CreatingChallengeAndReward/Sprint1/SCRUM-35-ScriptDocumentation.md` | This document. |

## Files Modified

| File | Change |
|---|---|
| `Game/Source/GrantMoney/Public/GrantMoneyStatTypes.h` | New stat `Experience`, added at the **end** of `EGrantMoneyStat` so saved values keep their meaning. |
| `Game/Source/GrantMoney/Public/GrantMoneyPlayerCharacter.h`, `Private/GrantMoneyPlayerCharacter.cpp` | Player owns a `Pickups` component. `Experience` starts at 0 on `Stats`. New `GetPickups()` and `GetExperience()`. |
| `Game/Source/GrantMoney/Public/EnemyBase.h`, `Private/EnemyBase.cpp` | New `bDropsXpOrb`, `XpOrbClass`, `BonusPickupDrops`, and virtual `SpawnPickupDrops()`. `HandleDeath` calls it after capturing the death transform and before the kill event and destruction. |
| `Game/Source/GrantMoney/Private/SlimeEnemy.cpp` | `bDropsXpOrb = true`. |

## Files Not Changed

- `UStatsComponent`, `UHealthComponent`, `UCombatEventsSubsystem`, `UAttackHitComponent`, the weapon and the magic orb. Drops reuse the existing one-time death handling.
- `AGrantMoneyGameMode`, map and TileGen code, `MapArrays`, config files, `.uproject`, `Build.cs`. No new module dependencies and no new collision channel were needed.
- No Blueprint or other `.uasset`/`.umap` assets were created or edited. The only asset used is an **engine** material (`/Engine/EngineMaterials/EmissiveMeshMaterial`).

---

# Part B – Developer Implementation Guide

## 1. Objective

At the end:
- the player has a pickup radius, and anything pickup-like inside it flies to the player and applies its effect;
- the slime drops exactly one green XP orb where it dies;
- XP is a running total on the player's stats;
- adding a new pickup (a power-up, health, a stat boost) means writing **one function**, and making an enemy drop it means adding **one list entry**.

## 2. Why This System Exists

**Before this ticket**, enemies died (SCRUM-33) and announced it, but nothing was left behind and nothing could be collected. Several planned features are all "an object the player walks up to": XP, power-ups, SCRUM-53's stat boosts. If each were built separately, each would need its own detection, its own movement and its own once-only guard.

**Chosen architecture:**

- **One base class owns the life cycle.** `APickupBase` does locking on, pursuit and collecting. A pickup type adds only *what happens on arrival*.
- **The collector's component only finds pickups.** `UPickupComponent` never knows pickup types. It scans for `APickupBase` and asks each to start chasing.
- **XP is a stat.** The project already keeps player-owned numbers in `UStatsComponent` (SCRUM-33), so XP is the `Experience` stat, not a new field on the player.
- **Drops are data on the enemy.** One guaranteed XP orb is a flag plus a class. Everything else is a list of `FEnemyPickupDrop` entries, so a designer can add a rare drop in the Details panel without code.
- **Drops ride the existing death flow.** `HandleDeath` already runs once per enemy (health fires `OnDeath` once), so drops cannot duplicate.

**Alternatives considered:**

| Alternative | Decision |
|---|---|
| Overlap events on a sphere (`OnComponentBeginOverlap`) for detection | **Rejected.** Every pickup would need its own overlap handling and responses to every other object. A per-frame object-type sphere query needs no collision responses at all, and the radius can change live. Repeated detection is harmless because `BeginAttraction` and `TryCollect` are guarded. |
| A new collision channel "Pickup" in `DefaultEngine.ini` | **Deferred.** It would be cleaner, but needs a config change that can't be checked without the editor. The query uses `ECC_WorldDynamic` and filters by class. If many world-dynamic actors appear, switch to a dedicated channel. |
| An `IPickup` interface instead of a base class | **Rejected for now.** Locking on, movement and the once-only guard are the same for every pickup. A base class shares them. |
| Awarding XP by listening to `OnEnemyKilled` | **Rejected.** The ticket wants a physical orb the player collects, and XP given only at arrival. |
| XP as a plain `float` on the player | **Rejected.** The project keeps player-owned numbers in `UStatsComponent`. |
| Writing XP into a modifier instead of the base value | **Rejected.** Modifiers are for buffs and can be removed. Earned XP is permanent, so it lives in the **base value**. |

## 3. Prerequisites

- Unreal Engine 5.8 C++ project that builds (`GrantMoneyEditor` Development Editor Win64).
- SCRUM-33 merged: `UStatsComponent`, `UHealthComponent`, and `AEnemyBase::HandleDeath`.
- `AGrantMoneyPlayerCharacter` and `ASlimeEnemy`.
- To see drops in normal play, SCRUM-32's weapon (or use the `GrantMoney.DamageEnemies` console command from SCRUM-33).
- `scripts/run_tests.py` for command-line tests. Close the Unreal Editor first, because Live Coding breaks the build.

## 4. Architecture Overview

```
   Player                                          World
   ──────                                          ─────
   AGrantMoneyPlayerCharacter
     ├─ UStatsComponent ◄──── Experience (base value = running XP total)
     └─ UPickupComponent  (PickupRadius = 300 cm)
            │ every frame: sphere scan (ECC_WorldDynamic objects)
            ▼
        APickupBase found ──► BeginAttraction(player)       ← once; repeats ignored
                                   │ locks on, starts at BaseSpeed
                                   ▼
                             Tick (post-physics)
                               speed → MaxSpeed (Acceleration)
                               aim at player's CURRENT position
                               arrived, or step would overshoot?
                                   │ yes
                                   ▼
                             TryCollect(player)              ← once; bCollected guard
                                   ├─► OnCollected(player)   ← the ONLY thing a pickup type writes
                                   │      • AXpOrbPickup: Experience += XpValue
                                   │      • future power-up: AddModifier(...) + timer
                                   └─► Destroy()

   Enemy death
   ───────────
   UHealthComponent::OnDeath (fires once)
        └─► AEnemyBase::HandleDeath
               ├─ capture DeathTransform
               ├─ stop tick / collision / movement
               ├─ SpawnPickupDrops()
               │     ├─ bDropsXpOrb → spawn XpOrbClass at death location (always)
               │     └─ for each BonusPickupDrops entry: roll DropChance → spawn
               ├─ UCombatEventsSubsystem::OnEnemyKilled
               └─ Destroy() / SetLifeSpan(DeathCleanupDelay)
```

**Responsibility boundaries:**

| Class | Owns | Does not do |
|---|---|---|
| `UPickupComponent` | The radius, and finding pickups | Movement, collecting, or knowing any pickup type |
| `APickupBase` | Locking on, speed, arrival, once-only collection, removal | What the pickup does (`OnCollected`) |
| `AXpOrbPickup` | XP value, placeholder look, adding XP | Leveling, UI |
| `UStatsComponent` | The `Experience` number | Deciding when XP is earned |
| `AEnemyBase` | What an enemy drops when it dies | Anything about how pickups move |
| `FEnemyPickupDrop` | One class plus one chance | Spawning |

## 5. AI-Assisted Planning

### AI Prompt

> Implement SCRUM-35 in Grant Money Survivor. Add a reusable pickup system and make guaranteed XP orbs the first pickup type. When a slime dies, it drops a small glowing green orb. When the player gets close enough, the orb flies toward them and awards XP on arrival. For this milestone, XP is a running total.
>
> Inspect the repository first. Find the active player class, the shared enemy base, and the zero-health death flow used by SCRUM-32. Follow the project's existing Unreal C++ conventions and wire the feature into the normal play flow.
>
> (The prompt then gave four numbered requirements: a shared pickup system with a collection hook; enemy drops with independent chances, keeping XP at 100%; XP orb behaviour with editable base speed, acceleration, max speed and collection distance; and player XP stored using the existing stats approach. It ended with PIE verification and a summary request.)

This was the actual opening prompt, shortened. It worked because it named the classes to find, separated "shared code" from "what each pickup does", and listed the exact tuning values to expose. `CLAUDE.md` also told the AI to inspect only relevant files, keep gameplay in C++, and write a test for every new `.cpp`.

### Expected AI Output

- A short list of the real classes found (player, enemy base, death flow) before any code.
- A design where adding a pickup type does not touch the pickup component.
- Code plus Automation tests, with real build and test output.
- A summary of files, editor setup, defaults and where to add a new pickup.
- Honest reporting of what was **not** run (here, PIE).

### Human Review

Before accepting, check:
- **The hook:** does `OnCollected` get called from one place only, and is a once-only guard in front of it?
- **Drops:** is the XP orb outside the chance list, so no bonus entry can lower it? Are drops spawned inside the existing death handler and not on a second code path?
- **Movement:** is speed multiplied by frame time (frame-rate independent)? Is there an arrival check that survives a long frame?
- **Stats:** was `Experience` added at the **end** of the enum? Is XP written to the base value?
- **Numbers:** is `MaxSpeed` above the player's real top speed? The default is 1200 against the character default of 600, but a Blueprint may override `MaxWalkSpeed`. Check it in the editor.
- **Evidence:** are claimed test results backed by real tool output? Is PIE marked as not run?

## 6. Step-by-Step Implementation

The work was done in four steps, with a compile and a full `GrantMoney.*` run at the end.

### Step 1 – The shared pickup base

1. **(Git Bash)** Create the branch from an up-to-date `main` (the repository is on `SCRUM-35-xp-drops`):
   ```bash
   git checkout main && git pull && git checkout -b SCRUM-35-xp-drops
   ```
2. **(IDE)** Create `APickupBase` (`UCLASS(Abstract)`, so only subclasses can be spawned):
   - Root `USphereComponent` (12 cm), `QueryOnly`, object type `ECC_WorldDynamic`, **all responses Ignore**, no overlap events. It exists only so a scan can find the actor.
   - A `UStaticMeshComponent` called `PickupMesh` for the look (no collision).
   - Tick is **off** until a collector locks on, and runs after physics so the pickup aims at where the player ended up this frame.
3. **(IDE)** `BeginAttraction(Collector)`: returns false if already collected, if the collector is invalid, or if the pickup is already chasing a live collector. Otherwise stores a `TWeakObjectPtr`, sets the speed to `BaseSpeed`, and turns tick on.
4. **(IDE)** `Tick`:
   ```cpp
   const float TopSpeed = FMath::Max(MaxSpeed, BaseSpeed);
   const float NewSpeed = FMath::Min(CurrentSpeed + Acceleration * DeltaSeconds, TopSpeed);
   const double Step = 0.5 * (CurrentSpeed + NewSpeed) * DeltaSeconds;   // average speed over the frame
   CurrentSpeed = NewSpeed;

   const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();   // current position, so it follows turns
   const double Distance = ToTarget.Size();

   if (Distance - Step <= CollectionDistance)   // arrived, or would pass the range this frame
   {
       SetActorLocation(GetActorLocation() + ToTarget.GetSafeNormal() * FMath::Min(Step, Distance));
       TryCollect(Target);
       return;
   }
   SetActorLocation(GetActorLocation() + ToTarget / Distance * Step);
   ```
   *Why the average speed:* using the speed at both ends of the frame gives the same distance at 30, 60 or 240 fps (tested). *Why the second condition:* a long frame or a fast orb could otherwise jump past the player and never collect.
5. **(IDE)** `TryCollect(Collector)`:
   ```cpp
   if (bCollected || !IsValid(Collector) || IsActorBeingDestroyed()) { return false; }
   bCollected = true;           // set BEFORE the effect, so nothing can re-enter
   SetActorTickEnabled(false);
   OnCollected(Collector);      // virtual: the pickup type's effect
   Destroy();
   return true;
   ```
6. **(IDE)** If the collector is destroyed mid-chase, `StopAttraction()` turns tick off and the pickup waits for another collector.
7. **(IDE)** Write `PickupBaseTests.cpp` and the helpers `Tests/PickupTestScene.h` and `Tests/PickupTestTypes.h`:
   - The scene is a game world with a frozen stand-in player (`ACharacter` + Stats + `UPickupComponent`). Tests move the player by hand, which simulates sprinting.
   - `ATestBuffPickup` is a **second pickup type**. It counts collections and adds a stat modifier in `OnCollected`.
8. **(Terminal)** Build and test.

### Step 2 – The player's pickup component and the XP orb

1. **(IDE)** Create `UPickupComponent`:
   - `PickupRadius` (default 300 cm, editable).
   - Each tick it runs `OverlapMultiByObjectType` for `ECC_WorldDynamic` in a sphere around the owner (the owner is ignored), casts each hit actor to `APickupBase`, and calls `BeginAttraction`. Other world-dynamic actors (projectiles, props) are skipped by the cast.
2. **(IDE)** Add `Experience` at the **end** of `EGrantMoneyStat`.
3. **(IDE)** Create `AXpOrbPickup : APickupBase`:
   - `XpValue = 5`.
   - Constructor: engine sphere mesh scaled 0.24 (24 cm), and `GlowMaterial` = `/Engine/EngineMaterials/EmissiveMeshMaterial`.
   - `BeginPlay`: makes a dynamic material instance and sets the vector parameter `EmissiveColor` to green `(0.2, 3.0, 0.3)`. Values above 1 glow brighter. If a swapped material has no such parameter, nothing breaks.
   - `OnCollected`:
     ```cpp
     const float NewTotal = Stats->GetBaseStat(EGrantMoneyStat::Experience) + XpValue;
     Stats->SetBaseStat(EGrantMoneyStat::Experience, NewTotal);
     ```
     It works for any collector with a `UStatsComponent`, and logs a warning if there is none.
4. **(IDE)** Player: create `Pickups = CreateDefaultSubobject<UPickupComponent>(TEXT("Pickups"))`, call `Stats->SetBaseStat(EGrantMoneyStat::Experience, 0.0f)` in the constructor, and add `GetExperience()`.
5. **(IDE)** Write `PickupComponentTests.cpp` and `XpOrbPickupTests.cpp`.
6. **(Terminal)** Build and test.

### Step 3 – Enemy drops

1. **(IDE)** Create `FEnemyPickupDrop` (`PickupClass`, `DropChance` 0–1). `ShouldDrop(Roll)` returns false with no class or a chance of 0, true at a chance of 1, and otherwise `Roll < DropChance`.

   *Why the special cases:* `FMath::FRand()` can return exactly 0 or exactly 1, so "0% never drops" and "100% always drops" are not left to the roll.
2. **(IDE)** In `AEnemyBase` add:
   - `bDropsXpOrb` (default **false**, so a plain enemy drops nothing),
   - `XpOrbClass` (defaults to `AXpOrbPickup`),
   - `TArray<FEnemyPickupDrop> BonusPickupDrops`,
   - virtual `SpawnPickupDrops()`.
3. **(IDE)** `SpawnPickupDrops` spawns at `DeathTransform.GetLocation()` with `AlwaysSpawn`:
   ```cpp
   if (bDropsXpOrb && XpOrbClass) { World->SpawnActor<AXpOrbPickup>(XpOrbClass, SpawnTransform, SpawnParams); }
   for (const FEnemyPickupDrop& Drop : BonusPickupDrops)
   {
       if (Drop.ShouldDrop(FMath::FRand())) { World->SpawnActor<APickupBase>(Drop.PickupClass, SpawnTransform, SpawnParams); }
   }
   ```
   The XP orb is **not** in the list, and each entry rolls on its own, so no bonus entry can lower or replace the XP drop.
4. **(IDE)** In `HandleDeath`, call `SpawnPickupDrops()` right after the enemy is stopped and **before** the kill event and `Destroy()`. Do **not** add a second "already dead" flag. `UHealthComponent::ApplyDamage` already fires `OnDeath` once, which is why simultaneous lethal hits drop once.
5. **(IDE)** `ASlimeEnemy` constructor: `bDropsXpOrb = true;`.
6. **(IDE)** Write `EnemyDropTests.cpp`.
7. **(Terminal)** Build and test.

### Step 4 – Whole-suite check

1. **(Terminal)**
   ```powershell
   python scripts/run_tests.py
   ```
   *Expect:* `62/62 tests passed (filter 'GrantMoney').`
2. **(Editor)** Do the PIE checks in [B9](#9-verification), and record the media listed at the end of this document.

## 6A. Recipe: Add a New Pickup and Make It a Rare Drop

**This is the part to hand to a teammate.** It adds a temporary-buff power-up and makes slimes drop it 5% of the time, on top of the guaranteed XP orb.

**What you will touch:** two new files, plus one list entry. **What you will not touch:** `UPickupComponent`, the player, `APickupBase`, `AXpOrbPickup`.

### 6A.1 Write the pickup class

`Public/HealthBoostPickup.h` (a health boost here, because the player's weapon does not read the `Damage` stat yet; see the caution below):

```cpp
#pragma once

#include "CoreMinimal.h"
#include "PickupBase.h"
#include "HealthBoostPickup.generated.h"

UCLASS()
class GRANTMONEY_API AHealthBoostPickup : public APickupBase
{
    GENERATED_BODY()

public:
    AHealthBoostPickup();

    // +0.5 = +50% max health while the buff lasts.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Buff")
    float MaxHealthBonus = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Buff", meta = (ClampMin = "0.1", Units = "s"))
    float Duration = 15.0f;

protected:
    virtual void OnCollected(AActor* Collector) override;
};
```

`Private/HealthBoostPickup.cpp`:

```cpp
#include "HealthBoostPickup.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "StatsComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AHealthBoostPickup::AHealthBoostPickup()
{
    // Own look. The base class already gave you collision, a PickupMesh and the movement defaults.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Mesh.Succeeded())
    {
        PickupMesh->SetStaticMesh(Mesh.Object);
        PickupMesh->SetRelativeScale3D(FVector(0.25));
    }
}

void AHealthBoostPickup::OnCollected(AActor* Collector)
{
    UStatsComponent* Stats = Collector->FindComponentByClass<UStatsComponent>();
    if (!Stats)
    {
        return;
    }

    const FGrantMoneyStatModifierHandle Handle =
        Stats->AddModifier(EGrantMoneyStat::MaxHealth, EGrantMoneyStatModifierOp::Multiplicative, MaxHealthBonus);

    // This pickup is destroyed right after OnCollected returns, so the timer must NOT belong to it.
    // A timer on the world, with a weak reference to the stats component, outlives the pickup.
    FTimerHandle Unused;
    Collector->GetWorldTimerManager().SetTimer(
        Unused,
        FTimerDelegate::CreateWeakLambda(Stats, [Stats, Handle]() { Stats->RemoveModifier(Handle); }),
        Duration, false);
}
```

> **Not compiled or tested.** This snippet shows the pattern. The tested equivalent is `ATestBuffPickup` in `Private/Tests/PickupTestTypes.h`, which adds a modifier through the same hook. Build it and add a test for the timer before relying on it.

Notes on the hook:
- `OnCollected` runs **once**, then the pickup is destroyed. Never keep the effect alive in members of the pickup.
- Several pickups stack: each adds its own modifier and has its own timer. If you want "refresh instead of stack", store the handle and timer on the collector, not here.
- The health component already raises current health when max health goes up (SCRUM-33), so the buff is visible immediately.
- **Caution for damage buffs:** the starter weapon's damage is the weapon's own `Damage` property, copied into each orb (`FallbackDamage`). The player's `Damage` stat is **not** read by it yet. A `Damage` modifier will change the stat but not the shots until the weapon is wired to stats.

### 6A.2 Make slimes drop it, 5% of the time

Pick one:

**Option 1 – In the editor, no code (preferred for tuning).** Open the slime Blueprint (or the placed slime in the level). In Details, **Enemy|Drops → Bonus Pickup Drops**: click `+`, set `Pickup Class` = `HealthBoostPickup`, `Drop Chance` = `0.05`.

**Option 2 – In C++, as the slime's default.** In `ASlimeEnemy::ASlimeEnemy()`:

```cpp
FEnemyPickupDrop RareHealthBoost;
RareHealthBoost.PickupClass = AHealthBoostPickup::StaticClass();
RareHealthBoost.DropChance = 0.05f;   // 5% per death
BonusPickupDrops.Add(RareHealthBoost);
```
and add `#include "HealthBoostPickup.h"`.

**What you get:** every dead slime drops one XP orb (100%). In addition, about 1 in 20 also drops one health boost at the same spot. The two rolls are independent. No bonus entry can reduce the XP orb, because the orb is not part of the list.

### 6A.3 Add a test (required by `CLAUDE.md`)

Add `HealthBoostPickupTests.cpp` next to the code. Copy the pattern from `PickupBaseTests.cpp`:

```cpp
PickupTest::FScene Scene;
Scene.Create(*this);
Scene.SpawnPickup<AHealthBoostPickup>(FVector(150.0, 0.0, 0.0));
Scene.Step(2.0f);
// assert the modifier exists (Stats->GetModifiers()), then Scene.Step(Duration) and assert it is gone
```

### 6A.4 Checklist for any new pickup

- [ ] Derives from `APickupBase`. Overrides only `OnCollected`.
- [ ] Never destroys itself (`TryCollect` does that) and never calls `OnCollected` itself.
- [ ] Anything timed lives on the **collector** or the **world**, not on the pickup.
- [ ] `MaxSpeed` stays above the player's top speed (the default is fine).
- [ ] If it changes a stat, the stat exists in `EGrantMoneyStat`. New stats are added only at the **end**.
- [ ] A test using `PickupTest::FScene`.
- [ ] Added to `BonusPickupDrops` with a chance between 0 and 1. Not added as `XpOrbClass`.

## 7. AI Prompts by Stage

Reusable versions of the prompts, for a different game.

### Prompt A – Inspect and plan

> Read the ticket below. Find my player class, my shared enemy base and its death flow. Do not read generated folders or binary assets. List the classes the pickup system must connect to, how the player already stores numeric stats, and the smallest design in which a new pickup type needs no change to the player. Propose files to create and change. Ask me about any decision that is mine. Do not write code yet.

### Prompt B – Shared pickup core

> Create an abstract pickup actor that owns everything shared: lock onto a collector, fly toward the collector's current position every frame with acceleration to a maximum speed, collect on arrival (including when one frame would overshoot), and collect only once. Subclasses override one virtual function for the effect. Make movement frame-rate independent. Add Automation tests for frame-rate independence, no overshoot, once-only collection, and a second pickup type.

### Prompt C – Detection

> Add a component for the player with an editable radius that finds pickups of any type inside it and tells them to lock on. It must not know pickup types. Repeated detection must be harmless. Test that the radius is respected, that pursuit continues after the player leaves the radius, and that non-pickups are ignored.

### Prompt D – Drops

> Make enemies drop one guaranteed pickup at their death location through the existing death handler, before they are destroyed. Add a reusable list of extra pickup classes, each with an independent chance. The guaranteed drop must not depend on that list. Reuse the existing once-only death handling, and test simultaneous lethal hits.

### Prompt E – Debug a failing test

> This Automation test fails with `<exact message>`. Before changing code, decide whether the code or the test is wrong, and explain why. Change only the side that is wrong.

### Prompt F – PIE verification when Unreal cannot be run

> I can't run PIE right now. Give me the exact editor steps and console commands to verify each acceptance criterion, and the expected Output Log lines. Do not claim anything was verified in PIE.

## 8. Common Mistakes

These all happened, or were caught, during this ticket.

### 8.1 Build error: a parameter named like a member
- **Symptom:** `error C4458: declaration of 'Collector' hides class member` in `PickupBase.h`, `XpOrbPickup.cpp` and `PickupTestTypes.h`.
- **Likely cause:** the base class stored the locked-on actor in a member called `Collector`, and the `OnCollected(AActor* Collector)` parameter in every subclass used the same name. Unreal builds treat shadowing as an error.
- **Fix:** rename the member to `LockedCollector`. Keep the parameter name, so subclass code reads naturally.
- **Lesson:** a name that is natural for a parameter is a bad name for a member of a class whose subclasses will override that function. *Real build failure.*

### 8.2 Test failure: the test's estimate was wrong, not the code
- **Symptom:** `GrantMoney.Pickups.PickupBase.CatchesMovingCollector` failed with "Expected 'The pickup fell behind at first…' to be true" (61/62).
- **Likely cause:** the test asserted that the orb's largest gap exceeded 250 cm. That was a hand calculation. The orb aims diagonally at the moving player, so it closes the gap faster than the estimate allowed. The pursuit itself worked.
- **Fix:** remove the brittle gap assertion. Keep the two that matter: the orb is collected exactly once, and the player was well outside the original 300 cm radius when it was caught.
- **Lesson:** assert outcomes, not intermediate numbers you estimated by hand. Decide which side is wrong before changing code. *Real test failure.*

### 8.3 `FMath::FRand()` can return exactly 1.0
- **Symptom (avoided):** `Roll < DropChance` with a 0% chance would still drop when the roll is exactly 0, and the code would rely on `Roll < 1.0` being true for a 100% chance.
- **Fix:** `ShouldDrop` treats a chance of 0 as "never" and a chance of 1 as "always" before it looks at the roll.
- **Lesson:** make the endpoints of a probability explicit. *Caught while designing Step 3.*

### 8.4 A damage buff that changes the stat but not the shots
- **Symptom (avoided):** the first draft of the example power-up buffed the `Damage` stat. Nothing would have visibly changed.
- **Likely cause:** `UStarterWeaponComponent::Damage` is a plain property copied into the projectile. The player's `Damage` stat is not read.
- **Fix:** the example uses `MaxHealth`, which `UHealthComponent` does react to. The header example in `PickupBase.h` was corrected the same way.
- **Lesson:** before documenting an example effect, check that the system it targets really reads that stat. *Caught while writing this document.*

### 8.5 Timers on a pickup that is about to be destroyed
- **Symptom (to avoid):** a buff pickup starts a timer on itself to remove the buff, but the buff never expires.
- **Likely cause:** `TryCollect` calls `Destroy()` straight after `OnCollected`, so the pickup's timers die with it.
- **Fix:** put the timer on the world's timer manager with a weak reference to the stats component (see B6A.1), or on the collector.
- **Lesson:** a pickup's `OnCollected` is its last act. *Read from source; a design rule, not a failure that happened.*

### 8.6 Engine material parameter name not confirmed on screen
- **Symptom (possible):** the orb appears with the material's default colour instead of green.
- **Likely cause:** the code sets a vector parameter named `EmissiveColor` on `/Engine/EngineMaterials/EmissiveMeshMaterial`. The name was read from the asset's internal strings, but it has not been checked on screen. If it is wrong, the parameter is ignored silently.
- **Fix:** if the orb is not green, open the material in the Editor, check the parameter name, and update `GlowParameterName` (it is editable). Or point `GlowMaterial` at your own emissive material.
- **Lesson:** `SetVectorParameterValue` never reports a wrong name. Look at it. *Open item; verified only by PIE (see media 13 and 14).*

## 9. Verification

### Performed

| Test | Action | Result | Kind |
|---|---|---|---|
| Build | `GrantMoneyEditor` via `python scripts/run_tests.py` | Succeeded (after the 8.1 fix) | Compiled |
| Full run, first | `GrantMoney.*` tests | 61/62 (8.2) | Automation test |
| Full run, final | `GrantMoney.*` tests | **62/62 passed** (40 existing, 22 new) | Automation test |

**The 22 new tests:**

| Test group | Tests |
|---|---|
| `PickupBase` | CollectsOnce, WaitsUntilAttracted, AcceleratesToMaxSpeed, FrameRateIndependent, CollectsInsteadOfOvershooting, CatchesMovingCollector, CollectorDestroyed |
| `PickupComponent` | RadiusDecidesWhatIsNoticed, PursuitOutlivesTheRadius, SupportsAnyPickupType, IgnoresNonPickups, PlayerDefaults |
| `XpOrb` | AddsXpOnArrivalOnly, SeveralOrbsAddUp, OutOfRangeOrbIsNeverCollected, TuningDefaults |
| `EnemyDrops` | SlimeDropsExactlyOneOrb, SimultaneousLethalHitsDropOnce, EnemyBaseDropsNothingByDefault, BonusDropsAreIndependentOfXp, EachBonusEntryRollsOnItsOwn, ChanceRolls |

The test world has no rendering, no Game Mode and no real map. Characters are frozen, and the test moves the stand-in player by hand. These tests say nothing about how the orb looks, whether the glow is green, or how it feels at real speeds.

**To rerun:**
- In the Editor: Session Frontend → Automation, filter `GrantMoney.Pickups`, run.
- From the command line: `python scripts/run_tests.py --filter GrantMoney.Pickups`.

### Pending (developer, in the Editor)

No sprint exists in the project. To simulate one, select the player in the PIE outliner and set `Character Movement → Max Walk Speed` to about 1000 while playing.

| Test | Action | Expected Result |
|---|---|---|
| Player components | Open the player Blueprint or select the player, Details panel | A `Pickups` component with `Pickup Radius` 300, and `Stats` with `Experience` 0 in Base Stats |
| Player top speed | Details → Character Movement → Max Walk Speed | Note the value. It must be below the orb's `Max Speed` (1200). If a Blueprint raised it, raise the orb's value. |
| Slime flag | Select a slime, Details → Enemy\|Drops | `Drops Xp Orb` checked, `Xp Orb Class` = `XpOrbPickup`, `Bonus Pickup Drops` empty |
| One orb per slime | PIE, console `GrantMoney.DamageEnemies 100` with N slimes in the level | N small glowing green orbs, one per death spot, none stacked at the origin |
| Orb waits | Stay more than 300 cm from an orb for several seconds | The orb does not move |
| Lock on | Walk inside 300 cm | The orb starts moving toward you, slowly at first and then faster |
| Arrival | Let it reach you | Log: `XpOrbPickup: <player> gained 5.0 XP (total 5.0)`. The orb disappears. |
| Once per orb | Select the player, Stats → Base Stats | `Experience` increased by exactly 5, and stays the same after the orb is gone |
| Several orbs | Collect 4 orbs | `Experience` = 20 (4 × 5) |
| Pursuit after leaving radius | Attract an orb, then run away beyond 300 cm | The orb still follows |
| Catch-up on sprint | Max Walk Speed ≈ 1000, attract an orb, sprint in a straight line | The orb lags at first, then accelerates and reaches you |
| Follows turns | Attract an orb, then turn sharply several times | The orb curves toward your current position, not your old one |
| Edit tuning live | Change Pickup Radius to 600 on the player's `Pickups` | Orbs are attracted from further away |
| Edit orb value | Set `Xp Value` on the XP orb class (Blueprint subclass or placed orb) to 25 | The next orb adds 25 |
| Rare drop (optional) | Follow B6A with Drop Chance 1.0 on a test pickup | Slime drops both a green orb and the test pickup. XP is still awarded. |
| Packaged build | — | Not required for this ticket; not performed |

Results: _you fill in_ (only with what was actually observed).

## 10. Known Limitations

- **PIE has not been run.** All behaviour above is from Automation tests.
- **The glow colour is unconfirmed on screen** (8.6). The material is an engine one, with an additive blend.
- **No debug view of the pickup radius.** The detection is a scene query, so `show collision` does not draw it. A temporary `DrawDebugSphere` while recording is the quickest way.
- **Orbs never expire.** An uncollected orb stays forever. With many kills far from the player, they pile up. A lifetime or magnet-all-on-wave-end is a later decision.
- **No visible orb pooling or batching.** Each orb is an actor. Large numbers may need pooling, and a dedicated collision channel would make the scan cheaper.
- **The scan runs every frame, using `ECC_WorldDynamic`.** Other world-dynamic actors are returned by the query and then skipped by class.
- **The orb flies to the player's actor origin in 3D**, starting from the slime capsule's centre (about 30 cm above ground). Flying up and into the player is expected.
- **The player's top speed is not read at runtime.** `MaxSpeed` (1200) is a number a human keeps above the player's top speed (600 by default). If the player gets faster than the orb, the player can outrun it.
- **XP is a float stat** (exact up to about 16 million). `GetExperience()` reads the final stat value, so a modifier on `Experience` would distort the displayed total. Don't put modifiers on it. Use another stat for XP gain multipliers.
- **XP orb value is per orb class, not per enemy.** A tougher enemy needs its own orb subclass or setting on `XpOrbClass`.
- **Nothing consumes `Experience` yet.** No level, no HUD, no upgrade choice (a later ticket).
- **No sound, no pickup VFX, no hit feedback.**
- The example power-up in B6A is not compiled, and the player's `Damage` stat is not read by the weapon yet (8.4).
- Not replicated (the game is single-player).
- `ATestBuffPickup` is a `UCLASS`, so it is compiled into every build of the game module, although only the tests use it.

## 11. Extension Ideas

- **Any genre with collectables.** Coins, health hearts, ammo and gems are all `APickupBase` subclasses. Only `OnCollected` differs.
- **Magnet power-up.** A pickup whose effect raises `PickupRadius` for 10 seconds (`UPickupComponent::PickupRadius` is public and editable). It needs a timer on the collector or world.
- **Permanent stat boosts (SCRUM-53).** A pickup whose effect is an `Additive` modifier it never removes.
- **Health drops.** `OnCollected` calls `UHealthComponent::Heal`.
- **Level-up.** Bind to `UStatsComponent::OnStatChanged` and check for the `Experience` stat reaching a threshold. `XpOrbPickup` does not change.
- **Drop tables.** Create a data asset holding a `TArray<FEnemyPickupDrop>` and let an enemy reference it. Several enemy types can then share one table.
- **Rarity scaling.** A spawner multiplies `DropChance` for tougher enemies.

## 12. Source-Control Checkpoint

| Item | Value |
|---|---|
| Jira | SCRUM-35 |
| Branch | `SCRUM-35-xp-drops` |
| PR title | `SCRUM-35: Reusable pickup system and guaranteed XP orb drops` |

**Staging (only milestone files):**
```bash
git add Game/Source/GrantMoney/Public/PickupBase.h Game/Source/GrantMoney/Private/PickupBase.cpp \
        Game/Source/GrantMoney/Public/PickupComponent.h Game/Source/GrantMoney/Private/PickupComponent.cpp \
        Game/Source/GrantMoney/Public/XpOrbPickup.h Game/Source/GrantMoney/Private/XpOrbPickup.cpp \
        Game/Source/GrantMoney/Public/EnemyPickupDrop.h \
        Game/Source/GrantMoney/Private/Tests/PickupTestTypes.h Game/Source/GrantMoney/Private/Tests/PickupTestScene.h \
        Game/Source/GrantMoney/Private/PickupBaseTests.cpp Game/Source/GrantMoney/Private/PickupComponentTests.cpp \
        Game/Source/GrantMoney/Private/XpOrbPickupTests.cpp Game/Source/GrantMoney/Private/EnemyDropTests.cpp \
        Game/Source/GrantMoney/Public/GrantMoneyStatTypes.h \
        Game/Source/GrantMoney/Public/GrantMoneyPlayerCharacter.h Game/Source/GrantMoney/Private/GrantMoneyPlayerCharacter.cpp \
        Game/Source/GrantMoney/Public/EnemyBase.h Game/Source/GrantMoney/Private/EnemyBase.cpp \
        Game/Source/GrantMoney/Private/SlimeEnemy.cpp \
        docs/course/Module3-CreatingChallengeAndReward/Sprint1/SCRUM-35-ScriptDocumentation.md
```

Raw recordings stay out of Git (`Media/SCRUM-35/` is outside the `AI-Native-Game-Guru` folder).

**Checklist:**
- [x] Project compiles (`GrantMoneyEditor`)
- [x] Automation tests pass (62/62)
- [ ] Feature checked in Unreal (PIE checks above)
- [ ] Teammate review
- [x] No generated Unreal folders, binaries or credentials staged
- [ ] Media captured (list below)

---

# Part C – AI Workflow Log

**Tool:** GitHub Copilot (an AI assistant in VS Code) working in the repository. It ran the build and tests itself through `scripts/run_tests.py`. It did **not** run Unreal in PIE.

| # | Purpose | Prompt (developer) | Result | Decision | Verification |
|---|---|---|---|---|---|
| 1 | Inspect and design | The ticket prompt (section 5) | Read `CLAUDE.md`, `AEnemyBase`, `ASlimeEnemy`, the player class, `UStatsComponent`, `UHealthComponent`, `UCombatEventsSubsystem`, the weapon and the magic orb. Found that death is already one-time and that player numbers live in stats. Chose stat-based XP, a base class for pickups, and a scan for detection. | **Accepted** | — |
| 2 | Shared pickup and XP orb | (continued from 1) | `APickupBase`, `UPickupComponent`, `AXpOrbPickup`, the `Experience` stat, and the player wiring. First build failed (8.1). | **Accepted after fix** | Failed build → fixed |
| 3 | Enemy drops | (continued from 1) | `FEnemyPickupDrop`, drop settings on `AEnemyBase`, the call in `HandleDeath`, and the slime flag. | **Accepted** | Compiled |
| 4 | Tests | (continued from 1) | 22 tests with a second pickup type (`ATestBuffPickup`). One failed because the test, not the code, was wrong (8.2). | **Accepted after test fix** | 61/62 → 62/62 |
| 5 | Verification plan | "…If you cannot run Unreal, provide the exact editor steps for those checks." | PIE steps and a note that no sprint input exists. | **Accepted** | Not run |
| 6 | Documentation | "write this into a SCRUM-35 doc similar to SCRUM-33-ScriptDocumentation… informing the future worker about adding a rare powerup… detailed media capture list" | This document. Caught that the example buff targeted a stat the weapon ignores (8.4) and fixed the header comment. | — | Checked against the source and the SCRUM-33 document |

**Where AI helped most:**
- Reading the existing death flow and stats before writing anything, so nothing was duplicated.
- Writing the second pickup type as a test, which proves the "no change to the pickup component" claim.
- Noticing endpoints (`FRand` can be 1.0, a `Damage` stat that nothing reads).

**Where it needed a human:**
- Confirming the glow looks green and the orb feels right (not verified).
- Checking that `Max Walk Speed` was not changed in the Player Blueprint.
- Deciding XP-per-enemy and drop rates (placeholders here).

---

# Part D – Video Voice-Over Script

## Opening

**SAY:** "So far, when a slime dies it just disappears. Today it leaves something behind: a small green orb. Walk close and it flies to you and gives you XP. The orb is the first pickup, but the real work today is a pickup system that the next ten pickups will reuse without changing a line of the player."

## Acceptance Criteria

**SAY:** "At the end of this part:
- every slime drops exactly one orb where it dies;
- the orb waits until we come close, then flies at us and keeps following even if we run;
- XP goes up exactly once per orb, and several orbs add up;
- and a teammate can add a rare power-up that drops next to the guaranteed XP, by writing one function and one list entry."

## Engineering Principle

**SAY:** "Our principle today: **share the how, write only the what.** Finding the pickup, flying to the player and collecting only once are the same for every pickup. The only thing that differs is what happens on arrival. So that's the only thing a new pickup writes."

## Prerequisites

**SAY:** "You need:
- a C++ Unreal project that builds;
- a player and an enemy that can die;
- a place the player's numbers live. In GrantMoney that's the stats component from the combat ticket."

## Step-by-Step Demonstration

### 1. Plan with AI before writing code

- **SAY:** "I give the AI the ticket and tell it to find my player class, my enemy base and the death flow first. No code yet."
- **SHOW:** The AI's list of real classes, and its note that death already fires once and that the player's numbers live in stats.
- **DO:** Confirm the design: XP as a stat, a pickup base class, a scan for detection.
- **AI PROMPT:** Prompt A (B7).
- **VERIFY:** The plan names real files, and doesn't invent a second death path. *(media 02)*

> **Here is an engineering discussion.** We could detect pickups with overlap events. We didn't. Overlap events mean every pickup needs its own collision setup, and has to say which things it overlaps. A scan around the player needs none of that, the radius can change while playing, and repeated detection is harmless. The cost is a little work every frame.

### 2. The shared pickup base

- **SAY:** "The pickup base locks onto the player, speeds up toward the player's current position, and collects. Collection can happen only once, even if the same pickup is hit twice in a frame. A flag is set before the effect runs."
- **SHOW:** `APickupBase::Tick`, `TryCollect`, and the `OnCollected` declaration.
- **DO:** Build. Run Automation → `GrantMoney.Pickups.PickupBase`.
- **AI PROMPT:** Prompt B.
- **VERIFY:** The seven pickup base tests are green. *(media 03, 04)*

> **Here is an engineering discussion.** Movement uses the average speed over the frame, so the orb travels the same distance at 30 or 240 frames per second. And we check "would this step pass the player?" and not only "are we close enough?". Without that, a fast orb on a slow frame could jump over you and never be collected.

### 3. The player's radius and the XP orb

- **SAY:** "The player gets a pickup component. It only finds pickups and says 'come here'. The XP orb adds to the Experience stat when it arrives. We store it as a stat because that's where our player numbers already live."
- **SHOW:** `UPickupComponent::AttractPickupsInRadius`, `AXpOrbPickup::OnCollected`, and the player constructor.
- **DO:** Build and test `GrantMoney.Pickups.PickupComponent` and `GrantMoney.Pickups.XpOrb`.
- **AI PROMPT:** Prompt C.
- **VERIFY:** The tests are green. In the Details panel the player has `Pickups` and `Experience` 0. *(media 05, 06, 09)*

### 4. Enemy drops

- **SAY:** "The slime drops through the death handler we already had. That handler runs once, so two bullets landing together can't give two orbs. The XP orb isn't in the chance list, so nothing else can lower it. The list is for extras, each with its own chance."
- **SHOW:** `SpawnPickupDrops`, `FEnemyPickupDrop`, and the slime's Details panel under Enemy | Drops.
- **DO:** Build and test `GrantMoney.Pickups.EnemyDrops`.
- **AI PROMPT:** Prompt D.
- **VERIFY:** The six drop tests are green. *(media 07, 08, 11)*

### 5. Play it

- **SAY:** "Now let's see it. In PIE I kill the slimes with the console command, and count the orbs."
- **SHOW:** The Output Log filter on `XpOrbPickup`, and the player's Experience in Details.
- **DO:** Run `GrantMoney.DamageEnemies 100`. Stay outside the radius. Walk in. Sprint away. Turn. Collect several.
- **VERIFY:** Everything in section B9, *Pending*. *(media 10, 12 to 21)*

### 6. Add a rare power-up (the handoff)

- **SAY:** "To prove it's reusable, here's how a teammate adds a power-up. One class overriding one function, and one entry in the slime's drop list with a chance of five percent."
- **SHOW:** Section B6A on screen, and the slime's Bonus Pickup Drops array.
- **DO:** Do not build it on camera unless you have compiled and tested it first.
- **VERIFY:** The example matches the tested `ATestBuffPickup`. *(media 22, 23)*

## Common Mistakes

**SAY:** "Here are mistakes that came up.

- **A parameter and a member with the same name.** Unreal treats that shadowing as a build error.
- **A test that asserted a number I'd worked out by hand.** The orb behaved correctly. The estimate was wrong. Assert the outcome, not the estimate.
- **Probability endpoints.** The random function can return exactly one, so 'never' and 'always' are decided before the roll.
- **A buff for a stat nothing reads.** Our weapon doesn't use the damage stat yet, so we used max health for the example.
- **A timer on the pickup.** The pickup is destroyed right after its effect, and takes the timer with it."

## Verification Checkpoint

**SAY:** "Now let's verify our result."

**SHOW:**
- The build result.
- Automation: 62 of 62 `GrantMoney` tests green.
- In PIE: one orb per slime, the orb waiting outside the radius, catching up after a sprint, and Experience going up by exactly the orb's value.

*(Record the PIE parts only once they've actually been performed.)*

## AI Review

**SAY:** "Let's review what AI actually did here.
- It read the death flow and the stats before writing, and reused both.
- It wrote the code, the tests and a second pickup type that proves the design.
- It caused one build error and one wrong test, and both are in the mistakes list.
- It could not play the game, and said so.

The human decisions:
- how fast orbs should feel;
- how much XP a slime is worth and how rare a power-up is;
- whether the glow actually looks right."

## Engineering Pause

**SAY:** "Here is an engineering pause. Consider this question: if adding a new pickup meant editing the player's pickup component, what would we have built? One XP feature, or a pickup system?

The test of a system is whether the second user needs the first one's code to change."

## Completion Criteria

**SAY:** "This milestone is complete when:
- every slime drops exactly one orb;
- the orb waits, locks on, follows, and is collected once;
- XP is a running total on the player;
- a rare power-up can be added with one function and one list entry;
- and the automated tests and the in-editor checks have been run."

## Next Milestone

**SAY:** "Next, we can use that XP total. A level-up listens for the Experience stat reaching a threshold, and offers an upgrade. The orb doesn't change at all."

---

# Media Capture List

Store files under `Media/SCRUM-35/` (outside the repository, like the other tickets), named exactly as below. Numbers follow the order of the implementation steps. Only record what actually happens. Do not edit results.

**Recording setup (do once):**
- Resolution 1920×1080, Editor and PIE at the same size. Hide unrelated panels.
- Output Log docked, with filter text `XpOrbPickup`. Run the console with the backtick key.
- Player selected in the PIE **World Outliner** with the **Details** panel visible. Search the Details panel for `Experience` so the running total stays on screen.
- At least 4 slimes in the level, plus a clear stretch of open floor for sprinting.
- For the sprint clips, set the player's `Max Walk Speed` to about 1000 while playing, and say so in the voice-over. Reset afterwards.
- Keep the clips short (10 to 25 seconds) and cut idle time.
- For radius clips, either keep the player a known distance from the orb or use a temporary `DrawDebugSphere`. Do not commit the debug code.

## A. Setup and design (Steps 1 to 3 of the voice-over)

| # | File | Type | Show | Proves |
|---|---|---|---|---|
| 01 | `01-branch-created.png` | Screenshot | Terminal: `git branch --show-current` printing `SCRUM-35-xp-drops` | Work is on the ticket branch |
| 02 | `02-existing-systems-reused.png` | Screenshot | `EnemyBase.cpp` `HandleDeath` (with `DeathTransform`) next to `StatsComponent.h` | The design reuses the existing death flow and stats |
| 03 | `03-pickup-base-header.png` | Screenshot | `PickupBase.h`: the class comment with the example, the four tuning properties, and `OnCollected` | The one hook a pickup type overrides |
| 04 | `04-pickup-tick-and-collect.png` | Screenshot | `PickupBase.cpp`: `Tick` (average speed, `Distance - Step <= CollectionDistance`) and `TryCollect` (`bCollected` set before the effect) | Frame-rate independence, no overshoot, once-only collection |
| 05 | `05-pickup-component.png` | Screenshot | `PickupComponent.cpp`: `AttractPickupsInRadius` | Detection that knows no pickup type |
| 06 | `06-xp-orb-on-collected.png` | Screenshot | `XpOrbPickup.cpp`: `OnCollected` adding to the `Experience` base stat, plus the `Experience` entry at the end of `GrantMoneyStatTypes.h` | XP is a stat, appended at the end of the enum |
| 07 | `07-enemy-drops-code.png` | Screenshot | `EnemyBase.cpp`: `SpawnPickupDrops` and its call in `HandleDeath`, plus `EnemyPickupDrop.h` | XP outside the chance list, drops inside the one-time death handler |
| 08 | `08-all-tests-pass.png` | Screenshot | Automation window (or the `run_tests.py` output) showing `62/62 tests passed`, with `GrantMoney.Pickups` expanded | Build and automated tests are green |

## B. Editor setup (before playing)

| # | File | Type | Show | Proves |
|---|---|---|---|---|
| 09 | `09-player-pickups-component.png` | Screenshot | Player Details: `Pickups` component with `Pickup Radius` 300, and `Stats` → Base Stats → `Experience` 0 | The player owns the component and the stat |
| 10 | `10-player-max-walk-speed.png` | Screenshot | Player Details → Character Movement → `Max Walk Speed`, next to the orb's `Max Speed` 1200 | The orb can outrun the player |
| 11 | `11-slime-drop-settings.png` | Screenshot | Slime Details → Enemy\|Drops: `Drops Xp Orb` checked, `Xp Orb Class` = `XpOrbPickup`, `Bonus Pickup Drops` empty | The guaranteed drop is enabled for slimes only |
| 12 | `12-orb-tuning-properties.png` | Screenshot | An XP orb (placed temporarily, or a Blueprint subclass) Details: `Base Speed` 400, `Acceleration` 1500, `Max Speed` 1200, `Collection Distance` 40, `Xp Value` 5 | Every tuning value is exposed and editable |

## C. Behaviour in Play In Editor

| # | File | Type | Show | Proves |
|---|---|---|---|---|
| 13 | `13-one-orb-per-slime.mp4` | Clip | Several slimes in view. Console `GrantMoney.DamageEnemies 100`. Count the orbs: one green glowing sphere per slime at its death spot. Pause and point at each. | Exactly one orb per slime, at the death location, and the glow looks green (**also confirms 8.6**) |
| 14 | `14-orb-glow-closeup.png` | Screenshot | Close-up of an orb on the floor | The placeholder look: small, green, emissive |
| 15 | `15-orb-waits-outside-radius.mp4` | Clip | Stay outside the radius for about 5 seconds, with the orb in frame. Then step toward it. | The orb does not move outside the radius, and wakes up inside it |
| 16 | `16-orb-collected-xp-log.mp4` | Clip | Walk in. The orb flies to you. Output Log shows `gained 5.0 XP (total 5.0)`. Details shows `Experience` 5. | XP is added on arrival, by the orb's own value |
| 17 | `17-several-orbs-add-up.mp4` | Clip | Collect 4 orbs one after another. `Experience` goes 5, 10, 15, 20 with one log line each. | Each orb counts exactly once, and the total adds up |
| 18 | `18-orb-keeps-pursuing.mp4` | Clip | Attract an orb, then run well beyond 300 cm. The orb keeps following and is collected. | The orb stays locked on after the player leaves the radius |
| 19 | `19-sprint-catch-up.mp4` | Clip | `Max Walk Speed` about 1000. Attract an orb. Sprint in a straight line. The orb lags, accelerates, and catches you. | Acceleration to a max speed above the player's speed |
| 20 | `20-orb-follows-turns.mp4` | Clip | Attract an orb, then change direction sharply several times. The orb curves toward you each time. | It aims at the current position every frame |
| 21 | `21-live-radius-edit.mp4` | Clip | While playing, change `Pickup Radius` from 300 to 600 on the player's `Pickups`. A previously waiting orb now starts flying. | The radius is editable and used immediately |

## D. The handoff (rare power-up)

Only record these after you have actually built the power-up from B6A and its test passes.

| # | File | Type | Show | Proves |
|---|---|---|---|---|
| 22 | `22-bonus-drop-entry.png` | Screenshot | Slime Details → Enemy\|Drops → `Bonus Pickup Drops` with one entry: your pickup class, `Drop Chance` 0.05 (or 1.0 for the demo) | A rare drop is one list entry |
| 23 | `23-power-up-and-xp-together.mp4` | Clip | With `Drop Chance` 1.0 for the demo: kill a slime. Both the green orb and the power-up appear. Collect both. XP goes up, and the buff effect shows (for example Max Health in the `Health` component). Reset the chance to 0.05 afterwards. | The bonus drop works next to the guaranteed XP orb, through the same hook, without changing the player |

## E. Optional extras

| # | File | Type | Show | Proves |
|---|---|---|---|---|
| 24 | `24-no-duplicate-on-simultaneous-hits.mp4` | Clip | With the starter weapon firing and several slimes close together, count orbs after a burst of kills. Never more than one per slime. | Simultaneous lethal hits do not duplicate drops |
| 25 | `25-ai-session-transcript.png` | Screenshot | The AI conversation: the ticket prompt, the build error (8.1), and the test failure (8.2) | The AI workflow log is real |

**Media notes:**
- Record **13**, **16**, **19** first. If those three look right, the rest will follow.
- If the orb is **not** green in 13, stop and fix 8.6 before recording the rest.
- Do not record clips as evidence for anything that did not happen. If a check fails, keep the clip and add it to Common Mistakes.
