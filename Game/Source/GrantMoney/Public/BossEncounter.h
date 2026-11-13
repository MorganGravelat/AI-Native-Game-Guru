// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/Actor.h"
#include "RunController.h"
#include "BossEncounter.generated.h"

class AEnemyBase;
class AEnemySpawner;
class UHealthComponent;

UENUM(BlueprintType)
enum class EBossEncounterState : uint8
{
	// The run has not reached BossPhase yet.
	Waiting,
	// BossPhase started but the boss is not in the world yet: placing it, or waiting to retry after a failed placement.
	Spawning,
	// The boss is alive.
	Fighting,
	// The boss died and the run was asked to win.
	Defeated,
	// Every placement attempt failed. The run is stuck in BossPhase and the HUD says why (GetSpawnDiagnostic).
	Failed,
	// The run ended (for example the player died) before the boss was defeated.
	Cancelled
};

/**
 * Runs the one boss fight of a run. When the ARunController enters BossPhase it spawns exactly one
 * BossClass at a location validated by the level's AEnemySpawner (FindSpawnLocationForClass: playable cells,
 * ground, clearance), starts its emergence, and watches it. The boss's death asks the run to win (once).
 *
 *  - Normal slimes that are alive stay alive. Whether new ones spawn is the run's policy
 *    (ARunController::bAllowNormalSpawnsInBossPhase, off by default).
 *  - A failed placement is retried every SpawnRetryIntervalSeconds, at most MaxSpawnAttempts times in total.
 *    Each failure is logged and shown by the HUD (GetSpawnDiagnostic); after the last one the encounter is
 *    Failed and the message stays, so the run can never sit in an empty boss phase without saying so.
 *  - Once the run is Won or Lost the encounter cancels its retries and ignores the boss's death, so a late
 *    callback cannot change a finished run. The state machine only ever moves forward.
 *
 * The game mode creates one (or uses a level's own, to change BossClass, name or retry numbers) and binds it
 * to the run controller. The HUD reads GetBoss, IsArrivalMessageVisible and GetSpawnDiagnostic.
 */
UCLASS()
class GRANTMONEY_API ABossEncounter : public AActor
{
	GENERATED_BODY()

public:
	ABossEncounter();

	// The boss to spawn. Defaults to ABossSlimeEnemy.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	TSubclassOf<AEnemyBase> BossClass;

	// Shown above the boss health bar.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	FString BossDisplayName = TEXT("Slime Boss");

	// Ground-plane distance from the player at which the boss may appear.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Spawn", meta = (ClampMin = "0.0", Units = "cm"))
	float MinSpawnDistance = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Spawn", meta = (ClampMin = "0.0", Units = "cm"))
	float MaxSpawnDistance = 1200.0f;

	// Total placement attempts (each one is a full bounded search by the spawner) before giving up.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Spawn", meta = (ClampMin = "1"))
	int32 MaxSpawnAttempts = 5;

	// Game seconds between a failed attempt and the next.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Spawn", meta = (ClampMin = "0.05", Units = "s"))
	float SpawnRetryIntervalSeconds = 1.0f;

	// How long the "Boss Arrived" message stays on screen after the boss appears.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|UI", meta = (ClampMin = "0.0", Units = "s"))
	float ArrivalMessageSeconds = 4.0f;

	// Listens to the run for BossPhase and for the end of the run. Safe to call again with the same run.
	// If the run is already in BossPhase, the fight starts now.
	void BindToRunController(ARunController* InRunController);

	// Uses this spawner for placement instead of the first one in the level. For tests and custom setups.
	void SetSpawner(AEnemySpawner* InSpawner);

	// Finds the one encounter in World, or null.
	static ABossEncounter* Find(const UWorld* World);

	// One placement attempt: counts it, spawns the boss on success, otherwise schedules the retry or fails.
	// Does nothing unless the encounter is Spawning and the run is still in BossPhase. Public so tests can
	// step the retries without waiting for the timer.
	void AttemptSpawn();

	UFUNCTION(BlueprintPure, Category = "Boss")
	EBossEncounterState GetState() const { return State; }

	// The boss while Fighting (and after it died until it is destroyed), else null.
	UFUNCTION(BlueprintPure, Category = "Boss")
	AEnemyBase* GetBoss() const;

	int32 GetSpawnAttempts() const { return SpawnAttempts; }

	// Times a boss has been spawned this run. Never above 1.
	int32 GetBossSpawnCount() const { return BossSpawnCount; }

	// True for ArrivalMessageSeconds of game time after the boss appears.
	UFUNCTION(BlueprintPure, Category = "Boss")
	bool IsArrivalMessageVisible() const;

	// Why the boss is not here, while Spawning (after a failed attempt) or Failed; empty otherwise.
	UFUNCTION(BlueprintPure, Category = "Boss")
	FString GetSpawnDiagnostic() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleBossPhaseStarted();

	UFUNCTION()
	void HandleRunStateChanged(EGrantMoneyRunState OldState, EGrantMoneyRunState NewState);

	UFUNCTION()
	void HandleBossDeath(UHealthComponent* DepletedHealth, AActor* DamageCauser);

	void BeginEncounter();
	void CancelPendingWork();
	AEnemySpawner* ResolveSpawner();

	EBossEncounterState State = EBossEncounterState::Waiting;

	UPROPERTY(Transient)
	TWeakObjectPtr<ARunController> RunController;

	UPROPERTY(Transient)
	TWeakObjectPtr<AEnemySpawner> SpawnerOverride;

	UPROPERTY(Transient)
	TWeakObjectPtr<AEnemyBase> Boss;

	FTimerHandle RetryTimer;
	int32 SpawnAttempts = 0;
	int32 BossSpawnCount = 0;
	double ArrivalTime = -1.0;
	FString LastFailureReason;
};
