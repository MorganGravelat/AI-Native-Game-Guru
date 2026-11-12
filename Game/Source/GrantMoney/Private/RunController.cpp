// Fill out your copyright notice in the Description page of Project Settings.

#include "RunController.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"

ARunController::ARunController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = false;
	SetCanBeDamaged(false);

	// A root so the controller can be placed in a level.
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ARunController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Normally a paused world does not tick actors at all; this keeps the rule even if something ticks us anyway.
	const UWorld* World = GetWorld();
	if (!World || World->IsPaused())
	{
		return;
	}

	AdvanceRun(DeltaSeconds);
}

void ARunController::StartRun()
{
	if (bStartRequested)
	{
		return;
	}

	bStartRequested = true;
	UE_LOG(LogTemp, Log, TEXT("RunController: run starting (duration %.0f s, start delay %.1f s)."), RunDurationSeconds, StartDelaySeconds);

	// With no delay the run is Active immediately instead of one tick later.
	AdvanceRun(0.0f);
}

void ARunController::AdvanceRun(float DeltaSeconds)
{
	const float Delta = FMath::Max(0.0f, DeltaSeconds);

	switch (State)
	{
	case EGrantMoneyRunState::Starting:
		if (!bStartRequested)
		{
			return;
		}

		StartDelayElapsed += Delta;
		if (StartDelayElapsed >= StartDelaySeconds)
		{
			TryTransition(EGrantMoneyRunState::Active);
			BroadcastTimeIfChanged();
		}
		return;

	case EGrantMoneyRunState::Active:
		ElapsedSeconds = FMath::Min(ElapsedSeconds + Delta, RunDurationSeconds);
		BroadcastTimeIfChanged();

		if (GetRemainingSeconds() <= 0.0f && TryTransition(EGrantMoneyRunState::BossPhase))
		{
			OnBossPhaseStarted.Broadcast();
		}
		return;

	default:
		// BossPhase, Won and Lost: the survival clock no longer moves.
		return;
	}
}

bool ARunController::RequestWin()
{
	return TryTransition(EGrantMoneyRunState::Won);
}

bool ARunController::RequestLoss()
{
	return TryTransition(EGrantMoneyRunState::Lost);
}

float ARunController::GetRemainingSeconds() const
{
	return FMath::Max(0.0f, RunDurationSeconds - ElapsedSeconds);
}

bool ARunController::TryTransition(EGrantMoneyRunState NewState)
{
	bool bAllowed = false;
	switch (State)
	{
	case EGrantMoneyRunState::Starting:
		bAllowed = NewState == EGrantMoneyRunState::Active;
		break;
	case EGrantMoneyRunState::Active:
		bAllowed = NewState == EGrantMoneyRunState::BossPhase || NewState == EGrantMoneyRunState::Lost;
		break;
	case EGrantMoneyRunState::BossPhase:
		bAllowed = NewState == EGrantMoneyRunState::Won || NewState == EGrantMoneyRunState::Lost;
		break;
	default:
		break;
	}

	if (!bAllowed)
	{
		return false;
	}

	const EGrantMoneyRunState OldState = State;
	State = NewState;
	UE_LOG(LogTemp, Log, TEXT("RunController: %s -> %s (elapsed %.1f s)."),
		*GetStateLabel(OldState), *GetStateLabel(NewState), ElapsedSeconds);

	OnStateChanged.Broadcast(OldState, NewState);
	return true;
}

void ARunController::BroadcastTimeIfChanged()
{
	const int32 WholeSeconds = FMath::CeilToInt32(GetRemainingSeconds());
	if (WholeSeconds == LastReportedWholeSeconds)
	{
		return;
	}

	LastReportedWholeSeconds = WholeSeconds;
	OnTimeChanged.Broadcast(ElapsedSeconds, GetRemainingSeconds());
}

FString ARunController::GetStateLabel(EGrantMoneyRunState InState)
{
	switch (InState)
	{
	case EGrantMoneyRunState::Starting:  return TEXT("Starting");
	case EGrantMoneyRunState::Active:    return TEXT("Active");
	case EGrantMoneyRunState::BossPhase: return TEXT("Boss Phase");
	case EGrantMoneyRunState::Won:       return TEXT("Won");
	case EGrantMoneyRunState::Lost:      return TEXT("Lost");
	}

	return TEXT("Unknown");
}

FString ARunController::FormatClock(float Seconds)
{
	const int32 Whole = FMath::Max(0, FMath::CeilToInt32(Seconds));
	return FString::Printf(TEXT("%d:%02d"), Whole / 60, Whole % 60);
}
