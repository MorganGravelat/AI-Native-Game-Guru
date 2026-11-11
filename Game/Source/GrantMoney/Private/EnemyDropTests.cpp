#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "EnemyPickupDrop.h"
#include "StatBoostPickup.h"
#include "Tests/PickupTestScene.h"

// Enemy drops: the guaranteed XP orb and independent bonus pickups, spawned from the existing death flow.
// Enemies are killed through their health component, as in EnemyBaseTests; that is the same path the
// engine's damage events take, and it runs the death handling exactly once per enemy.

namespace
{
    // Kills the enemy where it stands. Returns false if the enemy was not alive to kill.
    bool Kill(AEnemyBase* Enemy)
    {
        return Enemy->GetHealth()->ApplyDamage(1000.0f, nullptr) > 0.0f;
    }

    // A scene whose pickup radius is tiny, so dropped pickups stay where they landed and can be counted.
    bool CreateIdleScene(PickupTest::FScene& Scene, FAutomationTestBase& Test)
    {
        return Scene.Create(Test, 1.0f);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemyDropSlimeDropsOneOrbTest,
    "GrantMoney.Pickups.EnemyDrops.SlimeDropsExactlyOneOrb",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemyDropSlimeDropsOneOrbTest::RunTest(const FString& Parameters)
{
    PickupTest::FScene Scene;
    if (!CreateIdleScene(Scene, *this))
    {
        return false;
    }

    const FVector DeathSpot(1500.0, 400.0, 0.0);
    ASlimeEnemy* Slime = Scene.SpawnSlime(DeathSpot);

    // Rare stat boosts are random; this test is about the guaranteed orb only.
    Slime->BonusPickupDrops.Reset();
    TestEqual(TEXT("Nothing is dropped while the slime lives"), Scene.CountPickups(), 0);

    TestTrue(TEXT("The slime dies"), Kill(Slime));
    TestEqual(TEXT("The orb exists in the same frame, before the slime despawns"), Scene.CountPickups<AXpOrbPickup>(), 1);

    const AXpOrbPickup* Orb = Scene.FirstPickup<AXpOrbPickup>();
    if (TestNotNull(TEXT("The orb is found"), Orb))
    {
        TestTrue(TEXT("It is at the death location"), Orb->GetActorLocation().Equals(DeathSpot, 0.01));
    }

    Scene.Step(1.0f);
    TestEqual(TEXT("Still exactly one orb after the slime is gone"), Scene.CountPickups(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemyDropNoDuplicateOnSimultaneousHitsTest,
    "GrantMoney.Pickups.EnemyDrops.SimultaneousLethalHitsDropOnce",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemyDropNoDuplicateOnSimultaneousHitsTest::RunTest(const FString& Parameters)
{
    PickupTest::FScene Scene;
    if (!CreateIdleScene(Scene, *this))
    {
        return false;
    }

    ASlimeEnemy* Slime = Scene.SpawnSlime(FVector(1500.0, 0.0, 0.0));
    UHealthComponent* Health = Slime->GetHealth();

    // Three lethal hits land in the same frame.
    Health->ApplyDamage(1000.0f, nullptr);
    Health->ApplyDamage(1000.0f, nullptr);
    Health->ApplyDamage(1000.0f, nullptr);

    Scene.Step(0.5f);
    TestEqual(TEXT("One death, one orb"), Scene.CountPickups<AXpOrbPickup>(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemyDropBaseDropsNothingTest,
    "GrantMoney.Pickups.EnemyDrops.EnemyBaseDropsNothingByDefault",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemyDropBaseDropsNothingTest::RunTest(const FString& Parameters)
{
    PickupTest::FScene Scene;
    if (!CreateIdleScene(Scene, *this))
    {
        return false;
    }

    AEnemyBase* Enemy = Scene.SpawnFrozen<AEnemyBase>(FVector(1500.0, 0.0, 0.0));
    TestFalse(TEXT("Dropping XP is opt-in"), Enemy->bDropsXpOrb);
    Kill(Enemy);
    Scene.Step(0.2f);
    TestEqual(TEXT("A plain enemy drops nothing"), Scene.CountPickups(), 0);

    TestTrue(TEXT("Slimes turn the guaranteed drop on"), GetDefault<ASlimeEnemy>()->bDropsXpOrb);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemyDropBonusIsIndependentTest,
    "GrantMoney.Pickups.EnemyDrops.BonusDropsAreIndependentOfXp",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemyDropBonusIsIndependentTest::RunTest(const FString& Parameters)
{
    // A bonus chance of 0 and of 1: XP drops either way, and the bonus drops only at 1.
    for (const float BonusChance : { 0.0f, 1.0f })
    {
        PickupTest::FScene Scene;
        if (!CreateIdleScene(Scene, *this))
        {
            return false;
        }

        ASlimeEnemy* Slime = Scene.SpawnSlime(FVector(1500.0, 0.0, 0.0));
        FEnemyPickupDrop Rare;
        Rare.PickupClass = ATestBuffPickup::StaticClass();
        Rare.DropChance = BonusChance;
        Slime->BonusPickupDrops.Add(Rare);

        Kill(Slime);
        Scene.Step(0.2f);

        const FString Label = FString::Printf(TEXT("bonus chance %.0f"), BonusChance);
        TestEqual(*(Label + TEXT(": guaranteed XP orb is always dropped")), Scene.CountPickups<AXpOrbPickup>(), 1);
        TestEqual(*(Label + TEXT(": bonus drop follows its own chance")), Scene.CountPickups<ATestBuffPickup>(), BonusChance > 0.0f ? 1 : 0);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemyDropSeveralBonusEntriesTest,
    "GrantMoney.Pickups.EnemyDrops.EachBonusEntryRollsOnItsOwn",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemyDropSeveralBonusEntriesTest::RunTest(const FString& Parameters)
{
    PickupTest::FScene Scene;
    if (!CreateIdleScene(Scene, *this))
    {
        return false;
    }

    ASlimeEnemy* Slime = Scene.SpawnSlime(FVector(1500.0, 0.0, 0.0));

    FEnemyPickupDrop Sure;
    Sure.PickupClass = ATestBuffPickup::StaticClass();
    Sure.DropChance = 1.0f;

    FEnemyPickupDrop Never;
    Never.PickupClass = ATestBuffPickup::StaticClass();
    Never.DropChance = 0.0f;

    FEnemyPickupDrop NoClass;
    NoClass.DropChance = 1.0f;

    Slime->BonusPickupDrops = { Sure, Never, Sure, NoClass };
    Kill(Slime);
    Scene.Step(0.2f);

    TestEqual(TEXT("Two sure entries drop, the 0% and the class-less entries do not"), Scene.CountPickups<ATestBuffPickup>(), 2);
    TestEqual(TEXT("Plus the XP orb"), Scene.CountPickups<AXpOrbPickup>(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemyDropShouldDropTest,
    "GrantMoney.Pickups.EnemyDrops.ChanceRolls",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemyDropShouldDropTest::RunTest(const FString& Parameters)
{
    FEnemyPickupDrop Drop;
    Drop.PickupClass = ATestBuffPickup::StaticClass();

    Drop.DropChance = 1.0f;
    TestTrue(TEXT("100% drops on the lowest roll"), Drop.ShouldDrop(0.0f));
    TestTrue(TEXT("100% drops even on a roll of exactly 1"), Drop.ShouldDrop(1.0f));

    Drop.DropChance = 0.0f;
    TestFalse(TEXT("0% never drops, even on a roll of 0"), Drop.ShouldDrop(0.0f));

    Drop.DropChance = 0.25f;
    TestTrue(TEXT("25% drops on a roll below 0.25"), Drop.ShouldDrop(0.24f));
    TestFalse(TEXT("25% does not drop on a roll of 0.25"), Drop.ShouldDrop(0.25f));
    TestFalse(TEXT("25% does not drop on a high roll"), Drop.ShouldDrop(0.9f));

    Drop.PickupClass = nullptr;
    Drop.DropChance = 1.0f;
    TestFalse(TEXT("No class never drops"), Drop.ShouldDrop(0.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemyDropSlimeStatBoostsTest,
    "GrantMoney.Pickups.EnemyDrops.SlimeDropsRareStatBoosts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEnemyDropSlimeStatBoostsTest::RunTest(const FString& Parameters)
{
    // Code defaults: one rare entry per boost the player can use.
    const TArray<FEnemyPickupDrop>& Defaults = GetDefault<ASlimeEnemy>()->BonusPickupDrops;
    TestEqual(TEXT("Four stat boost entries"), Defaults.Num(), 4);
    for (const FEnemyPickupDrop& Drop : Defaults)
    {
        TestTrue(TEXT("Each entry is a stat boost"), Drop.PickupClass && Drop.PickupClass->IsChildOf(AStatBoostPickup::StaticClass()));
        TestEqual(TEXT("Each uses the default chance"), Drop.DropChance, ASlimeEnemy::DefaultStatBoostDropChance);
    }
    TestTrue(TEXT("The default chance is rare (5% or less)"), ASlimeEnemy::DefaultStatBoostDropChance > 0.0f && ASlimeEnemy::DefaultStatBoostDropChance <= 0.05f);

    // The chance is editable per entry: at 100% every boost drops, beside the guaranteed XP orb.
    PickupTest::FScene Scene;
    if (!CreateIdleScene(Scene, *this))
    {
        return false;
    }

    ASlimeEnemy* Slime = Scene.SpawnSlime(FVector(1500.0, 0.0, 0.0));
    for (FEnemyPickupDrop& Drop : Slime->BonusPickupDrops)
    {
        Drop.DropChance = 1.0f;
    }

    Kill(Slime);
    Scene.Step(0.2f);
    TestEqual(TEXT("All four boosts dropped"), Scene.CountPickups<AStatBoostPickup>(), 4);
    TestEqual(TEXT("Plus the XP orb"), Scene.CountPickups<AXpOrbPickup>(), 1);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
