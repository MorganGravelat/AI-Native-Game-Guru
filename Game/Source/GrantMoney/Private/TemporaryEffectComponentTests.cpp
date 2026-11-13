#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/StaticMeshComponent.h"
#include "TemporaryEffect.h"
#include "TemporaryEffectComponent.h"
#include "Tests/TemporaryEffectTestScene.h"

// The runner for temporary effects (SCRUM-39): durations, expiry, stacking, the visible aura, and that no
// buff can outlive its timer or leave a stat changed.

namespace EffectComponentTest
{
    UStatModifierEffect* MakeSpeedBuff(float Magnitude, float Duration, FName StackKey = NAME_None)
    {
        UStatModifierEffect* Template = NewObject<UStatModifierEffect>();
        Template->Stat = EGrantMoneyStat::MoveSpeed;
        Template->ModifierOp = EGrantMoneyStatModifierOp::Multiplicative;
        Template->Magnitude = Magnitude;
        Template->Duration = Duration;
        Template->StackKey = StackKey;
        return Template;
    }

    UStatModifierEffect* MakePermanentSpeedBuff(float Magnitude, FName StackKey = NAME_None)
    {
        UStatModifierEffect* Template = NewObject<UStatModifierEffect>();
        Template->Stat = EGrantMoneyStat::MoveSpeed;
        Template->ModifierOp = EGrantMoneyStatModifierOp::Multiplicative;
        Template->Magnitude = Magnitude;
        Template->bPermanent = true;
        Template->StackKey = StackKey;
        return Template;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTemporaryEffectComponentDurationTest,
    "GrantMoney.Effects.TemporaryEffectComponent.DurationAndMagnitudeAreConfigurable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTemporaryEffectComponentDurationTest::RunTest(const FString& Parameters)
{
    EffectTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    UTemporaryEffectComponent* Effects = UTemporaryEffectComponent::FindOrAdd(Scene.Player);
    if (!TestNotNull(TEXT("Component added on demand"), Effects))
    {
        return false;
    }
    TestTrue(TEXT("FindOrAdd reuses the existing component"), UTemporaryEffectComponent::FindOrAdd(Scene.Player) == Effects);

    UStatModifierEffect* Template = EffectComponentTest::MakeSpeedBuff(0.25f, 2.0f);
    UTemporaryEffect* Applied = Effects->ApplyEffect(Template);
    if (!TestNotNull(TEXT("Applied"), Applied))
    {
        return false;
    }

    TestTrue(TEXT("A copy is applied, not the template"), Applied != Template);
    TestFalse(TEXT("The template is never applied"), Template->IsApplied());
    TestEqual(TEXT("Magnitude: 600 x 1.25 = 750"), Scene.MoveSpeed(), 750.0f);
    TestEqual(TEXT("Duration: 2 s left"), Effects->GetRemainingSeconds(Applied), 2.0f);

    Scene.Step(1.0f);
    TestEqual(TEXT("Still active after 1 s"), Scene.MoveSpeed(), 750.0f);
    TestTrue(TEXT("About 1 s left"), FMath::IsNearlyEqual(Effects->GetRemainingSeconds(Applied), 1.0f, 0.05f));

    Scene.Step(1.1f);
    TestEqual(TEXT("Expired: back to 600"), Scene.MoveSpeed(), EffectTest::BaseMoveSpeed);
    TestEqual(TEXT("No effects left"), Effects->GetActiveEffectCount(), 0);
    TestEqual(TEXT("No modifiers left"), Scene.Stats->GetModifiers().Num(), 0);
    TestFalse(TEXT("Stops ticking with nothing to do"), Effects->IsComponentTickEnabled());

    TestNotNull(TEXT("The same template can be applied again"), Effects->ApplyEffect(Template));
    TestEqual(TEXT("Second use works the same"), Scene.MoveSpeed(), 750.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTemporaryEffectComponentBaseStatsTest,
    "GrantMoney.Effects.TemporaryEffectComponent.ExpiryNeverCorruptsBaseStats",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTemporaryEffectComponentBaseStatsTest::RunTest(const FString& Parameters)
{
    EffectTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    // A permanent upgrade before the buff and another one during it.
    Scene.Stats->AddModifier(EGrantMoneyStat::MoveSpeed, EGrantMoneyStatModifierOp::Additive, 100.0f);

    UTemporaryEffectComponent* Effects = UTemporaryEffectComponent::FindOrAdd(Scene.Player);
    Effects->ApplyEffect(EffectComponentTest::MakeSpeedBuff(0.5f, 1.0f));
    TestEqual(TEXT("(600 + 100) x 1.5"), Scene.MoveSpeed(), 1050.0f);

    Scene.Stats->AddModifier(EGrantMoneyStat::MoveSpeed, EGrantMoneyStatModifierOp::Multiplicative, 0.1f);
    TestEqual(TEXT("Upgrade during the buff: 700 x 1.6"), Scene.MoveSpeed(), 1120.0f);

    Scene.Step(1.2f);
    TestEqual(TEXT("After expiry only the permanent upgrades remain: 700 x 1.1"), Scene.MoveSpeed(), 770.0f);
    TestEqual(TEXT("Base value never changed"), Scene.Stats->GetBaseStat(EGrantMoneyStat::MoveSpeed), EffectTest::BaseMoveSpeed);
    TestEqual(TEXT("Both permanent modifiers kept"), Scene.Stats->GetModifiers().Num(), 2);

    Effects->RemoveAllEffects();
    TestEqual(TEXT("Removing again changes nothing"), Scene.MoveSpeed(), 770.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTemporaryEffectComponentStackingTest,
    "GrantMoney.Effects.TemporaryEffectComponent.StackKeyRefreshesInsteadOfStacking",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTemporaryEffectComponentStackingTest::RunTest(const FString& Parameters)
{
    EffectTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    UTemporaryEffectComponent* Effects = UTemporaryEffectComponent::FindOrAdd(Scene.Player);

    UStatModifierEffect* Keyed = EffectComponentTest::MakeSpeedBuff(0.5f, 2.0f, TEXT("Test.Speed"));
    UTemporaryEffect* First = Effects->ApplyEffect(Keyed);
    Scene.Step(1.5f);
    UTemporaryEffect* Second = Effects->ApplyEffect(Keyed);

    TestTrue(TEXT("Same key: the running effect is returned"), First == Second);
    TestEqual(TEXT("Still one effect"), Effects->GetActiveEffectCount(), 1);
    TestEqual(TEXT("Timer restarted to 2 s"), Effects->GetRemainingSeconds(First), 2.0f);
    TestEqual(TEXT("Not stacked: still 900"), Scene.MoveSpeed(), 900.0f);

    Effects->RemoveAllEffects();

    UStatModifierEffect* Unkeyed = EffectComponentTest::MakeSpeedBuff(0.5f, 2.0f);
    Effects->ApplyEffect(Unkeyed);
    Effects->ApplyEffect(Unkeyed);
    TestEqual(TEXT("No key: two effects"), Effects->GetActiveEffectCount(), 2);
    TestEqual(TEXT("Stacked: 600 x 2"), Scene.MoveSpeed(), 1200.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTemporaryEffectComponentAuraTest,
    "GrantMoney.Effects.TemporaryEffectComponent.AuraShowsWhileActive",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTemporaryEffectComponentAuraTest::RunTest(const FString& Parameters)
{
    EffectTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    UTemporaryEffectComponent* Effects = UTemporaryEffectComponent::FindOrAdd(Scene.Player);
    TestEqual(TEXT("No aura before"), Scene.AuraLightCount(), 0);

    UTemporaryEffect* Applied = Effects->ApplyEffect(EffectComponentTest::MakeSpeedBuff(0.5f, 1.0f));
    TestEqual(TEXT("One aura light while active"), Scene.AuraLightCount(), 1);
    if (TestEqual(TEXT("One active effect"), Effects->GetActiveEffects().Num(), 1))
    {
        const UStaticMeshComponent* Ring = Effects->GetActiveEffects()[0].AuraRing;
        TestTrue(TEXT("A ring is drawn at the feet"), Ring != nullptr && Ring->IsRegistered());
    }

    Scene.Step(1.2f);
    TestEqual(TEXT("Aura removed on expiry"), Scene.AuraLightCount(), 0);
    TestFalse(TEXT("The expired copy is no longer applied"), Applied->IsApplied());

    UStatModifierEffect* Quiet = EffectComponentTest::MakeSpeedBuff(0.5f, 1.0f);
    Quiet->bShowAura = false;
    Effects->ApplyEffect(Quiet);
    TestEqual(TEXT("No aura when turned off"), Scene.AuraLightCount(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTemporaryEffectComponentCleanupTest,
    "GrantMoney.Effects.TemporaryEffectComponent.DestroyedComponentEndsEverything",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTemporaryEffectComponentCleanupTest::RunTest(const FString& Parameters)
{
    EffectTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    UTemporaryEffectComponent* Effects = UTemporaryEffectComponent::FindOrAdd(Scene.Player);
    Effects->ApplyEffect(EffectComponentTest::MakeSpeedBuff(0.5f, 60.0f));
    Effects->ApplyEffect(NewObject<UTestCountingEffect>());
    TestEqual(TEXT("Two running"), Effects->GetActiveEffectCount(), 2);

    Effects->DestroyComponent();
    TestEqual(TEXT("Speed restored the moment the runner goes away"), Scene.MoveSpeed(), EffectTest::BaseMoveSpeed);
    TestEqual(TEXT("No modifiers left behind"), Scene.Stats->GetModifiers().Num(), 0);
    TestEqual(TEXT("The other effect was removed exactly once"), UTestCountingEffect::Removes, 1);
    TestEqual(TEXT("Aura removed"), Scene.AuraLightCount(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTemporaryEffectComponentPauseTest,
    "GrantMoney.Effects.TemporaryEffectComponent.PauseFreezesTheDuration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTemporaryEffectComponentPauseTest::RunTest(const FString& Parameters)
{
    // SCRUM-39: a buff's duration must not drain while the pause menu or a level-up choice holds the
    // game paused (UTemporaryEffectComponent ticks with bTickEvenWhenPaused = false, the same policy
    // ARunController and AEnemySpawner use).
    EffectTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    UTemporaryEffectComponent* Effects = UTemporaryEffectComponent::FindOrAdd(Scene.Player);
    UTemporaryEffect* Applied = Effects->ApplyEffect(EffectComponentTest::MakeSpeedBuff(0.5f, 5.0f));
    Scene.Step(1.0f);
    const float RemainingBeforePause = Effects->GetRemainingSeconds(Applied);
    TestTrue(TEXT("Some time has passed"), RemainingBeforePause < 5.0f);

    Scene.SetPaused(true);
    Scene.Step(5.0f);
    TestEqual(TEXT("A paused world does not advance the buff's timer"), Effects->GetRemainingSeconds(Applied), RemainingBeforePause);
    TestEqual(TEXT("The stat stays buffed while paused"), Scene.MoveSpeed(), 900.0f);

    Scene.SetPaused(false);
    Scene.Step(RemainingBeforePause + 0.5f);
    TestEqual(TEXT("Resumes counting down and expires once unpaused"), Scene.MoveSpeed(), EffectTest::BaseMoveSpeed);
    TestEqual(TEXT("No effects left"), Effects->GetActiveEffectCount(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTemporaryEffectComponentPermanentNeverExpiresTest,
    "GrantMoney.Effects.TemporaryEffectComponent.PermanentEffectNeverExpiresAndNeedsNoTick",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTemporaryEffectComponentPermanentNeverExpiresTest::RunTest(const FString& Parameters)
{
    // Small, permanent totem rewards (SCRUM-39 follow-up): no timer, no expiry, no countdown to show.
    EffectTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    UTemporaryEffectComponent* Effects = UTemporaryEffectComponent::FindOrAdd(Scene.Player);
    UTemporaryEffect* Applied = Effects->ApplyEffect(EffectComponentTest::MakePermanentSpeedBuff(0.1f));
    if (!TestNotNull(TEXT("Applied"), Applied))
    {
        return false;
    }

    TestEqual(TEXT("600 x 1.1"), Scene.MoveSpeed(), 660.0f);
    TestEqual(TEXT("No countdown to report"), Effects->GetRemainingSeconds(Applied), -1.0f);
    TestFalse(TEXT("Nothing to tick for: a permanent effect alone needs no clock"), Effects->IsComponentTickEnabled());

    Scene.Step(60.0f);
    TestEqual(TEXT("Still buffed after a long time"), Scene.MoveSpeed(), 660.0f);
    TestEqual(TEXT("Still the one effect"), Effects->GetActiveEffectCount(), 1);

    TestTrue(TEXT("Still removable explicitly"), Effects->RemoveEffect(Applied));
    TestEqual(TEXT("Removed on request"), Scene.MoveSpeed(), EffectTest::BaseMoveSpeed);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTemporaryEffectComponentPermanentStackKeyIsNoOpTest,
    "GrantMoney.Effects.TemporaryEffectComponent.PermanentStackKeyRepeatIsANoOp",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTemporaryEffectComponentPermanentStackKeyIsNoOpTest::RunTest(const FString& Parameters)
{
    EffectTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    UTemporaryEffectComponent* Effects = UTemporaryEffectComponent::FindOrAdd(Scene.Player);
    UStatModifierEffect* Template = EffectComponentTest::MakePermanentSpeedBuff(0.1f, TEXT("Totem.MoveSpeed"));

    UTemporaryEffect* First = Effects->ApplyEffect(Template);
    UTemporaryEffect* Second = Effects->ApplyEffect(Template);

    TestTrue(TEXT("Same key: the same permanent effect is returned, not a new one"), First == Second);
    TestEqual(TEXT("Still one effect"), Effects->GetActiveEffectCount(), 1);
    TestEqual(TEXT("Not doubled: still just +10%"), Scene.MoveSpeed(), 660.0f);
    TestEqual(TEXT("One modifier, not two"), Scene.Stats->GetModifiers().Num(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTemporaryEffectComponentNewTypeTest,
    "GrantMoney.Effects.TemporaryEffectComponent.NewEffectTypesNeedNoChanges",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTemporaryEffectComponentNewTypeTest::RunTest(const FString& Parameters)
{
    EffectTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    UTemporaryEffectComponent* Effects = UTemporaryEffectComponent::FindOrAdd(Scene.Player);

    UTestCountingEffect* Template = NewObject<UTestCountingEffect>();
    Template->Duration = 0.5f;
    TestNotNull(TEXT("A new effect type runs through the same component"), Effects->ApplyEffect(Template));
    TestEqual(TEXT("Applied once"), UTestCountingEffect::Applies, 1);

    Scene.Step(0.6f);
    TestEqual(TEXT("Removed once on expiry"), UTestCountingEffect::Removes, 1);
    TestEqual(TEXT("Speed never touched"), Scene.MoveSpeed(), EffectTest::BaseMoveSpeed);

    Template->bRefuse = true;
    TestNull(TEXT("A refusing effect is not kept"), Effects->ApplyEffect(Template));
    TestEqual(TEXT("Nothing running"), Effects->GetActiveEffectCount(), 0);
    TestEqual(TEXT("No aura for a refused effect"), Scene.AuraLightCount(), 0);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
