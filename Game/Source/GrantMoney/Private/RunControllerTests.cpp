#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "RunController.h"
#include "Tests/AutomationCommon.h"
#include "Tests/RunTestListener.h"

namespace RunTest
{
    // An empty game world with play started, one run controller with a 10 second run and no start delay,
    // and a listener on its events. The run has not been started.
    struct FScene
    {
        FTestWorldWrapper WorldWrapper;
        ARunController* Run = nullptr;
        URunTestListener* Listener = nullptr;

        bool Create(FAutomationTestBase& Test, float DurationSeconds = 10.0f, float StartDelaySeconds = 0.0f)
        {
            if (!WorldWrapper.CreateTestWorld(EWorldType::Game) || !WorldWrapper.BeginPlayInTestWorld())
            {
                WorldWrapper.ForwardErrorMessages(&Test);
                return false;
            }

            Run = WorldWrapper.GetTestWorld()->SpawnActor<ARunController>();
            if (!Test.TestNotNull(TEXT("Run controller spawned"), Run))
            {
                return false;
            }

            Run->RunDurationSeconds = DurationSeconds;
            Run->StartDelaySeconds = StartDelaySeconds;

            Listener = NewObject<URunTestListener>();
            Run->OnStateChanged.AddDynamic(Listener, &URunTestListener::OnStateChanged);
            Run->OnTimeChanged.AddDynamic(Listener, &URunTestListener::OnTimeChanged);
            Run->OnBossPhaseStarted.AddDynamic(Listener, &URunTestListener::OnBossPhaseStarted);
            return true;
        }

        UWorld* World() const { return WorldWrapper.GetTestWorld(); }

        // Pauses or resumes the world the way the pause menu does: a player state holds the pause.
        void SetPaused(bool bPaused)
        {
            World()->GetWorldSettings()->SetPauserPlayerState(bPaused ? World()->SpawnActor<APlayerState>() : nullptr);
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRunControllerDefaultsTest,
    "GrantMoney.Run.RunController.Defaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRunControllerDefaultsTest::RunTest(const FString& Parameters)
{
    const ARunController* Defaults = GetDefault<ARunController>();
    TestEqual(TEXT("Run lasts 3 minutes"), Defaults->RunDurationSeconds, 180.0f);

    RunTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    TestTrue(TEXT("Starts in Starting"), Scene.Run->GetState() == EGrantMoneyRunState::Starting);
    TestFalse(TEXT("Not started"), Scene.Run->HasStarted());
    TestEqual(TEXT("No survival time"), Scene.Run->GetElapsedSeconds(), 0.0f);
    TestEqual(TEXT("Full time remaining"), Scene.Run->GetRemainingSeconds(), 10.0f);
    TestFalse(TEXT("No spawning while Starting"), Scene.Run->IsNormalSpawningAllowed());

    // Time does not pass until the run is started.
    Scene.Run->AdvanceRun(5.0f);
    TestTrue(TEXT("Still Starting without StartRun"), Scene.Run->GetState() == EGrantMoneyRunState::Starting);
    TestEqual(TEXT("Still no survival time"), Scene.Run->GetElapsedSeconds(), 0.0f);
    TestEqual(TEXT("No events"), Scene.Listener->Transitions.Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRunControllerStartDelayTest,
    "GrantMoney.Run.RunController.StartDelay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRunControllerStartDelayTest::RunTest(const FString& Parameters)
{
    {
        RunTest::FScene Scene;
        if (!Scene.Create(*this, 10.0f, 1.0f))
        {
            return false;
        }

        Scene.Run->StartRun();
        TestTrue(TEXT("Starting during the start delay"), Scene.Run->GetState() == EGrantMoneyRunState::Starting);
        TestTrue(TEXT("StartRun was recorded"), Scene.Run->HasStarted());

        Scene.Run->AdvanceRun(0.6f);
        TestTrue(TEXT("Still Starting at 0.6 s"), Scene.Run->GetState() == EGrantMoneyRunState::Starting);

        Scene.Run->AdvanceRun(0.6f);
        TestTrue(TEXT("Active after the delay"), Scene.Run->GetState() == EGrantMoneyRunState::Active);
        TestEqual(TEXT("The delay is not survival time"), Scene.Run->GetElapsedSeconds(), 0.0f);
        TestTrue(TEXT("Spawning allowed once Active"), Scene.Run->IsNormalSpawningAllowed());

        // A second StartRun changes nothing.
        Scene.Run->StartRun();
        TestEqual(TEXT("One transition so far"), Scene.Listener->Transitions.Num(), 1);
    }

    // With no delay the run is Active as soon as it is started.
    RunTest::FScene Instant;
    if (!Instant.Create(*this, 10.0f, 0.0f))
    {
        return false;
    }
    Instant.Run->StartRun();
    TestTrue(TEXT("No delay: Active immediately"), Instant.Run->GetState() == EGrantMoneyRunState::Active);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRunControllerClockTest,
    "GrantMoney.Run.RunController.Clock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRunControllerClockTest::RunTest(const FString& Parameters)
{
    RunTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    Scene.Run->StartRun();
    TestEqual(TEXT("The first time event reports the full duration"), Scene.Listener->LastRemainingSeconds, 10.0f);
    const int32 EventsAtStart = Scene.Listener->TimeChangedCount;

    Scene.Run->AdvanceRun(3.5f);
    TestEqual(TEXT("Elapsed"), Scene.Run->GetElapsedSeconds(), 3.5f);
    TestEqual(TEXT("Remaining is duration minus elapsed"), Scene.Run->GetRemainingSeconds(), 6.5f);
    TestEqual(TEXT("Clock rounds up"), ARunController::FormatClock(Scene.Run->GetRemainingSeconds()), FString(TEXT("0:07")));

    // The clock follows the editable duration.
    Scene.Run->RunDurationSeconds = 100.0f;
    TestEqual(TEXT("A longer run has more time left"), Scene.Run->GetRemainingSeconds(), 96.5f);
    Scene.Run->RunDurationSeconds = 10.0f;

    // Time events fire when the whole second on the clock changes, not every frame.
    const int32 EventsBefore = Scene.Listener->TimeChangedCount;
    Scene.Run->AdvanceRun(0.1f);
    Scene.Run->AdvanceRun(0.1f);
    TestEqual(TEXT("No event within the same second"), Scene.Listener->TimeChangedCount, EventsBefore);
    Scene.Run->AdvanceRun(0.5f);
    TestEqual(TEXT("An event when the clock drops a second"), Scene.Listener->TimeChangedCount, EventsBefore + 1);
    TestTrue(TEXT("More than the first event happened"), Scene.Listener->TimeChangedCount > EventsAtStart);

    TestEqual(TEXT("Clock format 3:00"), ARunController::FormatClock(180.0f), FString(TEXT("3:00")));
    TestEqual(TEXT("Clock format 0:00"), ARunController::FormatClock(0.0f), FString(TEXT("0:00")));
    TestEqual(TEXT("Clock format never negative"), ARunController::FormatClock(-4.0f), FString(TEXT("0:00")));
    TestEqual(TEXT("Clock format 1:05"), ARunController::FormatClock(64.2f), FString(TEXT("1:05")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRunControllerBossPhaseOnceTest,
    "GrantMoney.Run.RunController.BossPhaseOnce",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRunControllerBossPhaseOnceTest::RunTest(const FString& Parameters)
{
    RunTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    Scene.Run->StartRun();
    Scene.Run->AdvanceRun(9.9f);
    TestTrue(TEXT("Still Active just before zero"), Scene.Run->GetState() == EGrantMoneyRunState::Active);
    TestEqual(TEXT("No boss event yet"), Scene.Listener->BossPhaseStartedCount, 0);

    // One long step past zero still lands exactly on the end of the run.
    Scene.Run->AdvanceRun(5.0f);
    TestTrue(TEXT("BossPhase at zero"), Scene.Run->GetState() == EGrantMoneyRunState::BossPhase);
    TestEqual(TEXT("Remaining is exactly zero"), Scene.Run->GetRemainingSeconds(), 0.0f);
    TestEqual(TEXT("Elapsed stops at the duration"), Scene.Run->GetElapsedSeconds(), 10.0f);
    TestFalse(TEXT("Normal spawning stops"), Scene.Run->IsNormalSpawningAllowed());
    TestEqual(TEXT("Final clock reads 0:00"), ARunController::FormatClock(Scene.Run->GetRemainingSeconds()), FString(TEXT("0:00")));
    TestEqual(TEXT("Entered BossPhase once"), Scene.Listener->BossPhaseEntries, 1);
    TestEqual(TEXT("Boss event fired once"), Scene.Listener->BossPhaseStartedCount, 1);

    // More time, and a second StartRun, never repeat it.
    Scene.Run->AdvanceRun(30.0f);
    Scene.Run->StartRun();
    TestEqual(TEXT("Still one BossPhase entry"), Scene.Listener->BossPhaseEntries, 1);
    TestEqual(TEXT("Still one boss event"), Scene.Listener->BossPhaseStartedCount, 1);
    TestEqual(TEXT("The clock stays frozen"), Scene.Run->GetElapsedSeconds(), 10.0f);
    TestEqual(TEXT("Two transitions in the whole run"), Scene.Listener->Transitions.Num(), 2);
    TestTrue(TEXT("Starting to Active"), Scene.Listener->Transitions[0] == TPair<EGrantMoneyRunState, EGrantMoneyRunState>(EGrantMoneyRunState::Starting, EGrantMoneyRunState::Active));
    TestTrue(TEXT("Active to BossPhase"), Scene.Listener->Transitions[1] == TPair<EGrantMoneyRunState, EGrantMoneyRunState>(EGrantMoneyRunState::Active, EGrantMoneyRunState::BossPhase));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRunControllerEndStatesTest,
    "GrantMoney.Run.RunController.WinAndLoss",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRunControllerEndStatesTest::RunTest(const FString& Parameters)
{
    {
        RunTest::FScene Scene;
        if (!Scene.Create(*this))
        {
            return false;
        }

        TestFalse(TEXT("Cannot lose before the run starts"), Scene.Run->RequestLoss());
        TestFalse(TEXT("Cannot win before the run starts"), Scene.Run->RequestWin());

        Scene.Run->StartRun();
        TestFalse(TEXT("Cannot win while Active: the boss must come first"), Scene.Run->RequestWin());
        TestTrue(TEXT("Still Active"), Scene.Run->GetState() == EGrantMoneyRunState::Active);

        Scene.Run->AdvanceRun(10.0f);
        TestTrue(TEXT("BossPhase"), Scene.Run->GetState() == EGrantMoneyRunState::BossPhase);
        TestTrue(TEXT("Win from BossPhase"), Scene.Run->RequestWin());
        TestTrue(TEXT("Won"), Scene.Run->GetState() == EGrantMoneyRunState::Won);
        TestFalse(TEXT("Cannot lose after winning"), Scene.Run->RequestLoss());
        TestFalse(TEXT("Cannot win twice"), Scene.Run->RequestWin());
        Scene.Run->AdvanceRun(5.0f);
        TestTrue(TEXT("Won is final"), Scene.Run->GetState() == EGrantMoneyRunState::Won);
    }

    // Loss from Active.
    RunTest::FScene Lost;
    if (!Lost.Create(*this))
    {
        return false;
    }
    Lost.Run->StartRun();
    Lost.Run->AdvanceRun(2.0f);
    TestTrue(TEXT("Lose from Active"), Lost.Run->RequestLoss());
    TestTrue(TEXT("Lost"), Lost.Run->GetState() == EGrantMoneyRunState::Lost);
    TestFalse(TEXT("Lost is final"), Lost.Run->RequestLoss());
    Lost.Run->AdvanceRun(20.0f);
    TestEqual(TEXT("The clock stops when the run is lost"), Lost.Run->GetElapsedSeconds(), 2.0f);
    TestEqual(TEXT("No boss phase after a loss"), Lost.Listener->BossPhaseEntries, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRunControllerPauseTest,
    "GrantMoney.Run.RunController.PauseFreezesTime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRunControllerPauseTest::RunTest(const FString& Parameters)
{
    RunTest::FScene Scene;
    if (!Scene.Create(*this, 100.0f))
    {
        return false;
    }

    Scene.Run->StartRun();

    // Real world ticks move the clock.
    Scene.WorldWrapper.TickTestWorld(0.1f);
    Scene.WorldWrapper.TickTestWorld(0.1f);
    const float BeforePause = Scene.Run->GetElapsedSeconds();
    TestTrue(TEXT("The clock runs while the game runs"), BeforePause > 0.15f);

    Scene.SetPaused(true);
    TestTrue(TEXT("The test world reports paused"), Scene.World()->IsPaused());

    Scene.WorldWrapper.TickTestWorld(5.0f);
    TestEqual(TEXT("A paused world does not advance the clock"), Scene.Run->GetElapsedSeconds(), BeforePause);

    // Even a direct tick (something ticking us while paused) must not move it.
    Scene.Run->Tick(5.0f);
    TestEqual(TEXT("Ticking while paused does not advance the clock"), Scene.Run->GetElapsedSeconds(), BeforePause);
    TestEqual(TEXT("No state change while paused"), Scene.Listener->Transitions.Num(), 1);

    Scene.SetPaused(false);
    Scene.Run->Tick(2.0f);
    TestEqual(TEXT("The clock resumes from where it stopped"), Scene.Run->GetElapsedSeconds(), BeforePause + 2.0f, 0.001f);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
