// Fill out your copyright notice in the Description page of Project Settings.

#include "BossEncounter.h"

#include "BossSlimeEnemy.h"
#include "Components/SceneComponent.h"
#include "EnemyBase.h"
#include "EnemySpawner.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GrantMoneyAudio.h"
#include "GrantMoneyAudioSubsystem.h"
#include "TimerManager.h"

ABossEncounter::ABossEncounter()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);

	// A root so the encounter can be placed in a level. Its position does not matter.
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	BossClass = ABossSlimeEnemy::StaticClass();
	ArrivalSound = GrantMoneyAudio::DefaultSound(TEXT("/Game/Sound/Enemy/SlimeBossIntroSound.SlimeBossIntroSound"));
	BossMusic = GrantMoneyAudio::DefaultSound(TEXT("/Game/Sound/Music/SlimeBossTheme.SlimeBossTheme"));
}

void ABossEncounter::BeginPlay()
{
	Super::BeginPlay();

	// The game mode normally binds this; this covers an encounter that exists before any game mode setup.
	if (!RunController.IsValid())
	{
		BindToRunController(ARunController::Find(GetWorld()));
	}
}

void ABossEncounter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelPendingWork();

	if (ARunController* Run = RunController.Get())
	{
		Run->OnBossPhaseStarted.RemoveDynamic(this, &ABossEncounter::HandleBossPhaseStarted);
		Run->OnStateChanged.RemoveDynamic(this, &ABossEncounter::HandleRunStateChanged);
	}

	if (AEnemyBase* LiveBoss = Boss.Get())
	{
		if (UHealthComponent* Health = LiveBoss->GetHealth())
		{
			Health->OnDeath.RemoveDynamic(this, &ABossEncounter::HandleBossDeath);
		}
	}

	Super::EndPlay(EndPlayReason);
}

ABossEncounter* ABossEncounter::Find(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ABossEncounter> It(World); It; ++It)
	{
		return *It;
	}

	return nullptr;
}

void ABossEncounter::BindToRunController(ARunController* InRunController)
{
	if (!IsValid(InRunController))
	{
		return;
	}

	if (ARunController* Previous = RunController.Get())
	{
		if (Previous != InRunController)
		{
			Previous->OnBossPhaseStarted.RemoveDynamic(this, &ABossEncounter::HandleBossPhaseStarted);
			Previous->OnStateChanged.RemoveDynamic(this, &ABossEncounter::HandleRunStateChanged);
		}
	}

	RunController = InRunController;
	InRunController->OnBossPhaseStarted.AddUniqueDynamic(this, &ABossEncounter::HandleBossPhaseStarted);
	InRunController->OnStateChanged.AddUniqueDynamic(this, &ABossEncounter::HandleRunStateChanged);

	if (InRunController->GetState() == EGrantMoneyRunState::BossPhase)
	{
		BeginEncounter();
	}
}

void ABossEncounter::SetSpawner(AEnemySpawner* InSpawner)
{
	SpawnerOverride = InSpawner;
}

AEnemySpawner* ABossEncounter::ResolveSpawner()
{
	if (AEnemySpawner* Override = SpawnerOverride.Get())
	{
		return Override;
	}

	for (TActorIterator<AEnemySpawner> It(GetWorld()); It; ++It)
	{
		return *It;
	}

	return nullptr;
}

void ABossEncounter::HandleBossPhaseStarted()
{
	BeginEncounter();
}

void ABossEncounter::BeginEncounter()
{
	// Exactly one boss per run: the encounter leaves Waiting once and never comes back.
	if (State != EBossEncounterState::Waiting)
	{
		return;
	}

	State = EBossEncounterState::Spawning;
	SpawnAttempts = 0;
	LastFailureReason.Reset();
	UE_LOG(LogTemp, Log, TEXT("BossEncounter: boss phase started; spawning the boss (up to %d attempts)."), MaxSpawnAttempts);
	AttemptSpawn();
}

void ABossEncounter::AttemptSpawn()
{
	UWorld* World = GetWorld();
	if (!World || State != EBossEncounterState::Spawning)
	{
		return;
	}

	// A retry timer or a late call must not act on a run that has moved on.
	if (const ARunController* Run = RunController.Get())
	{
		if (Run->GetState() != EGrantMoneyRunState::BossPhase)
		{
			CancelPendingWork();
			State = EBossEncounterState::Cancelled;
			return;
		}
	}

	++SpawnAttempts;

	FVector CapsuleCentre = FVector::ZeroVector;
	AEnemySpawner* Spawner = ResolveSpawner();
	const AActor* Player = UGameplayStatics::GetPlayerPawn(this, 0);

	if (!BossClass)
	{
		LastFailureReason = TEXT("BossClass is not set");
	}
	else if (!Spawner)
	{
		LastFailureReason = TEXT("the level has no EnemySpawner to validate the location");
	}
	else if (!Spawner->FindSpawnLocationForClass(BossClass, MinSpawnDistance, MaxSpawnDistance, CapsuleCentre))
	{
		LastFailureReason = TEXT("no valid location found (map not built, no player, or no clear ground)");
	}
	else
	{
		const FVector ToPlayer = Player ? Player->GetActorLocation() - CapsuleCentre : FVector::ForwardVector;
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AEnemyBase* NewBoss = World->SpawnActor<AEnemyBase>(BossClass, FTransform(FRotator(0.0, ToPlayer.Rotation().Yaw, 0.0), CapsuleCentre), Params);

		if (NewBoss)
		{
			Boss = NewBoss;
			++BossSpawnCount;
			State = EBossEncounterState::Fighting;
			ArrivalTime = World->GetTimeSeconds();
			LastFailureReason.Reset();

			NewBoss->GetHealth()->OnDeath.AddUniqueDynamic(this, &ABossEncounter::HandleBossDeath);
			NewBoss->BeginEmergence();
			StartBossAudio();

			UE_LOG(LogTemp, Log, TEXT("BossEncounter: boss %s spawned at %s on attempt %d."), *NewBoss->GetName(), *CapsuleCentre.ToCompactString(), SpawnAttempts);
			return;
		}

		LastFailureReason = TEXT("SpawnActor returned null");
	}

	if (SpawnAttempts >= FMath::Max(1, MaxSpawnAttempts))
	{
		State = EBossEncounterState::Failed;
		UE_LOG(LogTemp, Error, TEXT("BossEncounter: the boss could not be spawned after %d attempts: %s. The run stays in BossPhase."), SpawnAttempts, *LastFailureReason);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("BossEncounter: boss spawn attempt %d of %d failed: %s. Retrying in %.1f s."), SpawnAttempts, MaxSpawnAttempts, *LastFailureReason, SpawnRetryIntervalSeconds);
	World->GetTimerManager().SetTimer(RetryTimer, this, &ABossEncounter::AttemptSpawn, FMath::Max(0.05f, SpawnRetryIntervalSeconds), false);
}

void ABossEncounter::HandleBossDeath(UHealthComponent* DepletedHealth, AActor* DamageCauser)
{
	// Ignore a death that arrives after the run ended (the state was set to Cancelled) or twice.
	if (State != EBossEncounterState::Fighting)
	{
		return;
	}

	State = EBossEncounterState::Defeated;
	UE_LOG(LogTemp, Log, TEXT("BossEncounter: the boss was defeated; asking the run to win."));

	// RequestWin refuses if the run is already over or the player is dead (a same-tick death resolves to Lost).
	ARunController* Run = RunController.Get();
	if (!Run || !Run->RequestWin())
	{
		if (Run)
		{
			UE_LOG(LogTemp, Log, TEXT("BossEncounter: the run did not accept the win (state %s)."), *ARunController::GetStateLabel(Run->GetState()));
		}

		State = EBossEncounterState::Cancelled;
	}
}

void ABossEncounter::HandleRunStateChanged(EGrantMoneyRunState OldState, EGrantMoneyRunState NewState)
{
	if (NewState != EGrantMoneyRunState::Won && NewState != EGrantMoneyRunState::Lost)
	{
		return;
	}

	// The run is over: no more retries, and nothing the boss does from now on may reach the run.
	CancelPendingWork();

	if (State == EBossEncounterState::Waiting || State == EBossEncounterState::Spawning || State == EBossEncounterState::Fighting)
	{
		State = EBossEncounterState::Cancelled;
	}
}

void ABossEncounter::CancelPendingWork()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RetryTimer);
		World->GetTimerManager().ClearTimer(BossMusicTimer);
	}
}

void ABossEncounter::StartBossAudio()
{
	// The level music gives way to the arrival sound.
	if (UGrantMoneyAudioSubsystem* Audio = UGrantMoneyAudioSubsystem::Get(this))
	{
		Audio->PlayMusic(TSoftObjectPtr<USoundBase>(), LevelMusicFadeOutSeconds);
	}

	GrantMoneyAudio::Play2D(this, ArrivalSound, ArrivalSoundVolume);

	if (BossMusic.IsNull())
	{
		return;
	}

	// The boss theme starts when the arrival sound ends. Without a (finite) arrival sound it starts at once.
	float DelaySeconds = 0.0f;
	if (const USoundBase* Arrival = GrantMoneyAudio::Resolve(ArrivalSound))
	{
		const float Duration = Arrival->GetDuration();
		DelaySeconds = Duration < INDEFINITELY_LOOPING_DURATION ? FMath::Max(Duration, 0.0f) : 0.0f;
	}

	UWorld* World = GetWorld();
	if (World && DelaySeconds > UE_KINDA_SMALL_NUMBER)
	{
		World->GetTimerManager().SetTimer(BossMusicTimer, this, &ABossEncounter::StartBossMusic, DelaySeconds, false);
	}
	else
	{
		StartBossMusic();
	}
}

void ABossEncounter::StartBossMusic()
{
	if (UGrantMoneyAudioSubsystem* Audio = UGrantMoneyAudioSubsystem::Get(this))
	{
		Audio->PlayMusic(BossMusic, BossMusicFadeInSeconds);
	}
}

AEnemyBase* ABossEncounter::GetBoss() const
{
	return Boss.Get();
}

bool ABossEncounter::IsArrivalMessageVisible() const
{
	const UWorld* World = GetWorld();
	return World && State == EBossEncounterState::Fighting && ArrivalTime >= 0.0 && World->GetTimeSeconds() - ArrivalTime < ArrivalMessageSeconds;
}

FString ABossEncounter::GetSpawnDiagnostic() const
{
	if (State == EBossEncounterState::Failed)
	{
		return FString::Printf(TEXT("BOSS FAILED TO SPAWN after %d attempts: %s"), SpawnAttempts, *LastFailureReason);
	}

	if (State == EBossEncounterState::Spawning && SpawnAttempts > 0 && !LastFailureReason.IsEmpty())
	{
		return FString::Printf(TEXT("Boss spawn failed (attempt %d of %d): %s. Retrying..."), SpawnAttempts, MaxSpawnAttempts, *LastFailureReason);
	}

	return FString();
}
