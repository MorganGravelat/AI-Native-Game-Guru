// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/GameModeBase.h"
#include "RunController.h"
#include "GrantMoneyGameMode.generated.h"

class AMapCameraPawn;
class ASkyLightingActor;
class AEnemySpawner;
class ARewardTotemSpawner;
class ABossEncounter;
class ARunController;

/**
 * Spawns the player character on a safe cell of the level's generated map and
 * has the player possess it. The PlayerStart is only a fallback.
 * The Module 1 camera is spawned as a separate, unpossessed actor, kept as the
 * player's view target, and set to follow the character.
 *
 * It also sets up the timed run: one ARunController and, unless the level has its own, a default
 * AEnemySpawner. The run is started once the map has been built and the player pawn exists.
 * A RunController or EnemySpawner placed in the level is used instead of a default one, so a level
 * can set its own duration, enemy type and spawn numbers. The same goes for the reward totems (SCRUM-39):
 * a default ARewardTotemSpawner scatters them over the map unless the level has its own spawner.
 *
 * On a loss (the player's HealthComponent reaching zero, see AGrantMoneyPlayerCharacter), it pauses the
 * game and shows the local player's Game Over screen with survival time and XP (SCRUM-44); on a win (the
 * boss dying, see ABossEncounter) it shows the same screen as Victory. A boss encounter is created
 * (or a level's own used) and bound to the run, like the spawner. Restart and
 * Quit are owned entirely by that screen (UGameOverWidget); this class only reacts to the state change.
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

	// Adds the skybox and global lighting to levels that do not have one.
	virtual void BeginPlay() override;

	// Spawns on the generated map's safe start. Falls back to StartSpot (the PlayerStart) with an error log.
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

	// The run's clock and state, or null before the first player has joined.
	ARunController* GetRunController() const { return RunController; }

protected:
	// Finds the level's TileMapRenderer and returns the first safe cell where PawnClass fits without blocking overlaps.
	// Logs the reason and returns false on failure.
	bool FindSafeSpawnLocation(UClass* PawnClass, FVector& OutLocation) const;

	// Makes sure the level has a run controller and a spawner, then starts the run as soon as it is ready.
	void PrepareRun();

	// True when the level's map has been built and the player has a pawn. OutReason says what is missing.
	bool IsRunReady(FString& OutReason) const;

	// Polls IsRunReady and starts the run once.
	void CheckRunReady();

	// Bound to RunController->OnStateChanged. On Lost, pauses the game and shows the Game Over screen
	// with the player's survival time and XP (SCRUM-44); on Won (the boss died) it does the same with the
	// Victory screen. Does nothing for every other transition.
	UFUNCTION()
	void HandleRunStateChanged(EGrantMoneyRunState OldState, EGrantMoneyRunState NewState);

	// Defaults to AGrantMoneyPlayerCharacter.
	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSubclassOf<APawn> PlayerCharacterClass;

	// Defaults to AMapCameraPawn.
	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSubclassOf<AMapCameraPawn> CameraPawnClass;

	// Defaults to ASkyLightingActor. Spawned only when the level has no ASkyLightingActor. Null disables it.
	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSubclassOf<ASkyLightingActor> SkyLightingClass;

	// Defaults to ARunController (3 minute run). Spawned only when the level has no ARunController. Make a
	// Blueprint child to change the duration for every level, or place one in a level to change it for that level.
	UPROPERTY(EditDefaultsOnly, Category = "Run")
	TSubclassOf<ARunController> RunControllerClass;

	// Defaults to AEnemySpawner (slimes). Spawned only when the level has no AEnemySpawner. Null disables it.
	UPROPERTY(EditDefaultsOnly, Category = "Run")
	TSubclassOf<AEnemySpawner> EnemySpawnerClass;

	// Defaults to ARewardTotemSpawner (3 totems). Spawned only when the level has no ARewardTotemSpawner. Null disables it.
	UPROPERTY(EditDefaultsOnly, Category = "Run")
	TSubclassOf<ARewardTotemSpawner> RewardTotemSpawnerClass;

	// Defaults to ABossEncounter (one slime boss when the run enters BossPhase). Spawned only when the level has
	// no ABossEncounter. Null disables the boss, so the run can then only end in a loss.
	UPROPERTY(EditDefaultsOnly, Category = "Run")
	TSubclassOf<ABossEncounter> BossEncounterClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<AMapCameraPawn> CameraPawn;

	UPROPERTY(Transient)
	TObjectPtr<ARunController> RunController;

	FTimerHandle RunReadyTimer;
	int32 RunReadyChecks = 0;
};
