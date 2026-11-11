#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GrantMoneyPlayerCharacter.h"
#include "Tests/WeaponTestScene.h"

// Targeting and firing tests. Slimes are frozen in place, so these exercise the weapon's
// choices, not enemy movement. The orb's flight and damage are covered in MagicOrbProjectileTests.cpp.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStarterWeaponDefaultsTest,
    "GrantMoney.Weapons.StarterWeapon.Defaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStarterWeaponDefaultsTest::RunTest(const FString& Parameters)
{
    const UStarterWeaponComponent* Weapon = GetDefault<UStarterWeaponComponent>();
    TestEqual(TEXT("Damage"), Weapon->Damage, 10.0f);
    TestEqual(TEXT("Fire interval"), Weapon->FireInterval, 1.0f);
    TestEqual(TEXT("Targeting range"), Weapon->TargetRange, 800.0f);
    TestEqual(TEXT("Projectile speed"), Weapon->ProjectileSpeed, 1000.0f);
    TestEqual(TEXT("Projectile lifetime"), Weapon->ProjectileLifetime, 2.0f);

    // The player starts the run with the weapon, with no setup.
    const AGrantMoneyPlayerCharacter* Player = GetDefault<AGrantMoneyPlayerCharacter>();
    TestNotNull(TEXT("The player owns a starter weapon"), Player->GetStarterWeapon());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStarterWeaponTargetsAnyDirectionTest,
    "GrantMoney.Weapons.StarterWeapon.TargetsAnyDirection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStarterWeaponTargetsAnyDirectionTest::RunTest(const FString& Parameters)
{
    WeaponTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    // The slime is directly behind a shooter that faces +X.
    ASlimeEnemy* Slime = Scene.SpawnSlime(FVector(-300.0, 0.0, 0.0));
    Scene.Shooter->SetActorRotation(FRotator::ZeroRotator);

    TestTrue(TEXT("A slime behind the shooter is targeted"), Scene.Weapon->SelectTarget() == Slime);

    // Whatever way the shooter faces makes no difference.
    for (const double Yaw : { 90.0, 180.0, -90.0 })
    {
        Scene.Shooter->SetActorRotation(FRotator(0.0, Yaw, 0.0));
        TestTrue(FString::Printf(TEXT("Still targeted when facing yaw %.0f"), Yaw), Scene.Weapon->SelectTarget() == Slime);
    }

    AMagicOrbProjectile* Orb = Scene.Weapon->FireAt(Slime);
    if (!TestNotNull(TEXT("An orb is fired"), Orb))
    {
        return false;
    }

    const FVector Heading = WeaponTest::VelocityOf(Orb).GetSafeNormal();
    TestTrue(TEXT("The orb heads backward, toward the slime"), Heading.Equals(FVector(-1.0, 0.0, 0.0), 0.001));
    TestEqual(TEXT("At the weapon's projectile speed"), static_cast<float>(WeaponTest::VelocityOf(Orb).Size()), 1000.0f, 0.5f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStarterWeaponPicksNearestTest,
    "GrantMoney.Weapons.StarterWeapon.PicksNearest",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStarterWeaponPicksNearestTest::RunTest(const FString& Parameters)
{
    WeaponTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    // The far slime is spawned first, so the answer is not just "the first one found".
    Scene.SpawnSlime(FVector(0.0, 500.0, 0.0));
    ASlimeEnemy* Near = Scene.SpawnSlime(FVector(-300.0, 0.0, 0.0));
    TestTrue(TEXT("The closer of two living slimes is targeted"), Scene.Weapon->SelectTarget() == Near);

    // Distance is measured on the ground plane, so a slime 200 cm higher is not "farther".
    ASlimeEnemy* Raised = Scene.SpawnSlime(FVector(250.0, 0.0, 200.0));
    TestTrue(TEXT("Height does not count toward distance"), Scene.Weapon->SelectTarget() == Raised);

    // Range is a hard limit.
    Scene.Weapon->TargetRange = 200.0f;
    TestNull(TEXT("Nothing is targeted when every slime is beyond range"), Scene.Weapon->SelectTarget());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStarterWeaponSkipsOutOfRangeTest,
    "GrantMoney.Weapons.StarterWeapon.SkipsOutOfRange",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStarterWeaponSkipsOutOfRangeTest::RunTest(const FString& Parameters)
{
    WeaponTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    // Range is 800: both slimes are out of it.
    ASlimeEnemy* First = Scene.SpawnSlime(FVector(900.0, 0.0, 0.0));
    ASlimeEnemy* Second = Scene.SpawnSlime(FVector(0.0, -1200.0, 0.0));

    TestNull(TEXT("Nothing in range"), Scene.Weapon->SelectTarget());
    TestFalse(TEXT("TryFire skips the shot"), Scene.Weapon->TryFire());
    TestEqual(TEXT("No orb was fired"), Scene.CountOrbs(), 0);

    // Three whole intervals pass with nothing in range: still nothing fired, and the weapon keeps checking.
    Scene.Step(3.1f);
    TestEqual(TEXT("Still no orbs after several intervals"), Scene.CountOrbs(), 0);
    TestEqual(TEXT("First slime untouched"), WeaponTest::HealthOf(First), 30.0f);
    TestEqual(TEXT("Second slime untouched"), WeaponTest::HealthOf(Second), 30.0f);

    // A slime walks into range. The next interval's check finds it, with no other trigger.
    First->SetActorLocation(FVector(600.0, 0.0, 0.0));
    Scene.Step(1.0f);
    TestEqual(TEXT("The next interval fires at it"), Scene.CountOrbs(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStarterWeaponRetargetsAfterDeathTest,
    "GrantMoney.Weapons.StarterWeapon.RetargetsAfterDeath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStarterWeaponRetargetsAfterDeathTest::RunTest(const FString& Parameters)
{
    WeaponTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    ASlimeEnemy* Near = Scene.SpawnSlime(FVector(300.0, 0.0, 0.0));
    ASlimeEnemy* Middle = Scene.SpawnSlime(FVector(0.0, 500.0, 0.0));
    ASlimeEnemy* Far = Scene.SpawnSlime(FVector(-700.0, 0.0, 0.0));
    TestTrue(TEXT("Starts on the nearest"), Scene.Weapon->SelectTarget() == Near);

    // Dead but still in the world (a death animation would look like this): it must not be chosen.
    Near->DeathCleanupDelay = 5.0f;
    Near->GetHealth()->ApplyDamage(1000.0f);
    TestFalse(TEXT("The dead slime is still in the world"), Near->IsActorBeingDestroyed());
    TestTrue(TEXT("A new target is picked after the first dies"), Scene.Weapon->SelectTarget() == Middle);

    // Dead and already removed.
    Middle->GetHealth()->ApplyDamage(1000.0f);
    TestTrue(TEXT("The removed slime is not chosen either"), Scene.Weapon->SelectTarget() == Far);

    Far->GetHealth()->ApplyDamage(1000.0f);
    TestNull(TEXT("With every slime dead there is no target"), Scene.Weapon->SelectTarget());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStarterWeaponFiresOnIntervalTest,
    "GrantMoney.Weapons.StarterWeapon.FiresOnInterval",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStarterWeaponFiresOnIntervalTest::RunTest(const FString& Parameters)
{
    WeaponTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    // Far enough that an orb is still in the air a moment after it is fired (0.7 s of flight).
    ASlimeEnemy* Slime = Scene.SpawnSlime(FVector(700.0, 0.0, 0.0));

    Scene.Step(0.9f);
    TestEqual(TEXT("Nothing fires before the first interval ends"), Scene.CountOrbs(), 0);

    Scene.Step(0.25f);
    TestEqual(TEXT("One orb after the first interval (t = 1.15 s)"), Scene.CountOrbs(), 1);
    TestEqual(TEXT("It has not landed yet"), WeaponTest::HealthOf(Slime), 30.0f);

    Scene.Step(0.7f);
    TestEqual(TEXT("The first orb landed (t = 1.85 s): 10 damage"), WeaponTest::HealthOf(Slime), 20.0f);
    TestEqual(TEXT("and was removed"), Scene.CountOrbs(), 0);

    Scene.Step(0.3f);
    TestEqual(TEXT("A second orb after the second interval (t = 2.15 s)"), Scene.CountOrbs(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStarterWeaponStopsWhenRemovedTest,
    "GrantMoney.Weapons.StarterWeapon.StopsWhenRemoved",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStarterWeaponStopsWhenRemovedTest::RunTest(const FString& Parameters)
{
    {
        // The owner (the player) is removed.
        WeaponTest::FScene Scene;
        if (!Scene.Create(*this))
        {
            return false;
        }

        Scene.SpawnSlime(FVector(500.0, 0.0, 0.0));
        Scene.Step(0.5f);
        Scene.Shooter->Destroy();
        Scene.Step(4.0f);
        TestEqual(TEXT("No orbs are fired after the player is removed"), Scene.CountOrbs(), 0);
    }

    {
        // Only the weapon is removed.
        WeaponTest::FScene Scene;
        if (!Scene.Create(*this))
        {
            return false;
        }

        ASlimeEnemy* Slime = Scene.SpawnSlime(FVector(500.0, 0.0, 0.0));
        Scene.Step(0.5f);
        Scene.Weapon->DestroyComponent();
        Scene.Step(4.0f);
        TestEqual(TEXT("No orbs are fired after the weapon is removed"), Scene.CountOrbs(), 0);
        TestEqual(TEXT("The slime was never hit"), WeaponTest::HealthOf(Slime), 30.0f);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStarterWeaponUsesItsSettingsTest,
    "GrantMoney.Weapons.StarterWeapon.UsesItsSettings",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStarterWeaponUsesItsSettingsTest::RunTest(const FString& Parameters)
{
    WeaponTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    // The slime is raised, so the aim must tilt up toward its collision center.
    ASlimeEnemy* Slime = Scene.SpawnSlime(FVector(400.0, 0.0, 300.0));
    Scene.Weapon->Damage = 7.0f;
    Scene.Weapon->ProjectileSpeed = 500.0f;
    Scene.Weapon->ProjectileLifetime = 1.5f;

    AMagicOrbProjectile* Orb = Scene.Weapon->FireAt(Slime);
    if (!TestNotNull(TEXT("An orb is fired"), Orb))
    {
        return false;
    }

    TestEqual(TEXT("Damage comes from the weapon"), Orb->GetAttackHit()->GetDamage(), 7.0f);
    TestEqual(TEXT("Speed comes from the weapon"), static_cast<float>(WeaponTest::VelocityOf(Orb).Size()), 500.0f, 0.5f);
    TestEqual(TEXT("Lifetime comes from the weapon"), Orb->GetLifetimeSeconds(), 1.5f);

    const FVector ToSlimeCenter = (Slime->GetActorLocation() - Scene.Shooter->GetActorLocation()).GetSafeNormal();
    TestTrue(TEXT("Aimed at the slime's collision center"), WeaponTest::VelocityOf(Orb).GetSafeNormal().Equals(ToSlimeCenter, 0.001));
    TestTrue(TEXT("The orb belongs to the shooter"), Orb->GetInstigator() == Scene.Shooter && Orb->GetOwner() == Scene.Shooter);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
