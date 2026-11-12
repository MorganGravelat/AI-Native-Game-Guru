#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "LevelComponent.h"
#include "StatsComponent.h"
#include "Tests/AutomationCommon.h"
#include "Tests/ProgressionTestListener.h"

namespace LevelTest
{
    // An empty game world with play started, an actor with a UStatsComponent and a ULevelComponent on
    // top of it. Thresholds are small, fixed numbers (not the real game's) so the maths in each test is
    // easy to check by hand. Destroyed when this goes out of scope.
    struct FScene
    {
        FTestWorldWrapper WorldWrapper;
        AActor* Actor = nullptr;
        UStatsComponent* Stats = nullptr;
        ULevelComponent* Level = nullptr;
        UProgressionTestListener* Listener = nullptr;

        bool Create(FAutomationTestBase& Test, const TArray<float>& Thresholds = { 10.0f, 20.0f, 30.0f })
        {
            if (!WorldWrapper.CreateTestWorld(EWorldType::Game) || !WorldWrapper.BeginPlayInTestWorld())
            {
                WorldWrapper.ForwardErrorMessages(&Test);
                return false;
            }

            Actor = World()->SpawnActor<AActor>();
            if (!Test.TestNotNull(TEXT("Actor spawned"), Actor))
            {
                return false;
            }

            // Stats is registered first so the level component finds it in BeginPlay.
            Stats = NewObject<UStatsComponent>(Actor);
            Stats->SetBaseStat(EGrantMoneyStat::Experience, 0.0f);
            Stats->RegisterComponent();

            Level = NewObject<ULevelComponent>(Actor);
            Level->LevelThresholds = Thresholds;
            Level->RegisterComponent();

            Listener = NewObject<UProgressionTestListener>();
            Level->OnLevelUp.AddDynamic(Listener, &UProgressionTestListener::OnLevelUp);
            return true;
        }

        UWorld* World() const { return WorldWrapper.GetTestWorld(); }

        void GainXp(float Amount)
        {
            Stats->SetBaseStat(EGrantMoneyStat::Experience, Stats->GetBaseStat(EGrantMoneyStat::Experience) + Amount);
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLevelComponentDefaultsTest,
    "GrantMoney.Progression.LevelComponent.Defaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLevelComponentDefaultsTest::RunTest(const FString& Parameters)
{
    const ULevelComponent* Defaults = GetDefault<ULevelComponent>();
    TestTrue(TEXT("Has at least one threshold"), !Defaults->LevelThresholds.IsEmpty());

    LevelTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    TestEqual(TEXT("Starts at level 1"), Scene.Level->GetLevel(), 1);
    TestEqual(TEXT("No XP into the level yet"), Scene.Level->GetXpIntoLevel(), 0.0f);
    TestEqual(TEXT("10 XP needed for level 2"), Scene.Level->GetXpToNextLevel(), 10.0f);
    TestEqual(TEXT("No progress yet"), Scene.Level->GetProgressFraction(), 0.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLevelComponentGainsLevelTest,
    "GrantMoney.Progression.LevelComponent.GainsLevel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLevelComponentGainsLevelTest::RunTest(const FString& Parameters)
{
    LevelTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    Scene.GainXp(6.0f);
    TestEqual(TEXT("Not enough for level 2 yet"), Scene.Level->GetLevel(), 1);
    TestEqual(TEXT("Progress recorded"), Scene.Level->GetXpIntoLevel(), 6.0f);
    TestEqual(TEXT("No level-up fired"), Scene.Listener->LevelUps.Num(), 0);

    Scene.GainXp(4.0f); // total 10: exactly the threshold
    TestEqual(TEXT("Reaches level 2 exactly on the threshold"), Scene.Level->GetLevel(), 2);
    TestEqual(TEXT("Excess XP is preserved: none left over here"), Scene.Level->GetXpIntoLevel(), 0.0f);
    TestEqual(TEXT("One level-up fired"), Scene.Listener->LevelUps.Num(), 1);

    Scene.GainXp(15.0f); // level 2 needs 20; 15 of it is progress, not enough yet
    TestEqual(TEXT("Still level 2"), Scene.Level->GetLevel(), 2);
    TestEqual(TEXT("15 XP into level 2"), Scene.Level->GetXpIntoLevel(), 15.0f);
    TestEqual(TEXT("20 XP needed for level 3"), Scene.Level->GetXpToNextLevel(), 20.0f);
    TestEqual(TEXT("Progress fraction"), Scene.Level->GetProgressFraction(), 0.75f);
    TestEqual(TEXT("Still one level-up"), Scene.Listener->LevelUps.Num(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLevelComponentExcessXpPreservedTest,
    "GrantMoney.Progression.LevelComponent.ExcessXpPreserved",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLevelComponentExcessXpPreservedTest::RunTest(const FString& Parameters)
{
    LevelTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    // One pickup worth more than the threshold: the excess must count toward the next level, not vanish.
    Scene.GainXp(13.0f); // threshold is 10
    TestEqual(TEXT("Reaches level 2"), Scene.Level->GetLevel(), 2);
    TestEqual(TEXT("3 XP carried over, not discarded"), Scene.Level->GetXpIntoLevel(), 3.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLevelComponentMultipleThresholdsTest,
    "GrantMoney.Progression.LevelComponent.MultipleThresholdsInOneGain",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLevelComponentMultipleThresholdsTest::RunTest(const FString& Parameters)
{
    LevelTest::FScene Scene;
    if (!Scene.Create(*this, { 10.0f, 20.0f, 30.0f }))
    {
        return false;
    }

    // One huge pickup: 10 (-> level 2) + 20 (-> level 3) + 5 leftover. Crosses two thresholds at once.
    Scene.GainXp(35.0f);
    TestEqual(TEXT("Reaches level 3"), Scene.Level->GetLevel(), 3);
    TestEqual(TEXT("5 XP carried over into level 3"), Scene.Level->GetXpIntoLevel(), 5.0f);
    TestEqual(TEXT("Fired twice, once per level gained"), Scene.Listener->LevelUps.Num(), 2);
    TestTrue(TEXT("First: 1 -> 2"), Scene.Listener->LevelUps.IsValidIndex(0) && Scene.Listener->LevelUps[0] == TPair<int32, int32>(2, 1));
    TestTrue(TEXT("Second: 2 -> 3"), Scene.Listener->LevelUps.IsValidIndex(1) && Scene.Listener->LevelUps[1] == TPair<int32, int32>(3, 2));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLevelComponentThresholdArrayExhaustedTest,
    "GrantMoney.Progression.LevelComponent.ThresholdArrayExhausted",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLevelComponentThresholdArrayExhaustedTest::RunTest(const FString& Parameters)
{
    // Only one threshold defined: every level past 1 must still work, reusing it.
    LevelTest::FScene Scene;
    if (!Scene.Create(*this, { 10.0f }))
    {
        return false;
    }

    Scene.GainXp(10.0f);
    TestEqual(TEXT("Level 2"), Scene.Level->GetLevel(), 2);
    TestEqual(TEXT("Still needs 10 for level 3 (the one entry repeats)"), Scene.Level->GetXpToNextLevel(), 10.0f);

    Scene.GainXp(25.0f); // 2 more levels (10, 10) plus 5 left over
    TestEqual(TEXT("Level 4"), Scene.Level->GetLevel(), 4);
    TestEqual(TEXT("5 XP carried over"), Scene.Level->GetXpIntoLevel(), 5.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLevelComponentIgnoresOtherStatsTest,
    "GrantMoney.Progression.LevelComponent.IgnoresOtherStatsAndLoss",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLevelComponentIgnoresOtherStatsTest::RunTest(const FString& Parameters)
{
    LevelTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    // A change to an unrelated stat must not be mistaken for XP.
    Scene.Stats->SetBaseStat(EGrantMoneyStat::Damage, 50.0f);
    TestEqual(TEXT("Unaffected by other stats"), Scene.Level->GetLevel(), 1);
    TestEqual(TEXT("No XP recorded from an unrelated stat"), Scene.Level->GetXpIntoLevel(), 0.0f);

    // Experience only ever grows in this project, but a decrease (if one ever happened) must not grant
    // negative progress or a spurious level-up.
    Scene.GainXp(5.0f);
    Scene.Stats->SetBaseStat(EGrantMoneyStat::Experience, 2.0f); // a decrease
    TestEqual(TEXT("A decrease does not remove progress"), Scene.Level->GetXpIntoLevel(), 5.0f);
    TestEqual(TEXT("A decrease does not change the level"), Scene.Level->GetLevel(), 1);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
