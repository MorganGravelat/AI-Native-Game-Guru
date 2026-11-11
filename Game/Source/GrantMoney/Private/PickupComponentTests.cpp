#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GrantMoneyPlayerCharacter.h"
#include "Tests/PickupTestScene.h"

// The player's pickup radius (UPickupComponent): which pickups it notices, and that any pickup
// type works through the same code. The pickup radius in these tests is 300 cm.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPickupComponentRadiusTest,
    "GrantMoney.Pickups.PickupComponent.RadiusDecidesWhatIsNoticed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPickupComponentRadiusTest::RunTest(const FString& Parameters)
{
    PickupTest::FScene Scene;
    if (!Scene.Create(*this, 300.0f))
    {
        return false;
    }

    AXpOrbPickup* Inside = Scene.SpawnPickup<AXpOrbPickup>(FVector(200.0, 0.0, 0.0));
    AXpOrbPickup* Outside = Scene.SpawnPickup<AXpOrbPickup>(FVector(500.0, 0.0, 0.0));
    Scene.WorldWrapper.TickTestWorld(0.02f);

    TestTrue(TEXT("The orb inside the radius locks on"), Inside->IsAttracted());
    TestTrue(TEXT("It locked onto the collector"), Inside->GetCollector() == Scene.Collector);
    TestFalse(TEXT("The orb outside the radius waits"), Outside->IsAttracted());

    const FVector WaitingSpot = Outside->GetActorLocation();
    Scene.Step(0.5f);
    TestTrue(TEXT("The waiting orb has not moved"), Outside->GetActorLocation().Equals(WaitingSpot, 0.01));

    Scene.Pickups->PickupRadius = 600.0f;
    Scene.WorldWrapper.TickTestWorld(0.02f);
    TestTrue(TEXT("A larger radius picks up the next orb"), Outside->IsAttracted());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPickupComponentKeepsPursuingTest,
    "GrantMoney.Pickups.PickupComponent.PursuitOutlivesTheRadius",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPickupComponentKeepsPursuingTest::RunTest(const FString& Parameters)
{
    PickupTest::FScene Scene;
    if (!Scene.Create(*this, 300.0f))
    {
        return false;
    }

    AXpOrbPickup* Orb = Scene.SpawnPickup<AXpOrbPickup>(FVector(250.0, 0.0, 0.0));
    Scene.WorldWrapper.TickTestWorld(0.02f);
    TestTrue(TEXT("Locked on"), Orb->IsAttracted());

    // The collector jumps far away in one frame: the orb is now well outside the radius.
    Scene.Collector->SetActorLocation(FVector(0.0, 3000.0, 0.0));
    Scene.WorldWrapper.TickTestWorld(0.02f);
    TestTrue(TEXT("Still chasing outside the radius"), Orb->IsAttracted());

    Scene.Step(4.0f);
    TestEqual(TEXT("It caught up and paid out"), Scene.Experience(), Orb->GetXpValue());
    TestEqual(TEXT("And is gone"), Scene.CountPickups<AXpOrbPickup>(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPickupComponentAnyPickupTypeTest,
    "GrantMoney.Pickups.PickupComponent.SupportsAnyPickupType",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPickupComponentAnyPickupTypeTest::RunTest(const FString& Parameters)
{
    PickupTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    // A second pickup type that applies a temporary-style stat buff through the shared hook.
    // Nothing about UPickupComponent was changed for it.
    Scene.SpawnPickup<AXpOrbPickup>(FVector(150.0, 0.0, 0.0));
    Scene.SpawnPickup<ATestBuffPickup>(FVector(0.0, 150.0, 0.0));
    Scene.Step(2.0f);

    TestEqual(TEXT("The XP orb paid out"), Scene.Experience(), 5.0f);
    TestEqual(TEXT("The buff pickup applied its effect"), Scene.Stats->GetStat(EGrantMoneyStat::Damage), 5.0f);
    TestEqual(TEXT("Both are gone"), Scene.CountPickups(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPickupComponentIgnoresOthersTest,
    "GrantMoney.Pickups.PickupComponent.IgnoresNonPickups",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPickupComponentIgnoresOthersTest::RunTest(const FString& Parameters)
{
    PickupTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    // A slime is next to the collector. It is not a pickup and must be left alone.
    ASlimeEnemy* Slime = Scene.SpawnSlime(FVector(100.0, 0.0, 0.0));
    TestEqual(TEXT("A scan with nothing to pick up starts nothing"), Scene.Pickups->AttractPickupsInRadius(), 0);
    Scene.Step(0.2f);
    TestTrue(TEXT("The slime is untouched"), IsValid(Slime) && !Slime->IsActorBeingDestroyed());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPickupComponentPlayerHasItTest,
    "GrantMoney.Pickups.PickupComponent.PlayerDefaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPickupComponentPlayerHasItTest::RunTest(const FString& Parameters)
{
    const AGrantMoneyPlayerCharacter* Player = GetDefault<AGrantMoneyPlayerCharacter>();
    if (!TestNotNull(TEXT("The player has a pickup component"), Player->GetPickups()))
    {
        return false;
    }

    TestEqual(TEXT("Default pickup radius"), Player->GetPickups()->PickupRadius, 300.0f);
    TestEqual(TEXT("XP starts at zero"), Player->GetExperience(), 0.0f);
    TestTrue(TEXT("XP is a stat on the player's Stats component"), Player->GetStats()->HasStat(EGrantMoneyStat::Experience));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
