// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GrantMoneyPlayerCharacter.generated.h"

/**
 * The player's playable character. Uses the standard ACharacter capsule and
 * CharacterMovementComponent. Movement input, animation and combat are added in
 * later tickets. The view comes from the map camera, not from this class.
 */
UCLASS()
class GRANTMONEY_API AGrantMoneyPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGrantMoneyPlayerCharacter();

protected:
	virtual void PossessedBy(AController* NewController) override;
};
