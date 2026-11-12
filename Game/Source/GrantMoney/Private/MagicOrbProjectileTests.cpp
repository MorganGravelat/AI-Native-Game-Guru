#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/StaticMeshComponent.h"
#include "StatsComponent.h"
#include "Tests/WeaponTestScene.h"

// Flight, hit and expiry tests for the magic orb. The shooter's weapon is told to wait 1000 s
// between shots, so only the orbs these tests fire by hand exist. The orb damages through the
// engine's damage path (UGameplayStatics::ApplyDamage), as in the game.

namespace
{
    constexpr float NoAutoFire = 1000.0f;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMagicOrbNonLethalHitTest,
    "GrantMoney.Weapons.MagicOrb.NonLethalHit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMagicOrbNonLethalHitTest::RunTest(const FString& Parameters)
{
    WeaponTest::FScene Scene;
    if (!Scene.Create(*this, NoAutoFire))
    {
        return false;
    }

    ASlimeEnemy* Slime = Scene.SpawnSlime(FVector(300.0, 0.0, 0.0));
    TestEqual(TEXT("Slime starts at full health"), WeaponTest::HealthOf(Slime), 30.0f);

    TestNotNull(TEXT("An orb is fired"), Scene.Weapon->FireAt(Slime));
    TestEqual(TEXT("One orb in the air"), Scene.CountOrbs(), 1);

    Scene.Step(0.6f);
    TestEqual(TEXT("The hit took the weapon's damage (10) off the slime's health"), WeaponTest::HealthOf(Slime), 20.0f);
    TestFalse(TEXT("The slime survives a non-lethal hit"), Slime->IsActorBeingDestroyed());
    TestEqual(TEXT("No kill is reported"), Scene.Listener->EnemyKilledCount, 0);
    TestEqual(TEXT("The orb is removed by the hit"), Scene.CountOrbs(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMagicOrbLethalHitTest,
    "GrantMoney.Weapons.MagicOrb.LethalHitDespawnsOnce",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMagicOrbLethalHitTest::RunTest(const FString& Parameters)
{
    WeaponTest::FScene Scene;
    if (!Scene.Create(*this, NoAutoFire))
    {
        return false;
    }

    Scene.Weapon->Damage = 100.0f;
    ASlimeEnemy* Slime = Scene.SpawnSlime(FVector(300.0, 0.0, 0.0));
    const TWeakObjectPtr<ASlimeEnemy> WeakSlime(Slime);

    // Two orbs arrive in the same frame, so a double kill would show up here.
    TestNotNull(TEXT("First orb fired"), Scene.Weapon->FireAt(Slime));
    TestNotNull(TEXT("Second orb fired"), Scene.Weapon->FireAt(Slime));
    Scene.Step(0.6f);

    TestEqual(TEXT("Death is handled exactly once"), Scene.Listener->EnemyKilledCount, 1);
    TestFalse(TEXT("The slime is gone from the world"), WeakSlime.IsValid());
    TestTrue(TEXT("The kill event was reported before removal"), Scene.Listener->bKilledEnemyWasAliveWhenReported);
    TestTrue(TEXT("The killer reported is an orb"), Cast<AMagicOrbProjectile>(Scene.Listener->LastKiller.Get(true)) != nullptr);
    TestTrue(TEXT("The death spot was captured where the slime died"), Scene.Listener->LastKilledDeathTransform.GetLocation().Equals(FVector(300.0, 0.0, 0.0), 0.01));

    // The orb that killed it is used up. The other flew on through the empty spot and expires with its lifetime.
    Scene.Step(2.1f);
    TestEqual(TEXT("No orbs remain"), Scene.CountOrbs(), 0);
    TestEqual(TEXT("Still exactly one kill"), Scene.Listener->EnemyKilledCount, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMagicOrbHitsWhatItTouchesTest,
    "GrantMoney.Weapons.MagicOrb.HitsWhatItTouches",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMagicOrbHitsWhatItTouchesTest::RunTest(const FString& Parameters)
{
    WeaponTest::FScene Scene;
    if (!Scene.Create(*this, NoAutoFire))
    {
        return false;
    }

    // Aimed at the far slime, but another stands in the way.
    ASlimeEnemy* InTheWay = Scene.SpawnSlime(FVector(200.0, 0.0, 0.0));
    ASlimeEnemy* Aimed = Scene.SpawnSlime(FVector(500.0, 0.0, 0.0));
    TestNotNull(TEXT("An orb is fired at the far slime"), Scene.Weapon->FireAt(Aimed));
    Scene.Step(0.8f);

    TestEqual(TEXT("The slime actually hit takes the damage"), WeaponTest::HealthOf(InTheWay), 20.0f);
    TestEqual(TEXT("The slime that was aimed at is untouched"), WeaponTest::HealthOf(Aimed), 30.0f);
    TestEqual(TEXT("The orb is used up on the first slime"), Scene.CountOrbs(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMagicOrbDamagesOncePerOrbTest,
    "GrantMoney.Weapons.MagicOrb.DamagesOncePerOrb",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMagicOrbDamagesOncePerOrbTest::RunTest(const FString& Parameters)
{
    WeaponTest::FScene Scene;
    if (!Scene.Create(*this, NoAutoFire))
    {
        return false;
    }

    // Two slimes on the same spot: the orb touches both in the same frame.
    ASlimeEnemy* First = Scene.SpawnSlime(FVector(300.0, 0.0, 0.0));
    ASlimeEnemy* Second = Scene.SpawnSlime(FVector(300.0, 0.0, 0.0));
    TestNotNull(TEXT("An orb is fired"), Scene.Weapon->FireAt(First));
    Scene.Step(0.6f);

    const float TotalDamage = (30.0f - WeaponTest::HealthOf(First)) + (30.0f - WeaponTest::HealthOf(Second));
    TestEqual(TEXT("One orb deals its damage once, in total"), TotalDamage, 10.0f);
    TestEqual(TEXT("The orb is gone"), Scene.CountOrbs(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMagicOrbMissExpiresTest,
    "GrantMoney.Weapons.MagicOrb.MissExpires",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMagicOrbMissExpiresTest::RunTest(const FString& Parameters)
{
    WeaponTest::FScene Scene;
    if (!Scene.Create(*this, NoAutoFire))
    {
        return false;
    }

    Scene.Weapon->ProjectileLifetime = 0.5f;
    ASlimeEnemy* Slime = Scene.SpawnSlime(FVector(500.0, 0.0, 0.0));
    TestNotNull(TEXT("An orb is fired"), Scene.Weapon->FireAt(Slime));

    // The slime steps out of the line of fire after the aim is taken: the orb does not follow it.
    Slime->SetActorLocation(FVector(500.0, 400.0, 0.0));

    Scene.Step(0.2f);
    AMagicOrbProjectile* Orb = Scene.FirstOrb();
    if (!TestNotNull(TEXT("The orb is still flying 0.2 s in"), Orb))
    {
        return false;
    }

    // A straight line along the aim: about 200 cm out in 0.2 s, no sideways drift, no drop.
    const FVector Position = Orb->GetActorLocation();
    TestEqual(TEXT("Travelled about speed x time"), static_cast<float>(Position.X), 200.0f, 30.0f);
    TestEqual(TEXT("No sideways drift"), static_cast<float>(Position.Y), 0.0f, 0.5f);
    TestEqual(TEXT("No fall"), static_cast<float>(Position.Z), 0.0f, 0.5f);

    Scene.Step(0.5f);
    TestEqual(TEXT("The missed orb expired after its lifetime"), Scene.CountOrbs(), 0);
    TestEqual(TEXT("The slime was never hit"), WeaponTest::HealthOf(Slime), 30.0f);
    TestEqual(TEXT("No kill"), Scene.Listener->EnemyKilledCount, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMagicOrbIgnoresShooterTest,
    "GrantMoney.Weapons.MagicOrb.IgnoresShooter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMagicOrbIgnoresShooterTest::RunTest(const FString& Parameters)
{
    WeaponTest::FScene Scene;
    if (!Scene.Create(*this, NoAutoFire))
    {
        return false;
    }

    // Give the shooter health, so hurting it would be possible if the orb did not ignore it.
    UHealthComponent* ShooterHealth = NewObject<UHealthComponent>(Scene.Shooter);
    ShooterHealth->RegisterComponent();

    ASlimeEnemy* Slime = Scene.SpawnSlime(FVector(700.0, 0.0, 0.0));
    TestNotNull(TEXT("An orb is fired"), Scene.Weapon->FireAt(Slime));

    // The orb starts inside the shooter's own capsule.
    Scene.Step(0.1f);
    TestEqual(TEXT("The shooter is not hurt"), ShooterHealth->GetHealth(), ShooterHealth->GetMaxHealth());
    TestEqual(TEXT("The orb was not used up by the shooter"), Scene.CountOrbs(), 1);

    Scene.Step(0.8f);
    TestEqual(TEXT("The orb went on to hit the slime"), WeaponTest::HealthOf(Slime), 20.0f);
    TestEqual(TEXT("The shooter is still not hurt"), ShooterHealth->GetHealth(), ShooterHealth->GetMaxHealth());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMagicOrbPassesDeadSlimeTest,
    "GrantMoney.Weapons.MagicOrb.PassesDeadSlime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMagicOrbPassesDeadSlimeTest::RunTest(const FString& Parameters)
{
    WeaponTest::FScene Scene;
    if (!Scene.Create(*this, NoAutoFire))
    {
        return false;
    }

    // A dead slime that is still in the world (cleanup delay) is not a target and does not use up the orb.
    ASlimeEnemy* Dead = Scene.SpawnSlime(FVector(200.0, 0.0, 0.0));
    ASlimeEnemy* Alive = Scene.SpawnSlime(FVector(500.0, 0.0, 0.0));
    Dead->DeathCleanupDelay = 5.0f;
    Dead->GetHealth()->ApplyDamage(1000.0f);
    TestEqual(TEXT("The first slime died"), Scene.Listener->EnemyKilledCount, 1);

    TestNotNull(TEXT("An orb is fired at the living slime"), Scene.Weapon->FireAt(Alive));
    Scene.Step(0.8f);

    TestEqual(TEXT("The living slime took the hit"), WeaponTest::HealthOf(Alive), 20.0f);
    TestEqual(TEXT("The dead slime did not report a second death"), Scene.Listener->EnemyKilledCount, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMagicOrbExplosionHitsNearbyEnemiesTest,
    "GrantMoney.Weapons.MagicOrb.ExplosionHitsNearbyEnemiesOnExhaustion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMagicOrbExplosionHitsNearbyEnemiesTest::RunTest(const FString& Parameters)
{
    WeaponTest::FScene Scene;
    if (!Scene.Create(*this, NoAutoFire))
    {
        return false;
    }

    // No stats component needed: with none, GetEffectiveExplosionRadius() falls back to this property.
    Scene.Weapon->ExplosionRadius = 100.0f;

    // Default Penetration 0: the orb explodes the instant its one direct hit exhausts it.
    ASlimeEnemy* Direct = Scene.SpawnSlime(FVector(300.0, 0.0, 0.0));
    // Off the orb's flight line (perpendicular distance 50 cm, more than the 10 + 30 cm needed for a
    // direct hit), but within the 100 cm blast once Direct is hit nearby.
    ASlimeEnemy* InBlast = Scene.SpawnSlime(FVector(300.0, 50.0, 0.0));
    // Far enough off-line that even the blast cannot reach it.
    ASlimeEnemy* OutOfBlast = Scene.SpawnSlime(FVector(300.0, 300.0, 0.0));

    AMagicOrbProjectile* Orb = Scene.Weapon->FireAt(Direct);
    if (!TestNotNull(TEXT("An orb is fired"), Orb))
    {
        return false;
    }
    TestEqual(TEXT("The orb carries the weapon's explosion radius"), Orb->GetExplosionRadius(), 100.0f);

    Scene.Step(0.35f);
    TestEqual(TEXT("The directly hit slime took exactly one hit's damage"), WeaponTest::HealthOf(Direct), 20.0f);
    TestEqual(TEXT("A nearby slime within the blast was also hit, by the explosion, not a direct touch"), WeaponTest::HealthOf(InBlast), 20.0f);
    TestEqual(TEXT("A slime outside the blast radius is untouched"), WeaponTest::HealthOf(OutOfBlast), 30.0f);
    TestTrue(TEXT("The orb is mid-burst"), Orb->IsExploding());
    TestFalse(TEXT("Not yet removed: the burst visual is still playing"), Orb->IsActorBeingDestroyed());

    // The visual grows while the burst plays, then the orb is removed once it finishes.
    UStaticMeshComponent* Mesh = Orb->FindComponentByClass<UStaticMeshComponent>();
    if (TestNotNull(TEXT("The orb has its placeholder mesh"), Mesh))
    {
        const double ScaleRightAfterTheBurstStarted = Mesh->GetRelativeScale3D().X;
        Scene.Step(Orb->ExplosionVisualSeconds * 0.5f);
        TestTrue(TEXT("Still growing partway through the burst"), Mesh->GetRelativeScale3D().X > ScaleRightAfterTheBurstStarted);
    }

    Scene.Step(Orb->ExplosionVisualSeconds);
    TestTrue(TEXT("Removed once the burst visual finishes"), Orb->IsActorBeingDestroyed());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMagicOrbExplosionOnlyOnFirstContactTest,
    "GrantMoney.Weapons.MagicOrb.ExplosionOnlyOnFirstContact",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMagicOrbExplosionOnlyOnFirstContactTest::RunTest(const FString& Parameters)
{
    // With penetration, the orb must explode exactly once, at its first direct hit, and keep flying
    // through its remaining penetration targets afterward without exploding again.
    WeaponTest::FScene Scene;
    if (!Scene.Create(*this, NoAutoFire))
    {
        return false;
    }

    Scene.Weapon->ExplosionRadius = 100.0f;

    UStatsComponent* Stats = NewObject<UStatsComponent>(Scene.Shooter);
    Stats->RegisterComponent();
    Stats->AddModifier(EGrantMoneyStat::Penetration, EGrantMoneyStatModifierOp::Additive, 2.0f);

    // Three slimes in a row along the orb's path (penetration 2 means up to 3 direct hits), each with a
    // neighbour 50 cm off to the side: within the 100 cm blast, but not on the flight line itself.
    ASlimeEnemy* First = Scene.SpawnSlime(FVector(300.0, 0.0, 0.0));
    ASlimeEnemy* NearFirst = Scene.SpawnSlime(FVector(300.0, 50.0, 0.0));
    ASlimeEnemy* Second = Scene.SpawnSlime(FVector(400.0, 0.0, 0.0));
    ASlimeEnemy* NearSecond = Scene.SpawnSlime(FVector(400.0, 50.0, 0.0));
    ASlimeEnemy* Third = Scene.SpawnSlime(FVector(500.0, 0.0, 0.0));
    ASlimeEnemy* NearThird = Scene.SpawnSlime(FVector(500.0, 50.0, 0.0));

    AMagicOrbProjectile* Orb = Scene.Weapon->FireAt(First);
    if (!TestNotNull(TEXT("An orb is fired"), Orb))
    {
        return false;
    }

    Scene.Step(1.0f); // enough for all three direct hits and the one burst's visual to finish

    TestEqual(TEXT("First slime: a direct hit"), WeaponTest::HealthOf(First), 20.0f);
    TestEqual(TEXT("Beside the first: hit by the one explosion, at the first contact"), WeaponTest::HealthOf(NearFirst), 20.0f);
    TestEqual(TEXT("Second slime: a direct hit via penetration, no second explosion"), WeaponTest::HealthOf(Second), 20.0f);
    TestEqual(TEXT("Beside the second: the explosion charge was already spent, so untouched"), WeaponTest::HealthOf(NearSecond), 30.0f);
    TestEqual(TEXT("Third slime: a direct hit via penetration, no third explosion"), WeaponTest::HealthOf(Third), 20.0f);
    TestEqual(TEXT("Beside the third: still untouched"), WeaponTest::HealthOf(NearThird), 30.0f);
    TestTrue(TEXT("The explosion charge is spent"), Orb->HasExploded());
    TestEqual(TEXT("The orb is used up (penetration 2 = 3 hits) and removed"), Scene.CountOrbs(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMagicOrbExplosionOnExpiryTest,
    "GrantMoney.Weapons.MagicOrb.ExplosionAlsoHappensOnAMiss",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMagicOrbExplosionOnExpiryTest::RunTest(const FString& Parameters)
{
    WeaponTest::FScene Scene;
    if (!Scene.Create(*this, NoAutoFire))
    {
        return false;
    }

    Scene.Weapon->ExplosionRadius = 100.0f;
    Scene.Weapon->ProjectileLifetime = 0.5f;
    // Default speed (1000 cm/s): the orb travels about 500 cm before its lifetime runs out.

    // Far enough off-line (perpendicular distance 60 cm) to never be hit directly by the flying orb,
    // but close to where it expires, so the burst (not a direct touch) is what reaches it.
    ASlimeEnemy* NearExpiry = Scene.SpawnSlime(FVector(500.0, 60.0, 0.0));
    // Only used to aim the orb far down the same line; well out of range of the direct flight or the burst.
    ASlimeEnemy* AimTarget = Scene.SpawnSlime(FVector(5000.0, 0.0, 0.0));

    AMagicOrbProjectile* Orb = Scene.Weapon->FireAt(AimTarget);
    if (!TestNotNull(TEXT("An orb is fired"), Orb))
    {
        return false;
    }

    TestEqual(TEXT("Full health before the orb expires"), WeaponTest::HealthOf(NearExpiry), 30.0f);
    Scene.Step(0.7f); // past the 0.5 s lifetime
    TestEqual(TEXT("A miss that still carries an explosion upgrade bursts where it expires"), WeaponTest::HealthOf(NearExpiry), 20.0f);
    TestEqual(TEXT("The aim target, far out of blast range, is untouched"), WeaponTest::HealthOf(AimTarget), 30.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMagicOrbNoExplosionWithoutUpgradeTest,
    "GrantMoney.Weapons.MagicOrb.NoExplosionWithoutTheUpgrade",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMagicOrbNoExplosionWithoutUpgradeTest::RunTest(const FString& Parameters)
{
    // Regression guard: the default (ExplosionRadius 0) must behave exactly as before this ticket.
    WeaponTest::FScene Scene;
    if (!Scene.Create(*this, NoAutoFire))
    {
        return false;
    }

    TestEqual(TEXT("No explosion radius by default"), Scene.Weapon->ExplosionRadius, 0.0f);

    ASlimeEnemy* Direct = Scene.SpawnSlime(FVector(300.0, 0.0, 0.0));
    ASlimeEnemy* Nearby = Scene.SpawnSlime(FVector(300.0, 50.0, 0.0));

    AMagicOrbProjectile* Orb = Scene.Weapon->FireAt(Direct);
    if (!TestNotNull(TEXT("An orb is fired"), Orb))
    {
        return false;
    }

    Scene.Step(0.6f);
    TestEqual(TEXT("The directly hit slime took the hit"), WeaponTest::HealthOf(Direct), 20.0f);
    TestEqual(TEXT("A nearby slime is untouched: no explosion without the upgrade"), WeaponTest::HealthOf(Nearby), 30.0f);
    TestFalse(TEXT("Not exploding"), Orb->IsExploding());
    TestTrue(TEXT("Removed immediately, as before"), Orb->IsActorBeingDestroyed());
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
