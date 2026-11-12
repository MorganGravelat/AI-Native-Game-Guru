#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ContactDamageComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GrantMoneyPlayerCharacter.h"
#include "HealthComponent.h"
#include "RunController.h"
#include "StarterWeaponComponent.h"
#include "StatsComponent.h"
#include "Tests/AutomationCommon.h"
#include "Tests/CombatTestListener.h"
#include "Tests/EmergenceTestTypes.h"
#include "Tests/RunTestListener.h"

// SCRUM-44: the player's zero-health event ends the run as a loss. These tests exercise that path
// end to end (a real AGrantMoneyPlayerCharacter and a real ARunController, not a hand-rolled stand-in),
// since AGrantMoneyPlayerCharacter::HandleHealthDepleted is where the two systems are actually connected.

namespace RunLossTest
{
    // An empty game world with play started, a real AGrantMoneyPlayerCharacter (frozen in place, weapon
    // disarmed so it cannot kill the attacking enemy mid-test) and a real ARunController, started and
    // Active. Listeners are bound on the player's health and on the run's state, exactly as the game
    // mode and the player character bind them in production. Destroyed when this goes out of scope.
    struct FScene
    {
        FTestWorldWrapper WorldWrapper;
        AGrantMoneyPlayerCharacter* Player = nullptr;
        ARunController* Run = nullptr;
        UCombatTestListener* HealthListener = nullptr;
        URunTestListener* RunListener = nullptr;

        bool Create(FAutomationTestBase& Test)
        {
            if (!WorldWrapper.CreateTestWorld(EWorldType::Game) || !WorldWrapper.BeginPlayInTestWorld())
            {
                WorldWrapper.ForwardErrorMessages(&Test);
                return false;
            }

            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            Player = World()->SpawnActor<AGrantMoneyPlayerCharacter>(AGrantMoneyPlayerCharacter::StaticClass(), FTransform::Identity, Params);
            Run = World()->SpawnActor<ARunController>();
            if (!Test.TestNotNull(TEXT("Player spawned"), Player) || !Test.TestNotNull(TEXT("Run controller spawned"), Run))
            {
                return false;
            }

            // Frozen: nothing here is testing movement, and an unfrozen capsule would fall with no ground.
            Player->GetCharacterMovement()->DisableMovement();

            // The weapon fires at any AEnemyBase in range by itself; with no range it cannot kill the
            // attacker spawned by the cadence test, which would otherwise end that test early.
            Player->GetStarterWeapon()->TargetRange = 0.0f;

            Run->StartDelaySeconds = 0.0f;
            Run->RunDurationSeconds = 180.0f;
            Run->StartRun();
            if (!Test.TestTrue(TEXT("Run is Active"), Run->GetState() == EGrantMoneyRunState::Active))
            {
                return false;
            }

            HealthListener = NewObject<UCombatTestListener>();
            Player->GetHealth()->OnDeath.AddDynamic(HealthListener, &UCombatTestListener::OnDeath);

            RunListener = NewObject<URunTestListener>();
            Run->OnStateChanged.AddDynamic(RunListener, &URunTestListener::OnStateChanged);
            return true;
        }

        UWorld* World() const { return WorldWrapper.GetTestWorld(); }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerDeathTransitionsRunToLostTest,
    "GrantMoney.Run.Loss.PlayerDeathTransitionsRunToLost",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerDeathTransitionsRunToLostTest::RunTest(const FString& Parameters)
{
    RunLossTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    // Non-lethal: the run must not react to ordinary damage, only to death.
    Scene.Player->GetHealth()->ApplyDamage(50.0f);
    TestEqual(TEXT("No death yet"), Scene.HealthListener->DeathCount, 0);
    TestTrue(TEXT("Still Active"), Scene.Run->GetState() == EGrantMoneyRunState::Active);

    // Lethal: AGrantMoneyPlayerCharacter::HandleHealthDepleted (bound in BeginPlay) asks the run to end.
    Scene.Player->GetHealth()->ApplyDamage(1000.0f);
    TestEqual(TEXT("Death reported once"), Scene.HealthListener->DeathCount, 1);
    TestTrue(TEXT("The run is Lost"), Scene.Run->GetState() == EGrantMoneyRunState::Lost);
    TestEqual(TEXT("Exactly one transition"), Scene.RunListener->Transitions.Num(), 1);
    TestTrue(TEXT("Active to Lost"), Scene.RunListener->Transitions[0] == TPair<EGrantMoneyRunState, EGrantMoneyRunState>(EGrantMoneyRunState::Active, EGrantMoneyRunState::Lost));

    // Further damage: HealthComponent itself guards a dead owner, so there is nothing more to react to.
    Scene.Player->GetHealth()->ApplyDamage(10.0f);
    TestEqual(TEXT("No second death"), Scene.HealthListener->DeathCount, 1);
    TestEqual(TEXT("Still exactly one transition"), Scene.RunListener->Transitions.Num(), 1);
    TestTrue(TEXT("Lost is final"), Scene.Run->GetState() == EGrantMoneyRunState::Lost);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSlimeContactKillsPlayerAtConfiguredCadenceTest,
    "GrantMoney.Run.Loss.SlimeContactKillsPlayerAtConfiguredCadence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSlimeContactKillsPlayerAtConfiguredCadenceTest::RunTest(const FString& Parameters)
{
    RunLossTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    // An AEnemyBase (slime contact damage is the base class's; ASlimeEnemy itself changes nothing about
    // it) standing in contact range, chasing the player directly rather than through GetPlayerPawn (this
    // test world possesses nothing). Default stats: ContactDamage 10, AttackInterval 1 s, MaxHealth 30.
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AChaseTestEnemy* Enemy = Scene.World()->SpawnActor<AChaseTestEnemy>(AChaseTestEnemy::StaticClass(), FTransform(FVector(20.0, 0.0, 0.0)), Params);
    if (!TestNotNull(TEXT("Enemy spawned"), Enemy))
    {
        return false;
    }

    Enemy->GetCharacterMovement()->DisableMovement();
    Enemy->ChaseTarget = Scene.Player;
    TestTrue(TEXT("Within contact reach from the start"), Enemy->GetContactDamage()->IsInReach(Scene.Player));

    const float StartingHealth = Scene.Player->GetHealth()->GetHealth();
    TestEqual(TEXT("Starts at full health"), StartingHealth, 100.0f);

    // 0.25 s steps (exact in binary floating point, unlike 0.1 s) so four steps make exactly one
    // AttackInterval with no rounding drift to make the cadence check flaky.
    constexpr float StepSeconds = 0.25f;
    constexpr int32 StepsPerInterval = 4;
    const int32 HitsToKill = FMath::CeilToInt32(StartingHealth / 10.0f); // ContactDamage = 10 per hit

    for (int32 Window = 0; Window < HitsToKill; ++Window)
    {
        const float HealthBeforeWindow = Scene.Player->GetHealth()->GetHealth();

        for (int32 SubStep = 0; SubStep < StepsPerInterval; ++SubStep)
        {
            Scene.WorldWrapper.TickTestWorld(StepSeconds);
        }

        const float HealthAfterWindow = Scene.Player->GetHealth()->GetHealth();
        TestEqual(*FString::Printf(TEXT("Exactly one hit (10 damage) in window %d, not one per sub-step"), Window),
            HealthBeforeWindow - HealthAfterWindow, 10.0f);
    }

    TestTrue(TEXT("Health reached zero"), Scene.Player->GetHealth()->IsDead());
    TestEqual(TEXT("Death reported once"), Scene.HealthListener->DeathCount, 1);
    TestTrue(TEXT("The run is Lost"), Scene.Run->GetState() == EGrantMoneyRunState::Lost);
    TestEqual(TEXT("Exactly one transition"), Scene.RunListener->Transitions.Num(), 1);

    // Continued overlap after death: no more damage (HealthComponent guards it), no second transition.
    for (int32 SubStep = 0; SubStep < StepsPerInterval * 2; ++SubStep)
    {
        Scene.WorldWrapper.TickTestWorld(StepSeconds);
    }
    TestEqual(TEXT("Health stays at zero"), Scene.Player->GetHealth()->GetHealth(), 0.0f);
    TestEqual(TEXT("Still exactly one transition"), Scene.RunListener->Transitions.Num(), 1);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
