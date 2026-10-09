// Fill out your copyright notice in the Description page of Project Settings.


#include "MapCameraPlayerController.h"

#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "PauseMenuWidget.h"
#include "UObject/ConstructorHelpers.h"

AMapCameraPlayerController::AMapCameraPlayerController()
{
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> ContextFinder(TEXT("/Game/Input/IMC_MapCamera.IMC_MapCamera"));

	if (ContextFinder.Succeeded())
	{
		MapCameraMappingContext = ContextFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> PauseActionFinder(TEXT("/Game/Input/IA_Pause.IA_Pause"));
	static ConstructorHelpers::FClassFinder<UPauseMenuWidget> PauseMenuFinder(TEXT("/Game/UI/WBP_PauseMenu"));

	if (PauseActionFinder.Succeeded())
	{
		PauseAction = PauseActionFinder.Object;
	}

	if (PauseMenuFinder.Succeeded())
	{
		PauseMenuWidgetClass = PauseMenuFinder.Class;
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

	// Pause lives on the controller, not the Pawn, so it keeps working if the Pawn changes.
	if (!PauseAction)
	{
		UE_LOG(LogTemp, Error, TEXT("MapCameraPlayerController: IA_Pause is not set."));
		return;
	}

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInput->BindAction(PauseAction, ETriggerEvent::Started, this, &AMapCameraPlayerController::TogglePauseMenu);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MapCameraPlayerController: InputComponent is not an EnhancedInputComponent."));
	}
}

void AMapCameraPlayerController::TogglePauseMenu()
{
	if (IsValid(PauseMenuWidget) && PauseMenuWidget->IsInViewport())
	{
		ClosePauseMenu();
	}
	else
	{
		OpenPauseMenu();
	}
}

void AMapCameraPlayerController::OpenPauseMenu()
{
	if (!PauseMenuWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("MapCameraPlayerController: WBP_PauseMenu class is not set."));
		return;
	}

	if (!IsValid(PauseMenuWidget))
	{
		PauseMenuWidget = CreateWidget<UPauseMenuWidget>(this, PauseMenuWidgetClass);
	}

	if (!PauseMenuWidget)
	{
		return;
	}

	PauseMenuWidget->AddToViewport();
	UGameplayStatics::SetGamePaused(this, true);
	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void AMapCameraPlayerController::ClosePauseMenu()
{
	UGameplayStatics::SetGamePaused(this, false);

	if (IsValid(PauseMenuWidget))
	{
		PauseMenuWidget->RemoveFromParent();
	}

	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}
