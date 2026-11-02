// Fill out your copyright notice in the Description page of Project Settings.


#include "GrantMoneyPlayerCharacter.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Math/RotationMatrix.h"
#include "UObject/ConstructorHelpers.h"

AGrantMoneyPlayerCharacter::AGrantMoneyPlayerCharacter()
{
	// Nothing to update per frame yet.
	PrimaryActorTick.bCanEverTick = false;

	// Movement is relative to the camera, so the controller's rotation must not turn the character.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Face the direction of travel.
	GetCharacterMovement()->bOrientRotationToMovement = true;

	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionFinder(TEXT("/Game/Input/IA_CameraMove.IA_CameraMove"));

	if (MoveActionFinder.Succeeded())
	{
		MoveAction = MoveActionFinder.Object;
	}

	// Slightly narrower front-to-back than side-to-side, so the facing direction is visible.
	// Sized to fit inside the default capsule (radius 34, half height 88).
	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
	PlaceholderMesh->SetupAttachment(RootComponent);
	PlaceholderMesh->SetRelativeScale3D(FVector(0.4, 0.55, 1.7));
	PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderMesh->SetCanEverAffectNavigation(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));

	if (CubeFinder.Succeeded())
	{
		PlaceholderMesh->SetStaticMesh(CubeFinder.Object);
	}
}

void AGrantMoneyPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	UE_LOG(LogTemp, Log, TEXT("GrantMoneyPlayerCharacter: %s possessed by %s."), *GetName(), *GetNameSafe(NewController));
}

void AGrantMoneyPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogTemp, Error, TEXT("GrantMoneyPlayerCharacter: Input component is not an EnhancedInputComponent."));
		return;
	}

	if (!MoveAction)
	{
		UE_LOG(LogTemp, Error, TEXT("GrantMoneyPlayerCharacter: MoveAction (IA_CameraMove) is not set."));
		return;
	}

	// The mapping context is added by the PlayerController.
	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGrantMoneyPlayerCharacter::HandleMove);
}

void AGrantMoneyPlayerCharacter::HandleMove(const FInputActionValue& Value)
{
	// X is right, Y is forward.
	FVector2D Input = Value.Get<FVector2D>();

	// Cap at 1 so diagonals are not faster than straight movement.
	if (Input.SizeSquared() > 1.0)
	{
		Input.Normalize();
	}

	// Use whatever camera is currently the view, without knowing its class.
	FRotator ViewRotation = GetActorRotation();

	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (PlayerController->PlayerCameraManager)
		{
			ViewRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
		}
	}

	// Yaw only, so a pitched camera does not tilt movement off the ground plane.
	const FRotationMatrix YawMatrix(FRotator(0.0, ViewRotation.Yaw, 0.0));

	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), Input.Y);
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), Input.X);
}

