#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/DamageType.h"
#include "HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "StatsComponent.h"
#include "Tests/AutomationCommon.h"
#include "Tests/CombatTestListener.h"

namespace HealthTest
{
    // An empty game world with play started, so components run BeginPlay as soon as they register.
    // The world is destroyed when this goes out of scope.
    struct FTestActor
    {
        FTestWorldWrapper WorldWrapper;
        AActor* Actor = nullptr;
        UStatsComponent* Stats = nullptr;
        UHealthComponent* Health = nullptr;
        UCombatTestListener* Listener = nullptr;

        // With bWithStats, MaxHealth comes from a UStatsComponent; without it, from FallbackMaxHealth.
        bool Create(FAutomationTestBase& Test, bool bWithStats, float BaseMaxHealth)
        {
            if (!WorldWrapper.CreateTestWorld(EWorldType::Game) || !WorldWrapper.BeginPlayInTestWorld())
            {
                WorldWrapper.ForwardErrorMessages(&Test);
                return false;
            }

            Actor = WorldWrapper.GetTestWorld()->SpawnActor<AActor>();
            if (!Actor)
            {
                Test.AddError(TEXT("Could not spawn the test actor."));
                return false;
            }

            // Stats is registered first so the health component finds it in BeginPlay, like on the player.
            if (bWithStats)
            {
                Stats = NewObject<UStatsComponent>(Actor);
                Stats->SetBaseStat(EGrantMoneyStat::MaxHealth, BaseMaxHealth);
                Stats->RegisterComponent();
            }

            Health = NewObject<UHealthComponent>(Actor);
            Health->RegisterComponent();

            Listener = NewObject<UCombatTestListener>();
            Health->OnHealthChanged.AddDynamic(Listener, &UCombatTestListener::OnHealthChanged);
            Health->OnDeath.AddDynamic(Listener, &UCombatTestListener::OnDeath);

            return Test.TestTrue(TEXT("Health component began play"), Health->HasBegunPlay());
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHealthComponentStartsFullTest,
    "GrantMoney.Combat.HealthComponent.StartsFull",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHealthComponentStartsFullTest::RunTest(const FString& Parameters)
{
    {
        HealthTest::FTestActor WithStats;
        if (WithStats.Create(*this, true, 150.0f))
        {
            TestEqual(TEXT("Max health comes from the MaxHealth stat"), WithStats.Health->GetMaxHealth(), 150.0f);
            TestEqual(TEXT("Starts at full health"), WithStats.Health->GetHealth(), 150.0f);
            TestEqual(TEXT("Fraction is 1"), WithStats.Health->GetHealthFraction(), 1.0f);
            TestFalse(TEXT("Not dead"), WithStats.Health->IsDead());
        }
    }

    {
        HealthTest::FTestActor WithoutStats;
        if (WithoutStats.Create(*this, false, 0.0f))
        {
            TestEqual(TEXT("Without stats, the fallback max health is used"), WithoutStats.Health->GetMaxHealth(), 100.0f);
            TestEqual(TEXT("Starts at full fallback health"), WithoutStats.Health->GetHealth(), 100.0f);
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHealthComponentDamageAndHealTest,
    "GrantMoney.Combat.HealthComponent.DamageAndHeal",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHealthComponentDamageAndHealTest::RunTest(const FString& Parameters)
{
    HealthTest::FTestActor Test;
    if (!Test.Create(*this, true, 100.0f))
    {
        return false;
    }

    UHealthComponent* Health = Test.Health;

    TestEqual(TEXT("Damage returns the amount removed"), Health->ApplyDamage(30.0f), 30.0f);
    TestEqual(TEXT("Health after damage"), Health->GetHealth(), 70.0f);
    TestEqual(TEXT("Change event fired"), Test.Listener->HealthChangedCount, 1);
    TestEqual(TEXT("Change event reports a negative delta"), Test.Listener->LastDelta, -30.0f);

    TestEqual(TEXT("Zero damage is ignored"), Health->ApplyDamage(0.0f), 0.0f);
    TestEqual(TEXT("Negative damage is ignored (it is not a heal)"), Health->ApplyDamage(-50.0f), 0.0f);
    TestEqual(TEXT("Health unchanged by ignored damage"), Health->GetHealth(), 70.0f);
    TestEqual(TEXT("Ignored damage fires no event"), Test.Listener->HealthChangedCount, 1);

    TestEqual(TEXT("Heal returns the amount added"), Health->Heal(10.0f), 10.0f);
    TestEqual(TEXT("Heal is capped at max health"), Health->Heal(500.0f), 20.0f);
    TestEqual(TEXT("Health is full"), Health->GetHealth(), 100.0f);
    TestEqual(TEXT("Healing at full health does nothing"), Health->Heal(10.0f), 0.0f);
    TestEqual(TEXT("Negative heal is ignored"), Health->Heal(-10.0f), 0.0f);
    TestEqual(TEXT("Two heals fired events, the no-op heals did not"), Test.Listener->HealthChangedCount, 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHealthComponentDeathTest,
    "GrantMoney.Combat.HealthComponent.Death",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHealthComponentDeathTest::RunTest(const FString& Parameters)
{
    HealthTest::FTestActor Test;
    if (!Test.Create(*this, true, 100.0f))
    {
        return false;
    }

    UHealthComponent* Health = Test.Health;
    AActor* Causer = Test.Actor;

    TestEqual(TEXT("Overkill only removes the health that was left"), Health->ApplyDamage(250.0f, Causer), 100.0f);
    TestEqual(TEXT("Health is zero"), Health->GetHealth(), 0.0f);
    TestTrue(TEXT("Dead"), Health->IsDead());
    TestEqual(TEXT("Death fired once"), Test.Listener->DeathCount, 1);
    TestTrue(TEXT("Death reports the damage causer"), Test.Listener->LastDamageCauser.Get() == Causer);

    TestEqual(TEXT("Damage after death is ignored"), Health->ApplyDamage(10.0f, Causer), 0.0f);
    TestEqual(TEXT("Healing after death is ignored"), Health->Heal(10.0f), 0.0f);
    TestEqual(TEXT("Death still fired only once"), Test.Listener->DeathCount, 1);
    TestEqual(TEXT("Health stays at zero"), Health->GetHealth(), 0.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHealthComponentMaxHealthModifierTest,
    "GrantMoney.Combat.HealthComponent.MaxHealthModifier",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHealthComponentMaxHealthModifierTest::RunTest(const FString& Parameters)
{
    HealthTest::FTestActor Test;
    if (!Test.Create(*this, true, 100.0f))
    {
        return false;
    }

    UHealthComponent* Health = Test.Health;
    UStatsComponent* Stats = Test.Stats;

    Health->ApplyDamage(40.0f);
    TestEqual(TEXT("Damaged to 60"), Health->GetHealth(), 60.0f);

    const FGrantMoneyStatModifierHandle Buff = Stats->AddModifier(EGrantMoneyStat::MaxHealth, EGrantMoneyStatModifierOp::Additive, 50.0f);
    TestEqual(TEXT("Buff raises max health"), Health->GetMaxHealth(), 150.0f);
    TestEqual(TEXT("Buff also heals by the amount gained"), Health->GetHealth(), 110.0f);

    Stats->RemoveModifier(Buff);
    TestEqual(TEXT("Max health back to 100"), Health->GetMaxHealth(), 100.0f);
    TestEqual(TEXT("Losing max health clamps current health"), Health->GetHealth(), 100.0f);

    Health->ApplyDamage(50.0f);
    Stats->AddModifier(EGrantMoneyStat::MaxHealth, EGrantMoneyStatModifierOp::Multiplicative, -0.2f);
    TestEqual(TEXT("A smaller loss leaves current health alone when it still fits"), Health->GetHealth(), 50.0f);
    TestFalse(TEXT("Not dead"), Health->IsDead());

    // -0.2 + -1.0 is below -100%, so the stat clamps to exactly zero (no float rounding left over).
    Stats->AddModifier(EGrantMoneyStat::MaxHealth, EGrantMoneyStatModifierOp::Multiplicative, -1.0f);
    TestEqual(TEXT("Max health reduced to zero"), Health->GetMaxHealth(), 0.0f);
    TestTrue(TEXT("Max health reaching zero kills"), Health->IsDead());
    TestEqual(TEXT("Death fired once"), Test.Listener->DeathCount, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHealthComponentEngineDamageTest,
    "GrantMoney.Combat.HealthComponent.EngineDamage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHealthComponentEngineDamageTest::RunTest(const FString& Parameters)
{
    HealthTest::FTestActor Test;
    if (!Test.Create(*this, true, 100.0f))
    {
        return false;
    }

    // Use a second actor as the damage causer, as an enemy or projectile would be.
    AActor* Causer = Test.Actor->GetWorld()->SpawnActor<AActor>();

    UGameplayStatics::ApplyDamage(Test.Actor, 25.0f, nullptr, Causer, UDamageType::StaticClass());
    TestEqual(TEXT("UGameplayStatics::ApplyDamage reaches the health component"), Test.Health->GetHealth(), 75.0f);

    UGameplayStatics::ApplyDamage(Test.Actor, 500.0f, nullptr, Causer, UDamageType::StaticClass());
    TestTrue(TEXT("Engine damage can kill"), Test.Health->IsDead());
    TestTrue(TEXT("Death reports the engine damage causer"), Test.Listener->LastDamageCauser.Get() == Causer);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
