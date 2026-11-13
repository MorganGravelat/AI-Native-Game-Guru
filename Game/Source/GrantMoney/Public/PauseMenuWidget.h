#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

class UButton;
class USoundBase;

UCLASS()
class GRANTMONEY_API UPauseMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPauseMenuWidget(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;

    // Turns the background music down while the menu is open.
    virtual void NativeDestruct() override;

    // Played when either button is clicked. Empty for silence.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> ClickSound;

    // Names must match the buttons in WBP_PauseMenu exactly.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ResumeButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> QuitButton;

private:
    UFUNCTION()
    void HandleResume();

    UFUNCTION()
    void HandleQuit();
};