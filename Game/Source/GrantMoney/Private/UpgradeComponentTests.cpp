#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "StatsComponent.h"
#include "Tests/AutomationCommon.h"
#include "Tests/ProgressionTestListener.h"
#include "UpgradeComponent.h"

namespace UpgradeTest
{
    // An empty game world with play started, an actor with a UStatsComponent and a UUpgradeComponent on
    // top of it, seeded for repeatable rolls. Destroyed when this goes out of scope.
    struct FScene
    {
        FTestWorldWrapper WorldWrapper;
        AActor* Actor = nullptr;
        UStatsComponent* Stats = nullptr;
        UUpgradeComponent* Upgrades = nullptr;
        UProgressionTestListener* Listener = nullptr;

        int32& ChoiceReadyCount() const { return Listener->ChoiceReadyCount; }
        int32& ChoicesResolvedCount() const { return Listener->ChoicesResolvedCount; }

        bool Create(FAutomationTestBase& Test, int32 Seed = 1234)
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

            Stats = NewObject<UStatsComponent>(Actor);
            Stats->RegisterComponent();

            Upgrades = NewObject<UUpgradeComponent>(Actor);
            Upgrades->RegisterComponent();
            Upgrades->SetRandomSeed(Seed);

            Listener = NewObject<UProgressionTestListener>();
            Upgrades->OnChoiceReady.AddDynamic(Listener, &UProgressionTestListener::OnChoiceReady);
            Upgrades->OnChoicesResolved.AddDynamic(Listener, &UProgressionTestListener::OnChoicesResolved);
            return true;
        }

        UWorld* World() const { return WorldWrapper.GetTestWorld(); }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FUpgradeComponentDefaultsTest,
    "GrantMoney.Progression.UpgradeComponent.Defaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUpgradeComponentDefaultsTest::RunTest(const FString& Parameters)
{
    const UUpgradeComponent* Defaults = GetDefault<UUpgradeComponent>();

    // The five SCRUM-36 starter orb upgrades, each with a unique Id and a sensible stack limit.
    TestEqual(TEXT("Five starter upgrades"), Defaults->AvailableUpgrades.Num(), 5);
    TestEqual(TEXT("Offers up to three per choice"), Defaults->OptionsPerChoice, 3);

    TSet<FName> Ids;
    for (const FGrantMoneyUpgradeDefinition& Def : Defaults->AvailableUpgrades)
    {
        TestFalse(TEXT("Id is set"), Def.Id.IsNone());
        TestFalse(*FString::Printf(TEXT("%s's Id is unique"), *Def.Id.ToString()), Ids.Contains(Def.Id));
        Ids.Add(Def.Id);
        TestTrue(*FString::Printf(TEXT("%s has a positive stack limit"), *Def.Id.ToString()), Def.MaxStacks > 0);
    }

    TestTrue(TEXT("Includes Damage"), Ids.Contains(TEXT("Damage")));
    TestTrue(TEXT("Includes AttackSpeed"), Ids.Contains(TEXT("AttackSpeed")));
    TestTrue(TEXT("Includes Penetration"), Ids.Contains(TEXT("Penetration")));
    TestTrue(TEXT("Includes ProjectileCount"), Ids.Contains(TEXT("ProjectileCount")));
    TestTrue(TEXT("Includes MoveSpeed"), Ids.Contains(TEXT("MoveSpeed")));
    TestFalse(TEXT("ExplosionRadius is implemented but deliberately not offered yet"), Ids.Contains(TEXT("ExplosionRadius")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FUpgradeComponentChooseAppliesModifierTest,
    "GrantMoney.Progression.UpgradeComponent.ChooseAppliesModifier",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUpgradeComponentChooseAppliesModifierTest::RunTest(const FString& Parameters)
{
    UpgradeTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    TestFalse(TEXT("No choice active before any level is gained"), Scene.Upgrades->IsChoiceActive());

    Scene.Upgrades->NotifyLevelGained();
    TestTrue(TEXT("A choice becomes active"), Scene.Upgrades->IsChoiceActive());
    TestEqual(TEXT("One choice-ready event"), Scene.ChoiceReadyCount(), 1);
    TestTrue(TEXT("Up to three distinct options offered"), Scene.Upgrades->GetCurrentOptions().Num() > 0 && Scene.Upgrades->GetCurrentOptions().Num() <= 3);

    const FGrantMoneyUpgradeDefinition Chosen = Scene.Upgrades->GetCurrentOptions()[0];
    const float Before = Scene.Stats->GetStat(Chosen.Stat);

    TestTrue(TEXT("Choosing option 0 succeeds"), Scene.Upgrades->ChooseUpgrade(0));
    TestFalse(TEXT("No longer active (queue was empty)"), Scene.Upgrades->IsChoiceActive());
    TestEqual(TEXT("Resolved once"), Scene.ChoicesResolvedCount(), 1);
    TestEqual(TEXT("One stack recorded"), Scene.Upgrades->GetStackCount(Chosen.Id), 1);

    // Applied through the real UStatsComponent::AddModifier, the same call SCRUM-53 stat boosts use.
    const float After = Scene.Stats->GetStat(Chosen.Stat);
    if (Chosen.Op == EGrantMoneyStatModifierOp::Additive)
    {
        TestEqual(TEXT("Additive magnitude applied"), After - Before, Chosen.Magnitude);
    }
    else
    {
        TestEqual(TEXT("Multiplicative magnitude applied (base was 0)"), After, Before);
    }

    // Out-of-range / no-active-choice calls are refused, not silently accepted.
    TestFalse(TEXT("Cannot choose with no active choice"), Scene.Upgrades->ChooseUpgrade(0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FUpgradeComponentStackLimitTest,
    "GrantMoney.Progression.UpgradeComponent.StackLimitExcludesFromFutureChoices",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUpgradeComponentStackLimitTest::RunTest(const FString& Parameters)
{
    UpgradeTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    // One upgrade, a stack limit of 1, nothing else to offer: easy to drive to its limit deterministically.
    FGrantMoneyUpgradeDefinition OnlyUpgrade;
    OnlyUpgrade.Id = TEXT("TestUpgrade");
    OnlyUpgrade.Stat = EGrantMoneyStat::Damage;
    OnlyUpgrade.Op = EGrantMoneyStatModifierOp::Additive;
    OnlyUpgrade.Magnitude = 5.0f;
    OnlyUpgrade.MaxStacks = 1;
    Scene.Upgrades->AvailableUpgrades = { OnlyUpgrade };

    Scene.Upgrades->NotifyLevelGained();
    TestEqual(TEXT("Offered once"), Scene.Upgrades->GetCurrentOptions().Num(), 1);
    Scene.Upgrades->ChooseUpgrade(0);
    TestEqual(TEXT("At its stack limit"), Scene.Upgrades->GetStackCount(TEXT("TestUpgrade")), 1);

    // Leveling again: the only upgrade is now maxed, so there is nothing eligible to offer. The pending
    // choice is consumed silently (not stuck forever) and the queue resolves with no screen shown.
    const int32 ChoiceReadyBefore = Scene.ChoiceReadyCount();
    Scene.Upgrades->NotifyLevelGained();
    TestEqual(TEXT("No new choice: nothing eligible"), Scene.ChoiceReadyCount(), ChoiceReadyBefore);
    TestFalse(TEXT("Not active"), Scene.Upgrades->IsChoiceActive());
    TestEqual(TEXT("Still only one stack: the second level-up granted nothing"), Scene.Upgrades->GetStackCount(TEXT("TestUpgrade")), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FUpgradeComponentQueuedChoicesTest,
    "GrantMoney.Progression.UpgradeComponent.QueuedChoicesResumeAfterTheLast",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUpgradeComponentQueuedChoicesTest::RunTest(const FString& Parameters)
{
    UpgradeTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    // Two level-ups arrive before the player answers either (e.g. one big pickup crossed two
    // thresholds): both must be queued, and OnChoicesResolved must fire only once, after the second.
    Scene.Upgrades->NotifyLevelGained();
    Scene.Upgrades->NotifyLevelGained();
    TestEqual(TEXT("Two choices pending"), Scene.Upgrades->GetPendingChoiceCount(), 2);
    TestEqual(TEXT("Only the first choice was presented so far"), Scene.ChoiceReadyCount(), 1);
    TestEqual(TEXT("Not resolved yet"), Scene.ChoicesResolvedCount(), 0);

    Scene.Upgrades->ChooseUpgrade(0);
    TestEqual(TEXT("One choice left"), Scene.Upgrades->GetPendingChoiceCount(), 1);
    TestTrue(TEXT("Still (or again) active for the second choice"), Scene.Upgrades->IsChoiceActive());
    TestEqual(TEXT("The second choice was presented"), Scene.ChoiceReadyCount(), 2);
    TestEqual(TEXT("Still not resolved: one choice remains"), Scene.ChoicesResolvedCount(), 0);

    Scene.Upgrades->ChooseUpgrade(0);
    TestEqual(TEXT("No choices left"), Scene.Upgrades->GetPendingChoiceCount(), 0);
    TestFalse(TEXT("No longer active"), Scene.Upgrades->IsChoiceActive());
    TestEqual(TEXT("Resolved exactly once, after the final choice"), Scene.ChoicesResolvedCount(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FUpgradeComponentDistinctOptionsTest,
    "GrantMoney.Progression.UpgradeComponent.OffersDistinctOptions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUpgradeComponentDistinctOptionsTest::RunTest(const FString& Parameters)
{
    UpgradeTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    // Run the roll many times (different seeds) and check every result is internally distinct and
    // capped at OptionsPerChoice, never offering the same upgrade twice in one choice.
    for (int32 Seed = 0; Seed < 50; ++Seed)
    {
        Scene.Upgrades->SetRandomSeed(Seed);
        const TArray<FGrantMoneyUpgradeDefinition> Options = Scene.Upgrades->RollOptions();

        TestTrue(FString::Printf(TEXT("Seed %d: at most OptionsPerChoice"), Seed), Options.Num() <= Scene.Upgrades->OptionsPerChoice);

        TSet<FName> SeenIds;
        for (const FGrantMoneyUpgradeDefinition& Option : Options)
        {
            TestFalse(FString::Printf(TEXT("Seed %d: no duplicate Id in one choice"), Seed), SeenIds.Contains(Option.Id));
            SeenIds.Add(Option.Id);
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FUpgradeComponentDeterministicSeedTest,
    "GrantMoney.Progression.UpgradeComponent.SameSeedSameRoll",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUpgradeComponentDeterministicSeedTest::RunTest(const FString& Parameters)
{
    UpgradeTest::FScene Scene;
    if (!Scene.Create(*this, 42))
    {
        return false;
    }

    Scene.Upgrades->SetRandomSeed(42);
    const TArray<FGrantMoneyUpgradeDefinition> First = Scene.Upgrades->RollOptions();

    Scene.Upgrades->SetRandomSeed(42);
    const TArray<FGrantMoneyUpgradeDefinition> Second = Scene.Upgrades->RollOptions();

    TestEqual(TEXT("Same count"), First.Num(), Second.Num());
    for (int32 Index = 0; Index < First.Num(); ++Index)
    {
        TestEqual(*FString::Printf(TEXT("Same Id at %d"), Index), First[Index].Id, Second[Index].Id);
    }
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
