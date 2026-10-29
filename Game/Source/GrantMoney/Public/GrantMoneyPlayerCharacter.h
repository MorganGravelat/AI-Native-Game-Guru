// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GrantMoneyPlayerCharacter.generated.h"

class UAnimationAsset;
class UInputAction;
class UGrantMoneyCharacterVisualData;
struct FInputActionValue;

/**
 * The player's playable character. Uses the standard ACharacter capsule and
 * CharacterMovementComponent. Moves relative to the active camera's yaw.
 * Looks like Epic's Manny (SKM_Manny_Simple) and switches between an idle and
 * a jog animation based on ground speed. A UGrantMoneyCharacterVisualData asset
 * can override the mesh, animation and placement without recompiling. Combat is
 * added in a later ticket. The view comes from the map camera, not this class.
 */
UCLASS()
class GRANTMONEY_API AGrantMoneyPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGrantMoneyPlayerCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	void HandleMove(const FInputActionValue& Value);

	// Optional override of the built-in Manny defaults. Defaults to
	// /Game/Characters/Player/DA_PlayerVisual; if that asset doesn't exist the
	// Manny defaults set in the constructor are used.
	UPROPERTY(EditAnywhere, Category = "Visual")
	TSoftObjectPtr<UGrantMoneyCharacterVisualData> VisualData;

	// Played while standing still.
	UPROPERTY(EditAnywhere, Category = "Animation")
	TObjectPtr<UAnimationAsset> IdleAnimation;

	// Played while moving.
	UPROPERTY(EditAnywhere, Category = "Animation")
	TObjectPtr<UAnimationAsset> JogAnimation;

	// Ground speed (cm/s) above which the jog animation plays.
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (ClampMin = "0"))
	float JogSpeedThreshold = 5.0f;

	// Defaults to /Game/Input/IA_CameraMove (Axis2D: X right, Y forward). Must be
	// mapped in the mapping context the PlayerController adds.
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

private:
	void ApplyVisualData();
	void PlayLocomotionAnimation(UAnimationAsset* Animation);

	// The animation currently playing from the built-in idle/jog logic.
	UPROPERTY(Transient)
	TObjectPtr<UAnimationAsset> CurrentAnimation;

	// False when a data asset supplies its own animation setup.
	bool bUseBuiltInLocomotion = true;
};
