#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "BossEncounter.h"
#include "BossSlimeEnemy.h"
#include "GameOverWidget.h"
#include "GrantMoneyAudio.h"
#include "GrantMoneyAudioSubsystem.h"
#include "GrantMoneyGameMode.h"
#include "GrantMoneyPlayerCharacter.h"
#include "LevelUpWidget.h"
#include "PauseMenuWidget.h"
#include "RewardTotem.h"
#include "SlimeEnemy.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "Sound/SoundBase.h"
#include "StarterWeaponComponent.h"
#include "Tests/AutomationCommon.h"
#include "UObject/UnrealType.h"
#include "XpOrbPickup.h"

namespace AudioSlotTest
{
    struct FSlot
    {
        UClass* Class;
        const TCHAR* Property;
        const TCHAR* ExpectedAssetName;
    };

    // Every sound slot that has a default, and the file in Content/Sound it should point at.
    TArray<FSlot> Slots()
    {
        return {
            { ASlimeEnemy::StaticClass(), TEXT("HitSound"), TEXT("SlimeHit") },
            { ASlimeEnemy::StaticClass(), TEXT("DeathSound"), TEXT("SlimeDeath") },
            { ABossSlimeEnemy::StaticClass(), TEXT("HitSound"), TEXT("SlimeHit") },
            { ABossSlimeEnemy::StaticClass(), TEXT("DeathSound"), TEXT("SlimeBossDying") },
            { ABossSlimeEnemy::StaticClass(), TEXT("LungeWindupSound"), TEXT("SlimeBossDashWindup") },
            { ABossSlimeEnemy::StaticClass(), TEXT("LungeSound"), TEXT("SlimeBossDashSound") },
            { ABossEncounter::StaticClass(), TEXT("ArrivalSound"), TEXT("SlimeBossIntroSound") },
            { ABossEncounter::StaticClass(), TEXT("BossMusic"), TEXT("SlimeBossTheme") },
            { AXpOrbPickup::StaticClass(), TEXT("CollectSound"), TEXT("XP_Pickup") },
            { AGrantMoneyPlayerCharacter::StaticClass(), TEXT("HurtSound"), TEXT("PlayerHit") },
            { AGrantMoneyPlayerCharacter::StaticClass(), TEXT("LevelUpSound"), TEXT("LevelUp") },
            { UStarterWeaponComponent::StaticClass(), TEXT("FireSound"), TEXT("FireBolt") },
            { AGrantMoneyGameMode::StaticClass(), TEXT("MainMusic"), TEXT("Main_Theme") },
            { AGrantMoneyGameMode::StaticClass(), TEXT("VictorySound"), TEXT("Victory") },
            { AGrantMoneyGameMode::StaticClass(), TEXT("GameOverSound"), TEXT("GameOver") },
            { UPauseMenuWidget::StaticClass(), TEXT("ClickSound"), TEXT("ButtonPress") },
            { ULevelUpWidget::StaticClass(), TEXT("ClickSound"), TEXT("ButtonPress") },
            { UGameOverWidget::StaticClass(), TEXT("ClickSound"), TEXT("ButtonPress") },
        };
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAudioSlotDefaultsTest,
    "GrantMoney.Audio.DefaultSlotsPointAtRealAssets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAudioSlotDefaultsTest::RunTest(const FString& Parameters)
{
    for (const AudioSlotTest::FSlot& Slot : AudioSlotTest::Slots())
    {
        const FString Label = FString::Printf(TEXT("%s.%s"), *Slot.Class->GetName(), Slot.Property);

        const FSoftObjectProperty* Property = CastField<FSoftObjectProperty>(Slot.Class->FindPropertyByName(Slot.Property));
        if (!TestNotNull(*(Label + TEXT(" is a sound slot")), Property))
        {
            continue;
        }

        const FSoftObjectPath Path = Property->GetPropertyValue_InContainer(Slot.Class->GetDefaultObject()).ToSoftObjectPath();
        TestEqual(*(Label + TEXT(" default asset")), Path.GetAssetName(), FString(Slot.ExpectedAssetName));
        TestTrue(*(Label + TEXT(" asset exists in Content")), Path.TryLoad() != nullptr);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAudioEmptySlotTest,
    "GrantMoney.Audio.EmptySlotsAreSilentAndSafe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAudioEmptySlotTest::RunTest(const FString& Parameters)
{
    const TSoftObjectPtr<USoundBase> Empty;
    TestNull(TEXT("An empty slot resolves to nothing"), GrantMoneyAudio::Resolve(Empty));

    // Nothing to play must not crash or need a world.
    GrantMoneyAudio::Play2D(nullptr, Empty);
    GrantMoneyAudio::PlayAtLocation(nullptr, Empty, FVector::ZeroVector);
    TestNull(TEXT("No music is spawned for an empty slot"), GrantMoneyAudio::SpawnMusic(nullptr, Empty, 1.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTotemChargeSoundTimeTest,
    "GrantMoney.Audio.TotemChargeSoundFollowsTheBar",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTotemChargeSoundTimeTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Empty bar: the start of the sound"), ARewardTotem::ComputeChargeSoundTime(0.0f, 3.0f), 0.0f);
    TestEqual(TEXT("A third of the bar: one second in"), ARewardTotem::ComputeChargeSoundTime(1.0f / 3.0f, 3.0f), 1.0f, 1.e-4f);
    TestEqual(TEXT("Full bar: the end of the sound"), ARewardTotem::ComputeChargeSoundTime(1.0f, 3.0f), 3.0f);
    TestEqual(TEXT("Out-of-range progress is clamped"), ARewardTotem::ComputeChargeSoundTime(2.0f, 3.0f), 3.0f);
    TestEqual(TEXT("A longer charge maps onto a longer sound"), ARewardTotem::ComputeChargeSoundTime(0.5f, 6.0f), 3.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMusicDuckTest,
    "GrantMoney.Audio.MusicDuckHeldBySourcesUntilAllRelease",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMusicDuckTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game) || !WorldWrapper.BeginPlayInTestWorld())
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    UGrantMoneyAudioSubsystem* Audio = UGrantMoneyAudioSubsystem::Get(WorldWrapper.GetTestWorld());
    if (!TestNotNull(TEXT("The audio subsystem exists"), Audio))
    {
        return false;
    }

    Audio->MusicDuckVolume = 0.5f;
    TestFalse(TEXT("Not ducked to begin with"), Audio->IsMusicDucked());
    TestEqual(TEXT("Full volume to begin with"), Audio->GetMusicVolume(), 1.0f);

    UDamageType* MenuSource = NewObject<UDamageType>(GetTransientPackage());
    UDamageType* TotemSource = NewObject<UDamageType>(GetTransientPackage());

    Audio->AcquireMusicDuck(MenuSource);
    TestTrue(TEXT("Ducked while a menu holds it"), Audio->IsMusicDucked());
    TestEqual(TEXT("At the duck volume"), Audio->GetMusicVolume(), 0.5f);

    Audio->AcquireMusicDuck(TotemSource);
    Audio->AcquireMusicDuck(TotemSource);
    TestEqual(TEXT("Two sources do not stack"), Audio->GetMusicVolume(), 0.5f);

    Audio->ReleaseMusicDuck(MenuSource);
    TestTrue(TEXT("Still ducked while the totem holds it"), Audio->IsMusicDucked());

    Audio->ReleaseMusicDuck(TotemSource);
    TestFalse(TEXT("Back to normal once every source let go"), Audio->IsMusicDucked());
    TestEqual(TEXT("Full volume again"), Audio->GetMusicVolume(), 1.0f);

    Audio->ReleaseMusicDuck(TotemSource);
    TestFalse(TEXT("Releasing twice is harmless"), Audio->IsMusicDucked());

    // A source that is destroyed without releasing must not hold the music down for good.
    UDamageType* Forgetful = NewObject<UDamageType>(GetTransientPackage());
    Audio->AcquireMusicDuck(Forgetful);
    TestTrue(TEXT("Ducked by the forgetful source"), Audio->IsMusicDucked());
    Forgetful->MarkAsGarbage();
    CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
    TestFalse(TEXT("A destroyed source stops ducking"), Audio->IsMusicDucked());
    return true;
}

#endif
