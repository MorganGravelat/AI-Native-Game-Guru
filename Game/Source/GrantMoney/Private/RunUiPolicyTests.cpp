#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "RunUiPolicy.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRunUiPolicyTerminalStatesTest,
    "GrantMoney.UI.RunUiPolicy.OnlyWonAndLostAreTerminal",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRunUiPolicyTerminalStatesTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("Starting"), FRunUiPolicy::IsTerminal(EGrantMoneyRunState::Starting));
    TestFalse(TEXT("Active"), FRunUiPolicy::IsTerminal(EGrantMoneyRunState::Active));
    TestFalse(TEXT("BossPhase"), FRunUiPolicy::IsTerminal(EGrantMoneyRunState::BossPhase));
    TestTrue(TEXT("Won"), FRunUiPolicy::IsTerminal(EGrantMoneyRunState::Won));
    TestTrue(TEXT("Lost"), FRunUiPolicy::IsTerminal(EGrantMoneyRunState::Lost));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRunUiPolicyFinishedRunTest,
    "GrantMoney.UI.RunUiPolicy.FinishedRunAllowsNoPauseMenuNoUpgradesNoResume",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRunUiPolicyFinishedRunTest::RunTest(const FString& Parameters)
{
    // The whole point: a finished run can never be reopened into upgrade selection or resumed through the pause menu.
    TestFalse(TEXT("No pause menu after the run"), FRunUiPolicy::CanOpenPauseMenu(true, false));
    TestFalse(TEXT("No pause menu after the run, even with a level-up showing"), FRunUiPolicy::CanOpenPauseMenu(true, true));
    TestFalse(TEXT("No upgrade choice after the run"), FRunUiPolicy::CanShowLevelUp(true));
    TestFalse(TEXT("The pause menu cannot resume a finished run"), FRunUiPolicy::CanResumeFromPauseMenu(true, false));
    TestFalse(TEXT("Closing a level-up does not return to gameplay after the run"), FRunUiPolicy::ShouldReturnToGameplayAfterLevelUp(true));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRunUiPolicyRunningTest,
    "GrantMoney.UI.RunUiPolicy.RunningGameKeepsNormalFlow",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRunUiPolicyRunningTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Pause menu opens while playing"), FRunUiPolicy::CanOpenPauseMenu(false, false));
    TestFalse(TEXT("Pause menu cannot open over a forced level-up choice"), FRunUiPolicy::CanOpenPauseMenu(false, true));
    TestTrue(TEXT("Level-up choice shows while playing"), FRunUiPolicy::CanShowLevelUp(false));
    TestTrue(TEXT("Resume works from the pause menu"), FRunUiPolicy::CanResumeFromPauseMenu(false, false));
    TestFalse(TEXT("Resume must not unpause a level-up choice"), FRunUiPolicy::CanResumeFromPauseMenu(false, true));
    TestTrue(TEXT("Finishing the last level-up returns to gameplay"), FRunUiPolicy::ShouldReturnToGameplayAfterLevelUp(false));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
