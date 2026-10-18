// Fill out your copyright notice in the Description page of Project Settings.


#include "GrantMoneyGameMode.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GrantMoneyPlayerCharacter.h"
#include "MapCameraPawn.h"

AGrantMoneyGameMode::AGrantMoneyGameMode()
{
	PlayerCharacterClass = AGrantMoneyPlayerCharacter::StaticClass();
	CameraPawnClass = AMapCameraPawn::StaticClass();
	DefaultPawnClass = PlayerCharacterClass;
}

UClass* AGrantMoneyGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	return PlayerCharacterClass ? PlayerCharacterClass.Get() : Super::GetDefaultPawnClassForController_Implementation(InController);
}

void AGrantMoneyGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// Possessing the character must not move the view onto the character, which has no camera.
	NewPlayer->bAutoManageActiveCameraTarget = false;

	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	APawn* PlayerPawn = NewPlayer->GetPawn();
	if (!PlayerPawn)
	{
		UE_LOG(LogTemp, Error, TEXT("GrantMoneyGameMode: no pawn was spawned for %s."), *GetNameSafe(NewPlayer));
		return;
	}

	if (!IsValid(CameraPawn) && CameraPawnClass)
	{
		// Same transform the camera pawn used when it was the default pawn: the PlayerStart, yaw only.
		const AActor* StartSpot = NewPlayer->StartSpot.Get();
		const FTransform StartTransform = StartSpot ? StartSpot->GetActorTransform() : PlayerPawn->GetActorTransform();
		const FTransform SpawnTransform(FRotator(0.0, StartTransform.Rotator().Yaw, 0.0), StartTransform.GetLocation());

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		CameraPawn = GetWorld()->SpawnActor<AMapCameraPawn>(CameraPawnClass, SpawnTransform, SpawnParams);
	}

	if (IsValid(CameraPawn))
	{
		NewPlayer->SetViewTarget(CameraPawn);

		// The Module 1 camera becomes the gameplay camera: same arm, pitch, yaw and zoom, now tracking the player.
		CameraPawn->SetFollowTarget(PlayerPawn);

		UE_LOG(LogTemp, Log, TEXT("GrantMoneyGameMode: camera %s now follows %s."), *GetNameSafe(CameraPawn), *GetNameSafe(PlayerPawn));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GrantMoneyGameMode: camera pawn could not be spawned; falling back to the character's view."));
		NewPlayer->bAutoManageActiveCameraTarget = true;
		NewPlayer->AutoManageActiveCameraTarget(PlayerPawn);
	}
}

