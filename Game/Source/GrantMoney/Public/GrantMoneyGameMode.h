// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GrantMoneyGameMode.generated.h"

class AMapCameraPawn;

/**
 * Spawns the player character on a safe cell of the level's generated map and
 * has the player possess it. The PlayerStart is only a fallback.
 * The Module 1 camera is spawned as a separate, unpossessed actor, kept as the
 * player's view target, and set to follow the character.
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

	// Spawns on the generated map's safe start. Falls back to StartSpot (the PlayerStart) with an error log.
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

protected:
	// Finds the level's TileMapRenderer and returns the first safe cell where PawnClass fits without blocking overlaps.
	// Logs the reason and returns false on failure.
	bool FindSafeSpawnLocation(UClass* PawnClass, FVector& OutLocation) const;

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
