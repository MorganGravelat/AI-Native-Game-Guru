#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "BossEncounter.h"
#include "BossSlimeEnemy.h"
#include "Components/BoxComponent.h"
#include "EnemyBase.h"
#include "EnemySpawner.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GrantMoneyPlayerCharacter.h"
#include "HealthComponent.h"
#include "RunController.h"
#include "SlimeEnemy.h"
#include "StarterWeaponComponent.h"
#include "Tests/AutomationCommon.h"
#include "Tests/BossTestTypes.h"
#include "Tests/RunTestListener.h"

namespace BossEncounterTest
{
    const FVector PlayerLocation(1000.0, 1000.0, 100.0);

    // A game world with play started and everything the boss fight needs, wired as the game mode wires it:
    // a synthetic 20 x 20 cell map (world X and Y from -50 to 1950) with a ground slab under it, a real
    // player character (frozen, weapon disarmed) whose death ends the run, a 10 second run that is Active,
    // a spawner that validates locations on that map, and an encounter bound to the run. The boss appears
    // 300..700 cm from the player, which is always on the map. Nothing ticks unless a test steps it.
    struct FScene
    {
        FTestWorldWrapper WorldWrapper;
        BossTest::FTestArena Map;
        AGrantMoneyPlayerCharacter* Player = nullptr;
        ARunController* Run = nullptr;
        AEnemySpawner* Spawner = nullptr;
        ABossEncounter* Encounter = nullptr;
        URunTestListener* Listener = nullptr;

        bool Create(FAutomationTestBase& Test)
        {
            if (!WorldWrapper.CreateTestWorld(EWorldType::Game) || !WorldWrapper.BeginPlayInTestWorld())
            {
                WorldWrapper.ForwardErrorMessages(&Test);
                return false;
            }

            Map.Init(FIntPoint(0, 0), FIntPoint(19, 19));

            // The ground slab the spawner's downward trace needs (a plain blocking box).
            AActor* Ground = World()->SpawnActor<AActor>();
            UBoxComponent* Box = NewObject<UBoxComponent>(Ground, TEXT("Ground"));
            Box->SetBoxExtent(FVector(1000.0, 1000.0, 10.0));
            Ground->SetRootComponent(Box);
            Box->SetCollisionProfileName(TEXT("BlockAll"));
            Box->RegisterComponent();
            Box->SetWorldLocation(FVector(950.0, 950.0, -10.0));

            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            Player = World()->SpawnActor<AGrantMoneyPlayerCharacter>(AGrantMoneyPlayerCharacter::StaticClass(), FTransform(PlayerLocation), Params);
            Run = World()->SpawnActor<ARunController>();
            Spawner = World()->SpawnActor<AEnemySpawner>();
            Encounter = World()->SpawnActor<ABossEncounter>();
            if (!Test.TestNotNull(TEXT("Player spawned"), Player) || !Test.TestNotNull(TEXT("Run controller spawned"), Run)
                || !Test.TestNotNull(TEXT("Spawner spawned"), Spawner) || !Test.TestNotNull(TEXT("Encounter spawned"), Encounter))
            {
                return false;
            }

            Player->GetCharacterMovement()->DisableMovement();
            Player->GetStarterWeapon()->TargetRange = 0.0f;

            Run->StartDelaySeconds = 0.0f;
            Run->RunDurationSeconds = 10.0f;
            Run->SetPlayerHealthComponent(Player->GetHealth());

            Spawner->SetArenaOverride(Map.Arena);
            Spawner->SetCentreActor(Player);
            Spawner->SetRandomSeed(2026);
            Spawner->SetRunController(Run);
            Spawner->MinSpawnDistance = 300.0f;
            Spawner->MaxSpawnDistance = 600.0f;

            Encounter->MinSpawnDistance = 300.0f;
            Encounter->MaxSpawnDistance = 700.0f;
            Encounter->SetSpawner(Spawner);
            Encounter->BindToRunController(Run);

            Listener = NewObject<URunTestListener>();
            Run->OnStateChanged.AddDynamic(Listener, &URunTestListener::OnStateChanged);

            Run->StartRun();
            return Test.TestTrue(TEXT("Run is Active"), Run->GetState() == EGrantMoneyRunState::Active);
        }

        UWorld* World() const { return WorldWrapper.GetTestWorld(); }

        // Runs the survival clock out, which enters BossPhase and starts the encounter.
        void EnterBossPhase()
        {
            Run->AdvanceRun(Run->RunDurationSeconds);
        }

        // Makes every placement attempt fail: the ring lies far outside the map.
        void MakePlacementImpossible()
        {
            Encounter->MinSpawnDistance = 5000.0f;
            Encounter->MaxSpawnDistance = 5000.0f;
        }

        int32 CountTransitionsTo(EGrantMoneyRunState State) const
        {
            int32 Count = 0;
            for (const TPair<EGrantMoneyRunState, EGrantMoneyRunState>& Transition : Listener->Transitions)
            {
                Count += Transition.Value == State ? 1 : 0;
            }
            return Count;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBossEncounterDefaultsTest,
    "GrantMoney.Boss.Encounter.Defaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossEncounterDefaultsTest::RunTest(const FString& Parameters)
{
    const ABossEncounter* Defaults = GetDefault<ABossEncounter>();
    TestTrue(TEXT("Spawns the slime boss by default"), Defaults->BossClass == ABossSlimeEnemy::StaticClass());
    TestTrue(TEXT("At least one spawn attempt"), Defaults->MaxSpawnAttempts >= 1);
    TestTrue(TEXT("A positive retry interval"), Defaults->SpawnRetryIntervalSeconds > 0.0f);
    TestTrue(TEXT("Normal spawning stops during the boss phase by default"), !GetDefault<ARunController>()->bAllowNormalSpawnsInBossPhase);

    BossEncounterTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    TestTrue(TEXT("Waiting for the boss phase"), Scene.Encounter->GetState() == EBossEncounterState::Waiting);
    TestEqual(TEXT("No boss before the boss phase"), Scene.Encounter->GetBossSpawnCount(), 0);
    TestTrue(TEXT("No boss object"), Scene.Encounter->GetBoss() == nullptr);
    TestTrue(TEXT("No arrival message yet"), !Scene.Encounter->IsArrivalMessageVisible());
    TestTrue(TEXT("No diagnostic yet"), Scene.Encounter->GetSpawnDiagnostic().IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBossEncounterSpawnsOneBossTest,
    "GrantMoney.Boss.Encounter.SpawnsExactlyOneBossAtValidLocation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossEncounterSpawnsOneBossTest::RunTest(const FString& Parameters)
{
    BossEncounterTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    // Nothing yet while the run is Active, even if the encounter is poked.
    Scene.Encounter->AttemptSpawn();
    TestEqual(TEXT("No boss while the run is Active"), Scene.Encounter->GetBossSpawnCount(), 0);

    Scene.EnterBossPhase();
    TestTrue(TEXT("The run is in BossPhase"), Scene.Run->GetState() == EGrantMoneyRunState::BossPhase);
    TestTrue(TEXT("The boss is fighting"), Scene.Encounter->GetState() == EBossEncounterState::Fighting);
    TestEqual(TEXT("Exactly one boss was spawned"), Scene.Encounter->GetBossSpawnCount(), 1);
    TestEqual(TEXT("On the first attempt"), Scene.Encounter->GetSpawnAttempts(), 1);

    AEnemyBase* Boss = Scene.Encounter->GetBoss();
    if (!TestNotNull(TEXT("The boss exists"), Boss))
    {
        return false;
    }

    TestTrue(TEXT("It is the configured boss class"), Boss->IsA(ABossSlimeEnemy::StaticClass()));

    const FVector BossLocation = Boss->GetActorLocation();
    const float DistanceToPlayer = static_cast<float>(FVector::Dist2D(BossLocation, BossEncounterTest::PlayerLocation));
    TestTrue(TEXT("Placed inside the requested ring around the player"), DistanceToPlayer >= 299.0f && DistanceToPlayer <= 701.0f);
    TestTrue(TEXT("The whole capsule is on playable cells"), Scene.Map.Arena.IsFootprintPlayable(BossLocation, Boss->GetSimpleCollisionRadius()));
    TestTrue(TEXT("It starts emerging like any spawned enemy"), Boss->IsEmerging());
    TestTrue(TEXT("It cannot be targeted while emerging"), !Boss->CanBeTargeted());

    // Shared health data for the bar.
    TestEqual(TEXT("The health bar starts full"), Boss->GetHealth()->GetHealthFraction(), 1.0f);
    TestTrue(TEXT("The arrival message shows"), Scene.Encounter->IsArrivalMessageVisible());
    Scene.Encounter->ArrivalMessageSeconds = 0.0f;
    TestTrue(TEXT("The arrival message is time-limited"), !Scene.Encounter->IsArrivalMessageVisible());
    Scene.Encounter->ArrivalMessageSeconds = 4.0f;

    // Never a second boss, however the encounter is poked.
    Scene.Encounter->AttemptSpawn();
    Scene.Encounter->BindToRunController(Scene.Run);
    TestEqual(TEXT("Still exactly one boss"), Scene.Encounter->GetBossSpawnCount(), 1);

    Boss->GetHealth()->ApplyDamage(75.0f, Scene.Player);
    TestTrue(TEXT("Damage lowers the bar"), Boss->GetHealth()->GetHealthFraction() < 1.0f);
    TestTrue(TEXT("The boss is still the same actor"), Scene.Encounter->GetBoss() == Boss);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBossEncounterKeepsSlimesStopsSpawnsTest,
    "GrantMoney.Boss.Encounter.KeepsSlimesAndStopsNormalSpawns",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossEncounterKeepsSlimesStopsSpawnsTest::RunTest(const FString& Parameters)
{
    BossEncounterTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    for (int32 Try = 0; Try < 12 && Scene.Spawner->GetActiveEnemyCount() < 3; ++Try)
    {
        Scene.Spawner->TrySpawnEnemy();
    }

    const int32 SlimesBefore = Scene.Spawner->GetActiveEnemyCount();
    TestTrue(TEXT("Some normal slimes are alive"), SlimesBefore >= 1);
    TestTrue(TEXT("Normal spawning is on while Active"), Scene.Spawner->IsSpawningAllowed());

    Scene.EnterBossPhase();
    TestTrue(TEXT("The boss spawned"), Scene.Encounter->GetBoss() != nullptr);
    TestEqual(TEXT("Existing normal slimes stay alive"), Scene.Spawner->GetActiveEnemyCount(), SlimesBefore);
    TestFalse(TEXT("The boss does not count against the normal cap"), Scene.Encounter->GetBoss() && Scene.Spawner->GetActiveEnemyCount() > SlimesBefore);

    TestFalse(TEXT("No new normal spawns in BossPhase"), Scene.Spawner->IsSpawningAllowed());
    Scene.Spawner->AdvanceSpawning(100.0f);
    TestEqual(TEXT("A long wait spawns nothing"), Scene.Spawner->GetActiveEnemyCount(), SlimesBefore);

    // The policy is configurable.
    Scene.Run->bAllowNormalSpawnsInBossPhase = true;
    TestTrue(TEXT("The policy can keep normal spawning on"), Scene.Spawner->IsSpawningAllowed());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBossEncounterDeathWinsOnceTest,
    "GrantMoney.Boss.Encounter.BossDeathWinsExactlyOnce",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossEncounterDeathWinsOnceTest::RunTest(const FString& Parameters)
{
    BossEncounterTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    Scene.EnterBossPhase();
    AEnemyBase* Boss = Scene.Encounter->GetBoss();
    if (!TestNotNull(TEXT("The boss exists"), Boss))
    {
        return false;
    }

    UHealthComponent* BossHealth = Boss->GetHealth();
    BossHealth->ApplyDamage(10.0f, Scene.Player);
    TestTrue(TEXT("Damage alone does not end the run"), Scene.Run->GetState() == EGrantMoneyRunState::BossPhase);

    BossHealth->ApplyDamage(100000.0f, Scene.Player);
    TestTrue(TEXT("The boss is dead"), BossHealth->IsDead());
    TestTrue(TEXT("The run is Won"), Scene.Run->GetState() == EGrantMoneyRunState::Won);
    TestTrue(TEXT("The encounter is Defeated"), Scene.Encounter->GetState() == EBossEncounterState::Defeated);
    TestEqual(TEXT("Won was entered once"), Scene.CountTransitionsTo(EGrantMoneyRunState::Won), 1);

    // Late events change nothing.
    BossHealth->ApplyDamage(10.0f, Scene.Player);
    Scene.Encounter->AttemptSpawn();
    Scene.Encounter->BindToRunController(Scene.Run);
    TestEqual(TEXT("Still exactly one Won transition"), Scene.CountTransitionsTo(EGrantMoneyRunState::Won), 1);
    TestEqual(TEXT("No second boss"), Scene.Encounter->GetBossSpawnCount(), 1);
    TestTrue(TEXT("Won is final"), Scene.Run->GetState() == EGrantMoneyRunState::Won);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBossEncounterPlayerDeathDuringFightTest,
    "GrantMoney.Boss.Encounter.PlayerDeathDuringFightEndsRunAsLost",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossEncounterPlayerDeathDuringFightTest::RunTest(const FString& Parameters)
{
    BossEncounterTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    Scene.EnterBossPhase();
    AEnemyBase* Boss = Scene.Encounter->GetBoss();
    if (!TestNotNull(TEXT("The boss exists"), Boss))
    {
        return false;
    }

    // The real player death handler asks the run to end.
    Scene.Player->GetHealth()->ApplyDamage(100000.0f, Boss);
    TestTrue(TEXT("The run is Lost"), Scene.Run->GetState() == EGrantMoneyRunState::Lost);
    TestEqual(TEXT("Lost was entered once"), Scene.CountTransitionsTo(EGrantMoneyRunState::Lost), 1);
    TestTrue(TEXT("The encounter stops caring"), Scene.Encounter->GetState() == EBossEncounterState::Cancelled);

    // The boss dying afterwards (for example a late projectile hit) cannot turn a loss into a win.
    Boss->GetHealth()->ApplyDamage(100000.0f, Scene.Player);
    TestTrue(TEXT("Lost stays Lost"), Scene.Run->GetState() == EGrantMoneyRunState::Lost);
    TestEqual(TEXT("Never Won"), Scene.CountTransitionsTo(EGrantMoneyRunState::Won), 0);
    TestTrue(TEXT("The encounter did not move to Defeated"), Scene.Encounter->GetState() == EBossEncounterState::Cancelled);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBossEncounterSpawnFailureTest,
    "GrantMoney.Boss.Encounter.SpawnFailureRetriesBoundedWithDiagnostic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossEncounterSpawnFailureTest::RunTest(const FString& Parameters)
{
    // The final failure is logged as an error on purpose, so that it is visible in the log.
    AddExpectedError(TEXT("the boss could not be spawned"), EAutomationExpectedErrorFlags::Contains, 1);

    BossEncounterTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    Scene.MakePlacementImpossible();
    Scene.Encounter->MaxSpawnAttempts = 3;

    Scene.EnterBossPhase();
    TestTrue(TEXT("Still trying after the first failure"), Scene.Encounter->GetState() == EBossEncounterState::Spawning);
    TestEqual(TEXT("One attempt so far"), Scene.Encounter->GetSpawnAttempts(), 1);
    TestEqual(TEXT("No boss"), Scene.Encounter->GetBossSpawnCount(), 0);
    TestTrue(TEXT("The failure is visible"), Scene.Encounter->GetSpawnDiagnostic().Contains(TEXT("attempt 1 of 3")));
    TestTrue(TEXT("The run is not stuck silently: it is still in BossPhase with a message"), Scene.Run->GetState() == EGrantMoneyRunState::BossPhase);

    Scene.Encounter->AttemptSpawn();
    TestEqual(TEXT("Second attempt"), Scene.Encounter->GetSpawnAttempts(), 2);
    TestTrue(TEXT("Still retrying"), Scene.Encounter->GetState() == EBossEncounterState::Spawning);

    Scene.Encounter->AttemptSpawn();
    TestEqual(TEXT("Third attempt"), Scene.Encounter->GetSpawnAttempts(), 3);
    TestTrue(TEXT("Out of attempts: Failed"), Scene.Encounter->GetState() == EBossEncounterState::Failed);
    TestTrue(TEXT("The final diagnostic says so"), Scene.Encounter->GetSpawnDiagnostic().Contains(TEXT("FAILED")));

    // Bounded: no further attempts, no boss, and the search itself is bounded too.
    Scene.Encounter->AttemptSpawn();
    TestEqual(TEXT("No attempts beyond the limit"), Scene.Encounter->GetSpawnAttempts(), 3);
    TestEqual(TEXT("Every search is bounded by the spawner's attempt limit"), Scene.Spawner->GetPlacementAttemptCount(), 3 * Scene.Spawner->MaxPlacementAttempts);
    TestEqual(TEXT("Still no boss"), Scene.Encounter->GetBossSpawnCount(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBossEncounterRetrySucceedsTest,
    "GrantMoney.Boss.Encounter.RetrySucceedsAfterFailure",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossEncounterRetrySucceedsTest::RunTest(const FString& Parameters)
{
    BossEncounterTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    Scene.MakePlacementImpossible();
    Scene.EnterBossPhase();
    TestTrue(TEXT("The first attempt failed"), Scene.Encounter->GetState() == EBossEncounterState::Spawning);
    TestFalse(TEXT("A failure message is showing"), Scene.Encounter->GetSpawnDiagnostic().IsEmpty());

    // A later attempt can find room (here the ring is moved back onto the map).
    Scene.Encounter->MinSpawnDistance = 300.0f;
    Scene.Encounter->MaxSpawnDistance = 700.0f;
    Scene.Encounter->AttemptSpawn();
    TestTrue(TEXT("The retry spawned the boss"), Scene.Encounter->GetState() == EBossEncounterState::Fighting);
    TestEqual(TEXT("On the second attempt"), Scene.Encounter->GetSpawnAttempts(), 2);
    TestEqual(TEXT("Exactly one boss"), Scene.Encounter->GetBossSpawnCount(), 1);
    TestTrue(TEXT("The failure message is gone"), Scene.Encounter->GetSpawnDiagnostic().IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBossEncounterLateSpawnIgnoredTest,
    "GrantMoney.Boss.Encounter.LateSpawnCannotChangeTerminalRun",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossEncounterLateSpawnIgnoredTest::RunTest(const FString& Parameters)
{
    BossEncounterTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    // A retry is pending when the player dies.
    Scene.MakePlacementImpossible();
    Scene.EnterBossPhase();
    TestTrue(TEXT("A retry is pending"), Scene.Encounter->GetState() == EBossEncounterState::Spawning);

    Scene.Player->GetHealth()->ApplyDamage(100000.0f);
    TestTrue(TEXT("The run is Lost"), Scene.Run->GetState() == EGrantMoneyRunState::Lost);
    TestTrue(TEXT("The pending spawn is cancelled"), Scene.Encounter->GetState() == EBossEncounterState::Cancelled);

    // The late callback (even with room available now) must not create a boss in a finished run.
    Scene.Encounter->MinSpawnDistance = 300.0f;
    Scene.Encounter->MaxSpawnDistance = 700.0f;
    Scene.Encounter->AttemptSpawn();
    TestEqual(TEXT("No boss in a lost run"), Scene.Encounter->GetBossSpawnCount(), 0);
    TestTrue(TEXT("Still Lost"), Scene.Run->GetState() == EGrantMoneyRunState::Lost);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
