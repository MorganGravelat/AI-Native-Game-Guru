#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/SceneComponent.h"
#include "ContactDamageComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HealthComponent.h"
#include "StatsComponent.h"
#include "Tests/AutomationCommon.h"

namespace ContactTest
{
    // An empty game world with play started, containing a victim (health only, fallback max health 100)
    // at the origin and an attacker with a contact damage component. Neither actor has collision, so both
    // collision radii are 0 and reach is measured centre to centre. Destroyed when this goes out of scope.
    struct FScene
    {
        FTestWorldWrapper WorldWrapper;
        AActor* Victim = nullptr;
        AActor* Attacker = nullptr;
        UHealthComponent* VictimHealth = nullptr;
        UStatsComponent* AttackerStats = nullptr;
        UContactDamageComponent* Contact = nullptr;

        // Attacker stands AttackerX cm from the victim. With bAttackerHasStats, damage comes from its ContactDamage stat.
        bool Create(FAutomationTestBase& Test, float AttackerX, bool bAttackerHasStats, float ContactDamageStat = 0.0f)
        {
            if (!WorldWrapper.CreateTestWorld(EWorldType::Game) || !WorldWrapper.BeginPlayInTestWorld())
            {
                WorldWrapper.ForwardErrorMessages(&Test);
                return false;
            }

            Victim = SpawnWithRoot(FVector::ZeroVector);
            Attacker = SpawnWithRoot(FVector(AttackerX, 0.0, 0.0));
            if (!Victim || !Attacker)
            {
                Test.AddError(TEXT("Could not spawn the test actors."));
                return false;
            }

            VictimHealth = NewObject<UHealthComponent>(Victim);
            VictimHealth->RegisterComponent();

            // Stats first, so the contact component finds it in BeginPlay, like on AEnemyBase.
            if (bAttackerHasStats)
            {
                AttackerStats = NewObject<UStatsComponent>(Attacker);
                AttackerStats->SetBaseStat(EGrantMoneyStat::ContactDamage, ContactDamageStat);
                AttackerStats->RegisterComponent();
            }

            Contact = NewObject<UContactDamageComponent>(Attacker);
            Contact->RegisterComponent();
            return true;
        }

        // Advances world time by Seconds in small steps.
        void Wait(float Seconds)
        {
            constexpr float Step = 0.1f;
            for (float Elapsed = 0.0f; Elapsed < Seconds; Elapsed += Step)
            {
                WorldWrapper.TickTestWorld(Step);
            }
        }

    private:
        // An actor without a root component cannot be moved, so give it one.
        AActor* SpawnWithRoot(const FVector& Location)
        {
            AActor* Actor = WorldWrapper.GetTestWorld()->SpawnActor<AActor>();
            if (!Actor)
            {
                return nullptr;
            }

            USceneComponent* Root = NewObject<USceneComponent>(Actor);
            Actor->SetRootComponent(Root);
            Root->RegisterComponent();
            Actor->SetActorLocation(Location);
            return Actor;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContactDamageReachTest,
    "GrantMoney.Combat.ContactDamageComponent.Reach",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FContactDamageReachTest::RunTest(const FString& Parameters)
{
    {
        ContactTest::FScene Scene;
        if (Scene.Create(*this, 30.0f, false))
        {
            TestTrue(TEXT("30 cm away is within the default 40 cm reach"), Scene.Contact->IsInReach(Scene.Victim));
            TestTrue(TEXT("Damage sent"), Scene.Contact->TryApplyContactDamage(Scene.Victim));
            TestEqual(TEXT("Fallback damage (10) reached the victim's health"), Scene.VictimHealth->GetHealth(), 90.0f);
        }
    }

    {
        ContactTest::FScene Scene;
        if (Scene.Create(*this, 100.0f, false))
        {
            TestFalse(TEXT("100 cm away is out of reach"), Scene.Contact->IsInReach(Scene.Victim));
            TestFalse(TEXT("No damage sent"), Scene.Contact->TryApplyContactDamage(Scene.Victim));
            TestEqual(TEXT("Victim untouched"), Scene.VictimHealth->GetHealth(), 100.0f);

            Scene.Victim->SetActorLocation(FVector(100.0, 0.0, 500.0));
            TestTrue(TEXT("Height difference is ignored: same spot on the ground plane is in reach"), Scene.Contact->IsInReach(Scene.Victim));
        }
    }

    {
        ContactTest::FScene Scene;
        if (Scene.Create(*this, 0.0f, false))
        {
            TestFalse(TEXT("A null target is ignored"), Scene.Contact->TryApplyContactDamage(nullptr));
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContactDamageIntervalTest,
    "GrantMoney.Combat.ContactDamageComponent.Interval",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FContactDamageIntervalTest::RunTest(const FString& Parameters)
{
    ContactTest::FScene Scene;
    if (!Scene.Create(*this, 0.0f, false))
    {
        return false;
    }

    Scene.Contact->FallbackInterval = 1.0f;
    TestEqual(TEXT("Without stats, the fallback interval is used"), Scene.Contact->GetInterval(), 1.0f);

    TestTrue(TEXT("First touch hits immediately"), Scene.Contact->TryApplyContactDamage(Scene.Victim));
    TestFalse(TEXT("Second touch in the same frame is blocked"), Scene.Contact->TryApplyContactDamage(Scene.Victim));

    Scene.Wait(0.5f);
    TestFalse(TEXT("Still blocked half way through the interval"), Scene.Contact->TryApplyContactDamage(Scene.Victim));
    TestEqual(TEXT("Only one hit landed so far"), Scene.VictimHealth->GetHealth(), 90.0f);

    Scene.Wait(0.6f);
    TestTrue(TEXT("Hits again once the interval has passed"), Scene.Contact->TryApplyContactDamage(Scene.Victim));
    TestEqual(TEXT("Second hit landed"), Scene.VictimHealth->GetHealth(), 80.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContactDamageUsesStatTest,
    "GrantMoney.Combat.ContactDamageComponent.UsesStat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FContactDamageUsesStatTest::RunTest(const FString& Parameters)
{
    ContactTest::FScene Scene;
    if (!Scene.Create(*this, 0.0f, true, 15.0f))
    {
        return false;
    }

    // The attacker has no AttackInterval stat, so the fallback interval applies; 0 allows a hit every call.
    Scene.Contact->FallbackInterval = 0.0f;

    TestEqual(TEXT("Damage comes from the ContactDamage stat"), Scene.Contact->GetDamage(), 15.0f);
    Scene.Contact->TryApplyContactDamage(Scene.Victim);
    TestEqual(TEXT("Victim took the stat's damage"), Scene.VictimHealth->GetHealth(), 85.0f);

    Scene.AttackerStats->AddModifier(EGrantMoneyStat::ContactDamage, EGrantMoneyStatModifierOp::Multiplicative, 1.0f);
    TestEqual(TEXT("A +100% damage buff doubles it"), Scene.Contact->GetDamage(), 30.0f);
    Scene.Contact->TryApplyContactDamage(Scene.Victim);
    TestEqual(TEXT("Victim took the buffed damage"), Scene.VictimHealth->GetHealth(), 55.0f);

    Scene.AttackerStats->SetBaseStat(EGrantMoneyStat::ContactDamage, 0.0f);
    TestFalse(TEXT("Zero damage is not sent"), Scene.Contact->TryApplyContactDamage(Scene.Victim));
    TestEqual(TEXT("Victim unchanged"), Scene.VictimHealth->GetHealth(), 55.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContactDamageIntervalFromStatTest,
    "GrantMoney.Combat.ContactDamageComponent.IntervalFromStat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FContactDamageIntervalFromStatTest::RunTest(const FString& Parameters)
{
    ContactTest::FScene Scene;
    if (!Scene.Create(*this, 0.0f, true, 10.0f))
    {
        return false;
    }

    Scene.Contact->FallbackInterval = 0.0f;
    Scene.AttackerStats->SetBaseStat(EGrantMoneyStat::AttackInterval, 2.0f);
    TestEqual(TEXT("Interval comes from the AttackInterval stat, not the fallback"), Scene.Contact->GetInterval(), 2.0f);

    TestTrue(TEXT("First hit"), Scene.Contact->TryApplyContactDamage(Scene.Victim));
    Scene.Wait(1.2f);
    TestFalse(TEXT("A 2 s interval still blocks after 1.2 s"), Scene.Contact->TryApplyContactDamage(Scene.Victim));

    // -50% interval = attacks twice as often. Read live, so it applies to the cooldown already running.
    Scene.AttackerStats->AddModifier(EGrantMoneyStat::AttackInterval, EGrantMoneyStatModifierOp::Multiplicative, -0.5f);
    TestEqual(TEXT("Modifier halves the interval"), Scene.Contact->GetInterval(), 1.0f);
    TestTrue(TEXT("Faster attack rate allows the hit now"), Scene.Contact->TryApplyContactDamage(Scene.Victim));
    TestEqual(TEXT("Two hits landed"), Scene.VictimHealth->GetHealth(), 80.0f);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
