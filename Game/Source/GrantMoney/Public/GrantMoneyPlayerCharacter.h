// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "GrantMoneyPlayerCharacter.generated.h"

class UGrantMoneyCharacterVisualData;
class UInputAction;
class UStaticMeshComponent;
class UAnimationAsset;
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
	virtual void Tick(float DeltaSeconds) override;

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
	
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Animation")
	TObjectPtr<UAnimationAsset> IdleAnimation;

	UPROPERTY(EditAnywhere, Category = "Animation")
	TObjectPtr<UAnimationAsset> JogAnimation;

	// Ground speed (cm/s) above which the jog plays.
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (ClampMin = "0"))
	float JogSpeedThreshold = 5.0f;
	
	// Optional override of the built-in Manny defaults. If the asset doesn't
	// exist, the defaults set in the constructor are used.
	UPROPERTY(EditAnywhere, Category = "Visual")
	TSoftObjectPtr<UGrantMoneyCharacterVisualData> VisualData;
private:
	void PlayLocomotionAnimation(UAnimationAsset* Animation);

	UPROPERTY(Transient)
	TObjectPtr<UAnimationAsset> CurrentAnimation;
	
	void ApplyVisualData();

	// False when a data asset supplies its own animation setup.
	bool bUseBuiltInLocomotion = true;
};
