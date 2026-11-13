#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "BossSlimeEnemy.h"
#include "SlimeEnemy.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSlimeSquishPoseTest,
    "GrantMoney.Run.Slime.SquishPose",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSlimeSquishPoseTest::RunTest(const FString& Parameters)
{
    const FVector Base(0.6);
    const FVector BaseLocation(0.0, 0.0, -30.0);
    const float Bottom = -50.0f;
    const float RestBottomZ = static_cast<float>(BaseLocation.Z + Base.Z * Bottom);

    FVector Scale;
    FVector Location;

    // Zero amplitude and a zero swing both leave the base pose alone.
    ASlimeEnemy::ComputeSquishPose(Base, BaseLocation, Bottom, UE_HALF_PI, 0.1f, 0.0f, Scale, Location);
    TestTrue(TEXT("No amplitude keeps the scale"), Scale.Equals(Base, 1.e-4));
    TestTrue(TEXT("No amplitude keeps the location"), Location.Equals(BaseLocation, 1.e-3));

    ASlimeEnemy::ComputeSquishPose(Base, BaseLocation, Bottom, 0.0f, 0.1f, 1.0f, Scale, Location);
    TestTrue(TEXT("Phase 0 keeps the scale"), Scale.Equals(Base, 1.e-4));

    // Top of the swing: 10% taller and thinner, bottom unmoved.
    ASlimeEnemy::ComputeSquishPose(Base, BaseLocation, Bottom, UE_HALF_PI, 0.1f, 1.0f, Scale, Location);
    TestEqual(TEXT("Taller at the top of the swing"), static_cast<float>(Scale.Z), 0.6f * 1.1f, 1.e-4f);
    TestEqual(TEXT("Thinner at the top of the swing"), static_cast<float>(Scale.X), 0.6f * 0.95f, 1.e-4f);
    TestEqual(TEXT("Thinner on both horizontal axes"), static_cast<float>(Scale.Y), static_cast<float>(Scale.X), 1.e-6f);
    TestEqual(TEXT("Bottom stays put when taller"), static_cast<float>(Location.Z + Scale.Z * Bottom), RestBottomZ, 1.e-3f);

    // Bottom of the swing: flatter and wider, bottom still unmoved.
    ASlimeEnemy::ComputeSquishPose(Base, BaseLocation, Bottom, 3.0f * UE_HALF_PI, 0.1f, 1.0f, Scale, Location);
    TestEqual(TEXT("Flatter at the bottom of the swing"), static_cast<float>(Scale.Z), 0.6f * 0.9f, 1.e-4f);
    TestEqual(TEXT("Wider at the bottom of the swing"), static_cast<float>(Scale.X), 0.6f * 1.05f, 1.e-4f);
    TestEqual(TEXT("Bottom stays put when flatter"), static_cast<float>(Location.Z + Scale.Z * Bottom), RestBottomZ, 1.e-3f);

    // Half amplitude gives half the swing.
    ASlimeEnemy::ComputeSquishPose(Base, BaseLocation, Bottom, UE_HALF_PI, 0.1f, 0.5f, Scale, Location);
    TestEqual(TEXT("Half amplitude halves the swing"), static_cast<float>(Scale.Z), 0.6f * 1.05f, 1.e-4f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSlimeSquishDefaultsTest,
    "GrantMoney.Run.Slime.SquishDefaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSlimeSquishDefaultsTest::RunTest(const FString& Parameters)
{
    const ASlimeEnemy* Slime = GetDefault<ASlimeEnemy>();
    TestTrue(TEXT("Slimes squish while moving by default"), Slime->bSquishWhileMoving);
    TestTrue(TEXT("The swing is small"), Slime->SquishAmount > 0.0f && Slime->SquishAmount <= 0.2f);

    // The boss squishes like a slime when walking; its lunge shape comes from GetSquishOverride.
    TestTrue(TEXT("The boss squishes too"), GetDefault<ABossSlimeEnemy>()->bSquishWhileMoving);
    return true;
}

#endif
