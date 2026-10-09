#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

class UButton;

UCLASS()
class GRANTMONEY_API UPauseMenuWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

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