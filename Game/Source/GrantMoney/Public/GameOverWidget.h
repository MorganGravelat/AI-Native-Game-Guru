// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameOverWidget.generated.h"

class UButton;
class UTextBlock;

/**
 * Shown once the run ends in a loss (see GrantMoneyGameMode::HandleRunStateChanged). Displays the
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
    // Fills in the result text. Call before AddToViewport so nothing shows a stale "0:00" frame.
    void SetResults(float SurvivalSeconds, float Experience);

protected:
    virtual void NativeConstruct() override;

    // Names must match the widgets in WBP_GameOver exactly.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SurvivalTimeText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ExperienceText;

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
