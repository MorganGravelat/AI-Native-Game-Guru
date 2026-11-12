#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "CombatEventsSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ContactDamageComponent.h"
#include "EnemyBase.h"
#include "EnemyEmergenceComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HealthComponent.h"
#include "SlimeEnemy.h"
#include "StatsComponent.h"
#include "Tests/AutomationCommon.h"
#include "Tests/CombatTestListener.h"
#include "Tests/EmergenceTestTypes.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemyBaseDeathTransformTest,
    "GrantMoney.Combat.EnemyBase.DeathTransform",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemyBaseDeathTransformTest::RunTest(const FString& Parameters)
{
    EnemyTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    AEnemyBase* Enemy = Scene.Enemy;
    TestTrue(TEXT("Identity before death"), Enemy->GetDeathTransform().Equals(FTransform::Identity));

    // Move it first, so the recorded spot is where it died and not where it spawned.
    const FTransform DeathSpot(FRotator(0.0, 45.0, 0.0), FVector(300.0, -200.0, 50.0));
    Enemy->SetActorTransform(DeathSpot);

    Enemy->GetHealth()->ApplyDamage(1000.0f);
    TestEqual(TEXT("Kill reported"), Scene.Listener->EnemyKilledCount, 1);
    TestTrue(TEXT("The enemy was destroyed"), Enemy->IsActorBeingDestroyed());
    TestTrue(TEXT("Readable inside the kill event, before the enemy is removed"), Scene.Listener->LastKilledDeathTransform.Equals(DeathSpot, 0.01f));
    TestTrue(TEXT("Still the death spot after removal"), Enemy->GetDeathTransform().Equals(DeathSpot, 0.01f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemyBaseEmergenceGatesBehaviourTest,
    "GrantMoney.Combat.EnemyBase.EmergenceGatesPursuitAndDamage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemyBaseEmergenceGatesBehaviourTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game) || !WorldWrapper.BeginPlayInTestWorld())
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AChaseTestEnemy* Chaser = World->SpawnActor<AChaseTestEnemy>(AChaseTestEnemy::StaticClass(), FTransform(FVector(0.0, 0.0, 50.0)), Params);

    // A plain actor with health: pawns refuse engine damage in a world without a Game Mode (see the note at the top).
    AActor* Victim = World->SpawnActor<AActor>();
    USceneComponent* VictimRoot = NewObject<USceneComponent>(Victim, TEXT("Root"));
    Victim->SetRootComponent(VictimRoot);
    VictimRoot->RegisterComponent();
    VictimRoot->SetWorldLocation(FVector(500.0, 0.0, 50.0));
    UHealthComponent* VictimHealth = NewObject<UHealthComponent>(Victim, TEXT("Health"));
    VictimHealth->RegisterComponent();

    if (!TestNotNull(TEXT("Chaser spawned"), Chaser))
    {
        return false;
    }
    Chaser->ChaseTarget = Victim;

    // An enemy nobody told to emerge behaves as it always did.
    TestFalse(TEXT("Not emerging by default"), Chaser->IsEmerging());
    TestTrue(TEXT("Targetable by default"), Chaser->CanBeTargeted());
    Chaser->Tick(0.1f);
    TestFalse(TEXT("An ordinary enemy walks toward its target"), Chaser->GetPendingMovementInputVector().IsNearlyZero());
    Chaser->ConsumeMovementInputVector();

    // Emerging: no pursuit, no damage.
    Chaser->GetEmergence()->Duration = 1.0f;
    Chaser->BeginEmergence();
    TestTrue(TEXT("Emerging after BeginEmergence"), Chaser->IsEmerging());
    TestFalse(TEXT("Not targetable while emerging"), Chaser->CanBeTargeted());
    Chaser->Tick(0.1f);
    TestTrue(TEXT("No walking while emerging"), Chaser->GetPendingMovementInputVector().IsNearlyZero());

    // Put the victim within reach: still no damage while emerging.
    VictimRoot->SetWorldLocation(FVector(60.0, 0.0, 50.0));
    TestTrue(TEXT("The victim is within reach"), Chaser->GetContactDamage()->IsInReach(Victim));
    Chaser->Tick(0.1f);
    TestEqual(TEXT("No contact damage while emerging"), VictimHealth->GetHealth(), VictimHealth->GetMaxHealth());

    // Finished: both switch on.
    Chaser->GetEmergence()->AdvanceEmergence(1.5f);
    TestFalse(TEXT("No longer emerging"), Chaser->IsEmerging());
    TestTrue(TEXT("Targetable again"), Chaser->CanBeTargeted());
    VictimRoot->SetWorldLocation(FVector(500.0, 0.0, 50.0));
    Chaser->Tick(0.1f);
    TestFalse(TEXT("Walks toward the target after emerging"), Chaser->GetPendingMovementInputVector().IsNearlyZero());
    VictimRoot->SetWorldLocation(FVector(60.0, 0.0, 50.0));
    Chaser->Tick(0.1f);
    TestTrue(TEXT("Contact damage works after emerging"), VictimHealth->GetHealth() < VictimHealth->GetMaxHealth());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemyBaseDeathDuringEmergenceTest,
    "GrantMoney.Combat.EnemyBase.DeathDuringEmergence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemyBaseDeathDuringEmergenceTest::RunTest(const FString& Parameters)
{
    EnemyTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    AEnemyBase* Enemy = Scene.Enemy;
    Enemy->DeathCleanupDelay = 2.0f;
    Enemy->BeginEmergence();
    TestFalse(TEXT("A base enemy has no visual, so emergence ends at once"), Enemy->IsEmerging());

    // Slimes have a visual, so they stay emerging until it is done.
    ASlimeEnemy* Slime = Scene.WorldWrapper.GetTestWorld()->SpawnActor<ASlimeEnemy>();
    if (!TestNotNull(TEXT("Slime spawned"), Slime))
    {
        return false;
    }

    Slime->DeathCleanupDelay = 2.0f;
    Slime->BeginEmergence();
    TestTrue(TEXT("Slime is emerging"), Slime->IsEmerging());
    UStaticMeshComponent* Mesh = Slime->FindComponentByClass<UStaticMeshComponent>();
    TestTrue(TEXT("Visual is flat"), Mesh && Mesh->GetRelativeScale3D().Z < 0.1);

    Slime->GetHealth()->ApplyDamage(1000.0f);
    TestFalse(TEXT("A dead slime is no longer emerging"), Slime->IsEmerging());
    TestFalse(TEXT("A dead slime cannot be targeted"), Slime->CanBeTargeted());
    TestTrue(TEXT("The corpse is back at its normal shape"), Mesh && Mesh->GetRelativeScale3D().Equals(FVector(0.6), 0.001));
    TestEqual(TEXT("Kill reported once"), Scene.Listener->EnemyKilledCount, 1);

    // Emerging a dead enemy does nothing.
    Slime->BeginEmergence();
    TestFalse(TEXT("A dead enemy cannot start emerging"), Slime->IsEmerging());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemyBaseTraversalLimitsTest,
    "GrantMoney.Combat.EnemyBase.TraversalLimitsApplyToMovement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemyBaseTraversalLimitsTest::RunTest(const FString& Parameters)
{
    // SCRUM-29: MaxStepHeight/MaxWalkableFloorAngle must reach the real movement component (not just sit
    // unused on the enemy), and climbing onto another character's capsule must be turned back on, since
    // ACharacter's own constructor turns it off by default.
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game) || !WorldWrapper.BeginPlayInTestWorld())
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    AChaseTestEnemy* Chaser = World->SpawnActor<AChaseTestEnemy>();
    if (!TestNotNull(TEXT("Chaser spawned"), Chaser))
    {
        return false;
    }

    TestTrue(TEXT("Another enemy's capsule can be stepped up onto (the engine default is No)"),
        Chaser->GetCapsuleComponent()->CanCharacterStepUpOn == ECB_Yes);

    // Non-default values, so this cannot pass by coincidentally matching the engine's own defaults.
    Chaser->MaxStepHeight = 123.0f;
    Chaser->MaxWalkableFloorAngle = 61.0f;

    AActor* Target = World->SpawnActor<AActor>();
    USceneComponent* TargetRoot = NewObject<USceneComponent>(Target, TEXT("Root"));
    Target->SetRootComponent(TargetRoot);
    TargetRoot->RegisterComponent();
    TargetRoot->SetWorldLocation(FVector(500.0, 0.0, 0.0));
    Chaser->ChaseTarget = Target;

    Chaser->Tick(0.1f);

    const UCharacterMovementComponent* Movement = Chaser->GetCharacterMovement();
    TestEqual(TEXT("MaxStepHeight reaches the movement component"), Movement->MaxStepHeight, 123.0f);
    TestEqual(TEXT("MaxWalkableFloorAngle reaches the movement component"), Movement->GetWalkableFloorAngle(), 61.0f, 0.01f);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
