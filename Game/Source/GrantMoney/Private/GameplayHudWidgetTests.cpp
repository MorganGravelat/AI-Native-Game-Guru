#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayHudWidget.h"
#include "GrantMoneyPlayerCharacter.h"
#include "HealthComponent.h"
#include "LevelComponent.h"
#include "RunController.h"
#include "RunHudFormat.h"
#include "StarterWeaponComponent.h"
#include "StatsComponent.h"
#include "Tests/AutomationCommon.h"
#include "TemporaryBuffComponent.h"
#include "UpgradeComponent.h"

// The HUD's binding and values, end to end with real players and a real run controller. Only the on-screen
// layout is left to the Editor. The widget is created as a plain object and initialised by hand: no viewport
// or Slate is needed to read what it would show.

namespace GameplayHudTest
{
    struct FScene
    {
        FTestWorldWrapper WorldWrapper;
        AGrantMoneyPlayerCharacter* Player = nullptr;
        ARunController* Run = nullptr;
        UGameplayHudWidget* Hud = nullptr;

        bool Create(FAutomationTestBase& Test)
        {
            if (!WorldWrapper.CreateTestWorld(EWorldType::Game) || !WorldWrapper.BeginPlayInTestWorld())
            {
                WorldWrapper.ForwardErrorMessages(&Test);
                return false;
            }

            Player = SpawnPlayer();
            Run = World()->SpawnActor<ARunController>();
            if (!Test.TestNotNull(TEXT("Player spawned"), Player) || !Test.TestNotNull(TEXT("Run controller spawned"), Run))
            {
                return false;
            }

            Run->StartDelaySeconds = 0.0f;
            Run->RunDurationSeconds = 180.0f;
            Run->StartRun();

            Hud = NewObject<UGameplayHudWidget>(GetTransientPackage());
            Hud->Initialize();
            Hud->BindRun(Run);
            Hud->BindPlayer(Player);
            return true;
        }

        // Frozen and disarmed so the capsule neither falls nor shoots anything.
        AGrantMoneyPlayerCharacter* SpawnPlayer() const
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AGrantMoneyPlayerCharacter* Spawned = World()->SpawnActor<AGrantMoneyPlayerCharacter>(AGrantMoneyPlayerCharacter::StaticClass(), FTransform::Identity, Params);
            if (Spawned)
            {
                Spawned->GetCharacterMovement()->DisableMovement();
                Spawned->GetStarterWeapon()->TargetRange = 0.0f;
            }

            return Spawned;
        }

        static FString ExpectedHealth(const AGrantMoneyPlayerCharacter* For)
        {
            const UHealthComponent* Health = For->GetHealth();
            return FString::Printf(TEXT("HP  %s"), *FRunHudFormat::FormatHealth(Health->GetHealth(), Health->GetMaxHealth()));
        }

        UWorld* World() const { return WorldWrapper.GetTestWorld(); }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGameplayHudInitialValuesTest,
    "GrantMoney.UI.GameplayHud.BindingShowsCurrentValuesAtOnce",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGameplayHudInitialValuesTest::RunTest(const FString& Parameters)
{
    GameplayHudTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    // Nothing has changed since binding, so these can only come from the initial read.
    TestEqual(TEXT("Health"), Scene.Hud->GetHealthText(), GameplayHudTest::FScene::ExpectedHealth(Scene.Player));
    TestEqual(TEXT("Health bar is full"), Scene.Hud->GetHealthPercent(), 1.0f);
    TestEqual(TEXT("Level"), Scene.Hud->GetLevelText(), FString(TEXT("Level 1")));
    TestEqual(TEXT("XP bar starts empty"), Scene.Hud->GetXpPercent(), 0.0f);
    TestEqual(TEXT("Clock"), Scene.Hud->GetTimerText(), ARunController::FormatClock(Scene.Run->GetRemainingSeconds()));
    TestEqual(TEXT("Phase"), Scene.Hud->GetPhaseText(), FRunHudFormat::FormatPhase(EGrantMoneyRunState::Active, false));
    TestEqual(TEXT("Score"), Scene.Hud->GetScoreText(), FString(TEXT("0")));
    TestEqual(TEXT("No buffs listed"), Scene.Hud->GetVisibleBuffRowCount(), 0);
    TestEqual(TEXT("No upgrades listed"), Scene.Hud->GetVisibleUpgradeRowCount(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGameplayHudEventsTest,
    "GrantMoney.UI.GameplayHud.EventsUpdateHealthXpClockAndScore",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGameplayHudEventsTest::RunTest(const FString& Parameters)
{
    GameplayHudTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    UHealthComponent* Health = Scene.Player->GetHealth();
    Health->ApplyDamage(30.0f);
    TestEqual(TEXT("Health follows damage"), Scene.Hud->GetHealthText(), GameplayHudTest::FScene::ExpectedHealth(Scene.Player));
    TestEqual(TEXT("Health bar follows damage"), Scene.Hud->GetHealthPercent(), Health->GetHealthFraction());
    TestTrue(TEXT("Health bar is below full"), Scene.Hud->GetHealthPercent() < 1.0f);

    // A max-health change that moves current health is also reflected.
    Scene.Player->GetStats()->AddModifier(EGrantMoneyStat::MaxHealth, EGrantMoneyStatModifierOp::Additive, 50.0f);
    TestEqual(TEXT("Health follows max health"), Scene.Hud->GetHealthText(), GameplayHudTest::FScene::ExpectedHealth(Scene.Player));

    // 12 XP crosses the first threshold (10): level 2 with 2 XP into it.
    UStatsComponent* Stats = Scene.Player->GetStats();
    Stats->SetBaseStat(EGrantMoneyStat::Experience, Stats->GetBaseStat(EGrantMoneyStat::Experience) + 12.0f);
    const ULevelComponent* Level = Scene.Player->GetLevelComponent();
    TestEqual(TEXT("Level"), Scene.Hud->GetLevelText(), FRunHudFormat::FormatLevel(Level->GetLevel()));
    TestEqual(TEXT("Level went up"), Level->GetLevel(), 2);
    TestEqual(TEXT("XP text"), Scene.Hud->GetXpText(), FRunHudFormat::FormatXp(Level->GetXpIntoLevel(), Level->GetXpToNextLevel()));
    TestEqual(TEXT("XP bar"), Scene.Hud->GetXpPercent(), Level->GetProgressFraction());
    TestEqual(TEXT("Score counts XP: 12 * 10"), Scene.Hud->GetScoreText(), FString(TEXT("120")));

    // Three seconds of survival: the clock and the score move on the run's own time event.
    Scene.Run->AdvanceRun(3.0f);
    TestEqual(TEXT("Clock"), Scene.Hud->GetTimerText(), ARunController::FormatClock(Scene.Run->GetRemainingSeconds()));
    TestEqual(TEXT("Score counts time: 120 + 3 * 5"), Scene.Hud->GetScoreText(), FString(TEXT("135")));

    // The boss phase is announced through the state event.
    Scene.Run->AdvanceRun(Scene.Run->RunDurationSeconds);
    TestEqual(TEXT("Phase follows the run"), Scene.Hud->GetPhaseText(), FRunHudFormat::FormatPhase(EGrantMoneyRunState::BossPhase, false));
    TestEqual(TEXT("Clock reads zero"), Scene.Hud->GetTimerText(), FString(TEXT("0:00")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGameplayHudBuffsAndUpgradesTest,
    "GrantMoney.UI.GameplayHud.ListsBuffsWithTimeAndUpgradesTaken",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGameplayHudBuffsAndUpgradesTest::RunTest(const FString& Parameters)
{
    GameplayHudTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    UTemporaryBuffComponent* Buffs = Scene.Player->GetTemporaryBuffs();
    Buffs->AddBuff(TEXT("Surge"), FText::FromString(TEXT("Speed Surge")), EGrantMoneyStat::MoveSpeed, EGrantMoneyStatModifierOp::Multiplicative, 0.5f, 12.0f);
    TestEqual(TEXT("One buff row"), Scene.Hud->GetVisibleBuffRowCount(), 1);
    TestEqual(TEXT("Name and time"), Scene.Hud->GetBuffRowText(0), FString(TEXT("Speed Surge  12s")));

    // Time passing raises no event; the widget's tick re-reads it, which RefreshAll stands in for here.
    Buffs->AdvanceBuffs(4.0f);
    Scene.Hud->RefreshAll();
    TestEqual(TEXT("Remaining time counts down"), Scene.Hud->GetBuffRowText(0), FString(TEXT("Speed Surge  8.0s")));

    Buffs->AddBuff(TEXT("Shield"), FText::FromString(TEXT("Shield")), EGrantMoneyStat::MaxHealth, EGrantMoneyStatModifierOp::Additive, 20.0f, 30.0f);
    TestEqual(TEXT("Two buff rows"), Scene.Hud->GetVisibleBuffRowCount(), 2);

    Buffs->AdvanceBuffs(10.0f);
    TestEqual(TEXT("The expired buff's row is gone"), Scene.Hud->GetVisibleBuffRowCount(), 1);
    TestTrue(TEXT("The remaining row is the other buff"), Scene.Hud->GetBuffRowText(0).StartsWith(TEXT("Shield")));

    Buffs->ClearBuffs();
    TestEqual(TEXT("No buffs, no rows"), Scene.Hud->GetVisibleBuffRowCount(), 0);

    // Upgrades: level up, take the first offered option, and it is listed.
    UStatsComponent* Stats = Scene.Player->GetStats();
    Stats->SetBaseStat(EGrantMoneyStat::Experience, Stats->GetBaseStat(EGrantMoneyStat::Experience) + 10.0f);
    UUpgradeComponent* Upgrades = Scene.Player->GetUpgradeComponent();
    if (!TestTrue(TEXT("A choice is waiting"), Upgrades->IsChoiceActive()))
    {
        return false;
    }

    const FText Chosen = Upgrades->GetCurrentOptions()[0].DisplayName;
    Upgrades->ChooseUpgrade(0);
    TestEqual(TEXT("One upgrade row"), Scene.Hud->GetVisibleUpgradeRowCount(), 1);
    TestEqual(TEXT("It names the upgrade taken"), Scene.Hud->GetUpgradeRowText(0), Chosen.ToString());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGameplayHudRebindTest,
    "GrantMoney.UI.GameplayHud.ReplacedPlayerAndRunLeaveNoStaleSubscriptions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGameplayHudRebindTest::RunTest(const FString& Parameters)
{
    GameplayHudTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    AGrantMoneyPlayerCharacter* OldPlayer = Scene.Player;
    AGrantMoneyPlayerCharacter* NewPlayer = Scene.SpawnPlayer();
    if (!TestNotNull(TEXT("Replacement player spawned"), NewPlayer))
    {
        return false;
    }

    NewPlayer->GetHealth()->ApplyDamage(40.0f);
    Scene.Hud->BindPlayer(NewPlayer);
    TestTrue(TEXT("Bound to the replacement"), Scene.Hud->GetBoundPlayer() == NewPlayer);
    TestEqual(TEXT("Shows the replacement's health at once"), Scene.Hud->GetHealthText(), GameplayHudTest::FScene::ExpectedHealth(NewPlayer));

    // The old player is no longer listened to: hurting it must not change what is shown.
    const FString Before = Scene.Hud->GetHealthText();
    OldPlayer->GetHealth()->ApplyDamage(25.0f);
    TestEqual(TEXT("Old player's damage is ignored"), Scene.Hud->GetHealthText(), Before);

    OldPlayer->GetStats()->SetBaseStat(EGrantMoneyStat::Experience, OldPlayer->GetStats()->GetBaseStat(EGrantMoneyStat::Experience) + 5.0f);
    TestEqual(TEXT("Old player's XP does not touch the score"), Scene.Hud->GetScoreText(), FString(TEXT("0")));

    OldPlayer->GetTemporaryBuffs()->AddBuff(TEXT("Ghost"), FText::GetEmpty(), EGrantMoneyStat::Damage, EGrantMoneyStatModifierOp::Additive, 1.0f, 5.0f);
    TestEqual(TEXT("Old player's buffs do not show"), Scene.Hud->GetVisibleBuffRowCount(), 0);

    // The new player is.
    NewPlayer->GetHealth()->ApplyDamage(10.0f);
    TestEqual(TEXT("New player's damage is shown"), Scene.Hud->GetHealthText(), GameplayHudTest::FScene::ExpectedHealth(NewPlayer));

    // Binding the same player again must not make it listen twice or lose the values.
    Scene.Hud->BindPlayer(NewPlayer);
    TestEqual(TEXT("Rebinding the same player keeps the values"), Scene.Hud->GetHealthText(), GameplayHudTest::FScene::ExpectedHealth(NewPlayer));

    // Clearing leaves an empty, safe readout.
    Scene.Hud->BindPlayer(nullptr);
    TestTrue(TEXT("No player bound"), Scene.Hud->GetBoundPlayer() == nullptr);
    TestEqual(TEXT("Health placeholder"), Scene.Hud->GetHealthText(), FString(TEXT("HP  --")));
    TestEqual(TEXT("No buff rows"), Scene.Hud->GetVisibleBuffRowCount(), 0);
    NewPlayer->GetHealth()->ApplyDamage(5.0f);
    TestEqual(TEXT("A cleared player is no longer listened to"), Scene.Hud->GetHealthText(), FString(TEXT("HP  --")));

    // The run follows the same rule: a restart's new controller replaces the old one cleanly.
    ARunController* NewRun = Scene.World()->SpawnActor<ARunController>();
    NewRun->StartDelaySeconds = 0.0f;
    NewRun->RunDurationSeconds = 60.0f;
    NewRun->StartRun();
    Scene.Hud->BindRun(NewRun);
    TestTrue(TEXT("Bound to the new run"), Scene.Hud->GetBoundRun() == NewRun);
    TestEqual(TEXT("Shows the new run's clock at once"), Scene.Hud->GetTimerText(), FString(TEXT("1:00")));

    Scene.Run->AdvanceRun(20.0f);
    TestEqual(TEXT("The old run's time is ignored"), Scene.Hud->GetTimerText(), FString(TEXT("1:00")));
    NewRun->AdvanceRun(15.0f);
    TestEqual(TEXT("The new run's time is shown"), Scene.Hud->GetTimerText(), FString(TEXT("0:45")));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
