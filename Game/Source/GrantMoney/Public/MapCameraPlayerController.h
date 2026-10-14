// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MapCameraPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UPauseMenuWidget;
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

	// Camera actions forwarded to the view-target camera pawn.
	// Default to /Game/Input/IA_CameraTurn, IA_CameraLookUp and IA_CameraZoom.
	UPROPERTY(EditAnywhere, Category = "Camera|Input")
	TObjectPtr<UInputAction> CameraTurnAction;

	UPROPERTY(EditAnywhere, Category = "Camera|Input")
	TObjectPtr<UInputAction> CameraLookUpAction;

	UPROPERTY(EditAnywhere, Category = "Camera|Input")
	TObjectPtr<UInputAction> CameraZoomAction;

public:
	// Opens the pause menu if it is closed, closes it if it is open.
	void TogglePauseMenu();

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
};