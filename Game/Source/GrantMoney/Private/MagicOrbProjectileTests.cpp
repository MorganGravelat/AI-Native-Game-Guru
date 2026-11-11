#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

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

#endif // WITH_DEV_AUTOMATION_TESTS
