// Fill out your copyright notice in the Description page of Project Settings.


#include "GrantMoneyPlayerCharacter.h"

AGrantMoneyPlayerCharacter::AGrantMoneyPlayerCharacter()
{
	// Nothing to update per frame yet.
	PrimaryActorTick.bCanEverTick = false;

	// The controller has no look input yet, so don't let its rotation drive the character.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
}

void AGrantMoneyPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	UE_LOG(LogTemp, Log, TEXT("GrantMoneyPlayerCharacter: %s possessed by %s."), *GetName(), *GetNameSafe(NewController));
}

