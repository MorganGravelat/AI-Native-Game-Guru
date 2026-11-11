#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "GrantMoneyPlayerCharacter.h"
#include "Materials/MaterialInterface.h"
#include "StatBoostPickup.h"
#include "Tests/PickupTestScene.h"

// Stat boost pickups (SCRUM-53): a permanent stat modifier applied once on arrival, through the
// existing pickup pipeline, plus the ready-made boosts' defaults and the icon/sphere visual switch.

namespace StatBoostTest
{
    // The weapon's base damage in the game. Boosts are measured on top of it, as the weapon does.
    constexpr float WeaponBaseDamage = 10.0f;

    // How many modifiers the collector holds for Stat.
    int32 CountModifiers(const UStatsComponent* Stats, EGrantMoneyStat Stat)
    {
        int32 Count = 0;
        for (const FGrantMoneyStatModifier& Modifier : Stats->GetModifiers())
        {
            Count += Modifier.Stat == Stat ? 1 : 0;
        }
        return Count;
    }

    // Spawns a pickup of type T, lets Configure change it, then begins its play.
    template <typename T>
    T* SpawnConfigured(PickupTest::FScene& Scene, const FVector& Location, TFunctionRef<void(T&)> Configure)
    {
        const FTransform Transform(Location);
        T* Pickup = Scene.World()->SpawnActorDeferred<T>(T::StaticClass(), Transform, nullptr, nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (Pickup)
        {
            Configure(*Pickup);
            Pickup->FinishSpawning(Transform);
        }
        return Pickup;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatBoostReadyMadeDefaultsTest,
    "GrantMoney.Pickups.StatBoost.ReadyMadeDefaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStatBoostReadyMadeDefaultsTest::RunTest(const FString& Parameters)
{
    const AStatBoostPickup* Health = GetDefault<AMaxHealthBoostPickup>();
    TestTrue(TEXT("Max health boosts MaxHealth"), Health->Stat == EGrantMoneyStat::MaxHealth);
    TestTrue(TEXT("Max health is a flat amount"), Health->ModifierOp == EGrantMoneyStatModifierOp::Additive);
    TestEqual(TEXT("Max health amount"), Health->Amount, 10.0f);

    const AStatBoostPickup* Damage = GetDefault<ADamageBoostPickup>();
    TestTrue(TEXT("Damage boosts Damage"), Damage->Stat == EGrantMoneyStat::Damage);
    TestEqual(TEXT("Damage amount"), Damage->Amount, 2.0f);

    const AStatBoostPickup* Speed = GetDefault<AAttackSpeedBoostPickup>();
    TestTrue(TEXT("Attack speed boosts AttackInterval"), Speed->Stat == EGrantMoneyStat::AttackInterval);
    TestTrue(TEXT("Attack speed is a percentage"), Speed->ModifierOp == EGrantMoneyStatModifierOp::Multiplicative);
    TestTrue(TEXT("Attack speed shortens the interval (negative amount)"), Speed->Amount < 0.0f);

    const AStatBoostPickup* Penetration = GetDefault<APenetrationBoostPickup>();
    TestTrue(TEXT("Penetration boosts Penetration"), Penetration->Stat == EGrantMoneyStat::Penetration);
    TestEqual(TEXT("Penetration adds one enemy per pickup"), Penetration->Amount, 1.0f);

    const AStatBoostPickup* Contact = GetDefault<AContactDamageBoostPickup>();
    TestTrue(TEXT("Contact damage boosts ContactDamage"), Contact->Stat == EGrantMoneyStat::ContactDamage);
    TestTrue(TEXT("Contact damage amount is positive"), Contact->Amount > 0.0f);

    TestTrue(TEXT("Same pursuit tuning as every pickup"), Damage->MaxSpeed == GetDefault<AXpOrbPickup>()->MaxSpeed);
    TestTrue(TEXT("Icon parameter matches Paper2D sprite materials"), Damage->IconTextureParameterName == FName(TEXT("SpriteTexture")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatBoostAppliesOnArrivalTest,
    "GrantMoney.Pickups.StatBoost.AppliesOnceOnArrival",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStatBoostAppliesOnArrivalTest::RunTest(const FString& Parameters)
{
    PickupTest::FScene Scene;
    if (!Scene.Create(*this, 300.0f))
    {
        return false;
    }

    ADamageBoostPickup* Boost = Scene.SpawnPickup<ADamageBoostPickup>(FVector(250.0, 0.0, 0.0));
    TestNotNull(TEXT("The boost spawned"), Boost);

    Scene.Step(0.05f);
    TestEqual(TEXT("Nothing applied before arrival"), StatBoostTest::CountModifiers(Scene.Stats, EGrantMoneyStat::Damage), 0);

    Scene.Step(1.5f);
    TestEqual(TEXT("One modifier on arrival"), StatBoostTest::CountModifiers(Scene.Stats, EGrantMoneyStat::Damage), 1);
    TestEqual(TEXT("Weapon damage 10 becomes 12"),
        Scene.Stats->GetStatFromBase(EGrantMoneyStat::Damage, StatBoostTest::WeaponBaseDamage), 12.0f);
    TestEqual(TEXT("Consumed instantly: the pickup is gone"), Scene.CountPickups<AStatBoostPickup>(), 0);

    Scene.Step(1.0f);
    TestEqual(TEXT("Permanent and applied only once"), StatBoostTest::CountModifiers(Scene.Stats, EGrantMoneyStat::Damage), 1);
    TestEqual(TEXT("Other stats untouched"), Scene.Experience(), 0.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatBoostFlatAndPercentStackTest,
    "GrantMoney.Pickups.StatBoost.FlatAndPercentStack",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStatBoostFlatAndPercentStackTest::RunTest(const FString& Parameters)
{
    PickupTest::FScene Scene;
    if (!Scene.Create(*this, 300.0f))
    {
        return false;
    }

    Scene.SpawnPickup<ADamageBoostPickup>(FVector(150.0, 0.0, 0.0));
    Scene.SpawnPickup<ADamageBoostPickup>(FVector(0.0, 150.0, 0.0));
    StatBoostTest::SpawnConfigured<AStatBoostPickup>(Scene, FVector(-150.0, 0.0, 0.0), [](AStatBoostPickup& Percent)
    {
        Percent.Stat = EGrantMoneyStat::Damage;
        Percent.ModifierOp = EGrantMoneyStatModifierOp::Multiplicative;
        Percent.Amount = 0.5f;
    });

    Scene.Step(2.0f);
    TestEqual(TEXT("All three collected"), Scene.CountPickups<AStatBoostPickup>(), 0);
    TestEqual(TEXT("(10 + 2 + 2) x (1 + 50%) = 21"),
        Scene.Stats->GetStatFromBase(EGrantMoneyStat::Damage, StatBoostTest::WeaponBaseDamage), 21.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatBoostMaxHealthOnlyRaisesMaxTest,
    "GrantMoney.Pickups.StatBoost.MaxHealthRaisesOnlyTheMaximum",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStatBoostMaxHealthOnlyRaisesMaxTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("The player does not heal from max health boosts"),
        GetDefault<AGrantMoneyPlayerCharacter>()->GetHealth()->bHealOnMaxHealthGain);

    PickupTest::FScene Scene;
    if (!Scene.Create(*this, 300.0f))
    {
        return false;
    }

    // Set up like the player: a MaxHealth base on Stats, and a health component that does not heal on gain.
    Scene.Stats->SetBaseStat(EGrantMoneyStat::MaxHealth, 100.0f);
    UHealthComponent* Health = NewObject<UHealthComponent>(Scene.Collector);
    Health->bHealOnMaxHealthGain = false;
    Health->RegisterComponent();
    Health->ApplyDamage(30.0f);
    TestEqual(TEXT("Hurt to 70"), Health->GetHealth(), 70.0f);

    Scene.SpawnPickup<AMaxHealthBoostPickup>(FVector(200.0, 0.0, 0.0));
    Scene.Step(1.5f);

    TestEqual(TEXT("Max health 100 -> 110"), Health->GetMaxHealth(), 110.0f);
    TestEqual(TEXT("Current health is unchanged"), Health->GetHealth(), 70.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatBoostNoStatsTest,
    "GrantMoney.Pickups.StatBoost.NoStatsOrZeroAmountChangesNothing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStatBoostNoStatsTest::RunTest(const FString& Parameters)
{
    PickupTest::FScene Scene;
    if (!Scene.Create(*this, 1.0f))
    {
        return false;
    }

    AStatBoostPickup* Boost = Scene.SpawnPickup<APenetrationBoostPickup>(FVector(1000.0, 0.0, 0.0));
    if (!TestNotNull(TEXT("The boost spawned"), Boost))
    {
        return false;
    }

    // These two calls each log a "has no Stats component" warning, which is expected.
    AddExpectedMessage(TEXT("has no Stats component"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 2, false);
    AActor* Statless = Scene.World()->SpawnActor<AActor>();
    TestFalse(TEXT("No stats component: nothing applied"), Boost->ApplyBoost(Statless));
    TestFalse(TEXT("No collector: nothing applied"), Boost->ApplyBoost(nullptr));

    Boost->Amount = 0.0f;
    TestFalse(TEXT("A zero amount adds no modifier"), Boost->ApplyBoost(Scene.Collector));
    TestEqual(TEXT("Collector has no modifiers"), Scene.Stats->GetModifiers().Num(), 0);

    Boost->Amount = 1.0f;
    TestTrue(TEXT("A real amount applies"), Boost->ApplyBoost(Scene.Collector));
    TestEqual(TEXT("Penetration 0 -> 1"), Scene.Stats->GetStatFromBase(EGrantMoneyStat::Penetration, 0.0f), 1.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatBoostIconOrSphereTest,
    "GrantMoney.Pickups.StatBoost.IconReplacesSphere",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStatBoostIconOrSphereTest::RunTest(const FString& Parameters)
{
    PickupTest::FScene Scene;
    if (!Scene.Create(*this, 1.0f))
    {
        return false;
    }

    // No icon: the glowing placeholder sphere.
    AStatBoostPickup* Plain = Scene.SpawnPickup<ADamageBoostPickup>(FVector(1000.0, 0.0, 0.0));
    TestFalse(TEXT("No icon set: no billboard"), Plain->IsShowingIcon());
    TestTrue(TEXT("No icon set: the sphere is visible"), Plain->FindComponentByClass<UStaticMeshComponent>()->IsVisible());

    // With an icon. An engine material stands in for the default Paper2D one so the test does not
    // depend on that plugin; the real material is checked in the Editor (see the SCRUM-53 guide).
    UTexture2D* TestIcon = UTexture2D::CreateTransient(4, 4);
    AStatBoostPickup* WithIcon = StatBoostTest::SpawnConfigured<ADamageBoostPickup>(Scene, FVector(1000.0, 300.0, 0.0),
        [TestIcon](ADamageBoostPickup& Pickup)
        {
            Pickup.Icon = TestIcon;
            Pickup.IconMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")));
        });
    if (!TestNotNull(TEXT("The icon boost spawned"), WithIcon))
    {
        return false;
    }

    TestTrue(TEXT("Icon shown on the camera-facing billboard"), WithIcon->IsShowingIcon());
    TestFalse(TEXT("The sphere is hidden"), WithIcon->FindComponentByClass<UStaticMeshComponent>()->IsVisible());
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
