#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TemporaryEffect.h"
#include "Tests/TemporaryEffectTestScene.h"

// The temporary effect contract (SCRUM-39): Apply/Remove on one target, and the ready-made stat modifier
// effect, which only ever adds and removes a modifier, never the base value.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTemporaryEffectStatModifierApplyRemoveTest,
    "GrantMoney.Effects.TemporaryEffect.StatModifierAppliesAndRemoves",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTemporaryEffectStatModifierApplyRemoveTest::RunTest(const FString& Parameters)
{
    EffectTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    UStatModifierEffect* Effect = NewObject<UStatModifierEffect>();
    Effect->Stat = EGrantMoneyStat::MoveSpeed;
    Effect->ModifierOp = EGrantMoneyStatModifierOp::Multiplicative;
    Effect->Magnitude = 0.5f;

    TestTrue(TEXT("Applies to a target with stats"), Effect->Apply(Scene.Player));
    TestTrue(TEXT("Reports itself applied"), Effect->IsApplied());
    TestTrue(TEXT("Remembers its target"), Effect->GetTarget() == Scene.Player);
    TestEqual(TEXT("600 x 1.5 = 900"), Scene.MoveSpeed(), 900.0f);
    TestEqual(TEXT("Base value is untouched while active"), Scene.Stats->GetBaseStat(EGrantMoneyStat::MoveSpeed), EffectTest::BaseMoveSpeed);

    TestFalse(TEXT("Cannot be applied twice"), Effect->Apply(Scene.Player));
    TestEqual(TEXT("Still one modifier"), Scene.Stats->GetModifiers().Num(), 1);

    // An unrelated modifier added meanwhile must survive the effect's removal.
    Scene.Stats->AddModifier(EGrantMoneyStat::MoveSpeed, EGrantMoneyStatModifierOp::Additive, 100.0f);

    Effect->Remove();
    TestFalse(TEXT("No longer applied"), Effect->IsApplied());
    TestEqual(TEXT("Only its own modifier is removed: 600 + 100"), Scene.MoveSpeed(), 700.0f);

    Effect->Remove();
    TestEqual(TEXT("Removing twice changes nothing"), Scene.MoveSpeed(), 700.0f);
    TestEqual(TEXT("Base value never changed"), Scene.Stats->GetBaseStat(EGrantMoneyStat::MoveSpeed), EffectTest::BaseMoveSpeed);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTemporaryEffectRefusesTest,
    "GrantMoney.Effects.TemporaryEffect.RefusesWithoutStatsOrStrength",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTemporaryEffectRefusesTest::RunTest(const FString& Parameters)
{
    EffectTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    UStatModifierEffect* Effect = NewObject<UStatModifierEffect>();

    AddExpectedMessage(TEXT("has no Stats component"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);
    AActor* Statless = Scene.World()->SpawnActor<AActor>();
    TestFalse(TEXT("A target with no Stats component is refused"), Effect->Apply(Statless));
    TestFalse(TEXT("A null target is refused"), Effect->Apply(nullptr));
    TestFalse(TEXT("Refused means not applied"), Effect->IsApplied());

    Effect->Magnitude = 0.0f;
    TestFalse(TEXT("A zero magnitude is refused"), Effect->Apply(Scene.Player));
    TestEqual(TEXT("No modifier was added"), Scene.Stats->GetModifiers().Num(), 0);

    Effect->Remove();
    TestEqual(TEXT("Removing an effect that never applied is harmless"), Scene.MoveSpeed(), EffectTest::BaseMoveSpeed);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTemporaryEffectRemoveAfterStatsGoneTest,
    "GrantMoney.Effects.TemporaryEffect.RemoveAfterTargetIsGone",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTemporaryEffectRemoveAfterStatsGoneTest::RunTest(const FString& Parameters)
{
    EffectTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    UStatModifierEffect* Effect = NewObject<UStatModifierEffect>();
    TestTrue(TEXT("Applied"), Effect->Apply(Scene.Player));

    Scene.Player->Destroy();
    Scene.Step(0.05f);

    Effect->Remove();
    TestFalse(TEXT("Removal after the target is destroyed is safe"), Effect->IsApplied());
    TestFalse(TEXT("The handle is cleared"), Effect->GetModifierHandle().IsValid());
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
