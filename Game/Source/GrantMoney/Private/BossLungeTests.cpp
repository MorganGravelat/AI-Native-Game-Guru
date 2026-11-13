#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "BossLunge.h"
#include "Tests/BossTestTypes.h"

namespace
{
	FBossLungeSettings TestSettings()
	{
		FBossLungeSettings Settings;
		Settings.WindupSeconds = 1.0f;
		Settings.Speed = 1000.0f;
		Settings.DurationSeconds = 0.5f;
		Settings.CooldownSeconds = 2.0f;
		Settings.TriggerRange = 700.0f;

		// Single dash, so the phase tests below read plainly. The combo has its own test.
		Settings.ComboCount = 1;
		Settings.ComboWindupSeconds = 0.5f;
		return Settings;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBossLungeComboTest,
    "GrantMoney.Boss.Lunge.ComboDashesBackToBack",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossLungeComboTest::RunTest(const FString& Parameters)
{
    FBossLungeSettings Settings = TestSettings();
    Settings.ComboCount = 3;
    Settings.ComboWindupSeconds = 0.5f;

    FBossLungeState State;
    TestTrue(TEXT("Dash 1 windup"), State.Advance(Settings, 0.0f, true) == EBossLungeEvent::WindupStarted);
    TestEqual(TEXT("First windup is the full telegraph"), State.GetWindupSeconds(Settings), 1.0f);
    TestTrue(TEXT("Dash 1 starts"), State.Advance(Settings, 1.0f, true) == EBossLungeEvent::LungeStarted);

    // The first dash ends and the second windup begins at once: no cooldown in between.
    TestTrue(TEXT("Dash 1 ends into a combo windup"), State.Advance(Settings, 0.5f, true) == EBossLungeEvent::ComboWindupStarted);
    TestTrue(TEXT("Winding up again, not cooling down"), State.IsWindingUp());
    TestEqual(TEXT("Second dash index"), State.ComboIndex, 1);
    TestEqual(TEXT("Later windups use the shorter combo telegraph"), State.GetWindupSeconds(Settings), 0.5f);
    TestEqual(TEXT("The combo windup alpha runs over the combo windup"), State.GetWindupAlpha(Settings), 0.0f);
    State.Advance(Settings, 0.25f, false);
    TestEqual(TEXT("Halfway through the combo windup"), State.GetWindupAlpha(Settings), 0.5f);

    TestTrue(TEXT("Dash 2 starts even though the target left range"), State.Advance(Settings, 0.25f, false) == EBossLungeEvent::LungeStarted);
    TestTrue(TEXT("Dash 2 ends into a combo windup"), State.Advance(Settings, 0.5f, false) == EBossLungeEvent::ComboWindupStarted);
    TestEqual(TEXT("Third dash index"), State.ComboIndex, 2);
    TestTrue(TEXT("Dash 3 starts"), State.Advance(Settings, 0.5f, false) == EBossLungeEvent::LungeStarted);

    // The last dash ends the combo and starts the cooldown.
    TestTrue(TEXT("The last dash ends the combo"), State.Advance(Settings, 0.5f, false) == EBossLungeEvent::LungeEnded);
    TestTrue(TEXT("Cooling down"), State.Phase == EBossLungePhase::Cooldown);
    TestEqual(TEXT("The combo counter is reset"), State.ComboIndex, 0);
    TestTrue(TEXT("Cooldown ends"), State.Advance(Settings, 2.0f, true) == EBossLungeEvent::CooldownEnded);

    // A fresh combo starts from the full telegraph again.
    TestTrue(TEXT("Next combo winds up"), State.Advance(Settings, 0.0f, true) == EBossLungeEvent::WindupStarted);
    TestEqual(TEXT("Back to the full telegraph"), State.GetWindupSeconds(Settings), 1.0f);

    // Aborting (no room for the next dash) goes straight to the cooldown.
    State.Advance(Settings, 1.0f, true);
    State.Advance(Settings, 0.5f, true);
    TestEqual(TEXT("Mid-combo"), State.ComboIndex, 1);
    State.AbortCombo();
    TestTrue(TEXT("Aborted combo cools down"), State.Phase == EBossLungePhase::Cooldown);
    TestEqual(TEXT("Aborted combo counter reset"), State.ComboIndex, 0);

    // ComboCount 1 (or less) is a single lunge.
    Settings.ComboCount = 0;
    FBossLungeState Single;
    Single.Advance(Settings, 0.0f, true);
    Single.Advance(Settings, 1.0f, true);
    TestTrue(TEXT("A combo count below 1 still gives one dash"), Single.Advance(Settings, 0.5f, true) == EBossLungeEvent::LungeEnded);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBossLungePhasesTest,
    "GrantMoney.Boss.Lunge.PhasesAndCooldown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossLungePhasesTest::RunTest(const FString& Parameters)
{
    const FBossLungeSettings Settings = TestSettings();
    FBossLungeState State;

    TestTrue(TEXT("Out of range: nothing happens"), State.Advance(Settings, 1.0f, false) == EBossLungeEvent::None);
    TestTrue(TEXT("Still idle"), State.Phase == EBossLungePhase::Idle);

    TestTrue(TEXT("In range: the windup starts"), State.Advance(Settings, 0.0f, true) == EBossLungeEvent::WindupStarted);
    TestTrue(TEXT("Winding up"), State.IsWindingUp());
    TestEqual(TEXT("Windup starts at 0"), State.GetWindupAlpha(Settings), 0.0f);

    TestTrue(TEXT("Mid-windup: no event"), State.Advance(Settings, 0.5f, true) == EBossLungeEvent::None);
    TestEqual(TEXT("Halfway through the windup"), State.GetWindupAlpha(Settings), 0.5f);

    // Leaving range mid-windup does not cancel the telegraphed lunge.
    TestTrue(TEXT("Windup finishes even if the target left range"), State.Advance(Settings, 0.5f, false) == EBossLungeEvent::LungeStarted);
    TestTrue(TEXT("Lunging"), State.IsLunging());
    TestEqual(TEXT("Windup alpha is 0 outside the windup"), State.GetWindupAlpha(Settings), 0.0f);
    TestEqual(TEXT("Uncapped charge lasts the configured duration"), State.ActiveLungeDuration, 0.5f);

    TestTrue(TEXT("Mid-lunge: no event"), State.Advance(Settings, 0.25f, true) == EBossLungeEvent::None);
    TestTrue(TEXT("Lunge ends after its duration"), State.Advance(Settings, 0.25f, true) == EBossLungeEvent::LungeEnded);
    TestTrue(TEXT("Cooling down"), State.Phase == EBossLungePhase::Cooldown);

    // The cooldown blocks a new windup however close the target is.
    TestTrue(TEXT("No windup during the cooldown"), State.Advance(Settings, 1.0f, true) == EBossLungeEvent::None);
    TestTrue(TEXT("Still cooling down"), State.Phase == EBossLungePhase::Cooldown);
    TestTrue(TEXT("Cooldown ends"), State.Advance(Settings, 1.0f, true) == EBossLungeEvent::CooldownEnded);
    TestTrue(TEXT("Ready again"), State.Phase == EBossLungePhase::Idle);
    TestTrue(TEXT("The next windup can start"), State.Advance(Settings, 0.0f, true) == EBossLungeEvent::WindupStarted);

    State.Reset();
    TestTrue(TEXT("Reset returns to idle"), State.Phase == EBossLungePhase::Idle);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBossLungeEditableTimingTest,
    "GrantMoney.Boss.Lunge.TimingIsEditable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossLungeEditableTimingTest::RunTest(const FString& Parameters)
{
    FBossLungeSettings Settings = TestSettings();
    Settings.WindupSeconds = 0.25f;
    Settings.DurationSeconds = 0.1f;
    Settings.CooldownSeconds = 0.5f;

    FBossLungeState State;
    State.Advance(Settings, 0.0f, true);
    TestTrue(TEXT("A short windup ends sooner"), State.Advance(Settings, 0.25f, true) == EBossLungeEvent::LungeStarted);
    TestTrue(TEXT("A short lunge ends sooner"), State.Advance(Settings, 0.1f, true) == EBossLungeEvent::LungeEnded);
    TestTrue(TEXT("A short cooldown ends sooner"), State.Advance(Settings, 0.5f, true) == EBossLungeEvent::CooldownEnded);

    Settings.WindupSeconds = 0.0f;
    State.Advance(Settings, 0.0f, true);
    TestTrue(TEXT("Zero windup charges on the next step"), State.Advance(Settings, 0.0f, true) == EBossLungeEvent::LungeStarted);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBossLungeDurationLimitTest,
    "GrantMoney.Boss.Lunge.DurationLimitShortensCharge",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossLungeDurationLimitTest::RunTest(const FString& Parameters)
{
    const FBossLungeSettings Settings = TestSettings();

    FBossLungeState State;
    State.Advance(Settings, 0.0f, true);
    State.DurationLimit = 0.2f;
    State.Advance(Settings, 1.0f, true);
    TestEqual(TEXT("A map edge shortens the charge"), State.ActiveLungeDuration, 0.2f);
    TestTrue(TEXT("The shortened charge ends on time"), State.Advance(Settings, 0.2f, true) == EBossLungeEvent::LungeEnded);
    TestTrue(TEXT("The limit is cleared for the next lunge"), State.DurationLimit < 0.0f);

    // A limit longer than the configured duration changes nothing.
    FBossLungeState Roomy;
    Roomy.Advance(Settings, 0.0f, true);
    Roomy.DurationLimit = 5.0f;
    Roomy.Advance(Settings, 1.0f, true);
    TestEqual(TEXT("The configured duration is still the maximum"), Roomy.ActiveLungeDuration, 0.5f);

    TestEqual(TEXT("Safe duration is distance over speed"), FBossLungeMath::ComputeSafeDuration(250.0f, 1000.0f, 0.5f), 0.25f);
    TestEqual(TEXT("Safe duration never exceeds the maximum"), FBossLungeMath::ComputeSafeDuration(900.0f, 1000.0f, 0.5f), 0.5f);
    TestEqual(TEXT("No distance, no charge"), FBossLungeMath::ComputeSafeDuration(0.0f, 1000.0f, 0.5f), 0.0f);
    TestEqual(TEXT("No speed, no charge"), FBossLungeMath::ComputeSafeDuration(100.0f, 0.0f, 0.5f), 0.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBossLungeSafeDistanceTest,
    "GrantMoney.Boss.Lunge.StaysInsidePlayableArea",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossLungeSafeDistanceTest::RunTest(const FString& Parameters)
{
    BossTest::FTestArena Map;
    Map.Init(FIntPoint(0, 0), FIntPoint(19, 19));
    const FSpawnArena& Arena = Map.Arena;

    const FVector Start(1000.0, 1000.0, 0.0);
    const float Radius = 100.0f;

    TestEqual(TEXT("Open ground: the whole desired distance is safe"),
        FBossLungeMath::ComputeSafeDistance(Arena, Start, FVector::ForwardVector, 500.0f, Radius), 500.0f);

    // Toward the east edge (playable up to X = 1950): a long charge is cut short.
    const FVector NearEdge(1400.0, 1000.0, 0.0);
    const float Safe = FBossLungeMath::ComputeSafeDistance(Arena, NearEdge, FVector::ForwardVector, 1000.0f, Radius);
    TestTrue(TEXT("The charge toward an edge is shortened"), Safe > 0.0f && Safe < 1000.0f);
    TestTrue(TEXT("Its end point is still on playable ground"), Arena.IsFootprintPlayable(NearEdge + FVector::ForwardVector * Safe, Radius));
    TestFalse(TEXT("One more step would leave the playable area"), Arena.IsFootprintPlayable(NearEdge + FVector::ForwardVector * (Safe + 25.0f), Radius));

    // The direction is flattened and normalised, so a diagonal, tilted input behaves the same way.
    const FVector Diagonal(1.0, 1.0, 5.0);
    const float DiagonalSafe = FBossLungeMath::ComputeSafeDistance(Arena, FVector(1800.0, 1800.0, 0.0), Diagonal, 1000.0f, Radius);
    TestTrue(TEXT("A diagonal charge toward a corner is shortened"), DiagonalSafe < 1000.0f);

    TestEqual(TEXT("Starting at the very edge there is no room"),
        FBossLungeMath::ComputeSafeDistance(Arena, FVector(1900.0, 1000.0, 0.0), FVector::ForwardVector, 500.0f, Radius), 0.0f);

    TestEqual(TEXT("A zero direction cannot lunge"), FBossLungeMath::ComputeSafeDistance(Arena, Start, FVector::ZeroVector, 500.0f, Radius), 0.0f);
    TestEqual(TEXT("A zero distance cannot lunge"), FBossLungeMath::ComputeSafeDistance(Arena, Start, FVector::ForwardVector, 0.0f, Radius), 0.0f);

    FSpawnArena Invalid;
    TestEqual(TEXT("With no map there is no safe distance"), FBossLungeMath::ComputeSafeDistance(Invalid, Start, FVector::ForwardVector, 500.0f, Radius), 0.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBossLungeInterceptTest,
    "GrantMoney.Boss.Lunge.InterceptLeadsAMovingTarget",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossLungeInterceptTest::RunTest(const FString& Parameters)
{
    // A stationary target: the lead direction is just the straight line to it (the same as no lead at all).
    {
        FVector Direction;
        TestTrue(TEXT("A stationary target still has a direction"),
            FBossLungeMath::ComputeInterceptDirection(FVector(1000.0, 0.0, 0.0), FVector::ZeroVector, 1000.0f, 1.0f, Direction));
        TestTrue(TEXT("It points straight at the target"), Direction.Equals(FVector::ForwardVector, 0.001f));
    }

    // A target crossing at a right angle: the dash should lead ahead of its current position, in the
    // direction it is moving (worked out above: ~30 degrees off straight, toward +Y).
    {
        FVector Direction;
        TestTrue(TEXT("A valid lead exists"),
            FBossLungeMath::ComputeInterceptDirection(FVector(1000.0, 0.0, 0.0), FVector(0.0, 500.0, 0.0), 1000.0f, 0.0f, Direction));
        TestTrue(TEXT("It leads toward where the target is going (+Y)"), Direction.Y > 0.3f && Direction.Y < 0.7f);
        TestTrue(TEXT("Still a unit vector"), FMath::IsNearlyEqual(Direction.Size(), 1.0f, 0.01f));
    }

    // A longer delay before the dash launches (a longer windup) means the target has moved further by then,
    // so the lead angle should be larger, not the same.
    {
        FVector NoDelay;
        FVector WithDelay;
        FBossLungeMath::ComputeInterceptDirection(FVector(1000.0, 0.0, 0.0), FVector(0.0, 500.0, 0.0), 1000.0f, 0.0f, NoDelay);
        FBossLungeMath::ComputeInterceptDirection(FVector(1000.0, 0.0, 0.0), FVector(0.0, 500.0, 0.0), 1000.0f, 1.0f, WithDelay);
        TestTrue(TEXT("A longer windup leads further ahead"), WithDelay.Y > NoDelay.Y);
    }

    // A target directly ahead, moving directly away faster than the dash speed: it can never be met, so the
    // function falls back to the straight-line direction instead of inventing an impossible lead.
    {
        FVector Direction;
        TestFalse(TEXT("Too fast to intercept"),
            FBossLungeMath::ComputeInterceptDirection(FVector(1000.0, 0.0, 0.0), FVector(2000.0, 0.0, 0.0), 1000.0f, 0.0f, Direction));
        TestTrue(TEXT("Falls back to the straight line"), Direction.Equals(FVector::ForwardVector, 0.001f));
    }

    // No dash speed: nothing can be computed.
    {
        FVector Direction;
        TestFalse(TEXT("Zero speed cannot intercept"),
            FBossLungeMath::ComputeInterceptDirection(FVector(1000.0, 0.0, 0.0), FVector(0.0, 500.0, 0.0), 0.0f, 0.0f, Direction));
    }

    // No separation at all: there is no direction to report either way.
    {
        FVector Direction;
        TestFalse(TEXT("Coincident positions have no direction"),
            FBossLungeMath::ComputeInterceptDirection(FVector::ZeroVector, FVector(0.0, 500.0, 0.0), 1000.0f, 0.0f, Direction));
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
