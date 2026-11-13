// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameOverWidget.generated.h"

class UButton;
class USoundBase;
class UTextBlock;

/**
 * Shown once the run ends: in a loss (see GrantMoneyGameMode::HandleRunStateChanged) as the Game Over screen,
 * and in a win as the Victory screen, which is this same class with a different title. Displays the
 * survival time and the player's total XP, and offers Restart (reloads this level from scratch, which
 * resets every run-scoped actor and subsystem) or Quit. Follows the same conventions as
 * UPauseMenuWidget: the controller creates it, sets the input mode and cursor, and adds it to the
 * viewport; this class only owns its own button handlers.
 *
 * There is no score here: once the character has a level, that is the run's progress readout.
 */
UCLASS()
class GRANTMONEY_API UGameOverWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UGameOverWidget(const FObjectInitializer& ObjectInitializer);

    // Fills in the result text. Call before AddToViewport so nothing shows a stale "0:00" frame.
    // Title (for example "Victory!") replaces the text of the optional ResultTitleText; empty leaves it as designed.
    void SetResults(float SurvivalSeconds, float Experience, const FText& Title = FText::GetEmpty());

protected:
    virtual void NativeConstruct() override;

    // Played when Restart or Quit is clicked. Empty for silence.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> ClickSound;

    // Names must match the widgets in WBP_GameOver exactly.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SurvivalTimeText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ExperienceText;

    // Optional heading. Add a TextBlock with this name to show "Game Over" or "Victory!" from one widget.
    // Without it, the widget retitles whichever TextBlock currently reads "Game Over".
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> ResultTitleText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> RestartButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> QuitButton;

private:
    UFUNCTION()
    void HandleRestart();

    UFUNCTION()
    void HandleQuit();
};
