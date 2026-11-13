// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnemySpawnPlacement.h"
#include "GameFramework/Actor.h"
#include "EnemySpawner.generated.h"

class AEnemyBase;
class ARunController;

// What one SpawnBatch call did. Requested = Spawned + SkippedNoPosition + SkippedByCap.
struct FEnemySpawnBatchResult
{
	int32 Requested = 0;
	int32 Spawned = 0;

	// Enemies skipped because every placement attempt failed (map edge, wall, prop, other enemy).
	int32 SkippedNoPosition = 0;

	// Enemies skipped because the current cap (GetCurrentMaxActiveEnemies) was reached.
	int32 SkippedByCap = 0;
};

/**
 * Spawns enemies around the player, for as long as the run is Active. Reusable: put one in a level (or
 * a Blueprint child) and change the properties, or use several with different enemy classes. If the
 * level has none, the game mode spawns a default one for slimes.
 *
 * Spawning is not a fixed rate: it ramps up as the run goes on, from slow and small to fast and large.
 * Difficulty follows the ARunController's survival time (GetElapsedSeconds), going from 0 at the start of
 * the run to 1 at DifficultyRampSeconds and staying at 1 after that (and through BossPhase, though
 * spawning itself stops there — see IsSpawningAllowed). Since GetElapsedSeconds is itself frozen outside
 * the Active state, difficulty cannot progress while paused or during BossPhase either. At difficulty 0
 * the spawner uses the "AtStart" values below; at 1, the "AtEnd" values; in between it interpolates,
 * eased by DifficultyCurveExponent (or reshaped by DifficultyCurve if one is set) so the default feel
 * already begins gently and saves the busiest pressure for the end (SCRUM-37). Every
 * GetCurrentSpawnInterval() seconds of game time it tries to spawn GetCurrentBatchSize() enemies of
 * EnemyClass:
 *  - Position: a random point in the ring MinSpawnDistance..MaxSpawnDistance around the player (ground
 *    plane). A point is accepted only if the whole capsule footprint is on playable map cells
 *    (FSpawnArena), a downward trace finds the ground within GroundHeightTolerance of the map's tile
 *    height, and the enemy's capsule, with ClearancePadding added, overlaps nothing that blocks pawns.
 *  - Each enemy gets at most MaxPlacementAttempts tries. If none works it is skipped, not forced.
 *  - The cap: enemies that are alive and not yet removed count toward GetCurrentMaxActiveEnemies(),
 *    including ones still emerging. A dead or removed enemy frees its slot immediately, and the next
 *    batch replaces it.
 *  - The enemy's collision capsule is placed on the ground and it starts emerging (AEnemyBase::BeginEmergence).
 *
 * It spawns only while the ARunController in the level is Active, so it stops in BossPhase, and it
 * does nothing while the game is paused. Without a run controller it stays idle unless
 * bRequireActiveRun is turned off (difficulty then stays at the "AtStart" values unless
 * SetElapsedSecondsOverride is used).
 */
UCLASS()
class GRANTMONEY_API AEnemySpawner : public AActor
{
	GENERATED_BODY()

public:
	AEnemySpawner();

	virtual void Tick(float DeltaSeconds) override;

	// The enemy to spawn. Defaults to ASlimeEnemy.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	TSubclassOf<AEnemyBase> EnemyClass;

	// Closest an enemy may spawn to the player, measured on the ground plane.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning", meta = (ClampMin = "0.0", Units = "cm"))
	float MinSpawnDistance = 900.0f;

	// Farthest an enemy may spawn from the player. Values below the minimum are treated as the minimum.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning", meta = (ClampMin = "0.0", Units = "cm"))
	float MaxSpawnDistance = 1500.0f;

	// How spawning ramps up over the run: interval shrinks and batch size and the cap grow from the
	// "AtStart" values to the "AtEnd" values as the run's survival time goes from 0 to
	// DifficultyRampSeconds, then holds at the "AtEnd" values. Several spawners ramp together because
	// they all follow the same ARunController. By default (no DifficultyCurve asset) the ramp eases in
	// via DifficultyCurveExponent rather than rising in a straight line, so the run begins gently, picks
	// up through the middle, and is noticeably busier by the end (SCRUM-37). See GetDifficultyAlpha.

	// Seconds of game time between batches at the start of the run.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty", meta = (ClampMin = "0.05", Units = "s"))
	float SpawnIntervalAtStart = 3.5f;

	// Seconds of game time between batches once the ramp is complete.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty", meta = (ClampMin = "0.05", Units = "s"))
	float SpawnIntervalAtEnd = 0.75f;

	// Enemies tried per batch at the start of the run.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty", meta = (ClampMin = "1"))
	int32 BatchSizeAtStart = 1;

	// Enemies tried per batch once the ramp is complete.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty", meta = (ClampMin = "1"))
	int32 BatchSizeAtEnd = 4;

	// Most enemies from this spawner that may be alive at once (emerging ones included) at the start of the run.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty", meta = (ClampMin = "0"))
	int32 MaxActiveEnemiesAtStart = 6;

	// Most enemies from this spawner that may be alive at once once the ramp is complete.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty", meta = (ClampMin = "0"))
	int32 MaxActiveEnemiesAtEnd = 30;

	// Seconds of survival time to go from the "AtStart" values to the "AtEnd" values.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty", meta = (ClampMin = "0.01", Units = "s"))
	float DifficultyRampSeconds = 150.0f;

	// Optional. Reshapes the ramp: the straight-line fraction of DifficultyRampSeconds elapsed (0..1) is
	// looked up on this curve and the result (also expected 0..1) is used as the difficulty instead, so a
	// designer can make it start slower, end in a rush, or stair-step. Leave empty to use
	// DifficultyCurveExponent instead (a plain float, no curve asset needed).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	TObjectPtr<class UCurveFloat> DifficultyCurve;

	// Used only when DifficultyCurve is not set. Shapes the straight-line ramp fraction (0..1) by raising
	// it to this power before using it as the difficulty: above 1 eases in (slow at first, accelerating
	// toward the end — the default, so a run begins gently and is noticeably busier by the end without
	// needing a curve asset); 1 is a straight line; below 1 front-loads the difficulty instead.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty", meta = (ClampMin = "0.01"))
	float DifficultyCurveExponent = 1.6f;

	// Random positions tried for each enemy before it is skipped.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement", meta = (ClampMin = "1"))
	int32 MaxPlacementAttempts = 12;

	// Gap between the bottom of the capsule and the ground at spawn.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement", meta = (ClampMin = "0.0", Units = "cm"))
	float GroundClearance = 2.0f;

	// Extra radius added to the enemy's capsule for the footprint and overlap checks.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement", meta = (ClampMin = "0.0", Units = "cm"))
	float ClearancePadding = 5.0f;

	// How far above and below the tile height the ground trace reaches.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement", meta = (ClampMin = "1.0", Units = "cm"))
	float GroundTraceHalfHeight = 400.0f;

	// The ground found by the trace must be within this of the map's tile height, which rejects the tops of props and walls.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement", meta = (ClampMin = "0.0", Units = "cm"))
	float GroundHeightTolerance = 30.0f;

	// The trace channel the ground slabs block. The default tile ground blocks everything.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	// When true (the default) enemies spawn only while an ARunController in the level is Active.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	bool bRequireActiveRun = true;

	// Tries to spawn GetCurrentBatchSize() enemies now, ignoring the timer and the run state. Respects the cap.
	FEnemySpawnBatchResult SpawnBatch();

	// Tries once to spawn one enemy (placement retries included). Returns null if the cap is reached,
	// there is no player or map, or no valid position was found.
	AEnemyBase* TrySpawnEnemy();

	// Finds a valid spot for one enemy: the capsule centre to spawn at. False if every attempt failed.
	bool FindSpawnLocation(FVector& OutCapsuleCentre);

	// The same validated search for any enemy class and distance ring around the player (used to place the
	// boss, whose capsule is larger than EnemyClass's). Uses this spawner's arena, clearance and ground
	// settings and its bounded MaxPlacementAttempts. Does not check the run state or the enemy cap.
	bool FindSpawnLocationForClass(TSubclassOf<AEnemyBase> InEnemyClass, float MinDistance, float MaxDistance, FVector& OutCapsuleCentre);

	// Enemies from this spawner that are alive and not being removed. Dead or removed ones are not counted.
	UFUNCTION(BlueprintPure, Category = "Spawning")
	int32 GetActiveEnemyCount() const;

	// 0 at the start of the ramp (survival time 0), 1 once DifficultyRampSeconds of survival time have
	// passed, shaped by DifficultyCurve if one is set, else by DifficultyCurveExponent. Survival time
	// comes from the resolved ARunController, or from SetElapsedSecondsOverride if that was used. Frozen
	// whenever survival time is (paused, or outside the Active state — see ARunController::GetElapsedSeconds).
	UFUNCTION(BlueprintPure, Category = "Difficulty")
	float GetDifficultyAlpha() const;

	// SpawnIntervalAtStart..AtEnd at the current difficulty.
	UFUNCTION(BlueprintPure, Category = "Difficulty")
	float GetCurrentSpawnInterval() const;

	// BatchSizeAtStart..AtEnd at the current difficulty, rounded, never below 1.
	UFUNCTION(BlueprintPure, Category = "Difficulty")
	int32 GetCurrentBatchSize() const;

	// MaxActiveEnemiesAtStart..AtEnd at the current difficulty, rounded, never below 0.
	UFUNCTION(BlueprintPure, Category = "Difficulty")
	int32 GetCurrentMaxActiveEnemies() const;

	// How many random positions have been tried so far, across all enemies. Shows that retries are bounded.
	int32 GetPlacementAttemptCount() const { return PlacementAttemptCount; }

	// True if the run (or bRequireActiveRun) currently lets this spawner spawn. Looks up the run controller if needed.
	bool IsSpawningAllowed() const;

	// Moves the spawn timer forward by game time and spawns a batch when it is due. Tick calls this
	// unless the game is paused; tests and tools can call it directly.
	void AdvanceSpawning(float DeltaSeconds);

	// Uses this run controller instead of looking one up in the level.
	void SetRunController(ARunController* InRunController);

	// Uses this arena instead of the level's TileMapRenderer. For tests and custom setups.
	void SetArenaOverride(const FSpawnArena& InArena);

	// Measures distance from, and faces enemies toward, this actor instead of the first player's pawn.
	void SetCentreActor(AActor* InCentreActor);

	// Makes placement repeatable.
	void SetRandomSeed(int32 Seed);

	// Test/tool hook: uses this many seconds of survival time for the difficulty ramp instead of asking
	// the run controller. A negative value goes back to asking the run controller.
	void SetElapsedSecondsOverride(float Seconds);

protected:
	virtual void BeginPlay() override;

	// The actor enemies spawn around. Defaults to the first player's pawn.
	virtual AActor* FindCentreActor() const;

private:
	// Fills Arena from the override or the level's TileMapRenderer. False (with a logged reason) if the
	// map is not there or not built yet.
	bool ResolveArena();

	// One placement try at a ground-plane position. On success fills the capsule centre.
	bool TryPlaceAt(const FVector2D& GroundPlanePosition, float CapsuleRadius, float CapsuleHalfHeight, const AActor* Centre, FVector& OutCapsuleCentre) const;

	void ResolveRunController() const;

	// Survival seconds used for the difficulty ramp: the override if set, else the run controller's
	// elapsed time, else 0 (no run, no override: difficulty stays at the "AtStart" values).
	double GetEffectiveElapsedSeconds() const;

	// Forgets enemies that are gone or dead.
	void PurgeInactiveEnemies();

	static bool IsActiveEnemy(const AEnemyBase* Enemy);

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AEnemyBase>> SpawnedEnemies;

	UPROPERTY(Transient)
	mutable TWeakObjectPtr<ARunController> RunController;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CentreOverride;

	FSpawnArena Arena;
	bool bArenaOverridden = false;

	FRandomStream RandomStream;
	float SpawnTimer = 0.0f;
	int32 PlacementAttemptCount = 0;
	TOptional<float> ElapsedSecondsOverride;
	mutable double NextRunLookupTime = 0.0;
	mutable bool bLoggedNoRunController = false;
	bool bLoggedMapNotReady = false;
};
