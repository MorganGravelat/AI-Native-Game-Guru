#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "RewardTotem.h"
#include "RewardTotemSpawner.h"
#include "Engine/StaticMesh.h"
#include "TemporaryEffect.h"
#include "Tests/AutomationCommon.h"

// Totem placement at level start (SCRUM-39): random, but always on playable ground, clear of anything
// that blocks pawns, away from the player's start and spread apart.

namespace TotemSpawnTest
{
    constexpr int32 N = FGrantMoneyMapData::MapSize;

    // A game world with a synthetic map (playable cells MinCell..MaxCell), a ground slab under part of it,
    // a frozen stand-in player and a spawner configured but not yet run.
    struct FSpawnScene
    {
        FTestWorldWrapper WorldWrapper;
        uint16 Ground[N][N];
        uint8 Valid[N][N];
        FSpawnArena Arena;
        ACharacter* Player = nullptr;
        ARewardTotemSpawner* Spawner = nullptr;

        // SlabMaxX limits the ground slab (world X) so tests can leave part of the map without ground.
        bool Create(FAutomationTestBase& Test, const FIntPoint& MinCell, const FIntPoint& MaxCell, const FVector& PlayerLocation,
            int32 Seed = 2026, double SlabMaxX = TNumericLimits<double>::Max())
        {
            if (!WorldWrapper.CreateTestWorld(EWorldType::Game) || !WorldWrapper.BeginPlayInTestWorld())
            {
                WorldWrapper.ForwardErrorMessages(&Test);
                return false;
            }

            FMemory::Memzero(Ground, sizeof(Ground));
            FMemory::Memzero(Valid, sizeof(Valid));
            for (int32 X = MinCell.X; X <= MaxCell.X; ++X)
            {
                for (int32 Y = MinCell.Y; Y <= MaxCell.Y; ++Y)
                {
                    Valid[X][Y] = 1;
                    Ground[X][Y] = 1;
                }
            }

            Arena.Map.Ground = Ground;
            Arena.Map.Valid = Valid;
            Arena.Map.Width = N;
            Arena.Map.Height = N;

            const FVector Low((MinCell.X - 0.5) * 100.0, (MinCell.Y - 0.5) * 100.0, 0.0);
            const FVector High(FMath::Min((MaxCell.X + 0.5) * 100.0, SlabMaxX), (MaxCell.Y + 0.5) * 100.0, 0.0);
            AddBox(Low, High, -20.0, 0.0, true);

            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            Player = World()->SpawnActor<ACharacter>(ACharacter::StaticClass(), FTransform(PlayerLocation), Params);
            if (!Test.TestNotNull(TEXT("Player spawned"), Player))
            {
                return false;
            }
            Player->GetCharacterMovement()->DisableMovement();

            Spawner = World()->SpawnActorDeferred<ARewardTotemSpawner>(ARewardTotemSpawner::StaticClass(), FTransform::Identity);
            if (!Test.TestNotNull(TEXT("Spawner spawned"), Spawner))
            {
                return false;
            }
            Spawner->RandomSeed = Seed;
            Spawner->TotemCount = 3;
            Spawner->MinDistanceFromPlayer = 600.0f;
            Spawner->MinSpacing = 800.0f;
            Spawner->FinishSpawning(FTransform::Identity);
            Spawner->SetArenaOverride(Arena);
            Spawner->SetAvoidActorOverride(Player);
            return true;
        }

        UWorld* World() const { return WorldWrapper.GetTestWorld(); }

        // A static box from corner Low to corner High (XY), heights BottomZ..TopZ. BlocksTraces: blocks
        // everything, like the ground slabs. Otherwise it blocks only pawns, like a wall or a prop.
        AActor* AddBox(const FVector& Low, const FVector& High, double BottomZ, double TopZ, bool bBlocksTraces)
        {
            AActor* Actor = World()->SpawnActor<AActor>();
            UBoxComponent* Box = NewObject<UBoxComponent>(Actor, TEXT("Box"));
            Box->SetBoxExtent(FVector((High.X - Low.X) * 0.5, (High.Y - Low.Y) * 0.5, (TopZ - BottomZ) * 0.5));
            Actor->SetRootComponent(Box);
            if (bBlocksTraces)
            {
                Box->SetCollisionProfileName(TEXT("BlockAll"));
            }
            else
            {
                Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                Box->SetCollisionObjectType(ECC_WorldStatic);
                Box->SetCollisionResponseToAllChannels(ECR_Ignore);
                Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
            }
            Box->RegisterComponent();
            Box->SetWorldLocation(FVector((Low.X + High.X) * 0.5, (Low.Y + High.Y) * 0.5, (BottomZ + TopZ) * 0.5));
            return Actor;
        }

        TArray<FVector> TotemLocations() const
        {
            TArray<FVector> Result;
            for (const ARewardTotem* Totem : Spawner->GetSpawnedTotems())
            {
                Result.Add(Totem->GetActorLocation());
            }
            return Result;
        }
    };

    // 30 x 30 cells with the player in the middle.
    const FIntPoint MapMin(0, 0);
    const FIntPoint MapMax(29, 29);
    const FVector MapCentre(1450.0, 1450.0, 100.0);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRewardTotemSpawnerCandidatesTest,
    "GrantMoney.Rewards.TotemSpawner.CandidatesStayOffTheMapEdge",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRewardTotemSpawnerCandidatesTest::RunTest(const FString& Parameters)
{
    using namespace TotemSpawnTest;

    static uint16 Ground[N][N];
    static uint8 Valid[N][N];
    FMemory::Memzero(Ground, sizeof(Ground));
    FMemory::Memzero(Valid, sizeof(Valid));
    for (int32 X = 0; X < 10; ++X)
    {
        for (int32 Y = 0; Y < 10; ++Y)
        {
            Ground[X][Y] = 1;
            Valid[X][Y] = 1;
        }
    }

    FSpawnArena Arena;
    Arena.Map.Ground = Ground;
    Arena.Map.Valid = Valid;
    Arena.Map.Width = N;
    Arena.Map.Height = N;

    TArray<FVector> Positions;
    ARewardTotemSpawner::GatherCandidatePositions(Arena, 120.0f, Positions);
    TestEqual(TEXT("A 120 cm footprint rules out the outer ring of a 10 x 10 map: 8 x 8 left"), Positions.Num(), 64);
    for (const FVector& Position : Positions)
    {
        TestTrue(TEXT("Whole footprint is on the map"), Arena.IsFootprintPlayable(Position, 120.0f));
    }

    ARewardTotemSpawner::GatherCandidatePositions(FSpawnArena(), 120.0f, Positions);
    TestEqual(TEXT("No map, no candidates"), Positions.Num(), 0);

    TestTrue(TEXT("Far from all"), ARewardTotemSpawner::IsFarFromAll(FVector(0.0), { FVector(500.0, 0.0, 0.0) }, 400.0f));
    TestFalse(TEXT("Too close to one"), ARewardTotemSpawner::IsFarFromAll(FVector(0.0), { FVector(500.0, 0.0, 0.0), FVector(100.0, 0.0, 900.0) }, 400.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRewardTotemSpawnerPlacementTest,
    "GrantMoney.Rewards.TotemSpawner.SpawnsOnClearGroundAwayFromPlayer",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRewardTotemSpawnerPlacementTest::RunTest(const FString& Parameters)
{
    using namespace TotemSpawnTest;

    FSpawnScene Scene;
    if (!Scene.Create(*this, MapMin, MapMax, MapCentre))
    {
        return false;
    }

    TestEqual(TEXT("All three placed"), Scene.Spawner->SpawnTotems(), 3);
    const TArray<FVector> Locations = Scene.TotemLocations();
    for (int32 Index = 0; Index < Locations.Num(); ++Index)
    {
        const FVector& Location = Locations[Index];
        TestTrue(TEXT("Footprint on playable cells"), Scene.Arena.IsFootprintPlayable(Location, Scene.Spawner->FootprintRadius));
        TestTrue(TEXT("Standing on the ground"), FMath::IsNearlyEqual(Location.Z, 0.0, 1.0));
        TestTrue(TEXT("Away from the player"), FVector::Dist2D(Location, MapCentre) >= Scene.Spawner->MinDistanceFromPlayer);
        for (int32 Other = Index + 1; Other < Locations.Num(); ++Other)
        {
            TestTrue(TEXT("Spread apart"), FVector::Dist2D(Location, Locations[Other]) >= Scene.Spawner->MinSpacing);
        }
    }

    TestEqual(TEXT("Runs once: a second call places nothing"), Scene.Spawner->SpawnTotems(), 0);
    TestEqual(TEXT("Still three"), Scene.Spawner->GetSpawnedTotems().Num(), 3);
    TestTrue(TEXT("Marked as done"), Scene.Spawner->HasSpawned());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRewardTotemSpawnerBlockersTest,
    "GrantMoney.Rewards.TotemSpawner.AvoidsWallsPropsAndMissingGround",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRewardTotemSpawnerBlockersTest::RunTest(const FString& Parameters)
{
    using namespace TotemSpawnTest;

    // No ground slab past X = 2450, so the last ten columns are "playable" in the data but have no floor.
    FSpawnScene Scene;
    if (!Scene.Create(*this, MapMin, MapMax, MapCentre, 7, 2450.0))
    {
        return false;
    }

    // A pawn-blocking block over the low-X third of the map, like a wall or a prop with collision.
    Scene.AddBox(FVector(-50.0, -50.0, 0.0), FVector(950.0, 2950.0, 0.0), 0.0, 300.0, false);
    Scene.Spawner->MinDistanceFromPlayer = 0.0f;
    Scene.Spawner->MinSpacing = 300.0f;
    Scene.Spawner->TotemCount = 4;

    TestEqual(TEXT("All four placed in the clear middle band"), Scene.Spawner->SpawnTotems(), 4);
    for (const FVector& Location : Scene.TotemLocations())
    {
        TestTrue(TEXT("Clear of the blocker, footprint included"), Location.X >= 950.0 + Scene.Spawner->FootprintRadius - 1.0);
        TestTrue(TEXT("Only where there is ground"), Location.X <= 2450.0);
        TestTrue(TEXT("Not on top of the player"), FVector::Dist2D(Location, MapCentre) > Scene.Spawner->FootprintRadius);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRewardTotemSpawnerSeedTest,
    "GrantMoney.Rewards.TotemSpawner.SeedControlsLayout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRewardTotemSpawnerSeedTest::RunTest(const FString& Parameters)
{
    using namespace TotemSpawnTest;

    TArray<FVector> First;
    TArray<FVector> Repeat;
    TArray<FVector> Other;
    {
        FSpawnScene Scene;
        if (!Scene.Create(*this, MapMin, MapMax, MapCentre, 11))
        {
            return false;
        }
        Scene.Spawner->SpawnTotems();
        First = Scene.TotemLocations();
    }
    {
        FSpawnScene Scene;
        if (!Scene.Create(*this, MapMin, MapMax, MapCentre, 11))
        {
            return false;
        }
        Scene.Spawner->SpawnTotems();
        Repeat = Scene.TotemLocations();
    }
    {
        FSpawnScene Scene;
        if (!Scene.Create(*this, MapMin, MapMax, MapCentre, 12))
        {
            return false;
        }
        Scene.Spawner->SpawnTotems();
        Other = Scene.TotemLocations();
    }

    TestEqual(TEXT("Three each time"), First.Num(), 3);
    TestTrue(TEXT("Same seed, same layout"), First == Repeat);
    TestFalse(TEXT("Another seed, another layout"), First == Other);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRewardTotemSpawnerCrowdedTest,
    "GrantMoney.Rewards.TotemSpawner.SmallMapPlacesFewerAndWarns",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRewardTotemSpawnerCrowdedTest::RunTest(const FString& Parameters)
{
    using namespace TotemSpawnTest;

    FSpawnScene Scene;
    if (!Scene.Create(*this, FIntPoint(0, 0), FIntPoint(7, 7), FVector(-5000.0, -5000.0, 100.0)))
    {
        return false;
    }

    Scene.Spawner->TotemCount = 5;
    Scene.Spawner->MinSpacing = 1200.0f;

    AddExpectedMessage(TEXT("RewardTotemSpawner: placed"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);
    const int32 Placed = Scene.Spawner->SpawnTotems();
    TestTrue(TEXT("Some placed"), Placed >= 1);
    TestTrue(TEXT("But not all: the spacing does not fit"), Placed < 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRewardTotemSpawnerOverridesTest,
    "GrantMoney.Rewards.TotemSpawner.LookAndRewardOverrides",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRewardTotemSpawnerOverridesTest::RunTest(const FString& Parameters)
{
    using namespace TotemSpawnTest;

    FSpawnScene Scene;
    if (!Scene.Create(*this, MapMin, MapMax, MapCentre))
    {
        return false;
    }

    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStatModifierEffect* Damage = NewObject<UStatModifierEffect>(Scene.Spawner);
    Damage->Stat = EGrantMoneyStat::Damage;
    Damage->ModifierOp = EGrantMoneyStatModifierOp::Additive;
    Damage->Magnitude = 5.0f;
    Damage->Duration = 3.0f;

    Scene.Spawner->TotemMesh = Cube;
    Scene.Spawner->EffectsOverride.Reset();
    Scene.Spawner->EffectsOverride.Add(Damage);

    TestEqual(TEXT("All three placed"), Scene.Spawner->SpawnTotems(), 3);
    for (const ARewardTotem* Totem : Scene.Spawner->GetSpawnedTotems())
    {
        TestTrue(TEXT("Mesh from the spawner"), Totem->CustomMesh == Cube);
        if (!TestEqual(TEXT("Rewards from the spawner"), Totem->Effects.Num(), 1))
        {
            continue;
        }

        const UStatModifierEffect* Copy = Cast<UStatModifierEffect>(Totem->Effects[0]);
        TestTrue(TEXT("Each totem has its own copy"), Copy != nullptr && Copy != Damage);
        TestTrue(TEXT("Same stat"), Copy && Copy->Stat == EGrantMoneyStat::Damage);
        TestEqual(TEXT("Same duration"), Copy ? Copy->Duration : 0.0f, 3.0f);
        TestEqual(TEXT("Same magnitude"), Copy ? Copy->Magnitude : 0.0f, 5.0f);
    }
    return true;
}

namespace TotemSpawnTest
{
    // The stat each spawned totem's (single) reward changes, in spawn order.
    TArray<EGrantMoneyStat> RewardStats(const ARewardTotemSpawner* Spawner)
    {
        TArray<EGrantMoneyStat> Stats;
        for (const ARewardTotem* Totem : Spawner->GetSpawnedTotems())
        {
            const UStatModifierEffect* Effect = Totem->Effects.Num() == 1 ? Cast<UStatModifierEffect>(Totem->Effects[0]) : nullptr;
            Stats.Add(Effect ? Effect->Stat : EGrantMoneyStat::Experience);
        }
        return Stats;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRewardTotemSpawnerDefaultPoolTest,
    "GrantMoney.Rewards.TotemSpawner.DefaultRandomPool",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRewardTotemSpawnerDefaultPoolTest::RunTest(const FString& Parameters)
{
    const ARewardTotemSpawner* Defaults = GetDefault<ARewardTotemSpawner>();
    if (!TestEqual(TEXT("Four buffs in the default pool"), Defaults->RandomEffectPool.Num(), 4))
    {
        return false;
    }

    TSet<EGrantMoneyStat> Stats;
    TSet<FName> Keys;
    for (const UTemporaryEffect* Effect : Defaults->RandomEffectPool)
    {
        const UStatModifierEffect* Buff = Cast<UStatModifierEffect>(Effect);
        if (!TestNotNull(TEXT("Each is a stat buff"), Buff))
        {
            continue;
        }
        Stats.Add(Buff->Stat);
        Keys.Add(Buff->StackKey);
        TestEqual(TEXT("Each lasts 8 s"), Buff->Duration, 8.0f);
    }

    TestEqual(TEXT("Four different stats"), Stats.Num(), 4);
    TestEqual(TEXT("Four different stack keys, so different buffs stack"), Keys.Num(), 4);
    TestTrue(TEXT("Move speed is one of them"), Stats.Contains(EGrantMoneyStat::MoveSpeed));
    TestTrue(TEXT("Varied by default"), Defaults->bVaryBuffsAcrossTotems);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRewardTotemSpawnerRandomBuffsTest,
    "GrantMoney.Rewards.TotemSpawner.RandomBuffsPerLevel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRewardTotemSpawnerRandomBuffsTest::RunTest(const FString& Parameters)
{
    using namespace TotemSpawnTest;

    TArray<EGrantMoneyStat> First;
    TArray<EGrantMoneyStat> Repeat;
    {
        FSpawnScene Scene;
        if (!Scene.Create(*this, MapMin, MapMax, MapCentre, 31))
        {
            return false;
        }
        Scene.Spawner->TotemCount = 4;
        Scene.Spawner->MinSpacing = 600.0f;
        if (!TestEqual(TEXT("Four placed"), Scene.Spawner->SpawnTotems(), 4))
        {
            return false;
        }

        First = RewardStats(Scene.Spawner);
        TestEqual(TEXT("Dealt like cards: four totems, four different buffs"), TSet<EGrantMoneyStat>(TArrayView<const EGrantMoneyStat>(First)).Num(), 4);
        TestFalse(TEXT("Every totem got a buff"), First.Contains(EGrantMoneyStat::Experience));

        for (const ARewardTotem* Totem : Scene.Spawner->GetSpawnedTotems())
        {
            TestTrue(TEXT("Totem tinted with its buff's colour"), Totem->ReadyColor.Equals(Totem->Effects[0]->AuraColor));
            TestTrue(TEXT("A copy, not the pool entry"), !Scene.Spawner->RandomEffectPool.Contains(Totem->Effects[0]));
        }
    }
    {
        FSpawnScene Scene;
        if (!Scene.Create(*this, MapMin, MapMax, MapCentre, 31))
        {
            return false;
        }
        Scene.Spawner->TotemCount = 4;
        Scene.Spawner->MinSpacing = 600.0f;
        Scene.Spawner->SpawnTotems();
        Repeat = RewardStats(Scene.Spawner);
    }

    TestTrue(TEXT("Same seed, same buffs"), First == Repeat);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRewardTotemSpawnerOverrideBeatsPoolTest,
    "GrantMoney.Rewards.TotemSpawner.FixedOverrideBeatsRandomPool",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRewardTotemSpawnerOverrideBeatsPoolTest::RunTest(const FString& Parameters)
{
    using namespace TotemSpawnTest;

    {
        FSpawnScene Scene;
        if (!Scene.Create(*this, MapMin, MapMax, MapCentre))
        {
            return false;
        }

        UStatModifierEffect* Fixed = NewObject<UStatModifierEffect>(Scene.Spawner);
        Fixed->Stat = EGrantMoneyStat::Penetration;
        Scene.Spawner->EffectsOverride.Add(Fixed);

        Scene.Spawner->SpawnTotems();
        for (const EGrantMoneyStat Stat : RewardStats(Scene.Spawner))
        {
            TestTrue(TEXT("Every totem gives the fixed reward"), Stat == EGrantMoneyStat::Penetration);
        }
    }

    // With no pool and no override, the totem class's own default is kept.
    FSpawnScene Plain;
    if (!Plain.Create(*this, MapMin, MapMax, MapCentre))
    {
        return false;
    }
    Plain.Spawner->RandomEffectPool.Reset();
    Plain.Spawner->SpawnTotems();
    for (const EGrantMoneyStat Stat : RewardStats(Plain.Spawner))
    {
        TestTrue(TEXT("Totem default: move speed"), Stat == EGrantMoneyStat::MoveSpeed);
    }
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
