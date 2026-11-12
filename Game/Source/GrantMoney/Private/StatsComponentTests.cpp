#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "StatsComponent.h"
#include "Tests/CombatTestListener.h"

// The stats component needs no world: its logic is pure, so it is created as a plain object.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatsComponentBaseValueTest,
    "GrantMoney.Combat.StatsComponent.BaseValue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStatsComponentBaseValueTest::RunTest(const FString& Parameters)
{
    UStatsComponent* Stats = NewObject<UStatsComponent>();

    TestEqual(TEXT("A stat with no base value is 0"), Stats->GetStat(EGrantMoneyStat::MaxHealth), 0.0f);
    TestFalse(TEXT("HasStat is false before a base value is set"), Stats->HasStat(EGrantMoneyStat::MaxHealth));

    Stats->SetBaseStat(EGrantMoneyStat::MaxHealth, 100.0f);
    TestTrue(TEXT("HasStat is true once a base value is set"), Stats->HasStat(EGrantMoneyStat::MaxHealth));
    TestFalse(TEXT("Other stats are unaffected"), Stats->HasStat(EGrantMoneyStat::AttackInterval));
    TestEqual(TEXT("Base value"), Stats->GetBaseStat(EGrantMoneyStat::MaxHealth), 100.0f);
    TestEqual(TEXT("Final value equals base with no modifiers"), Stats->GetStat(EGrantMoneyStat::MaxHealth), 100.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatsComponentModifierMathTest,
    "GrantMoney.Combat.StatsComponent.ModifierMath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStatsComponentModifierMathTest::RunTest(const FString& Parameters)
{
    UStatsComponent* Stats = NewObject<UStatsComponent>();
    Stats->SetBaseStat(EGrantMoneyStat::MaxHealth, 100.0f);

    Stats->AddModifier(EGrantMoneyStat::MaxHealth, EGrantMoneyStatModifierOp::Additive, 20.0f);
    TestEqual(TEXT("Additive: 100 + 20"), Stats->GetStat(EGrantMoneyStat::MaxHealth), 120.0f);

    Stats->AddModifier(EGrantMoneyStat::MaxHealth, EGrantMoneyStatModifierOp::Multiplicative, 0.5f);
    TestEqual(TEXT("Multiplier applies after additive: (100 + 20) * 1.5"), Stats->GetStat(EGrantMoneyStat::MaxHealth), 180.0f);

    Stats->AddModifier(EGrantMoneyStat::MaxHealth, EGrantMoneyStatModifierOp::Multiplicative, 0.25f);
    TestEqual(TEXT("Multipliers add together: 120 * (1 + 0.5 + 0.25)"), Stats->GetStat(EGrantMoneyStat::MaxHealth), 210.0f);

    TestEqual(TEXT("Base value is unchanged by modifiers"), Stats->GetBaseStat(EGrantMoneyStat::MaxHealth), 100.0f);

    Stats->AddModifier(EGrantMoneyStat::MaxHealth, EGrantMoneyStatModifierOp::Multiplicative, -5.0f);
    TestEqual(TEXT("Final value never drops below zero"), Stats->GetStat(EGrantMoneyStat::MaxHealth), 0.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatsComponentRemoveModifierTest,
    "GrantMoney.Combat.StatsComponent.RemoveModifier",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStatsComponentRemoveModifierTest::RunTest(const FString& Parameters)
{
    UStatsComponent* Stats = NewObject<UStatsComponent>();
    Stats->SetBaseStat(EGrantMoneyStat::MaxHealth, 100.0f);

    const FGrantMoneyStatModifierHandle Flat = Stats->AddModifier(EGrantMoneyStat::MaxHealth, EGrantMoneyStatModifierOp::Additive, 50.0f);
    const FGrantMoneyStatModifierHandle Percent = Stats->AddModifier(EGrantMoneyStat::MaxHealth, EGrantMoneyStatModifierOp::Multiplicative, 1.0f);

    TestTrue(TEXT("Handles are valid"), Flat.IsValid() && Percent.IsValid());
    TestNotEqual(TEXT("Handles are unique"), Flat, Percent);
    TestEqual(TEXT("Both applied: (100 + 50) * 2"), Stats->GetStat(EGrantMoneyStat::MaxHealth), 300.0f);

    TestTrue(TEXT("Removing an active modifier succeeds"), Stats->RemoveModifier(Flat));
    TestEqual(TEXT("Only the multiplier remains: 100 * 2"), Stats->GetStat(EGrantMoneyStat::MaxHealth), 200.0f);

    TestFalse(TEXT("Removing the same handle twice fails"), Stats->RemoveModifier(Flat));
    TestFalse(TEXT("Removing an invalid handle fails"), Stats->RemoveModifier(FGrantMoneyStatModifierHandle()));

    TestTrue(TEXT("Removing the last modifier succeeds"), Stats->RemoveModifier(Percent));
    TestEqual(TEXT("Back to base"), Stats->GetStat(EGrantMoneyStat::MaxHealth), 100.0f);
    TestEqual(TEXT("No modifiers left"), Stats->GetModifiers().Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatsComponentChangeEventTest,
    "GrantMoney.Combat.StatsComponent.ChangeEvent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStatsComponentChangeEventTest::RunTest(const FString& Parameters)
{
    UStatsComponent* Stats = NewObject<UStatsComponent>();
    UCombatTestListener* Listener = NewObject<UCombatTestListener>();
    Stats->OnStatChanged.AddDynamic(Listener, &UCombatTestListener::OnStatChanged);

    Stats->SetBaseStat(EGrantMoneyStat::MaxHealth, 100.0f);
    TestEqual(TEXT("Setting the base fires once"), Listener->StatChangedCount, 1);
    TestEqual(TEXT("Old value"), Listener->LastStatOldValue, 0.0f);
    TestEqual(TEXT("New value"), Listener->LastStatNewValue, 100.0f);

    Stats->SetBaseStat(EGrantMoneyStat::MaxHealth, 100.0f);
    TestEqual(TEXT("Setting the same value does not fire"), Listener->StatChangedCount, 1);

    const FGrantMoneyStatModifierHandle Handle = Stats->AddModifier(EGrantMoneyStat::MaxHealth, EGrantMoneyStatModifierOp::Additive, 25.0f);
    TestEqual(TEXT("Adding a modifier fires"), Listener->StatChangedCount, 2);
    TestEqual(TEXT("Reports the final value"), Listener->LastStatNewValue, 125.0f);

    Stats->AddModifier(EGrantMoneyStat::MaxHealth, EGrantMoneyStatModifierOp::Additive, 0.0f);
    TestEqual(TEXT("A modifier that changes nothing does not fire"), Listener->StatChangedCount, 2);

    Stats->RemoveModifier(Handle);
    TestEqual(TEXT("Removing a modifier fires"), Listener->StatChangedCount, 3);
    TestEqual(TEXT("Back to base"), Listener->LastStatNewValue, 100.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatsComponentStatFromBaseTest,
    "GrantMoney.Combat.StatsComponent.StatFromBase",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStatsComponentStatFromBaseTest::RunTest(const FString& Parameters)
{
    UStatsComponent* Stats = NewObject<UStatsComponent>();

    TestEqual(TEXT("No modifiers: the given base comes back unchanged"), Stats->GetStatFromBase(EGrantMoneyStat::Damage, 10.0f), 10.0f);

    Stats->AddModifier(EGrantMoneyStat::Damage, EGrantMoneyStatModifierOp::Additive, 2.0f);
    Stats->AddModifier(EGrantMoneyStat::Damage, EGrantMoneyStatModifierOp::Multiplicative, 0.5f);
    Stats->AddModifier(EGrantMoneyStat::MaxHealth, EGrantMoneyStatModifierOp::Additive, 99.0f);
    TestEqual(TEXT("Same formula as GetStat, on the given base"), Stats->GetStatFromBase(EGrantMoneyStat::Damage, 10.0f), 18.0f);
    TestFalse(TEXT("No base value is stored"), Stats->HasStat(EGrantMoneyStat::Damage));

    Stats->SetBaseStat(EGrantMoneyStat::Damage, 4.0f);
    TestEqual(TEXT("GetStat uses the stored base"), Stats->GetStat(EGrantMoneyStat::Damage), 9.0f);
    TestEqual(TEXT("GetStatFromBase ignores the stored base"), Stats->GetStatFromBase(EGrantMoneyStat::Damage, 10.0f), 18.0f);

    Stats->AddModifier(EGrantMoneyStat::AttackInterval, EGrantMoneyStatModifierOp::Multiplicative, -2.0f);
    TestEqual(TEXT("Never below zero"), Stats->GetStatFromBase(EGrantMoneyStat::AttackInterval, 1.0f), 0.0f);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
