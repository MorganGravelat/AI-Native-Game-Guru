#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/PickupTestScene.h"

// Shared pickup behaviour (APickupBase): attraction, pursuit, acceleration, and collecting once.
// ATestBuffPickup stands in for a future power-up and counts every call to its collection hook.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPickupBaseCollectsOnceTest,
    "GrantMoney.Pickups.PickupBase.CollectsOnce",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPickupBaseCollectsOnceTest::RunTest(const FString& Parameters)
{
    PickupTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    ATestBuffPickup* Pickup = Scene.SpawnPickup<ATestBuffPickup>(FVector(1000.0, 0.0, 0.0));
    if (!TestNotNull(TEXT("Pickup spawned"), Pickup))
    {
        return false;
    }

    // Several overlap callbacks in one frame all try to collect.
    TestTrue(TEXT("The first attempt collects"), Pickup->TryCollect(Scene.Collector));
    TestFalse(TEXT("A second attempt is refused"), Pickup->TryCollect(Scene.Collector));
    TestFalse(TEXT("A third attempt is refused"), Pickup->TryCollect(Scene.Collector));
    TestEqual(TEXT("The effect hook ran once"), ATestBuffPickup::TotalCollections, 1);
    TestTrue(TEXT("The collector was passed to the hook"), ATestBuffPickup::LastCollector.Get() == Scene.Collector);
    TestTrue(TEXT("The pickup is removed after its effect"), Pickup->IsActorBeingDestroyed());
    TestEqual(TEXT("The hook's effect was applied once (+5 damage)"), Scene.Stats->GetStat(EGrantMoneyStat::Damage), 5.0f);
    TestFalse(TEXT("A pickup being removed cannot start chasing"), Pickup->BeginAttraction(Scene.Collector));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPickupBaseWaitsUntilAttractedTest,
    "GrantMoney.Pickups.PickupBase.WaitsUntilAttracted",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPickupBaseWaitsUntilAttractedTest::RunTest(const FString& Parameters)
{
    PickupTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    const FVector Start(1000.0, 0.0, 0.0);
    ATestBuffPickup* Pickup = Scene.SpawnPickup<ATestBuffPickup>(Start);
    Scene.Step(1.0f);
    TestFalse(TEXT("Not chasing without a collector"), Pickup->IsAttracted());
    TestTrue(TEXT("It has not moved"), Pickup->GetActorLocation().Equals(Start, 0.01));

    TestTrue(TEXT("Attraction starts"), Pickup->BeginAttraction(Scene.Collector));
    TestFalse(TEXT("A repeated request is ignored"), Pickup->BeginAttraction(Scene.Collector));
    TestEqual(TEXT("It starts at the base speed"), Pickup->GetCurrentSpeed(), Pickup->BaseSpeed);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPickupBaseAcceleratesTest,
    "GrantMoney.Pickups.PickupBase.AcceleratesToMaxSpeed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPickupBaseAcceleratesTest::RunTest(const FString& Parameters)
{
    PickupTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    // Far enough that it is still travelling after 3 seconds.
    ATestBuffPickup* Pickup = Scene.SpawnPickup<ATestBuffPickup>(FVector(100000.0, 0.0, 0.0));
    Pickup->BaseSpeed = 400.0f;
    Pickup->Acceleration = 1000.0f;
    Pickup->MaxSpeed = 1000.0f;
    Pickup->BeginAttraction(Scene.Collector);

    Scene.Step(0.2f);
    TestEqual(TEXT("Speed grows by acceleration x time"), Pickup->GetCurrentSpeed(), 600.0f, 1.0f);

    Scene.Step(3.0f);
    TestEqual(TEXT("Speed never exceeds the maximum"), Pickup->GetCurrentSpeed(), 1000.0f, 0.01f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPickupBaseFrameRateIndependentTest,
    "GrantMoney.Pickups.PickupBase.FrameRateIndependent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPickupBaseFrameRateIndependentTest::RunTest(const FString& Parameters)
{
    // The same two seconds of flight, simulated at 30, 60 and 240 fps, covers the same distance.
    TArray<double> Travelled;
    for (const float StepSeconds : { 1.0f / 30.0f, 1.0f / 60.0f, 1.0f / 240.0f })
    {
        PickupTest::FScene Scene;
        if (!Scene.Create(*this))
        {
            return false;
        }

        const FVector Start(100000.0, 0.0, 0.0);
        ATestBuffPickup* Pickup = Scene.SpawnPickup<ATestBuffPickup>(Start);
        Pickup->BeginAttraction(Scene.Collector);
        Scene.Step(2.0f, StepSeconds);
        Travelled.Add(FVector::Dist(Start, Pickup->GetActorLocation()));
    }

    TestEqual(TEXT("30 fps and 60 fps agree"), Travelled[0], Travelled[1], 5.0);
    TestEqual(TEXT("60 fps and 240 fps agree"), Travelled[1], Travelled[2], 5.0);

    // 400 -> 1200 cm/s over 0.533 s, then 1200 cm/s: 0.5*(400+1200)*0.5333 + 1200*1.4667 = 2186.7 cm.
    TestEqual(TEXT("Distance matches the acceleration curve"), Travelled[2], 2186.7, 10.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPickupBaseNoOvershootTest,
    "GrantMoney.Pickups.PickupBase.CollectsInsteadOfOvershooting",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPickupBaseNoOvershootTest::RunTest(const FString& Parameters)
{
    PickupTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    // A one-second frame at 1200 cm/s would carry the pickup 20 times past the collector.
    ATestBuffPickup* Pickup = Scene.SpawnPickup<ATestBuffPickup>(FVector(100.0, 0.0, 0.0));
    Pickup->BeginAttraction(Scene.Collector);
    Scene.WorldWrapper.TickTestWorld(1.0f);

    TestEqual(TEXT("Collected in that frame"), ATestBuffPickup::TotalCollections, 1);
    TestEqual(TEXT("Nothing is left to collect twice"), Scene.CountPickups<ATestBuffPickup>(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPickupBaseFollowsMovingCollectorTest,
    "GrantMoney.Pickups.PickupBase.CatchesMovingCollector",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPickupBaseFollowsMovingCollectorTest::RunTest(const FString& Parameters)
{
    PickupTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    ATestBuffPickup* Pickup = Scene.SpawnPickup<ATestBuffPickup>(FVector(200.0, 0.0, 0.0));
    Pickup->BeginAttraction(Scene.Collector);

    // The collector runs away along +Y at 900 cm/s (faster than a character's 600 walk speed),
    // then turns back along -X. The pickup has to follow both legs and arrive.
    constexpr float Dt = 1.0f / 60.0f;
    for (int32 Frame = 0; Frame < 600 && !Pickup->IsActorBeingDestroyed(); ++Frame)
    {
        const FVector Velocity = Frame < 120 ? FVector(0.0, 900.0, 0.0) : FVector(-900.0, 0.0, 0.0);
        Scene.Collector->SetActorLocation(Scene.Collector->GetActorLocation() + Velocity * Dt);
        Scene.WorldWrapper.TickTestWorld(Dt);
    }

    TestEqual(TEXT("Collected after catching up"), ATestBuffPickup::TotalCollections, 1);
    TestTrue(TEXT("The collector was far outside the original 300 cm radius when it was caught"),
        Scene.Collector->GetActorLocation().Size() > 300.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPickupBaseCollectorGoneTest,
    "GrantMoney.Pickups.PickupBase.CollectorDestroyed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPickupBaseCollectorGoneTest::RunTest(const FString& Parameters)
{
    PickupTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    ATestBuffPickup* Pickup = Scene.SpawnPickup<ATestBuffPickup>(FVector(1000.0, 0.0, 0.0));
    Pickup->BeginAttraction(Scene.Collector);
    Scene.Collector->Destroy();
    Scene.Step(0.2f);

    TestFalse(TEXT("The pickup stops chasing a destroyed collector"), Pickup->IsAttracted());
    TestEqual(TEXT("It was not collected"), ATestBuffPickup::TotalCollections, 0);
    TestEqual(TEXT("It stays in the world"), Scene.CountPickups<ATestBuffPickup>(), 1);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
