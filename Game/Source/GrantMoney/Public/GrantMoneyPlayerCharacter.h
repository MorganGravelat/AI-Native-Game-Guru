// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GrantMoneyPlayerCharacter.generated.h"

class UInputAction;
class UStaticMeshComponent;
struct FInputActionValue;

/**
 * The player's playable character. Uses the standard ACharacter capsule and
 * CharacterMovementComponent. Moves relative to the active camera's yaw.
 * Animation and combat are added in later tickets. The view comes from the
 * map camera, not from this class.
 */
UCLASS()
class GRANTMONEY_API AGrantMoneyPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGrantMoneyPlayerCharacter();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	virtual void PossessedBy(AController* NewController) override;

	void HandleMove(const FInputActionValue& Value);

	// Defaults to /Game/Input/IA_CameraMove (Axis2D: X right, Y forward). Must be
	// mapped in the mapping context the PlayerController adds.
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	// Temporary visible stand-in until the skeletal mesh and animation exist.
	// Collision comes from the capsule, so this has none.
	UPROPERTY(VisibleAnywhere, Category = "Placeholder")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;
};
