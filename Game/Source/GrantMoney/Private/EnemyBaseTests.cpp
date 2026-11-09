#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "CombatEventsSubsystem.h"
#include "EnemyBase.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HealthComponent.h"
#include "StatsComponent.h"
#include "Tests/AutomationCommon.h"
#include "Tests/CombatTestListener.h"

// Characters refuse engine damage (UGameplayStatics::ApplyDamage) in a world without a Game Mode
// (APawn::ShouldTakeDamage), and the test world has none. These tests therefore damage the enemy's
// health component directly; the engine path is covered by HealthComponent.EngineDamage and by the
// GrantMoney.DamageEnemies console command in PIE.

namespace EnemyTest
{
    // An empty game world with play started, one AEnemyBase (BeginPlay runs on spawn) and a listener
    // bound to UCombatEventsSubsystem::OnEnemyKilled. Destroyed when this goes out of scope.
    struct FScene
    {
        FTestWorldWrapper WorldWrapper;
        AEnemyBase* Enemy = nullptr;
        UCombatTestListener* Listener = nullptr;

        bool Create(FAutomationTestBase& Test)
        {
            if (!WorldWrapper.CreateTestWorld(EWorldType::Game) || !WorldWrapper.BeginPlayInTestWorld())
            {
                WorldWrapper.ForwardErrorMessages(&Test);
                return false;
            }

            UWorld* World = WorldWrapper.GetTestWorld();
            Enemy = World->SpawnActor<AEnemyBase>();
            if (!Test.TestNotNull(TEXT("Enemy spawned"), Enemy) || !Test.TestNotNull(TEXT("Enemy has health"), Enemy->GetHealth()))
            {
                return false;
            }

            UCombatEventsSubsystem* CombatEvents = World->GetSubsystem<UCombatEventsSubsystem>();
            if (!Test.TestNotNull(TEXT("Combat events subsystem exists"), CombatEvents))
            {
                return false;
            }

            Listener = NewObject<UCombatTestListener>();
            CombatEvents->OnEnemyKilled.AddDynamic(Listener, &UCombatTestListener::OnEnemyKilled);
            return true;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemyBaseCombatDefaultsTest,
    "GrantMoney.Combat.EnemyBase.CombatDefaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemyBaseCombatDefaultsTest::RunTest(const FString& Parameters)
{
    EnemyTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    const UStatsComponent* Stats = Scene.Enemy->GetStats();
    TestEqual(TEXT("MaxHealth stat"), Stats->GetStat(EGrantMoneyStat::MaxHealth), 30.0f);
    TestEqual(TEXT("ContactDamage stat"), Stats->GetStat(EGrantMoneyStat::ContactDamage), 10.0f);
    TestEqual(TEXT("AttackInterval stat"), Stats->GetStat(EGrantMoneyStat::AttackInterval), 1.0f);
    TestEqual(TEXT("Health reads max health from the stat"), Scene.Enemy->GetHealth()->GetMaxHealth(), 30.0f);
    TestEqual(TEXT("Starts at full health"), Scene.Enemy->GetHealth()->GetHealth(), 30.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemyBaseDeathTest,
    "GrantMoney.Combat.EnemyBase.Death",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemyBaseDeathTest::RunTest(const FString& Parameters)
{
    EnemyTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    AEnemyBase* Enemy = Scene.Enemy;
    AActor* Killer = Scene.WorldWrapper.GetTestWorld()->SpawnActor<AActor>();

    Enemy->GetHealth()->ApplyDamage(10.0f, Killer);
    TestEqual(TEXT("Non-lethal damage reports no kill"), Scene.Listener->EnemyKilledCount, 0);
    TestFalse(TEXT("Still alive and in the world"), Enemy->IsActorBeingDestroyed());

    Enemy->GetHealth()->ApplyDamage(100.0f, Killer);
    TestEqual(TEXT("Lethal damage reports one kill"), Scene.Listener->EnemyKilledCount, 1);
    // Get(true): the enemy is destroyed right after the event, which a plain Get() treats as null.
    TestTrue(TEXT("The kill reports this enemy"), Scene.Listener->LastKilledEnemy.Get(true) == Enemy);
    TestTrue(TEXT("The kill is reported before the enemy is destroyed"), Scene.Listener->bKilledEnemyWasAliveWhenReported);
    TestTrue(TEXT("The kill reports the damage causer"), Scene.Listener->LastKiller.Get() == Killer);
    TestTrue(TEXT("With no cleanup delay the enemy is destroyed straight away"), Enemy->IsActorBeingDestroyed());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemyBaseDelayedCleanupTest,
    "GrantMoney.Combat.EnemyBase.DelayedCleanup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemyBaseDelayedCleanupTest::RunTest(const FString& Parameters)
{
    EnemyTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    AEnemyBase* Enemy = Scene.Enemy;
    Enemy->DeathCleanupDelay = 2.0f;

    Enemy->GetHealth()->ApplyDamage(1000.0f);
    TestEqual(TEXT("Kill reported"), Scene.Listener->EnemyKilledCount, 1);
    TestFalse(TEXT("Not destroyed yet"), Enemy->IsActorBeingDestroyed());
    TestTrue(TEXT("Destruction is scheduled"), Enemy->GetLifeSpan() > 0.0f);
    TestFalse(TEXT("Stopped ticking (no chasing or contact hits)"), Enemy->IsActorTickEnabled());
    TestFalse(TEXT("Collision is off"), Enemy->GetActorEnableCollision());
    TestTrue(TEXT("Movement is off"), Enemy->GetCharacterMovement()->MovementMode == MOVE_None);

    Enemy->GetHealth()->ApplyDamage(10.0f);
    TestEqual(TEXT("Damage to a dead enemy reports no second kill"), Scene.Listener->EnemyKilledCount, 1);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
