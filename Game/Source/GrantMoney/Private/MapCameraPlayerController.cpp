// Fill out your copyright notice in the Description page of Project Settings.


#include "MapCameraPlayerController.h"

#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"

AMapCameraPlayerController::AMapCameraPlayerController()
{
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> ContextFinder(TEXT("/Game/Input/IMC_MapCamera.IMC_MapCamera"));

	if (ContextFinder.Succeeded())
	{
		MapCameraMappingContext = ContextFinder.Object;
	}
}

void AMapCameraPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController())
	{
		// Mouse drives yaw/pitch, so capture it and hide the cursor.
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
	}
}

void AMapCameraPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
	{
		return;
	}

	if (!MapCameraMappingContext)
	{
		UE_LOG(LogTemp, Error, TEXT("MapCameraPlayerController: IMC_MapCamera is not set."));
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(MapCameraMappingContext, MappingContextPriority);
	}
}
