// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MapCameraPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UPauseMenuWidget;

/**
 * Registers the map camera input mapping context for the local player and
 * owns the pause menu toggle. The camera Pawn owns the camera action bindings.
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

public:
	// Opens the pause menu if it is closed, closes it if it is open.
	void TogglePauseMenu();

private:
	void OpenPauseMenu();
	void ClosePauseMenu();

	// Created once and reused. The menu is open only while it is in the viewport.
	UPROPERTY(Transient)
	TObjectPtr<UPauseMenuWidget> PauseMenuWidget;
};