#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/StaticMeshComponent.h"
#include "GrantMoneyPlayerCharacter.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "RewardTotem.h"
#include "TemporaryEffect.h"
#include "TemporaryEffectComponent.h"
#include "Tests/TemporaryEffectTestScene.h"

// Reward totems (SCRUM-39): the bar fills only while the player stands in range, a full bar gives the
// player the totem's temporary effects, and the totem is then spent (or cools down, if reusable).

namespace TotemTest
{
    const FVector Near(100.0, 0.0, 100.0);
    const FVector Far(2000.0, 0.0, 100.0);

    // A totem at the origin that any ACharacter can charge in 1 s. Configure runs before BeginPlay.
    ARewardTotem* SpawnTotem(EffectTest::FScene& Scene, TFunctionRef<void(ARewardTotem&)> Configure = [](ARewardTotem&) {})
    {
        const FTransform Transform(FVector::ZeroVector);
        ARewardTotem* Totem = Scene.World()->SpawnActorDeferred<ARewardTotem>(ARewardTotem::StaticClass(), Transform, nullptr, nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (Totem)
        {
            Totem->ActivatorClass = ACharacter::StaticClass();
            Totem->ChargeSeconds = 1.0f;
            Configure(*Totem);
            Totem->FinishSpawning(Transform);
        }
        return Totem;
    }

    UStatModifierEffect* SpeedEffect(ARewardTotem& Totem)
    {
        return Totem.Effects.Num() > 0 ? Cast<UStatModifierEffect>(Totem.Effects[0]) : nullptr;
    }

    int32 ActiveEffects(const AActor* Actor)
    {
        const UTemporaryEffectComponent* Effects = Actor->FindComponentByClass<UTemporaryEffectComponent>();
        return Effects ? Effects->GetActiveEffectCount() : 0;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRewardTotemDefaultsTest,
    "GrantMoney.Rewards.Totem.Defaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRewardTotemDefaultsTest::RunTest(const FString& Parameters)
{
    const ARewardTotem* Totem = GetDefault<ARewardTotem>();
    TestTrue(TEXT("Only the player charges it"), Totem->ActivatorClass == AGrantMoneyPlayerCharacter::StaticClass());
    TestTrue(TEXT("Single use"), Totem->bSingleUse);
    TestTrue(TEXT("Takes a few seconds to charge"), Totem->ChargeSeconds > 1.0f);

    if (!TestEqual(TEXT("One reward by default"), Totem->Effects.Num(), 1))
    {
        return false;
    }

    const UStatModifierEffect* Speed = Cast<UStatModifierEffect>(Totem->Effects[0]);
    if (!TestNotNull(TEXT("The default reward is a stat modifier"), Speed))
    {
        return false;
    }
    TestTrue(TEXT("It boosts move speed"), Speed->Stat == EGrantMoneyStat::MoveSpeed);
    TestTrue(TEXT("By a percentage"), Speed->ModifierOp == EGrantMoneyStatModifierOp::Multiplicative);
    TestEqual(TEXT("+50%"), Speed->Magnitude, 0.5f);
    TestEqual(TEXT("For 8 s"), Speed->Duration, 8.0f);
    TestFalse(TEXT("With a stack key, so two totems refresh rather than stack"), Speed->StackKey.IsNone());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRewardTotemChargeTest,
    "GrantMoney.Rewards.Totem.ChargesOnlyWhilePlayerIsNear",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRewardTotemChargeTest::RunTest(const FString& Parameters)
{
    EffectTest::FScene Scene;
    if (!Scene.Create(*this, TotemTest::Far))
    {
        return false;
    }

    ARewardTotem* Totem = TotemTest::SpawnTotem(Scene);
    if (!TestNotNull(TEXT("Totem spawned"), Totem))
    {
        return false;
    }

    Scene.Step(0.5f);
    TestEqual(TEXT("Far away: no charge"), Totem->GetChargeProgress(), 0.0f);
    TestFalse(TEXT("Empty bar is hidden"), Totem->IsBarVisible());

    Scene.Player->SetActorLocation(TotemTest::Near);
    Scene.Step(0.5f);
    TestTrue(TEXT("Near: half full after 0.5 s"), FMath::IsNearlyEqual(Totem->GetChargeProgress(), 0.5f, 0.05f));
    TestTrue(TEXT("Bar is shown while charging"), Totem->IsBarVisible());
    TestEqual(TEXT("Nothing given before the bar is full"), Scene.MoveSpeed(), EffectTest::BaseMoveSpeed);

    Scene.Player->SetActorLocation(TotemTest::Far);
    Scene.Step(0.25f);
    TestTrue(TEXT("Leaving drains the bar"), FMath::IsNearlyEqual(Totem->GetChargeProgress(), 0.25f, 0.05f));
    TestTrue(TEXT("Still usable"), Totem->GetState() == ERewardTotemState::Ready);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRewardTotemActivateTest,
    "GrantMoney.Rewards.Totem.FullBarGivesTemporaryBuff",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRewardTotemActivateTest::RunTest(const FString& Parameters)
{
    EffectTest::FScene Scene;
    if (!Scene.Create(*this, TotemTest::Near))
    {
        return false;
    }

    ARewardTotem* Totem = TotemTest::SpawnTotem(Scene, [](ARewardTotem& Config)
    {
        TotemTest::SpeedEffect(Config)->Duration = 2.0f;
    });
    if (!TestNotNull(TEXT("Totem spawned"), Totem))
    {
        return false;
    }

    Scene.Step(1.2f);
    TestTrue(TEXT("Activated and spent"), Totem->GetState() == ERewardTotemState::Spent);
    TestEqual(TEXT("Buff active: 600 x 1.5"), Scene.MoveSpeed(), 900.0f);
    TestEqual(TEXT("One effect on the player"), TotemTest::ActiveEffects(Scene.Player), 1);
    TestEqual(TEXT("Visible aura on the player"), Scene.AuraLightCount(), 1);
    TestFalse(TEXT("Bar hidden once spent"), Totem->IsBarVisible());
    TestFalse(TEXT("The totem's own template is untouched"), TotemTest::SpeedEffect(*Totem)->IsApplied());

    Scene.Step(1.0f);
    TestEqual(TEXT("Standing there does not give it again"), TotemTest::ActiveEffects(Scene.Player), 1);

    Scene.Step(1.2f);
    TestEqual(TEXT("Expired: back to 600"), Scene.MoveSpeed(), EffectTest::BaseMoveSpeed);
    TestEqual(TEXT("Base never changed"), Scene.Stats->GetBaseStat(EGrantMoneyStat::MoveSpeed), EffectTest::BaseMoveSpeed);
    TestEqual(TEXT("No modifiers left"), Scene.Stats->GetModifiers().Num(), 0);
    TestEqual(TEXT("Aura gone"), Scene.AuraLightCount(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRewardTotemCooldownTest,
    "GrantMoney.Rewards.Totem.ReusableTotemCoolsDown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRewardTotemCooldownTest::RunTest(const FString& Parameters)
{
    EffectTest::FScene Scene;
    if (!Scene.Create(*this, TotemTest::Near))
    {
        return false;
    }

    ARewardTotem* Totem = TotemTest::SpawnTotem(Scene, [](ARewardTotem& Config)
    {
        Config.bSingleUse = false;
        Config.CooldownSeconds = 1.0f;
    });
    if (!TestNotNull(TEXT("Totem spawned"), Totem))
    {
        return false;
    }

    Scene.Step(1.1f);
    TestTrue(TEXT("Cooling down after use"), Totem->GetState() == ERewardTotemState::Cooldown);

    Scene.Step(0.5f);
    TestEqual(TEXT("No charge during cooldown"), Totem->GetChargeProgress(), 0.0f);

    Scene.Step(0.6f);
    TestTrue(TEXT("Ready again"), Totem->GetState() == ERewardTotemState::Ready);

    Scene.Step(1.1f);
    TestTrue(TEXT("Used a second time"), Totem->GetState() == ERewardTotemState::Cooldown);
    TestEqual(TEXT("Same stack key: refreshed, not stacked"), TotemTest::ActiveEffects(Scene.Player), 1);
    TestEqual(TEXT("Still +50%"), Scene.MoveSpeed(), 900.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRewardTotemActivatorClassTest,
    "GrantMoney.Rewards.Totem.OnlyActivatorClassCharges",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRewardTotemActivatorClassTest::RunTest(const FString& Parameters)
{
    EffectTest::FScene Scene;
    if (!Scene.Create(*this, TotemTest::Near))
    {
        return false;
    }

    // The stand-in is a plain ACharacter, so a totem left on the player class ignores it, like it ignores enemies.
    ARewardTotem* Totem = TotemTest::SpawnTotem(Scene, [](ARewardTotem& Config)
    {
        Config.ActivatorClass = AGrantMoneyPlayerCharacter::StaticClass();
    });
    if (!TestNotNull(TEXT("Totem spawned"), Totem))
    {
        return false;
    }

    Scene.Step(1.5f);
    TestEqual(TEXT("Other pawns do not charge it"), Totem->GetChargeProgress(), 0.0f);
    TestEqual(TEXT("Nothing given"), Scene.MoveSpeed(), EffectTest::BaseMoveSpeed);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRewardTotemNotConsumedTest,
    "GrantMoney.Rewards.Totem.NotUsedUpWhenNothingApplies",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRewardTotemNotConsumedTest::RunTest(const FString& Parameters)
{
    EffectTest::FScene Scene;
    if (!Scene.Create(*this, TotemTest::Far))
    {
        return false;
    }

    ARewardTotem* Totem = TotemTest::SpawnTotem(Scene);
    if (!TestNotNull(TEXT("Totem spawned"), Totem))
    {
        return false;
    }

    AddExpectedMessage(TEXT("has no Stats component"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);
    AddExpectedMessage(TEXT("was not used up"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);
    AActor* Statless = Scene.World()->SpawnActor<AActor>();
    TestEqual(TEXT("Nothing applied"), Totem->Activate(Statless), 0);
    TestTrue(TEXT("Still ready"), Totem->GetState() == ERewardTotemState::Ready);

    TestEqual(TEXT("The player still gets it"), Totem->Activate(Scene.Player), 1);
    TestTrue(TEXT("Spent now"), Totem->GetState() == ERewardTotemState::Spent);
    TestEqual(TEXT("A spent totem gives nothing more"), Totem->Activate(Scene.Player), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRewardTotemNewRewardTypeTest,
    "GrantMoney.Rewards.Totem.NewRewardTypesNeedNoChanges",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRewardTotemNewRewardTypeTest::RunTest(const FString& Parameters)
{
    EffectTest::FScene Scene;
    if (!Scene.Create(*this, TotemTest::Far))
    {
        return false;
    }

    ARewardTotem* Totem = TotemTest::SpawnTotem(Scene, [](ARewardTotem& Config)
    {
        UTestCountingEffect* Custom = NewObject<UTestCountingEffect>(&Config);
        Custom->Duration = 0.5f;
        Config.Effects.Add(Custom);
    });
    if (!TestNotNull(TEXT("Totem spawned"), Totem))
    {
        return false;
    }

    TestEqual(TEXT("Both rewards given"), Totem->Activate(Scene.Player), 2);
    TestEqual(TEXT("The new type applied once"), UTestCountingEffect::Applies, 1);
    TestEqual(TEXT("The speed buff applied alongside it"), Scene.MoveSpeed(), 900.0f);

    Scene.Step(0.6f);
    TestEqual(TEXT("The new type ends on its own timer"), UTestCountingEffect::Removes, 1);
    TestEqual(TEXT("The speed buff keeps its own timer"), Scene.MoveSpeed(), 900.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRewardTotemCustomLookTest,
    "GrantMoney.Rewards.Totem.CustomMeshAndMaterial",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRewardTotemCustomLookTest::RunTest(const FString& Parameters)
{
    EffectTest::FScene Scene;
    if (!Scene.Create(*this, TotemTest::Far))
    {
        return false;
    }

    // Engine assets stand in for a designer's mesh and material.
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
    if (!TestNotNull(TEXT("Cube loaded"), Cube) || !TestNotNull(TEXT("Material loaded"), Material))
    {
        return false;
    }

    auto PillarOf = [](const ARewardTotem* Totem) -> UStaticMeshComponent*
    {
        TArray<UStaticMeshComponent*> Meshes;
        Totem->GetComponents(Meshes);
        UStaticMeshComponent* const* Found = Meshes.FindByPredicate([](const UStaticMeshComponent* Mesh) { return Mesh->GetFName() == TEXT("TotemMesh"); });
        return Found ? *Found : nullptr;
    };

    ARewardTotem* Default = TotemTest::SpawnTotem(Scene);
    ARewardTotem* MeshOnly = TotemTest::SpawnTotem(Scene, [Cube](ARewardTotem& Config)
    {
        Config.CustomMesh = Cube;
        Config.CustomMeshScale = FVector(2.0);
    });
    ARewardTotem* WithMaterial = TotemTest::SpawnTotem(Scene, [Cube, Material](ARewardTotem& Config)
    {
        Config.CustomMesh = Cube;
        Config.CustomMaterial = Material;
    });
    if (!TestNotNull(TEXT("Spawned"), Default) || !TestNotNull(TEXT("Spawned"), MeshOnly) || !TestNotNull(TEXT("Spawned"), WithMaterial))
    {
        return false;
    }

    TestTrue(TEXT("Default pillar glows"), Cast<UMaterialInstanceDynamic>(PillarOf(Default)->GetMaterial(0)) != nullptr);

    TestTrue(TEXT("Custom mesh used"), PillarOf(MeshOnly)->GetStaticMesh() == Cube);
    TestTrue(TEXT("Custom mesh keeps its own material (no glow)"), Cast<UMaterialInstanceDynamic>(PillarOf(MeshOnly)->GetMaterial(0)) == nullptr);
    TestTrue(TEXT("Scaled"), PillarOf(MeshOnly)->GetRelativeScale3D().Equals(FVector(2.0)));
    TestTrue(TEXT("Bottom on the ground: 100 cm cube at scale 2 is lifted 100 cm"),
        FMath::IsNearlyEqual(PillarOf(MeshOnly)->GetRelativeLocation().Z, 100.0, 0.5));

    TestTrue(TEXT("Custom material used"), PillarOf(WithMaterial)->GetMaterial(0) == Material);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
