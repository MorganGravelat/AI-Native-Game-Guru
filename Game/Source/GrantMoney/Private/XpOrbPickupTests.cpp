#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GrantMoneyPlayerCharacter.h"
#include "Tests/PickupTestScene.h"

// The XP orb: adds its XP to the collector's running total once on arrival, and its tuning defaults.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FXpOrbAddsXpOnArrivalTest,
    "GrantMoney.Pickups.XpOrb.AddsXpOnArrivalOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FXpOrbAddsXpOnArrivalTest::RunTest(const FString& Parameters)
{
    PickupTest::FScene Scene;
    if (!Scene.Create(*this, 300.0f))
    {
        return false;
    }

    AXpOrbPickup* Orb = Scene.SpawnPickup<AXpOrbPickup>(FVector(280.0, 0.0, 0.0));
    Orb->XpValue = 7.0f;

    // In flight: locked on but not there yet.
    Scene.Step(0.1f);
    TestTrue(TEXT("The orb is on its way"), Orb->IsAttracted());
    TestEqual(TEXT("No XP before arrival"), Scene.Experience(), 0.0f);

    Scene.Step(1.0f);
    TestEqual(TEXT("XP is added on arrival, by the orb's own value"), Scene.Experience(), 7.0f);
    TestEqual(TEXT("The orb is removed"), Scene.CountPickups<AXpOrbPickup>(), 0);

    Scene.Step(1.0f);
    TestEqual(TEXT("XP does not change afterwards"), Scene.Experience(), 7.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FXpOrbSeveralAddUpTest,
    "GrantMoney.Pickups.XpOrb.SeveralOrbsAddUp",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FXpOrbSeveralAddUpTest::RunTest(const FString& Parameters)
{
    PickupTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    const float Values[] = { 5.0f, 10.0f, 2.5f, 5.0f };
    float Expected = 0.0f;
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Values); ++Index)
    {
        AXpOrbPickup* Orb = Scene.SpawnPickup<AXpOrbPickup>(FVector(100.0 + 40.0 * Index, 0.0, 0.0));
        Orb->XpValue = Values[Index];
        Expected += Values[Index];
    }

    Scene.Step(2.0f);
    TestEqual(TEXT("Every orb counted exactly once"), Scene.Experience(), Expected);
    TestEqual(TEXT("All orbs are gone"), Scene.CountPickups<AXpOrbPickup>(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FXpOrbNeverReachedTest,
    "GrantMoney.Pickups.XpOrb.OutOfRangeOrbIsNeverCollected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FXpOrbNeverReachedTest::RunTest(const FString& Parameters)
{
    PickupTest::FScene Scene;
    if (!Scene.Create(*this, 300.0f))
    {
        return false;
    }

    Scene.SpawnPickup<AXpOrbPickup>(FVector(900.0, 0.0, 0.0));
    Scene.Step(3.0f);
    TestEqual(TEXT("No XP"), Scene.Experience(), 0.0f);
    TestEqual(TEXT("The orb waits"), Scene.CountPickups<AXpOrbPickup>(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FXpOrbDefaultsTest,
    "GrantMoney.Pickups.XpOrb.TuningDefaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FXpOrbDefaultsTest::RunTest(const FString& Parameters)
{
    const AXpOrbPickup* Orb = GetDefault<AXpOrbPickup>();
    TestEqual(TEXT("Base speed"), Orb->BaseSpeed, 400.0f);
    TestEqual(TEXT("Acceleration"), Orb->Acceleration, 1500.0f);
    TestEqual(TEXT("Max speed"), Orb->MaxSpeed, 1200.0f);
    TestEqual(TEXT("Collection distance"), Orb->CollectionDistance, 40.0f);
    TestEqual(TEXT("XP value"), Orb->GetXpValue(), 5.0f);

    const float PlayerTopSpeed = GetDefault<AGrantMoneyPlayerCharacter>()->GetCharacterMovement()->MaxWalkSpeed;
    TestTrue(TEXT("The orb's max speed beats the player's top movement speed"), Orb->MaxSpeed > PlayerTopSpeed);
    TestTrue(TEXT("Base speed does not exceed max speed"), Orb->BaseSpeed <= Orb->MaxSpeed);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
