// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LevelUpWidget.generated.h"

class UButton;
class USoundBase;
class UTextBlock;
class UUpgradeComponent;

/**
 * Shown while UUpgradeComponent has an active choice (see AGrantMoneyPlayerCharacter and
 * AMapCameraPlayerController::ShowLevelUpChoice). Up to three placeholder buttons, one per offered
 * upgrade; fewer are shown (the rest collapsed) if fewer are eligible. Clicking one calls
 * UUpgradeComponent::ChooseUpgrade directly, the same way UGameOverWidget calls UGameplayStatics
 * directly: this widget only owns its own buttons, not the pause/show/hide decisions around it.
 *
 * RefreshOptions must be called (by the controller) every time UUpgradeComponent::OnChoiceReady fires,
 * including the second, third, ... choice from one queue, since each may offer different options.
 */
UCLASS()
class GRANTMONEY_API ULevelUpWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    ULevelUpWidget(const FObjectInitializer& ObjectInitializer);

    // Remembers which component to read options from and report choices back to. Call once, before the
    // first RefreshOptions.
    void SetUpgradeComponent(UUpgradeComponent* InUpgrades);

    // Re-reads InUpgrades->GetCurrentOptions() and updates the three buttons' text and visibility.
    void RefreshOptions();

protected:
    virtual void NativeConstruct() override;

    // Turns the background music down while the choice is on screen.
    virtual void NativeDestruct() override;

    // Played when an upgrade is chosen. Empty for silence.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> ClickSound;

    // Names must match the widgets in WBP_LevelUp exactly.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Option1Text;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> Option1Button;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Option2Text;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> Option2Button;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Option3Text;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> Option3Button;

private:
    UFUNCTION()
    void HandleOption1Clicked();

    UFUNCTION()
    void HandleOption2Clicked();

    UFUNCTION()
    void HandleOption3Clicked();

    void ChooseIndex(int32 Index);

    UPROPERTY(Transient)
    TWeakObjectPtr<UUpgradeComponent> Upgrades;
};
