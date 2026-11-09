#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AttackHitComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "HealthComponent.h"
#include "StatsComponent.h"
#include "Tests/AutomationCommon.h"
#include "Tests/CombatTestListener.h"

namespace AttackTest
{
    // An empty game world with play started: an "attack" actor (stand-in for a projectile) with a
    // UAttackHitComponent, and three targets with health (fallback max health 100). Plain AActors are
    // used because characters refuse engine damage in a world without a Game Mode.
    struct FScene
    {
        FTestWorldWrapper WorldWrapper;
        AActor* Attack = nullptr;
        UStatsComponent* AttackStats = nullptr;
        UAttackHitComponent* AttackHit = nullptr;
        UCombatTestListener* Listener = nullptr;
        TArray<AActor*> Targets;

        bool Create(FAutomationTestBase& Test, bool bWithStats = false)
        {
            if (!WorldWrapper.CreateTestWorld(EWorldType::Game) || !WorldWrapper.BeginPlayInTestWorld())
            {
                WorldWrapper.ForwardErrorMessages(&Test);
                return false;
            }

            Attack = World()->SpawnActor<AActor>();
            if (!Attack)
            {
                Test.AddError(TEXT("Could not spawn the attack actor."));
                return false;
            }

            // Stats first, so the hit component finds it in BeginPlay.
            if (bWithStats)
            {
                AttackStats = NewObject<UStatsComponent>(Attack);
                AttackStats->RegisterComponent();
            }

            AttackHit = NewObject<UAttackHitComponent>(Attack);
            AttackHit->RegisterComponent();

            Listener = NewObject<UCombatTestListener>();
            AttackHit->OnTargetHit.AddDynamic(Listener, &UCombatTestListener::OnAttackTargetHit);
            AttackHit->OnHitsExhausted.AddDynamic(Listener, &UCombatTestListener::OnAttackHitsExhausted);

            for (int32 Index = 0; Index < 3; ++Index)
            {
                Targets.Add(SpawnWithHealth<AActor>());
            }
            return true;
        }

        UWorld* World() const { return WorldWrapper.GetTestWorld(); }

        template <typename T>
        T* SpawnWithHealth()
        {
            T* Actor = World()->SpawnActor<T>();
            UHealthComponent* Health = NewObject<UHealthComponent>(Actor);
            Health->RegisterComponent();
            return Actor;
        }

        static float HealthOf(const AActor* Actor)
        {
            return Actor->FindComponentByClass<UHealthComponent>()->GetHealth();
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAttackHitOncePerTargetTest,
    "GrantMoney.Combat.AttackHitComponent.OncePerTarget",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAttackHitOncePerTargetTest::RunTest(const FString& Parameters)
{
    AttackTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    Scene.AttackHit->FallbackPenetration = 5;
    AActor* Target = Scene.Targets[0];

    TestTrue(TEXT("First overlap hits"), Scene.AttackHit->TryHit(Target) == EGrantMoneyHitResult::Hit);
    TestEqual(TEXT("Fallback damage (10) applied through engine damage"), AttackTest::FScene::HealthOf(Target), 90.0f);
    TestTrue(TEXT("Target is recorded"), Scene.AttackHit->HasHit(Target));

    // A projectile overlapping the same enemy on later frames, or with several components.
    TestTrue(TEXT("Second overlap is a duplicate"), Scene.AttackHit->TryHit(Target) == EGrantMoneyHitResult::AlreadyHit);
    TestTrue(TEXT("Third overlap is a duplicate"), Scene.AttackHit->TryHit(Target) == EGrantMoneyHitResult::AlreadyHit);
    TestEqual(TEXT("Duplicates deal no damage"), AttackTest::FScene::HealthOf(Target), 90.0f);
    TestEqual(TEXT("Duplicates do not use up penetration"), Scene.AttackHit->GetRemainingHits(), 5);
    TestEqual(TEXT("Hit event fired once"), Scene.Listener->AttackHitCount, 1);
    TestEqual(TEXT("Hit event reports the damage"), Scene.Listener->LastAttackDamage, 10.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAttackHitPenetrationTest,
    "GrantMoney.Combat.AttackHitComponent.Penetration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAttackHitPenetrationTest::RunTest(const FString& Parameters)
{
    {
        AttackTest::FScene Scene;
        if (Scene.Create(*this))
        {
            // Default penetration 0: the attack stops at its first target.
            TestEqual(TEXT("Penetration 0 allows one hit"), Scene.AttackHit->GetRemainingHits(), 1);
            TestTrue(TEXT("First target hit"), Scene.AttackHit->TryHit(Scene.Targets[0]) == EGrantMoneyHitResult::Hit);
            TestTrue(TEXT("Exhausted"), Scene.AttackHit->IsExhausted());
            TestEqual(TEXT("Exhausted event fired"), Scene.Listener->AttackExhaustedCount, 1);
            TestTrue(TEXT("Second target refused"), Scene.AttackHit->TryHit(Scene.Targets[1]) == EGrantMoneyHitResult::Exhausted);
            TestEqual(TEXT("Second target untouched"), AttackTest::FScene::HealthOf(Scene.Targets[1]), 100.0f);
        }
    }

    {
        AttackTest::FScene Scene;
        if (Scene.Create(*this))
        {
            Scene.AttackHit->FallbackPenetration = 1;
            TestTrue(TEXT("Hit 1"), Scene.AttackHit->TryHit(Scene.Targets[0]) == EGrantMoneyHitResult::Hit);
            TestFalse(TEXT("Not exhausted after 1 of 2"), Scene.AttackHit->IsExhausted());
            TestEqual(TEXT("No exhausted event yet"), Scene.Listener->AttackExhaustedCount, 0);
            TestTrue(TEXT("Hit 2 passes through to a second target"), Scene.AttackHit->TryHit(Scene.Targets[1]) == EGrantMoneyHitResult::Hit);
            TestTrue(TEXT("Third target refused"), Scene.AttackHit->TryHit(Scene.Targets[2]) == EGrantMoneyHitResult::Exhausted);
            TestEqual(TEXT("Exhausted event fired exactly once"), Scene.Listener->AttackExhaustedCount, 1);
            TestEqual(TEXT("Third target untouched"), AttackTest::FScene::HealthOf(Scene.Targets[2]), 100.0f);

            Scene.AttackHit->ResetHits();
            TestEqual(TEXT("ResetHits restores all hits"), Scene.AttackHit->GetRemainingHits(), 2);
            TestTrue(TEXT("After a reset, an earlier target can be hit again"), Scene.AttackHit->TryHit(Scene.Targets[0]) == EGrantMoneyHitResult::Hit);
            TestEqual(TEXT("Earlier target took a second hit"), AttackTest::FScene::HealthOf(Scene.Targets[0]), 80.0f);
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAttackHitNotATargetTest,
    "GrantMoney.Combat.AttackHitComponent.NotATarget",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAttackHitNotATargetTest::RunTest(const FString& Parameters)
{
    AttackTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    // Like a projectile spawned by a weapon (owner) and fired by the player (instigator), both with health.
    APawn* Player = Scene.SpawnWithHealth<APawn>();
    AActor* Weapon = Scene.SpawnWithHealth<AActor>();
    Scene.Attack->SetInstigator(Player);
    Scene.Attack->SetOwner(Weapon);

    AActor* Prop = Scene.World()->SpawnActor<AActor>();
    AActor* DeadEnemy = Scene.Targets[1];
    DeadEnemy->FindComponentByClass<UHealthComponent>()->ApplyDamage(1000.0f);

    TestTrue(TEXT("Null"), Scene.AttackHit->TryHit(nullptr) == EGrantMoneyHitResult::NotATarget);
    TestTrue(TEXT("The attack itself"), Scene.AttackHit->TryHit(Scene.Attack) == EGrantMoneyHitResult::NotATarget);
    TestTrue(TEXT("The instigator (the player who fired it)"), Scene.AttackHit->TryHit(Player) == EGrantMoneyHitResult::NotATarget);
    TestTrue(TEXT("The owner (the weapon)"), Scene.AttackHit->TryHit(Weapon) == EGrantMoneyHitResult::NotATarget);
    TestTrue(TEXT("An actor without health (wall, prop)"), Scene.AttackHit->TryHit(Prop) == EGrantMoneyHitResult::NotATarget);
    TestTrue(TEXT("A target that is already dead"), Scene.AttackHit->TryHit(DeadEnemy) == EGrantMoneyHitResult::NotATarget);

    TestEqual(TEXT("Player unharmed"), AttackTest::FScene::HealthOf(Player), 100.0f);
    TestEqual(TEXT("None of these used up the single hit"), Scene.AttackHit->GetRemainingHits(), 1);
    TestEqual(TEXT("No hit events"), Scene.Listener->AttackHitCount, 0);

    TestTrue(TEXT("A real target is still hit afterwards"), Scene.AttackHit->TryHit(Scene.Targets[0]) == EGrantMoneyHitResult::Hit);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAttackHitUsesStatsTest,
    "GrantMoney.Combat.AttackHitComponent.UsesStats",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAttackHitUsesStatsTest::RunTest(const FString& Parameters)
{
    AttackTest::FScene Scene;
    if (!Scene.Create(*this, true))
    {
        return false;
    }

    // Stats component present but no Damage/Penetration stats yet: fallbacks apply.
    TestEqual(TEXT("Missing Damage stat falls back"), Scene.AttackHit->GetDamage(), 10.0f);
    TestEqual(TEXT("Missing Penetration stat falls back"), Scene.AttackHit->GetPenetration(), 0);

    Scene.AttackStats->SetBaseStat(EGrantMoneyStat::Damage, 25.0f);
    Scene.AttackStats->SetBaseStat(EGrantMoneyStat::Penetration, 1.0f);
    Scene.AttackStats->AddModifier(EGrantMoneyStat::Penetration, EGrantMoneyStatModifierOp::Additive, 0.9f);
    TestEqual(TEXT("Damage from the stat"), Scene.AttackHit->GetDamage(), 25.0f);
    TestEqual(TEXT("Penetration 1.9 rounds down to 1"), Scene.AttackHit->GetPenetration(), 1);

    Scene.AttackStats->AddModifier(EGrantMoneyStat::Damage, EGrantMoneyStatModifierOp::Multiplicative, 1.0f);
    TestEqual(TEXT("A +100% damage buff doubles it"), Scene.AttackHit->GetDamage(), 50.0f);

    Scene.AttackHit->TryHit(Scene.Targets[0]);
    TestEqual(TEXT("Target took the buffed damage"), AttackTest::FScene::HealthOf(Scene.Targets[0]), 50.0f);

    Scene.AttackStats->AddModifier(EGrantMoneyStat::Penetration, EGrantMoneyStatModifierOp::Additive, 1.0f);
    TestEqual(TEXT("A +1 penetration upgrade allows one more target"), Scene.AttackHit->GetRemainingHits(), 2);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
