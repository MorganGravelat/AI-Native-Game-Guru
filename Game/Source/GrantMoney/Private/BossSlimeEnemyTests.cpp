#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/CapsuleComponent.h"
#include "ContactDamageComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HealthComponent.h"
#include "SlimeEnemy.h"
#include "StatsComponent.h"
#include "Tests/AutomationCommon.h"
#include "Tests/BossTestTypes.h"

namespace BossSlimeTest
{
    // A game world with play started, a frozen stand-in player, a boss that chases it, and a synthetic 20 x 20
    // cell map (world X and Y from -50 to 1950) given to the boss for its lunge bounds. Lunge timing is set to
    // round numbers: 1 s windup, 1000 cm/s for up to 0.5 s, 2 s cooldown, triggered within 700 cm.
    // Nothing ticks unless a test steps the boss with AdvanceLunge.
    struct FScene
    {
        FTestWorldWrapper WorldWrapper;
        BossTest::FTestArena Map;
        ACharacter* Player = nullptr;
        ATestBossSlime* Boss = nullptr;

        bool Create(FAutomationTestBase& Test, const FVector& BossLocation, const FVector& PlayerLocation, bool bGiveMap = true)
        {
            if (!WorldWrapper.CreateTestWorld(EWorldType::Game) || !WorldWrapper.BeginPlayInTestWorld())
            {
                WorldWrapper.ForwardErrorMessages(&Test);
                return false;
            }

            Map.Init(FIntPoint(0, 0), FIntPoint(19, 19));

            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            Player = World()->SpawnActor<ACharacter>(ACharacter::StaticClass(), FTransform(PlayerLocation), Params);
            Boss = World()->SpawnActor<ATestBossSlime>(ATestBossSlime::StaticClass(), FTransform(BossLocation), Params);
            if (!Test.TestNotNull(TEXT("Player spawned"), Player) || !Test.TestNotNull(TEXT("Boss spawned"), Boss))
            {
                return false;
            }

            Player->GetCharacterMovement()->DisableMovement();
            Boss->TestTarget = Player;
            Boss->DeathCleanupDelay = 60.0f;
            Boss->LungeWindupSeconds = 1.0f;
            Boss->LungeSpeed = 1000.0f;
            Boss->LungeDurationSeconds = 0.5f;
            Boss->LungeCooldownSeconds = 2.0f;
            Boss->LungeTriggerRange = 700.0f;
            Boss->LungeComboCount = 1;
            Boss->LungeComboWindupSeconds = 0.5f;
            if (bGiveMap)
            {
                Boss->SetArenaOverride(Map.Arena);
            }

            return true;
        }

        UWorld* World() const { return WorldWrapper.GetTestWorld(); }

        void Step(int32 Count, float DeltaSeconds)
        {
            for (int32 Index = 0; Index < Count; ++Index)
            {
                Boss->AdvanceLunge(DeltaSeconds);
            }
        }
    };

    const FVector OpenBoss(500.0, 1000.0, 100.0);
    const FVector OpenPlayer(1000.0, 1000.0, 100.0);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBossSlimeDefaultsTest,
    "GrantMoney.Boss.Slime.DefaultsAreIndependentAndBigger",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossSlimeDefaultsTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("The boss is its own class, derived from the slime"),
        ABossSlimeEnemy::StaticClass() != ASlimeEnemy::StaticClass() && ABossSlimeEnemy::StaticClass()->IsChildOf(ASlimeEnemy::StaticClass()));
    TestTrue(TEXT("A larger body than a slime"),
        GetDefault<ABossSlimeEnemy>()->GetCapsuleComponent()->GetUnscaledCapsuleRadius() > GetDefault<ASlimeEnemy>()->GetCapsuleComponent()->GetUnscaledCapsuleRadius());
    TestTrue(TEXT("A different colour from its telegraph colour"),
        !GetDefault<ABossSlimeEnemy>()->BodyColor.Equals(GetDefault<ABossSlimeEnemy>()->TelegraphColor, 0.1f));

    BossSlimeTest::FScene Scene;
    if (!Scene.Create(*this, BossSlimeTest::OpenBoss, BossSlimeTest::OpenPlayer))
    {
        return false;
    }

    // Shared systems: the boss reads its numbers from the same stats, health and contact damage as every enemy.
    TestEqual(TEXT("Boss health is the MaxHealth stat"), Scene.Boss->GetHealth()->GetMaxHealth(), Scene.Boss->GetStats()->GetStat(EGrantMoneyStat::MaxHealth));
    TestTrue(TEXT("The boss has much more health than a slime"), Scene.Boss->GetHealth()->GetMaxHealth() >= 10.0f * 30.0f);
    TestEqual(TEXT("Contact damage comes from the stat"), Scene.Boss->GetContactDamage()->GetDamage(), Scene.Boss->GetStats()->GetStat(EGrantMoneyStat::ContactDamage));
    TestTrue(TEXT("A controlled hit cadence: slower than a slime's one second"), Scene.Boss->GetContactDamage()->GetInterval() > 1.0f);

    const FBossLungeSettings Lunge = GetDefault<ABossSlimeEnemy>()->GetLungeSettings();
    TestTrue(TEXT("Default windup, speed, duration and cooldown are positive"),
        Lunge.WindupSeconds > 0.0f && Lunge.Speed > 0.0f && Lunge.DurationSeconds > 0.0f && Lunge.CooldownSeconds > 0.0f);
    TestTrue(TEXT("The telegraph is longer than a blink"), Lunge.WindupSeconds >= 0.5f);
    TestTrue(TEXT("The boss dashes in a combo by default"), Lunge.ComboCount >= 3);
    TestTrue(TEXT("Later dashes of a combo have a telegraph too"), Lunge.ComboWindupSeconds > 0.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBossSlimeComboRetargetsTest,
    "GrantMoney.Boss.Slime.ComboRetargetsAfterEachDash",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossSlimeComboRetargetsTest::RunTest(const FString& Parameters)
{
    BossSlimeTest::FScene Scene;
    if (!Scene.Create(*this, BossSlimeTest::OpenBoss, BossSlimeTest::OpenPlayer))
    {
        return false;
    }

    ATestBossSlime* Boss = Scene.Boss;
    Boss->LungeComboCount = 3;
    Boss->LungeComboWindupSeconds = 0.5f;

    // Dash 1 east, at the player's first position.
    Scene.Step(5, 0.25f);
    TestTrue(TEXT("Dash 1 is under way"), Boss->GetLungeState().IsLunging());
    TestTrue(TEXT("Dash 1 heads east"), Boss->GetLungeDirection().Equals(FVector::ForwardVector, 0.01f));

    // The player moves north; after dash 1 the boss stops, re-aims and telegraphs again.
    Scene.Player->SetActorLocation(FVector(500.0, 1500.0, 100.0));
    Scene.Step(2, 0.25f);
    TestTrue(TEXT("Dash 1 ended into another windup, not a cooldown"), Boss->GetLungeState().IsWindingUp());
    TestEqual(TEXT("It is the second dash"), Boss->GetLungeState().ComboIndex, 1);
    TestTrue(TEXT("It re-aimed at the player's new position (north)"), Boss->GetLungeDirection().Equals(FVector(0.0, 1.0, 0.0), 0.01f));
    TestTrue(TEXT("The body is planted during the telegraph"), Boss->GetCharacterMovement()->Velocity.IsNearlyZero());
    TestTrue(TEXT("Not walking during the telegraph"), Boss->ExposedMoveDirection().IsNearlyZero());

    Scene.Step(2, 0.25f);
    TestTrue(TEXT("Dash 2 is under way"), Boss->GetLungeState().IsLunging());

    // The player moves west of the boss for the third dash.
    Scene.Player->SetActorLocation(FVector(0.0, 1000.0, 100.0));
    Scene.Step(2, 0.25f);
    TestTrue(TEXT("Dash 2 ended into the third windup"), Boss->GetLungeState().IsWindingUp());
    TestEqual(TEXT("It is the third dash"), Boss->GetLungeState().ComboIndex, 2);
    TestTrue(TEXT("It re-aimed west"), Boss->GetLungeDirection().Equals(FVector(-1.0, 0.0, 0.0), 0.01f));

    Scene.Step(2, 0.25f);
    TestTrue(TEXT("Dash 3 is under way"), Boss->GetLungeState().IsLunging());
    Scene.Step(2, 0.25f);
    TestTrue(TEXT("After the third dash the combo is over: cooldown"), Boss->GetLungeState().Phase == EBossLungePhase::Cooldown);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBossSlimeComboEndsWithoutRoomTest,
    "GrantMoney.Boss.Slime.ComboEndsEarlyWithoutAimOrRoom",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossSlimeComboEndsWithoutRoomTest::RunTest(const FString& Parameters)
{
    BossSlimeTest::FScene Scene;
    if (!Scene.Create(*this, BossSlimeTest::OpenBoss, BossSlimeTest::OpenPlayer))
    {
        return false;
    }

    ATestBossSlime* Boss = Scene.Boss;
    Boss->LungeComboCount = 3;
    Boss->LungeComboWindupSeconds = 0.5f;

    Scene.Step(5, 0.25f);
    TestTrue(TEXT("Dash 1 is under way"), Boss->GetLungeState().IsLunging());

    // The player is exactly on the boss: there is no direction to dash in, so the combo ends.
    Scene.Player->SetActorLocation(Boss->GetActorLocation());
    Scene.Step(2, 0.25f);
    TestTrue(TEXT("No aim: the combo ends in a cooldown"), Boss->GetLungeState().Phase == EBossLungePhase::Cooldown);
    TestEqual(TEXT("The combo counter was reset"), Boss->GetLungeState().ComboIndex, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBossSlimeHealthOverrideTest,
    "GrantMoney.Boss.Slime.DebugMaxHealthOverride",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossSlimeHealthOverrideTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Off by default"), ABossSlimeEnemy::GetDebugMaxHealthOverride(), 0.0f);

    ABossSlimeEnemy::SetDebugMaxHealthOverride(5000.0f);
    {
        BossSlimeTest::FScene Scene;
        if (Scene.Create(*this, BossSlimeTest::OpenBoss, BossSlimeTest::OpenPlayer))
        {
            TestEqual(TEXT("A boss spawned with the override has that max health"), Scene.Boss->GetHealth()->GetMaxHealth(), 5000.0f);
            TestEqual(TEXT("And starts at full health"), Scene.Boss->GetHealth()->GetHealth(), 5000.0f);
        }
    }

    ABossSlimeEnemy::SetDebugMaxHealthOverride(-3.0f);
    TestEqual(TEXT("A non-positive value turns it off"), ABossSlimeEnemy::GetDebugMaxHealthOverride(), 0.0f);
    {
        BossSlimeTest::FScene Scene;
        if (Scene.Create(*this, BossSlimeTest::OpenBoss, BossSlimeTest::OpenPlayer))
        {
            TestTrue(TEXT("Back to the class default"), Scene.Boss->GetHealth()->GetMaxHealth() < 5000.0f);
        }
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBossSlimeLungeSequenceTest,
    "GrantMoney.Boss.Slime.LungeTelegraphChargeCooldown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossSlimeLungeSequenceTest::RunTest(const FString& Parameters)
{
    BossSlimeTest::FScene Scene;
    if (!Scene.Create(*this, BossSlimeTest::OpenBoss, BossSlimeTest::OpenPlayer))
    {
        return false;
    }

    ATestBossSlime* Boss = Scene.Boss;

    Scene.Step(1, 0.25f);
    TestTrue(TEXT("The target is in range: the windup starts"), Boss->GetLungeState().IsWindingUp());
    TestTrue(TEXT("The lunge aims at the target"), Boss->GetLungeDirection().Equals(FVector::ForwardVector, 0.01f));
    TestTrue(TEXT("Standing still while telegraphing"), Boss->ExposedMoveDirection().IsNearlyZero());
    TestEqual(TEXT("No walking speed while telegraphing"), Boss->ExposedDesiredSpeed(), 0.0f);

    // Moving the target mid-windup does not re-aim: the player can dodge a telegraphed lunge.
    Scene.Player->SetActorLocation(FVector(500.0, 1500.0, 100.0));
    Scene.Step(4, 0.25f);
    TestTrue(TEXT("The windup ends in a lunge"), Boss->GetLungeState().IsLunging());
    TestTrue(TEXT("The direction stayed fixed"), Boss->GetLungeDirection().Equals(FVector::ForwardVector, 0.01f));
    TestTrue(TEXT("Charging along the locked direction"), Boss->ExposedMoveDirection().Equals(FVector::ForwardVector, 0.01f));
    TestEqual(TEXT("Charging at the lunge speed"), Boss->ExposedDesiredSpeed(), 1000.0f);
    TestEqual(TEXT("The body is launched at the lunge speed"), static_cast<float>(Boss->GetCharacterMovement()->Velocity.X), 1000.0f, 0.5f);

    Scene.Step(1, 0.25f);
    TestTrue(TEXT("Still lunging halfway"), Boss->GetLungeState().IsLunging());
    Scene.Step(1, 0.25f);
    TestTrue(TEXT("The lunge ends after its duration"), Boss->GetLungeState().Phase == EBossLungePhase::Cooldown);

    // Back in range, but the cooldown blocks a new windup.
    Scene.Player->SetActorLocation(BossSlimeTest::OpenPlayer);
    Scene.Step(7, 0.25f);
    TestTrue(TEXT("No new windup during the cooldown"), Boss->GetLungeState().Phase == EBossLungePhase::Cooldown);
    Scene.Step(1, 0.25f);
    TestTrue(TEXT("Cooldown over: ready again"), Boss->GetLungeState().Phase == EBossLungePhase::Idle);
    Scene.Step(1, 0.25f);
    TestTrue(TEXT("The next lunge is telegraphed again"), Boss->GetLungeState().IsWindingUp());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBossSlimeLungeStaysInBoundsTest,
    "GrantMoney.Boss.Slime.LungeStaysInsidePlayableArea",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossSlimeLungeStaysInBoundsTest::RunTest(const FString& Parameters)
{
    // Boss near the east edge (playable up to X = 1950) with the player beyond it: the full lunge
    // (1000 cm/s for 1 s = 1000 cm) would leave the map.
    BossSlimeTest::FScene Scene;
    if (!Scene.Create(*this, FVector(1400.0, 1000.0, 100.0), FVector(1800.0, 1000.0, 100.0)))
    {
        return false;
    }

    ATestBossSlime* Boss = Scene.Boss;
    Boss->LungeDurationSeconds = 1.0f;

    Scene.Step(5, 0.25f);
    TestTrue(TEXT("There is room for a lunge, so it starts"), Boss->GetLungeState().IsLunging());

    const float Duration = Boss->GetLungeState().ActiveLungeDuration;
    TestTrue(TEXT("The lunge is shortened"), Duration > 0.0f && Duration < 1.0f);

    const float Radius = Boss->GetCapsuleComponent()->GetScaledCapsuleRadius() + Boss->LungeBoundsPadding;
    const FVector End = Boss->GetActorLocation() + Boss->GetLungeDirection() * Boss->LungeSpeed * Duration;
    TestTrue(TEXT("Its end point is inside the playable area"), Scene.Map.Arena.IsFootprintPlayable(End, Radius));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBossSlimeNoLungeWithoutRoomTest,
    "GrantMoney.Boss.Slime.NoLungeWithoutRoomOrMapOrRange",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossSlimeNoLungeWithoutRoomTest::RunTest(const FString& Parameters)
{
    {
        // Against the east edge with the player past it: no useful lunge, so no empty windup either.
        BossSlimeTest::FScene Scene;
        if (!Scene.Create(*this, FVector(1800.0, 1000.0, 100.0), FVector(1900.0, 1000.0, 100.0)))
        {
            return false;
        }

        Scene.Step(12, 0.25f);
        TestTrue(TEXT("No room: the boss never winds up"), Scene.Boss->GetLungeState().Phase == EBossLungePhase::Idle);
    }

    {
        BossSlimeTest::FScene Scene;
        if (!Scene.Create(*this, BossSlimeTest::OpenBoss, FVector(1800.0, 1000.0, 100.0)))
        {
            return false;
        }

        Scene.Step(12, 0.25f);
        TestTrue(TEXT("Target out of trigger range: no windup"), Scene.Boss->GetLungeState().Phase == EBossLungePhase::Idle);
    }

    {
        // The test world has no tile map, so with no override the boss has no bounds and must not lunge.
        BossSlimeTest::FScene Scene;
        if (!Scene.Create(*this, BossSlimeTest::OpenBoss, BossSlimeTest::OpenPlayer, /*bGiveMap=*/false))
        {
            return false;
        }

        Scene.Step(12, 0.25f);
        TestTrue(TEXT("No map, no lunge"), Scene.Boss->GetLungeState().Phase == EBossLungePhase::Idle);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBossSlimeDeathCancelsLungeTest,
    "GrantMoney.Boss.Slime.DeathCancelsLunge",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossSlimeDeathCancelsLungeTest::RunTest(const FString& Parameters)
{
    BossSlimeTest::FScene Scene;
    if (!Scene.Create(*this, BossSlimeTest::OpenBoss, BossSlimeTest::OpenPlayer))
    {
        return false;
    }

    Scene.Step(2, 0.25f);
    TestTrue(TEXT("Winding up"), Scene.Boss->GetLungeState().IsWindingUp());

    Scene.Boss->GetHealth()->ApplyDamage(100000.0f, Scene.Player);
    TestTrue(TEXT("The boss is dead"), Scene.Boss->GetHealth()->IsDead());
    TestTrue(TEXT("Death cancels the lunge"), Scene.Boss->GetLungeState().Phase == EBossLungePhase::Idle);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBossSlimeLeadsMovingTargetTest,
    "GrantMoney.Boss.Slime.LungeLeadStrengthBlendsTowardFullLead",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossSlimeLeadsMovingTargetTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Leads by 60% by default"), GetDefault<ABossSlimeEnemy>()->LungeLeadStrength, 0.6f);

    // The player is due east of the boss and moving north: a leading dash should blend toward the full
    // intercept direction (computed independently here) by exactly LungeLeadStrength.
    {
        BossSlimeTest::FScene Scene;
        if (!Scene.Create(*this, BossSlimeTest::OpenBoss, BossSlimeTest::OpenPlayer))
        {
            return false;
        }

        const FVector Offset = BossSlimeTest::OpenPlayer - BossSlimeTest::OpenBoss; // (500, 0, 0)
        const FVector TargetVelocity(0.0, 500.0, 0.0);
        Scene.Boss->TestTargetVelocity = TargetVelocity;

        Scene.Step(1, 0.25f);
        TestTrue(TEXT("Winding up"), Scene.Boss->GetLungeState().IsWindingUp());
        TestTrue(TEXT("The dash leads ahead of the target (+Y)"), Scene.Boss->GetLungeDirection().Y > 0.0f);

        FVector FullLead;
        FBossLungeMath::ComputeInterceptDirection(Offset, TargetVelocity, Scene.Boss->LungeSpeed, Scene.Boss->LungeWindupSeconds, FullLead);
        const FVector Expected = FMath::Lerp(Offset.GetSafeNormal(), FullLead, Scene.Boss->LungeLeadStrength).GetSafeNormal();
        TestTrue(TEXT("Blended exactly by LungeLeadStrength toward the full lead"), Scene.Boss->GetLungeDirection().Equals(Expected, 0.01f));
        TestFalse(TEXT("Less than the full (unblended) lead"), Scene.Boss->GetLungeDirection().Equals(FullLead, 0.01f));
    }

    // LungeLeadStrength 0: identical to the old "off" behaviour, aiming straight at the target.
    {
        BossSlimeTest::FScene Scene;
        if (!Scene.Create(*this, BossSlimeTest::OpenBoss, BossSlimeTest::OpenPlayer))
        {
            return false;
        }

        Scene.Boss->LungeLeadStrength = 0.0f;
        Scene.Boss->TestTargetVelocity = FVector(0.0, 500.0, 0.0);
        Scene.Step(1, 0.25f);
        TestTrue(TEXT("Winding up"), Scene.Boss->GetLungeState().IsWindingUp());
        TestTrue(TEXT("Zero strength: aims straight at the target"), Scene.Boss->GetLungeDirection().Equals(FVector::ForwardVector, 0.01f));
    }

    // LungeLeadStrength 1: the full, unblended intercept direction.
    {
        BossSlimeTest::FScene Scene;
        if (!Scene.Create(*this, BossSlimeTest::OpenBoss, BossSlimeTest::OpenPlayer))
        {
            return false;
        }

        const FVector Offset = BossSlimeTest::OpenPlayer - BossSlimeTest::OpenBoss;
        const FVector TargetVelocity(0.0, 500.0, 0.0);
        Scene.Boss->LungeLeadStrength = 1.0f;
        Scene.Boss->TestTargetVelocity = TargetVelocity;
        Scene.Step(1, 0.25f);

        FVector FullLead;
        FBossLungeMath::ComputeInterceptDirection(Offset, TargetVelocity, Scene.Boss->LungeSpeed, Scene.Boss->LungeWindupSeconds, FullLead);
        TestTrue(TEXT("Full strength: the whole computed lead"), Scene.Boss->GetLungeDirection().Equals(FullLead, 0.01f));
    }

    // A stationary target behaves exactly as before leading existed, whatever the strength.
    {
        BossSlimeTest::FScene Scene;
        if (!Scene.Create(*this, BossSlimeTest::OpenBoss, BossSlimeTest::OpenPlayer))
        {
            return false;
        }

        Scene.Step(1, 0.25f);
        TestTrue(TEXT("A stationary target gets the plain straight-line aim"), Scene.Boss->GetLungeDirection().Equals(FVector::ForwardVector, 0.01f));
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
