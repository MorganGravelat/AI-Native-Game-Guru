// Fill out your copyright notice in the Description page of Project Settings.


#include "MapCameraPlayerController.h"

#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameOverWidget.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "LevelUpWidget.h"
#include "MapCameraPawn.h"
#include "PauseMenuWidget.h"
#include "RunController.h"
#include "RunUiPolicy.h"
#include "UObject/ConstructorHelpers.h"
#include "UpgradeComponent.h"

AMapCameraPlayerController::AMapCameraPlayerController()
{
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> ContextFinder(TEXT("/Game/Input/IMC_MapCamera.IMC_MapCamera"));

	if (ContextFinder.Succeeded())
	{
		MapCameraMappingContext = ContextFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> PauseActionFinder(TEXT("/Game/Input/IA_Pause.IA_Pause"));
	static ConstructorHelpers::FClassFinder<UPauseMenuWidget> PauseMenuFinder(TEXT("/Game/UI/WBP_PauseMenu"));
	static ConstructorHelpers::FClassFinder<UGameOverWidget> GameOverFinder(TEXT("/Game/UI/WBP_GameOver"));
	static ConstructorHelpers::FClassFinder<ULevelUpWidget> LevelUpFinder(TEXT("/Game/UI/WBP_LevelUp"));
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

	if (GameOverFinder.Succeeded())
	{
		GameOverWidgetClass = GameOverFinder.Class;
	}

	if (LevelUpFinder.Succeeded())
	{
		LevelUpWidgetClass = LevelUpFinder.Class;
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
	const bool bLevelUpShowing = IsValid(LevelUpWidget) && LevelUpWidget->IsInViewport();
	const bool bPauseMenuOpen = IsValid(PauseMenuWidget) && PauseMenuWidget->IsInViewport();

	if (bPauseMenuOpen)
	{
		ResumeFromPauseMenu();
	}
	else if (FRunUiPolicy::CanOpenPauseMenu(IsRunFinished(), bLevelUpShowing))
	{
		OpenPauseMenu();
	}
}

bool AMapCameraPlayerController::IsRunFinished() const
{
	if (bResultShown)
	{
		return true;
	}

	const ARunController* Run = ARunController::Find(GetWorld());
	return Run && FRunUiPolicy::IsTerminal(Run->GetState());
}

void AMapCameraPlayerController::ResumeFromPauseMenu()
{
	const bool bLevelUpShowing = IsValid(LevelUpWidget) && LevelUpWidget->IsInViewport();
	if (FRunUiPolicy::CanResumeFromPauseMenu(IsRunFinished(), bLevelUpShowing))
	{
		ClosePauseMenu();
	}
}

void AMapCameraPlayerController::ShowGameOver(float SurvivalSeconds, float Experience)
{
	ShowRunResult(GameOverWidgetClass, FText::FromString(TEXT("Game Over")), SurvivalSeconds, Experience);
}

void AMapCameraPlayerController::ShowVictory(float SurvivalSeconds, float Experience)
{
	// WBP_GameOver doubles as the Victory screen unless a dedicated class is assigned.
	ShowRunResult(VictoryWidgetClass ? VictoryWidgetClass : GameOverWidgetClass, FText::FromString(TEXT("Victory!")), SurvivalSeconds, Experience);
}

void AMapCameraPlayerController::ShowRunResult(TSubclassOf<UGameOverWidget> WidgetClass, const FText& Title, float SurvivalSeconds, float Experience)
{
	if (!WidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("MapCameraPlayerController: the result screen class (WBP_GameOver) is not set."));
		return;
	}

	// The run just ended. From here on nothing but Restart and Quit may change the pause or the input mode.
	bResultShown = true;

	// A pause menu or a level-up choice open over the result screen would be confusing, and either could
	// otherwise hand control back to the game.
	ClosePauseMenu();
	if (IsValid(LevelUpWidget))
	{
		LevelUpWidget->RemoveFromParent();
	}

	// A run ends once, so this is created once; a repeated call just refreshes the text.
	if (!IsValid(GameOverWidget))
	{
		GameOverWidget = CreateWidget<UGameOverWidget>(this, WidgetClass);
	}

	if (!GameOverWidget)
	{
		return;
	}

	GameOverWidget->SetResults(SurvivalSeconds, Experience, Title);

	if (!GameOverWidget->IsInViewport())
	{
		GameOverWidget->AddToViewport(ResultZOrder);
	}

	// Same mechanism the pause menu uses: pausing also freezes the player's movement and weapon timer,
	// every enemy's pursuit and contact damage, spawning, and pickups flying in, because none of those
	// tick or run timers while the world is paused.
	UGameplayStatics::SetGamePaused(this, true);
	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(GameOverWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void AMapCameraPlayerController::ShowLevelUpChoice(UUpgradeComponent* InUpgrades)
{
	// A finished run is final: a late choice (for example XP collected on the frame the player died) must
	// neither cover the result screen nor take its input.
	if (!FRunUiPolicy::CanShowLevelUp(IsRunFinished()))
	{
		return;
	}

	if (!LevelUpWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("MapCameraPlayerController: WBP_LevelUp class is not set."));
		return;
	}

	// A level-up choice forces a decision; a pause menu open over it would be confusing.
	ClosePauseMenu();

	if (!IsValid(LevelUpWidget))
	{
		LevelUpWidget = CreateWidget<ULevelUpWidget>(this, LevelUpWidgetClass);
	}

	if (!LevelUpWidget)
	{
		return;
	}

	// Every call, not only the first: the player (and with it the upgrade component) can be replaced
	// while this controller and its widget live on, and the widget must never answer a stale component.
	LevelUpWidget->SetUpgradeComponent(InUpgrades);

	// Each call (including the second, third, ... of several queued choices) re-reads the component's
	// current options, since they can differ from one choice to the next.
	LevelUpWidget->RefreshOptions();

	if (!LevelUpWidget->IsInViewport())
	{
		LevelUpWidget->AddToViewport(LevelUpZOrder);
	}

	// Same mechanism as the pause menu and the Game Over screen: pausing also suspends combat, the run
	// timer and any timer-driven temporary effect, since none of those tick or run timers while paused.
	UGameplayStatics::SetGamePaused(this, true);
	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(LevelUpWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void AMapCameraPlayerController::HideLevelUpChoice()
{
	if (IsValid(LevelUpWidget))
	{
		LevelUpWidget->RemoveFromParent();
	}

	// If the run finished meanwhile the result screen owns the pause and the input mode; resuming here
	// would restart combat behind it.
	if (!FRunUiPolicy::ShouldReturnToGameplayAfterLevelUp(IsRunFinished()))
	{
		return;
	}

	UGameplayStatics::SetGamePaused(this, false);
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
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

	PauseMenuWidget->AddToViewport(PauseMenuZOrder);
	UGameplayStatics::SetGamePaused(this, true);
	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void AMapCameraPlayerController::ClosePauseMenu()
{
	// Nothing to undo if the menu is not open; in particular this must not unpause a game that a
	// level-up choice or the result screen is holding paused.
	if (!IsValid(PauseMenuWidget) || !PauseMenuWidget->IsInViewport())
	{
		return;
	}

	PauseMenuWidget->RemoveFromParent();
	UGameplayStatics::SetGamePaused(this, false);
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}
