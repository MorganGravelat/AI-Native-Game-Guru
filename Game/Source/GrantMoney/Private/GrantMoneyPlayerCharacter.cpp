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
#include "Animation/AnimationAsset.h"

AGrantMoneyPlayerCharacter::AGrantMoneyPlayerCharacter()
{
	// Nothing to update per frame yet.
	PrimaryActorTick.bCanEverTick = true;

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

	USkeletalMeshComponent* CharacterMesh = GetMesh();
	CharacterMesh->SetRelativeLocation(FVector(0.0, 0.0, -GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
	CharacterMesh->SetRelativeRotation(FRotator(0.0, -90.0, 0.0));
	CharacterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CharacterMesh->SetGenerateOverlapEvents(false);
	CharacterMesh->SetCanEverAffectNavigation(false);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyFinder(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));

	if (MannyFinder.Succeeded())
	{
		CharacterMesh->SetSkeletalMesh(MannyFinder.Object);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("GrantMoneyPlayerCharacter: Manny mesh is not available at /Game/Characters/Mannequins/Meshes/SKM_Manny_Simple."));
	}
	
	static ConstructorHelpers::FObjectFinder<UAnimationAsset> IdleFinder(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
	if (IdleFinder.Succeeded()) { IdleAnimation = IdleFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UAnimationAsset> JogFinder(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd.MF_Unarmed_Jog_Fwd"));
	if (JogFinder.Succeeded()) { JogAnimation = JogFinder.Object; }
	
	CharacterMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
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

void AGrantMoneyPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	PlayLocomotionAnimation(IdleAnimation);
}

void AGrantMoneyPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float GroundSpeed = GetVelocity().Size2D();
	PlayLocomotionAnimation(GroundSpeed > JogSpeedThreshold ? JogAnimation : IdleAnimation);
}

void AGrantMoneyPlayerCharacter::PlayLocomotionAnimation(UAnimationAsset* Animation)
{
	// Restart only when the desired animation actually changes.
	if (!Animation || Animation == CurrentAnimation)
	{
		return;
	}

	CurrentAnimation = Animation;
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GetMesh()->PlayAnimation(Animation, true);
}