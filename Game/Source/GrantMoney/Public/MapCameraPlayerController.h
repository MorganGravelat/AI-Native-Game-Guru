// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MapCameraPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UPauseMenuWidget;
class UGameOverWidget;
class AMapCameraPawn;
struct FInputActionValue;

/**
 * Registers the map camera input mapping context for the local player and
 * owns the pause menu toggle. It also forwards camera turn, look and zoom
 * input to the view-target camera pawn while a different pawn is possessed.
 */
UCLASS()
class GRANTMONEY_API AMapCameraPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMapCameraPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	// Defaults to /Game/Input/IMC_MapCamera.
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> MapCameraMappingContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	int32 MappingContextPriority = 0;

	// Defaults to /Game/Input/IA_Pause. Must have Trigger When Paused enabled.
	UPROPERTY(EditAnywhere, Category = "Pause")
	TObjectPtr<UInputAction> PauseAction;

	// Defaults to /Game/UI/WBP_PauseMenu.
	UPROPERTY(EditAnywhere, Category = "Pause")
	TSubclassOf<UPauseMenuWidget> PauseMenuWidgetClass;

	// Defaults to /Game/UI/WBP_GameOver.
	UPROPERTY(EditAnywhere, Category = "GameOver")
	TSubclassOf<UGameOverWidget> GameOverWidgetClass;

	// Camera actions forwarded to the view-target camera pawn.
	// Default to /Game/Input/IA_CameraTurn, IA_CameraLookUp and IA_CameraZoom.
	UPROPERTY(EditAnywhere, Category = "Camera|Input")
	TObjectPtr<UInputAction> CameraTurnAction;

	UPROPERTY(EditAnywhere, Category = "Camera|Input")
	TObjectPtr<UInputAction> CameraLookUpAction;

	UPROPERTY(EditAnywhere, Category = "Camera|Input")
	TObjectPtr<UInputAction> CameraZoomAction;

public:
	// Opens the pause menu if it is closed, closes it if it is open. Does nothing once the Game Over
	// screen is showing (ShowGameOver): the run is already over, so there is nothing to pause for.
	void TogglePauseMenu();

	// Closes the pause menu if it is open, shows the Game Over screen with SurvivalSeconds and
	// Experience, pauses the game (same mechanism as the pause menu: it also stops the player's
	// movement, weapon timer, every enemy's pursuit and contact damage, spawning and pickup attraction,
	// since none of those tick or run timers while the world is paused), and sets the cursor and input
	// mode the same way the pause menu does. Safe to call more than once; later calls just refresh the text.
	void ShowGameOver(float SurvivalSeconds, float Experience);

private:
	void OpenPauseMenu();
	void ClosePauseMenu();

	void HandleCameraTurn(const FInputActionValue& Value);
	void HandleCameraLookUp(const FInputActionValue& Value);
	void HandleCameraZoom(const FInputActionValue& Value);

	// The view-target camera pawn, or null if the view target is not a camera pawn
	// or is the possessed pawn (which then handles its own input).
	AMapCameraPawn* GetForwardedCameraPawn() const;

	// Created once and reused. The menu is open only while it is in the viewport.
	UPROPERTY(Transient)
	TObjectPtr<UPauseMenuWidget> PauseMenuWidget;

	// Created once and reused, the same way. Never removed once shown: the run is over, and the only
	// ways out are Restart (reloads the level) and Quit.
	UPROPERTY(Transient)
	TObjectPtr<UGameOverWidget> GameOverWidget;
};