#include "PauseMenuWidget.h"

#include "Components/Button.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UPauseMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

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

void UPauseMenuWidget::HandleResume()
{
    APlayerController* PC = GetOwningPlayer();

    if (!PC)
    {
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