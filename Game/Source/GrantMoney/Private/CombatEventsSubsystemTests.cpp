#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "CombatEventsSubsystem.h"
#include "EnemyBase.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "Tests/CombatTestListener.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCombatEventsSubsystemEnemyKilledTest,
    "GrantMoney.Combat.CombatEventsSubsystem.EnemyKilled",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCombatEventsSubsystemEnemyKilledTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game) || !WorldWrapper.BeginPlayInTestWorld())
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UCombatEventsSubsystem* CombatEvents = World->GetSubsystem<UCombatEventsSubsystem>();
    if (!TestNotNull(TEXT("Every game world has the combat events subsystem"), CombatEvents))
    {
        return false;
    }

    UCombatTestListener* Listener = NewObject<UCombatTestListener>();
    CombatEvents->OnEnemyKilled.AddDynamic(Listener, &UCombatTestListener::OnEnemyKilled);

    CombatEvents->NotifyEnemyKilled(nullptr, nullptr);
    TestEqual(TEXT("A null enemy is not broadcast"), Listener->EnemyKilledCount, 0);

    AEnemyBase* Enemy = World->SpawnActor<AEnemyBase>();
    AActor* Killer = World->SpawnActor<AActor>();
    CombatEvents->NotifyEnemyKilled(Enemy, Killer);

    TestEqual(TEXT("Broadcast once"), Listener->EnemyKilledCount, 1);
    TestTrue(TEXT("Reports the enemy"), Listener->LastKilledEnemy.Get() == Enemy);
    TestTrue(TEXT("Reports the killer"), Listener->LastKiller.Get() == Killer);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
