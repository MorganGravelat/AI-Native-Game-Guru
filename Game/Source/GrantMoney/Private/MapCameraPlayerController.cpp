// Fill out your copyright notice in the Description page of Project Settings.


#include "MapCameraPlayerController.h"

#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "MapCameraPawn.h"
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
	static ConstructorHelpers::FObjectFinder<UInputAction> TurnActionFinder(TEXT("/Game/Input/IA_CameraTurn.IA_CameraTurn"));
	static ConstructorHelpers::FObjectFinder<UInputAction> LookUpActionFinder(TEXT("/Game/Input/IA_CameraLookUp.IA_CameraLookUp"));
	static ConstructorHelpers::FObjectFinder<UInputAction> ZoomActionFinder(TEXT("/Game/Input/IA_CameraZoom.IA_CameraZoom"));

	if (PauseActionFinder.Succeeded())
	{
		PauseAction = PauseActionFinder.Object;
	}

	if (PauseMenuFinder.Succeeded())
	{
		PauseMenuWidgetClass = PauseMenuFinder.Class;
	}

	if (TurnActionFinder.Succeeded()) { CameraTurnAction = TurnActionFinder.Object; }
	if (LookUpActionFinder.Succeeded()) { CameraLookUpAction = LookUpActionFinder.Object; }
	if (ZoomActionFinder.Succeeded()) { CameraZoomAction = ZoomActionFinder.Object; }
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
	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);

	if (!EnhancedInput)
	{
		UE_LOG(LogTemp, Error, TEXT("MapCameraPlayerController: InputComponent is not an EnhancedInputComponent."));
		return;
	}

	if (PauseAction)
	{
		EnhancedInput->BindAction(PauseAction, ETriggerEvent::Started, this, &AMapCameraPlayerController::TogglePauseMenu);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MapCameraPlayerController: IA_Pause is not set."));
	}

	// Camera input also lives on the controller, because the camera pawn is not possessed.
	if (CameraTurnAction)
	{
		EnhancedInput->BindAction(CameraTurnAction, ETriggerEvent::Triggered, this, &AMapCameraPlayerController::HandleCameraTurn);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MapCameraPlayerController: IA_CameraTurn is not set."));
	}

	if (CameraLookUpAction)
	{
		EnhancedInput->BindAction(CameraLookUpAction, ETriggerEvent::Triggered, this, &AMapCameraPlayerController::HandleCameraLookUp);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MapCameraPlayerController: IA_CameraLookUp is not set."));
	}

	if (CameraZoomAction)
	{
		EnhancedInput->BindAction(CameraZoomAction, ETriggerEvent::Triggered, this, &AMapCameraPlayerController::HandleCameraZoom);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MapCameraPlayerController: IA_CameraZoom is not set."));
	}
}

AMapCameraPawn* AMapCameraPlayerController::GetForwardedCameraPawn() const
{
	AMapCameraPawn* CameraPawn = Cast<AMapCameraPawn>(GetViewTarget());

	// A possessed camera pawn binds and handles its own input.
	return (CameraPawn && CameraPawn != GetPawn()) ? CameraPawn : nullptr;
}

void AMapCameraPlayerController::HandleCameraTurn(const FInputActionValue& Value)
{
	if (AMapCameraPawn* CameraPawn = GetForwardedCameraPawn())
	{
		CameraPawn->AddCameraYaw(Value.Get<float>());
	}
}

void AMapCameraPlayerController::HandleCameraLookUp(const FInputActionValue& Value)
{
	if (AMapCameraPawn* CameraPawn = GetForwardedCameraPawn())
	{
		CameraPawn->AddCameraPitch(Value.Get<float>());
	}
}

void AMapCameraPlayerController::HandleCameraZoom(const FInputActionValue& Value)
{
	if (AMapCameraPawn* CameraPawn = GetForwardedCameraPawn())
	{
		CameraPawn->AddCameraZoom(Value.Get<float>());
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
