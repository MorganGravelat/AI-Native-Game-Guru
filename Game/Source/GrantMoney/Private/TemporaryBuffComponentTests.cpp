#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "StatsComponent.h"
#include "TemporaryBuffComponent.h"
#include "Tests/ProgressionTestListener.h"

// The buff component's clock is driven through AdvanceBuffs, so no world is needed.

namespace TemporaryBuffTest
{
    struct FSetup
    {
        UStatsComponent* Stats = NewObject<UStatsComponent>();
        UTemporaryBuffComponent* Buffs = NewObject<UTemporaryBuffComponent>();

        FSetup()
        {
            Stats->SetBaseStat(EGrantMoneyStat::MoveSpeed, 600.0f);
            Buffs->SetStatsComponent(Stats);
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTemporaryBuffAppliesAndExpiresTest,
    "GrantMoney.Progression.TemporaryBuff.AppliesThenExpires",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTemporaryBuffAppliesAndExpiresTest::RunTest(const FString& Parameters)
{
    TemporaryBuffTest::FSetup Setup;
    UProgressionTestListener* Listener = NewObject<UProgressionTestListener>();
    Setup.Buffs->OnBuffsChanged.AddDynamic(Listener, &UProgressionTestListener::OnBuffsChanged);

    TestTrue(TEXT("Buff accepted"), Setup.Buffs->AddBuff(TEXT("Surge"), FText::FromString(TEXT("Speed Surge")), EGrantMoneyStat::MoveSpeed, EGrantMoneyStatModifierOp::Multiplicative, 0.5f, 10.0f));
    TestEqual(TEXT("Stat is buffed: 600 * 1.5"), Setup.Stats->GetStat(EGrantMoneyStat::MoveSpeed), 900.0f);
    TestTrue(TEXT("Buff is listed"), Setup.Buffs->HasBuff(TEXT("Surge")));
    TestEqual(TEXT("Change event on add"), Listener->BuffsChangedCount, 1);

    Setup.Buffs->AdvanceBuffs(4.0f);
    TestEqual(TEXT("Still active"), Setup.Buffs->GetActiveBuffs().Num(), 1);
    TestEqual(TEXT("Remaining time counts down"), Setup.Buffs->GetActiveBuffs()[0].RemainingSeconds, 6.0f);
    TestEqual(TEXT("Duration is kept for a progress readout"), Setup.Buffs->GetActiveBuffs()[0].DurationSeconds, 10.0f);
    TestEqual(TEXT("No change event while only time passes"), Listener->BuffsChangedCount, 1);
    TestEqual(TEXT("Still buffed"), Setup.Stats->GetStat(EGrantMoneyStat::MoveSpeed), 900.0f);

    Setup.Buffs->AdvanceBuffs(6.0f);
    TestFalse(TEXT("Expired exactly at its duration"), Setup.Buffs->HasBuff(TEXT("Surge")));
    TestEqual(TEXT("Stat is back to base"), Setup.Stats->GetStat(EGrantMoneyStat::MoveSpeed), 600.0f);
    TestEqual(TEXT("The modifier is gone, not just hidden"), Setup.Stats->GetModifiers().Num(), 0);
    TestEqual(TEXT("Change event on expiry"), Listener->BuffsChangedCount, 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTemporaryBuffRefreshTest,
    "GrantMoney.Progression.TemporaryBuff.SameIdRefreshesInsteadOfStacking",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTemporaryBuffRefreshTest::RunTest(const FString& Parameters)
{
    TemporaryBuffTest::FSetup Setup;

    Setup.Buffs->AddBuff(TEXT("Surge"), FText::GetEmpty(), EGrantMoneyStat::MoveSpeed, EGrantMoneyStatModifierOp::Additive, 100.0f, 10.0f);
    Setup.Buffs->AdvanceBuffs(8.0f);
    Setup.Buffs->AddBuff(TEXT("Surge"), FText::GetEmpty(), EGrantMoneyStat::MoveSpeed, EGrantMoneyStatModifierOp::Additive, 100.0f, 10.0f);

    TestEqual(TEXT("One buff, not two"), Setup.Buffs->GetActiveBuffs().Num(), 1);
    TestEqual(TEXT("One modifier, not two"), Setup.Stats->GetModifiers().Num(), 1);
    TestEqual(TEXT("Value does not stack: 600 + 100"), Setup.Stats->GetStat(EGrantMoneyStat::MoveSpeed), 700.0f);
    TestEqual(TEXT("The clock restarted"), Setup.Buffs->GetActiveBuffs()[0].RemainingSeconds, 10.0f);

    Setup.Buffs->AddBuff(TEXT("Surge"), FText::GetEmpty(), EGrantMoneyStat::MoveSpeed, EGrantMoneyStatModifierOp::Additive, 250.0f, 5.0f);
    TestEqual(TEXT("A refresh with a new magnitude replaces the old one"), Setup.Stats->GetStat(EGrantMoneyStat::MoveSpeed), 850.0f);
    TestEqual(TEXT("And its new duration"), Setup.Buffs->GetActiveBuffs()[0].DurationSeconds, 5.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTemporaryBuffIndependentTest,
    "GrantMoney.Progression.TemporaryBuff.BuffsExpireIndependentlyAndCanBeRemoved",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTemporaryBuffIndependentTest::RunTest(const FString& Parameters)
{
    TemporaryBuffTest::FSetup Setup;

    Setup.Buffs->AddBuff(TEXT("Short"), FText::GetEmpty(), EGrantMoneyStat::MoveSpeed, EGrantMoneyStatModifierOp::Additive, 50.0f, 2.0f);
    Setup.Buffs->AddBuff(TEXT("Long"), FText::GetEmpty(), EGrantMoneyStat::Damage, EGrantMoneyStatModifierOp::Additive, 3.0f, 8.0f);
    Setup.Buffs->AddBuff(TEXT("Manual"), FText::GetEmpty(), EGrantMoneyStat::MaxHealth, EGrantMoneyStatModifierOp::Additive, 20.0f, 30.0f);

    Setup.Buffs->AdvanceBuffs(3.0f);
    TestFalse(TEXT("Short expired"), Setup.Buffs->HasBuff(TEXT("Short")));
    TestTrue(TEXT("Long remains"), Setup.Buffs->HasBuff(TEXT("Long")));
    TestEqual(TEXT("Short's stat is back"), Setup.Stats->GetStat(EGrantMoneyStat::MoveSpeed), 600.0f);
    TestEqual(TEXT("Long's stat is still buffed"), Setup.Stats->GetStat(EGrantMoneyStat::Damage), 3.0f);

    TestTrue(TEXT("Removing an active buff succeeds"), Setup.Buffs->RemoveBuff(TEXT("Manual")));
    TestFalse(TEXT("Removing it twice fails"), Setup.Buffs->RemoveBuff(TEXT("Manual")));
    TestEqual(TEXT("Removed buff's stat is back"), Setup.Stats->GetStat(EGrantMoneyStat::MaxHealth), 0.0f);

    Setup.Buffs->ClearBuffs();
    TestEqual(TEXT("Nothing active after clear"), Setup.Buffs->GetActiveBuffs().Num(), 0);
    TestEqual(TEXT("No modifiers left behind"), Setup.Stats->GetModifiers().Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTemporaryBuffRejectsBadInputTest,
    "GrantMoney.Progression.TemporaryBuff.RejectsInvalidBuffs",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTemporaryBuffRejectsBadInputTest::RunTest(const FString& Parameters)
{
    TemporaryBuffTest::FSetup Setup;

    TestFalse(TEXT("Zero duration is refused"), Setup.Buffs->AddBuff(TEXT("A"), FText::GetEmpty(), EGrantMoneyStat::Damage, EGrantMoneyStatModifierOp::Additive, 1.0f, 0.0f));
    TestFalse(TEXT("Negative duration is refused"), Setup.Buffs->AddBuff(TEXT("A"), FText::GetEmpty(), EGrantMoneyStat::Damage, EGrantMoneyStatModifierOp::Additive, 1.0f, -3.0f));
    TestFalse(TEXT("An empty id is refused"), Setup.Buffs->AddBuff(NAME_None, FText::GetEmpty(), EGrantMoneyStat::Damage, EGrantMoneyStatModifierOp::Additive, 1.0f, 5.0f));
    TestEqual(TEXT("Nothing applied"), Setup.Stats->GetModifiers().Num(), 0);

    UTemporaryBuffComponent* NoStats = NewObject<UTemporaryBuffComponent>();
    TestFalse(TEXT("Without a stats component there is nothing to buff"), NoStats->AddBuff(TEXT("A"), FText::GetEmpty(), EGrantMoneyStat::Damage, EGrantMoneyStatModifierOp::Additive, 1.0f, 5.0f));
    TestEqual(TEXT("And nothing is listed"), NoStats->GetActiveBuffs().Num(), 0);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
