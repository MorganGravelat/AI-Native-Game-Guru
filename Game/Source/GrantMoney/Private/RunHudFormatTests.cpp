#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "RunHudFormat.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRunHudFormatScoreTest,
    "GrantMoney.UI.RunHudFormat.ScoreMixesXpAndTime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRunHudFormatScoreTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Nothing earned"), FRunHudFormat::ComputeScore(0.0f, 0.0f), 0);
    TestEqual(TEXT("XP only"), FRunHudFormat::ComputeScore(12.0f, 0.0f), 120);
    TestEqual(TEXT("Time only"), FRunHudFormat::ComputeScore(0.0f, 20.0f), 100);
    TestEqual(TEXT("Both add: 30 XP * 10 + 60 s * 5"), FRunHudFormat::ComputeScore(30.0f, 60.0f), 600);
    TestEqual(TEXT("Fractions are dropped, not rounded up"), FRunHudFormat::ComputeScore(0.0f, 0.39f), 1);
    TestEqual(TEXT("Negative inputs never lower the score"), FRunHudFormat::ComputeScore(-5.0f, -5.0f), 0);
    TestTrue(TEXT("More of either never lowers it"), FRunHudFormat::ComputeScore(31.0f, 60.0f) > FRunHudFormat::ComputeScore(30.0f, 60.0f));
    TestEqual(TEXT("Huge values clamp instead of overflowing"), FRunHudFormat::ComputeScore(1.0e12f, 0.0f), MAX_int32);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRunHudFormatNumbersTest,
    "GrantMoney.UI.RunHudFormat.NumbersRead",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRunHudFormatNumbersTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Score under 1000"), FRunHudFormat::FormatScore(999), FString(TEXT("999")));
    TestEqual(TEXT("Score with one separator"), FRunHudFormat::FormatScore(1000), FString(TEXT("1,000")));
    TestEqual(TEXT("Score with two separators"), FRunHudFormat::FormatScore(1234567), FString(TEXT("1,234,567")));
    TestEqual(TEXT("Zero"), FRunHudFormat::FormatScore(0), FString(TEXT("0")));

    TestEqual(TEXT("Health"), FRunHudFormat::FormatHealth(80.0f, 100.0f), FString(TEXT("80 / 100")));
    TestEqual(TEXT("A living player with a sliver of health never reads 0"), FRunHudFormat::FormatHealth(0.2f, 100.0f), FString(TEXT("1 / 100")));
    TestEqual(TEXT("Dead reads 0"), FRunHudFormat::FormatHealth(0.0f, 100.0f), FString(TEXT("0 / 100")));

    TestEqual(TEXT("XP"), FRunHudFormat::FormatXp(12.7f, 25.0f), FString(TEXT("12 / 25 XP")));
    TestEqual(TEXT("Level"), FRunHudFormat::FormatLevel(3), FString(TEXT("Level 3")));
    TestEqual(TEXT("Level never reads below 1"), FRunHudFormat::FormatLevel(0), FString(TEXT("Level 1")));

    TestEqual(TEXT("Fraction"), FRunHudFormat::SafeFraction(25.0f, 100.0f), 0.25f);
    TestEqual(TEXT("Fraction over the maximum clamps"), FRunHudFormat::SafeFraction(150.0f, 100.0f), 1.0f);
    TestEqual(TEXT("Fraction with a zero maximum is 0, not NaN"), FRunHudFormat::SafeFraction(5.0f, 0.0f), 0.0f);
    TestEqual(TEXT("Negative value clamps to 0"), FRunHudFormat::SafeFraction(-5.0f, 100.0f), 0.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRunHudFormatBuffAndUpgradeTest,
    "GrantMoney.UI.RunHudFormat.BuffsUpgradesAndPhase",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRunHudFormatBuffAndUpgradeTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Long remaining time is whole seconds, rounded up"), FRunHudFormat::FormatBuffRemaining(12.1f), FString(TEXT("13s")));
    TestEqual(TEXT("Exactly ten"), FRunHudFormat::FormatBuffRemaining(10.0f), FString(TEXT("10s")));
    TestEqual(TEXT("Short remaining time shows a decimal"), FRunHudFormat::FormatBuffRemaining(4.26f), FString(TEXT("4.3s")));
    TestEqual(TEXT("Negative remaining reads 0"), FRunHudFormat::FormatBuffRemaining(-1.0f), FString(TEXT("0.0s")));
    TestEqual(TEXT("Buff line"), FRunHudFormat::FormatBuffLine(FText::FromString(TEXT("Speed Surge")), 12.0f), FString(TEXT("Speed Surge  12s")));
    TestEqual(TEXT("A permanent buff line has no time at all"), FRunHudFormat::FormatBuffLine(FText::FromString(TEXT("Swiftness")), 12.0f, true), FString(TEXT("Swiftness")));

    const FText Damage = FText::FromString(TEXT("Damage"));
    TestEqual(TEXT("First pick has no count"), FRunHudFormat::FormatUpgradeLine(Damage, 1, 5), FString(TEXT("Damage")));
    TestEqual(TEXT("Repeat picks show the count"), FRunHudFormat::FormatUpgradeLine(Damage, 3, 5), FString(TEXT("Damage x3")));
    TestEqual(TEXT("At the stack limit"), FRunHudFormat::FormatUpgradeLine(Damage, 5, 5), FString(TEXT("Damage x5 (max)")));
    TestEqual(TEXT("A one-pick upgrade is not flagged max"), FRunHudFormat::FormatUpgradeLine(Damage, 1, 1), FString(TEXT("Damage")));
    TestEqual(TEXT("Unlimited upgrades are never max"), FRunHudFormat::FormatUpgradeLine(Damage, 9, 0), FString(TEXT("Damage x9")));

    TestEqual(TEXT("Phase label"), FRunHudFormat::FormatPhase(EGrantMoneyRunState::BossPhase, false), ARunController::GetStateLabel(EGrantMoneyRunState::BossPhase));
    TestTrue(TEXT("Paused is appended"), FRunHudFormat::FormatPhase(EGrantMoneyRunState::Active, true).EndsWith(TEXT("(paused)")));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
