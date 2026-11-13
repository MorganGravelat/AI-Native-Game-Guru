// Fill out your copyright notice in the Description page of Project Settings.

// NO-TEST: pure UI glue (widget construction, text formatting, button bindings); no branching logic of
// its own to exercise headlessly. Covered by PIE verification, like PauseMenuWidget.cpp.

#include "GameOverWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "RunController.h"

void UGameOverWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // AddUnique so a re-constructed widget never double-binds.
    if (RestartButton)
    {
        RestartButton->OnClicked.AddUniqueDynamic(this, &UGameOverWidget::HandleRestart);
    }

    if (QuitButton)
    {
        QuitButton->OnClicked.AddUniqueDynamic(this, &UGameOverWidget::HandleQuit);
    }
}

void UGameOverWidget::SetResults(float SurvivalSeconds, float Experience, const FText& Title)
{
    if (!Title.IsEmpty())
    {
        // Prefer the explicitly named heading; otherwise retitle the existing "Game Over" heading, so a
        // win does not depend on the widget having been edited.
        UTextBlock* Heading = ResultTitleText;
        if (!Heading && WidgetTree)
        {
            WidgetTree->ForEachWidget([&Heading](UWidget* Widget)
            {
                UTextBlock* Text = Cast<UTextBlock>(Widget);
                if (!Heading && Text && Text->GetText().ToString().Contains(TEXT("game over"), ESearchCase::IgnoreCase))
                {
                    Heading = Text;
                }
            });

            if (!Heading)
            {
                UE_LOG(LogTemp, Warning, TEXT("GameOverWidget: %s has no TextBlock named 'ResultTitleText' and none reading 'Game Over', so the heading '%s' cannot be shown."), *GetName(), *Title.ToString());
            }
        }

        if (Heading)
        {
            Heading->SetText(Title);
        }
    }

    if (SurvivalTimeText)
    {
        SurvivalTimeText->SetText(FText::FromString(FString::Printf(TEXT("Survived %s"), *ARunController::FormatClock(SurvivalSeconds))));
    }

    if (ExperienceText)
    {
        ExperienceText->SetText(FText::FromString(FString::Printf(TEXT("XP: %d"), FMath::FloorToInt32(Experience))));
    }
}

void UGameOverWidget::HandleRestart()
{
    // Reloading the level is the simplest reliable restart: a fresh UWorld recreates every actor (player,
    // enemies, projectiles, pickups) and every UWorldSubsystem (for example UCombatEventsSubsystem) from
    // scratch, so nothing from the previous run can leak into the next one.
    UGameplayStatics::SetGamePaused(this, false);
    UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

void UGameOverWidget::HandleQuit()
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        return;
    }

    UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
}
