#include "PauseMenuWidget.h"

#include "Components/Button.h"
#include "GameFramework/PlayerController.h"
#include "GrantMoneyAudio.h"
#include "GrantMoneyAudioSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MapCameraPlayerController.h"

UPauseMenuWidget::UPauseMenuWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    ClickSound = GrantMoneyAudio::DefaultSound(TEXT("/Game/Sound/UI/ButtonPress.ButtonPress"));
}

void UPauseMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (UGrantMoneyAudioSubsystem* Audio = UGrantMoneyAudioSubsystem::Get(this))
    {
        Audio->AcquireMusicDuck(this);
    }

    // AddUnique so a re-constructed widget never double-binds.
    if (ResumeButton)
    {
        ResumeButton->OnClicked.AddUniqueDynamic(
            this,
            &UPauseMenuWidget::HandleResume
        );
    }

    if (QuitButton)
    {
        QuitButton->OnClicked.AddUniqueDynamic(
            this,
            &UPauseMenuWidget::HandleQuit
        );
    }
}

void UPauseMenuWidget::NativeDestruct()
{
    if (UGrantMoneyAudioSubsystem* Audio = UGrantMoneyAudioSubsystem::Get(this))
    {
        Audio->ReleaseMusicDuck(this);
    }

    Super::NativeDestruct();
}

void UPauseMenuWidget::HandleResume()
{
    GrantMoneyAudio::Play2D(this, ClickSound);

    APlayerController* PC = GetOwningPlayer();

    if (!PC)
    {
        return;
    }

    // The controller owns the pause and input state and refuses to resume a finished run.
    if (AMapCameraPlayerController* MapPC = Cast<AMapCameraPlayerController>(PC))
    {
        MapPC->ResumeFromPauseMenu();
        return;
    }

    UGameplayStatics::SetGamePaused(this, false);

    PC->bShowMouseCursor = false;

    FInputModeGameOnly InputMode;
    PC->SetInputMode(InputMode);

    RemoveFromParent();
}

void UPauseMenuWidget::HandleQuit()
{
    GrantMoneyAudio::Play2D(this, ClickSound);

    APlayerController* PC = GetOwningPlayer();

    if (!PC)
    {
        return;
    }

    UKismetSystemLibrary::QuitGame(
        this,
        PC,
        EQuitPreference::Quit,
        false
    );
}