// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RunHUD.generated.h"

class UGameplayHudWidget;

/**
 * Owns the gameplay HUD for one local player: creates a single UGameplayHudWidget when it begins play and
 * removes it when it ends, so there is never a duplicate or an orphan. A restart reloads the level, which
 * recreates this actor and the widget together. The widget finds and follows the player, run controller and
 * boss itself; this class only manages its life.
 *
 * The widget sits on viewport layer 0, below the pause menu, level-up choice and result screens
 * (AMapCameraPlayerController gives those higher layers), and ignores the mouse, so those screens keep
 * their input focus. The game mode makes this the player's HUD class.
 */
UCLASS()
class GRANTMONEY_API ARunHUD : public AHUD
{
	GENERATED_BODY()

public:
	ARunHUD();

	// The widget while it exists, else null.
	UGameplayHudWidget* GetGameplayHud() const { return GameplayHud; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// The class to create. Defaults to UGameplayHudWidget; a Blueprint child can restyle it.
	UPROPERTY(EditAnywhere, Category = "HUD")
	TSubclassOf<UGameplayHudWidget> GameplayHudClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<UGameplayHudWidget> GameplayHud;
};
