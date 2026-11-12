// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RunController.generated.h"

UENUM(BlueprintType)
enum class EGrantMoneyRunState : uint8
{
	// Before the run begins: waiting for the map and player to be ready, then the short start delay.
	Starting,
	// Survival time counts and normal enemy spawning is on.
	Active,
	// The survival timer reached zero. Normal spawning stops; the boss fight starts here (later ticket).
	BossPhase,
	// The boss was defeated.
	Won,
	// The player died, or the run was otherwise lost.
	Lost
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGrantMoneyRunStateChanged, EGrantMoneyRunState, OldState, EGrantMoneyRunState, NewState);

// Fires when the whole seconds left on the clock change (and once when the run becomes Active), at most once per second of game time.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGrantMoneyRunTimeChanged, float, ElapsedSeconds, float, RemainingSeconds);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGrantMoneyBossPhaseStarted);

/**
 * The one authoritative owner of a timed survival run: its state and its clock.
 *
 * States: Starting -> Active -> BossPhase -> Won, with Lost reachable from Active and BossPhase.
 * No state is ever re-entered, so a listener hears each transition at most once per run.
 *
 *  - Starting: waits for StartRun (called by the game mode once the map and player are ready), then for
 *    StartDelaySeconds. Neither counts as survival time.
 *  - Active: elapsed survival time grows with game time. Remaining time is RunDurationSeconds minus
 *    elapsed, so changing the duration changes the clock at once. When nothing is left the run enters
 *    BossPhase exactly once; the clock then stays at zero.
 *  - BossPhase: waits for RequestWin or RequestLoss, to be called by the boss integration.
 *
 * The clock moves only when the actor ticks, so pausing the game (UGameplayStatics::SetGamePaused) and time
 * dilation affect it the way they affect everything else in the game.
 *
 * A level can contain one of these (or a Blueprint child) to set its own duration; otherwise the
 * game mode spawns one with the defaults below. HUD, the enemy spawner (its difficulty ramp follows
 * GetElapsedSeconds) and boss code listen to the events or read the getters. There is no score here:
 * the character's level (once implemented) is the run's progress readout.
 */
UCLASS()
class GRANTMONEY_API ARunController : public AActor
{
	GENERATED_BODY()

public:
	ARunController();

	virtual void Tick(float DeltaSeconds) override;

	// Length of the survival phase. Initially 3 minutes.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run", meta = (ClampMin = "1.0", Units = "s"))
	float RunDurationSeconds = 180.0f;

	// Pause between StartRun and the run becoming Active, so the Starting state is visible. 0 = none.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run", meta = (ClampMin = "0.0", Units = "s"))
	float StartDelaySeconds = 1.0f;

	// Tells the run that the map and player are ready. The run becomes Active after StartDelaySeconds.
	// Ignored after the first call.
	UFUNCTION(BlueprintCallable, Category = "Run")
	void StartRun();

	// BossPhase -> Won. Returns false (and changes nothing) in any other state.
	UFUNCTION(BlueprintCallable, Category = "Run")
	bool RequestWin();

	// Active or BossPhase -> Lost. Returns false (and changes nothing) in any other state.
	UFUNCTION(BlueprintCallable, Category = "Run")
	bool RequestLoss();

	// Moves the run forward by game time. Tick calls this unless the game is paused; tests and tools can call it directly.
	void AdvanceRun(float DeltaSeconds);

	UFUNCTION(BlueprintPure, Category = "Run")
	EGrantMoneyRunState GetState() const { return State; }

	// True once StartRun has been called.
	UFUNCTION(BlueprintPure, Category = "Run")
	bool HasStarted() const { return bStartRequested; }

	// Survival time so far. Frozen outside the Active state.
	UFUNCTION(BlueprintPure, Category = "Run")
	float GetElapsedSeconds() const { return ElapsedSeconds; }

	// RunDurationSeconds minus elapsed, never below 0.
	UFUNCTION(BlueprintPure, Category = "Run")
	float GetRemainingSeconds() const;

	// True only in the Active state, when normal spawning should run.
	UFUNCTION(BlueprintPure, Category = "Run")
	bool IsNormalSpawningAllowed() const { return State == EGrantMoneyRunState::Active; }

	// Fires on every state change, with the old and new state.
	UPROPERTY(BlueprintAssignable, Category = "Run")
	FOnGrantMoneyRunStateChanged OnStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Run")
	FOnGrantMoneyRunTimeChanged OnTimeChanged;

	// Fires once, when the survival timer runs out, right after OnStateChanged. For the boss to hook into.
	UPROPERTY(BlueprintAssignable, Category = "Run")
	FOnGrantMoneyBossPhaseStarted OnBossPhaseStarted;

	// "Starting", "Active", "Boss Phase", "Won" or "Lost".
	static FString GetStateLabel(EGrantMoneyRunState InState);

	// Seconds as M:SS, rounding up so a clock reads 0:01 until it really is zero.
	static FString FormatClock(float Seconds);

private:
	// Applies a legal transition and broadcasts it. Returns false if NewState cannot follow the current state.
	bool TryTransition(EGrantMoneyRunState NewState);

	void BroadcastTimeIfChanged();

	EGrantMoneyRunState State = EGrantMoneyRunState::Starting;
	bool bStartRequested = false;
	float StartDelayElapsed = 0.0f;
	float ElapsedSeconds = 0.0f;

	// Whole seconds last reported by OnTimeChanged; -1 until the first report.
	int32 LastReportedWholeSeconds = -1;
};
