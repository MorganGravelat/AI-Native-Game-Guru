// Fill out your copyright notice in the Description page of Project Settings.


#include "MapCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "EnhancedInputComponent.h"
#include "EngineUtils.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "MapArrays.h"
#include "TileMapRenderer.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
AMapCameraPawn::AMapCameraPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(SceneRoot);

	// The arm is driven by its own relative rotation, not by the controller.
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritRoll = false;
	SpringArm->bInheritYaw = true;

	// Ground collision is intentionally out of scope for now.
	SpringArm->bDoCollisionTest = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->SetProjectionMode(ECameraProjectionMode::Perspective);
	Camera->bUsePawnControlRotation = false;

	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionFinder(TEXT("/Game/Input/IA_CameraMove.IA_CameraMove"));
	static ConstructorHelpers::FObjectFinder<UInputAction> TurnActionFinder(TEXT("/Game/Input/IA_CameraTurn.IA_CameraTurn"));
	static ConstructorHelpers::FObjectFinder<UInputAction> LookUpActionFinder(TEXT("/Game/Input/IA_CameraLookUp.IA_CameraLookUp"));
	static ConstructorHelpers::FObjectFinder<UInputAction> ZoomActionFinder(TEXT("/Game/Input/IA_CameraZoom.IA_CameraZoom"));

	if (MoveActionFinder.Succeeded()) { MoveAction = MoveActionFinder.Object; }
	if (TurnActionFinder.Succeeded()) { TurnAction = TurnActionFinder.Object; }
	if (LookUpActionFinder.Succeeded()) { LookUpAction = LookUpActionFinder.Object; }
	if (ZoomActionFinder.Succeeded()) { ZoomAction = ZoomActionFinder.Object; }

	ApplyCameraSettings();
}

void AMapCameraPawn::ApplyCameraSettings()
{
	// Keep ranges ordered so a bad edit cannot invert the clamps.
	const float ArmMin = FMath::Min(MinArmLength, MaxArmLength);
	const float ArmMax = FMath::Max(MinArmLength, MaxArmLength);
	const float PitchMin = FMath::Min(MinPitch, MaxPitch);
	const float PitchMax = FMath::Max(MinPitch, MaxPitch);

	DesiredArmLength = FMath::Clamp(InitialArmLength, ArmMin, ArmMax);
	CurrentPitch = FMath::Clamp(InitialPitch, PitchMin, PitchMax);

	SpringArm->TargetArmLength = DesiredArmLength;
	SpringArm->SetRelativeRotation(FRotator(CurrentPitch, 0.0f, 0.0f));

	SpringArm->bEnableCameraLag = bEnableCameraLag;
	SpringArm->CameraLagSpeed = CameraLagSpeed;
	SpringArm->bEnableCameraRotationLag = bEnableCameraRotationLag;
	SpringArm->CameraRotationLagSpeed = CameraRotationLagSpeed;

	Camera->SetProjectionMode(ECameraProjectionMode::Perspective);
	Camera->SetFieldOfView(FieldOfView);
}

void AMapCameraPawn::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyCameraSettings();
}

#if WITH_EDITOR
void AMapCameraPawn::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	ApplyCameraSettings();
}
#endif

// Called when the game starts or when spawned
void AMapCameraPawn::BeginPlay()
{
	Super::BeginPlay();

	ApplyCameraSettings();
	ApplyMapBounds();

	if (IsValid(FollowTarget))
	{
		SetActorLocation(FollowTarget->GetActorLocation() + FollowOffset);
	}
	else
	{
		SetActorLocation(ConstrainLocation(GetActorLocation()));
	}
}

void AMapCameraPawn::ApplyMapBounds()
{
	if (!bDeriveBoundsFromMap)
	{
		return;
	}

	// LevelToRender is public on the renderer, so the camera can read which map
	// is shown without the renderer knowing about the camera.
	int32 Level = 0;
	bool bFoundRenderer = false;

	for (TActorIterator<ATileMapRenderer> It(GetWorld()); It; ++It)
	{
		Level = It->LevelToRender;
		bFoundRenderer = true;
		break;
	}

	FGrantMoneyMapData MapData;

	if (!bFoundRenderer || !FMapArrays::GetMap(Level, MapData) || !MapData.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("MapCameraPawn: Could not derive bounds from the map; using manual bounds."));
		return;
	}

	int32 MinX = MAX_int32;
	int32 MinY = MAX_int32;
	int32 MaxX = MIN_int32;
	int32 MaxY = MIN_int32;

	// Same definition of playable as the boundary walls: valid cell with a ground tile.
	for (int32 X = 0; X < MapData.Width; ++X)
	{
		for (int32 Y = 0; Y < MapData.Height; ++Y)
		{
			if (MapData.Valid[X][Y] != 0 && MapData.Ground[X][Y] != 0)
			{
				MinX = FMath::Min(MinX, X);
				MinY = FMath::Min(MinY, Y);
				MaxX = FMath::Max(MaxX, X);
				MaxY = FMath::Max(MaxY, Y);
			}
		}
	}

	if (MinX > MaxX)
	{
		UE_LOG(LogTemp, Warning, TEXT("MapCameraPawn: Level %d has no playable cells; using manual bounds."), Level);
		return;
	}

	// Tile cells are centered on (X * TileSize, Y * TileSize).
	MovementBoundsMin = FVector2D(MinX * MapTileSize - MapBoundsPadding, MinY * MapTileSize - MapBoundsPadding);
	MovementBoundsMax = FVector2D(MaxX * MapTileSize + MapBoundsPadding, MaxY * MapTileSize + MapBoundsPadding);

	UE_LOG(LogTemp, Log, TEXT("MapCameraPawn: Level %d bounds X %.0f..%.0f Y %.0f..%.0f"),
		Level, MovementBoundsMin.X, MovementBoundsMax.X, MovementBoundsMin.Y, MovementBoundsMax.Y);
}

// Called every frame
void AMapCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsValid(FollowTarget))
	{
		// The target owns its own movement and collision; the camera just tracks it.
		SetActorLocation(FollowTarget->GetActorLocation() + FollowOffset);
	}
	else if (!MoveInput.IsNearlyZero())
	{
		// Move on the ground plane relative to the camera's yaw only.
		const FRotator YawRotation(0.0f, GetActorRotation().Yaw, 0.0f);
		const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Cap at 1 so diagonals are not faster than straight movement.
		FVector2D Input = MoveInput;
		if (Input.SizeSquared() > 1.0)
		{
			Input.Normalize();
		}

		const FVector Delta = (Forward * Input.Y + Right * Input.X) * (MoveSpeed * DeltaTime);
		SetActorLocation(ConstrainLocation(GetActorLocation() + Delta));
	}

	if (!FMath::IsNearlyEqual(SpringArm->TargetArmLength, DesiredArmLength, 0.01f))
	{
		SpringArm->TargetArmLength = (ZoomInterpSpeed > 0.0f)
			? FMath::FInterpTo(SpringArm->TargetArmLength, DesiredArmLength, DeltaTime, ZoomInterpSpeed)
			: DesiredArmLength;
	}
}

// Called to bind functionality to input
void AMapCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogTemp, Error, TEXT("MapCameraPawn: Input component is not an EnhancedInputComponent."));
		return;
	}

	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMapCameraPawn::HandleMove);
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Completed, this, &AMapCameraPawn::HandleMoveCompleted);
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Canceled, this, &AMapCameraPawn::HandleMoveCompleted);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MapCameraPawn: MoveAction (IA_CameraMove) is not set."));
	}

	if (TurnAction)
	{
		EnhancedInput->BindAction(TurnAction, ETriggerEvent::Triggered, this, &AMapCameraPawn::HandleTurn);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MapCameraPawn: TurnAction (IA_CameraTurn) is not set."));
	}

	if (LookUpAction)
	{
		EnhancedInput->BindAction(LookUpAction, ETriggerEvent::Triggered, this, &AMapCameraPawn::HandleLookUp);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MapCameraPawn: LookUpAction (IA_CameraLookUp) is not set."));
	}

	if (ZoomAction)
	{
		EnhancedInput->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AMapCameraPawn::HandleZoom);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MapCameraPawn: ZoomAction (IA_CameraZoom) is not set."));
	}
}

void AMapCameraPawn::HandleMove(const FInputActionValue& Value)
{
	MoveInput = Value.Get<FVector2D>();
}

void AMapCameraPawn::HandleMoveCompleted(const FInputActionValue& Value)
{
	MoveInput = FVector2D::ZeroVector;
}

void AMapCameraPawn::HandleTurn(const FInputActionValue& Value)
{
	// Mouse delta is already per-frame movement, so it is not scaled by DeltaTime.
	AddActorWorldRotation(FRotator(0.0f, Value.Get<float>() * TurnSensitivity, 0.0f));
}

void AMapCameraPawn::HandleLookUp(const FInputActionValue& Value)
{
	const float PitchMin = FMath::Min(MinPitch, MaxPitch);
	const float PitchMax = FMath::Max(MinPitch, MaxPitch);
	const float Direction = bInvertPitch ? -1.0f : 1.0f;

	CurrentPitch = FMath::Clamp(CurrentPitch + Value.Get<float>() * PitchSensitivity * Direction, PitchMin, PitchMax);
	SpringArm->SetRelativeRotation(FRotator(CurrentPitch, 0.0f, 0.0f));
}

void AMapCameraPawn::HandleZoom(const FInputActionValue& Value)
{
	const float ArmMin = FMath::Min(MinArmLength, MaxArmLength);
	const float ArmMax = FMath::Max(MinArmLength, MaxArmLength);

	// Wheel up (positive) zooms in.
	DesiredArmLength = FMath::Clamp(DesiredArmLength - Value.Get<float>() * ZoomStep, ArmMin, ArmMax);
}

FVector AMapCameraPawn::ConstrainLocation(const FVector& Location) const
{
	FVector Result = Location;

	if (bLockHeightToGround)
	{
		Result.Z = GroundZ + HeightAboveGround;
	}

	if (!bClampToBounds)
	{
		return Result;
	}

	Result.X = FMath::Clamp(Result.X, FMath::Min(MovementBoundsMin.X, MovementBoundsMax.X), FMath::Max(MovementBoundsMin.X, MovementBoundsMax.X));
	Result.Y = FMath::Clamp(Result.Y, FMath::Min(MovementBoundsMin.Y, MovementBoundsMax.Y), FMath::Max(MovementBoundsMin.Y, MovementBoundsMax.Y));
	return Result;
}

void AMapCameraPawn::SetFollowTarget(AActor* NewTarget)
{
	FollowTarget = NewTarget;

	if (!IsValid(FollowTarget))
	{
		SetActorLocation(ConstrainLocation(GetActorLocation()));
	}
}

void AMapCameraPawn::SetMovementBounds(const FVector2D& NewMin, const FVector2D& NewMax)
{
	MovementBoundsMin = NewMin;
	MovementBoundsMax = NewMax;
	SetActorLocation(ConstrainLocation(GetActorLocation()));
}
