// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GrantMoneyGameMode.generated.h"

class AMapCameraPawn;

/**
 * Spawns the player character at the PlayerStart and has the player possess it.
 * The Module 1 camera is spawned as a separate, unpossessed actor and kept as
 * the player's view target.
 */
UCLASS()
class GRANTMONEY_API AGrantMoneyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGrantMoneyGameMode();

	// Always returns PlayerCharacterClass, even if the Blueprint still saves an older DefaultPawnClass.
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

protected:
	// Defaults to AGrantMoneyPlayerCharacter.
	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSubclassOf<APawn> PlayerCharacterClass;

	// Defaults to AMapCameraPawn.
	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSubclassOf<AMapCameraPawn> CameraPawnClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<AMapCameraPawn> CameraPawn;
};
