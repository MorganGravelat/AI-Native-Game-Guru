// Fill out your copyright notice in the Description page of Project Settings.

#include "EnemySpawner.h"

#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Curves/CurveFloat.h"
#include "EnemyBase.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "RunController.h"
#include "SlimeEnemy.h"
#include "TileMapRenderer.h"

namespace
{
	// A surface this steep or steeper is not ground.
	constexpr double MinGroundNormalZ = 0.7;

	// How often to look for a run controller while none is known.
	constexpr double RunLookupIntervalSeconds = 0.5;
}

AEnemySpawner::AEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = false;
	SetCanBeDamaged(false);

	// A root so the spawner can be placed in a level. Its position does not matter; it spawns around the player.
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	EnemyClass = ASlimeEnemy::StaticClass();
}

void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	RandomStream.GenerateNewSeed();

	if (!EnemyClass)
	{
		UE_LOG(LogTemp, Error, TEXT("EnemySpawner: %s has no EnemyClass, so it will not spawn anything."), *GetName());
	}
}

void AEnemySpawner::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Normally a paused world does not tick actors at all; this keeps the rule even if something ticks us anyway.
	const UWorld* World = GetWorld();
	if (!World || World->IsPaused())
	{
		return;
	}

	AdvanceSpawning(DeltaSeconds);
}

void AEnemySpawner::AdvanceSpawning(float DeltaSeconds)
{
	if (!IsSpawningAllowed())
	{
		// A fresh interval starts when spawning is allowed again (for example when the run becomes Active).
		SpawnTimer = 0.0f;
		return;
	}

	SpawnTimer += FMath::Max(0.0f, DeltaSeconds);
	const float Interval = GetCurrentSpawnInterval();
	if (SpawnTimer < Interval)
	{
		return;
	}

	// One batch per due interval; a long frame does not cause a burst of catch-up batches.
	SpawnTimer = FMath::Min(SpawnTimer - Interval, Interval);
	SpawnBatch();
}

bool AEnemySpawner::IsSpawningAllowed() const
{
	if (!bRequireActiveRun)
	{
		return true;
	}

	ResolveRunController();
	return RunController.IsValid() && RunController->IsNormalSpawningAllowed();
}

void AEnemySpawner::ResolveRunController() const
{
	if (RunController.IsValid())
	{
		return;
	}

	const double Now = FPlatformTime::Seconds();
	const UWorld* World = GetWorld();
	if (!World || Now < NextRunLookupTime)
	{
		return;
	}

	NextRunLookupTime = Now + RunLookupIntervalSeconds;

	for (TActorIterator<ARunController> It(World); It; ++It)
	{
		RunController = *It;
		return;
	}

	if (!bLoggedNoRunController)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawner: %s found no RunController; it waits for one."), *GetName());
		bLoggedNoRunController = true;
	}
}

void AEnemySpawner::SetRunController(ARunController* InRunController)
{
	RunController = InRunController;
}

void AEnemySpawner::SetElapsedSecondsOverride(float Seconds)
{
	if (Seconds < 0.0f)
	{
		ElapsedSecondsOverride.Reset();
	}
	else
	{
		ElapsedSecondsOverride = Seconds;
	}
}

double AEnemySpawner::GetEffectiveElapsedSeconds() const
{
	if (ElapsedSecondsOverride.IsSet())
	{
		return ElapsedSecondsOverride.GetValue();
	}

	ResolveRunController();
	return RunController.IsValid() ? RunController->GetElapsedSeconds() : 0.0;
}

float AEnemySpawner::GetDifficultyAlpha() const
{
	const float RampSeconds = FMath::Max(0.01f, DifficultyRampSeconds);
	const float Linear = FMath::Clamp(static_cast<float>(GetEffectiveElapsedSeconds()) / RampSeconds, 0.0f, 1.0f);
	return DifficultyCurve ? FMath::Clamp(DifficultyCurve->GetFloatValue(Linear), 0.0f, 1.0f) : Linear;
}

float AEnemySpawner::GetCurrentSpawnInterval() const
{
	return FMath::Max(0.05f, FMath::Lerp(SpawnIntervalAtStart, SpawnIntervalAtEnd, GetDifficultyAlpha()));
}

int32 AEnemySpawner::GetCurrentBatchSize() const
{
	return FMath::Max(1, FMath::RoundToInt32(FMath::Lerp(static_cast<float>(BatchSizeAtStart), static_cast<float>(BatchSizeAtEnd), GetDifficultyAlpha())));
}

int32 AEnemySpawner::GetCurrentMaxActiveEnemies() const
{
	return FMath::Max(0, FMath::RoundToInt32(FMath::Lerp(static_cast<float>(MaxActiveEnemiesAtStart), static_cast<float>(MaxActiveEnemiesAtEnd), GetDifficultyAlpha())));
}

void AEnemySpawner::SetArenaOverride(const FSpawnArena& InArena)
{
	Arena = InArena;
	bArenaOverridden = true;
}

void AEnemySpawner::SetCentreActor(AActor* InCentreActor)
{
	CentreOverride = InCentreActor;
}

void AEnemySpawner::SetRandomSeed(int32 Seed)
{
	RandomStream.Initialize(Seed);
}

AActor* AEnemySpawner::FindCentreActor() const
{
	if (AActor* Override = CentreOverride.Get())
	{
		return Override;
	}

	return UGameplayStatics::GetPlayerPawn(this, 0);
}

bool AEnemySpawner::IsActiveEnemy(const AEnemyBase* Enemy)
{
	const UHealthComponent* Health = IsValid(Enemy) ? Enemy->GetHealth() : nullptr;
	return Health && !Health->IsDead() && !Enemy->IsActorBeingDestroyed();
}

int32 AEnemySpawner::GetActiveEnemyCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AEnemyBase>& Enemy : SpawnedEnemies)
	{
		Count += IsActiveEnemy(Enemy.Get()) ? 1 : 0;
	}

	return Count;
}

void AEnemySpawner::PurgeInactiveEnemies()
{
	SpawnedEnemies.RemoveAllSwap([](const TWeakObjectPtr<AEnemyBase>& Enemy)
	{
		return !IsActiveEnemy(Enemy.Get());
	});
}

bool AEnemySpawner::ResolveArena()
{
	if (bArenaOverridden)
	{
		return Arena.IsValid();
	}

	const ATileMapRenderer* Renderer = nullptr;
	for (TActorIterator<ATileMapRenderer> It(GetWorld()); It; ++It)
	{
		Renderer = *It;
		break;
	}

	// The ground slabs the trace needs only exist once the renderer has built the map.
	if (!Renderer || !Renderer->IsMapReady() || !FSpawnArena::FromRenderer(Renderer, Arena))
	{
		if (!bLoggedMapNotReady)
		{
			UE_LOG(LogTemp, Warning, TEXT("EnemySpawner: %s has no built map to spawn on yet."), *GetName());
			bLoggedMapNotReady = true;
		}

		return false;
	}

	bLoggedMapNotReady = false;
	return true;
}

bool AEnemySpawner::TryPlaceAt(const FVector2D& GroundPlanePosition, float CapsuleRadius, float CapsuleHalfHeight, const AActor* Centre, FVector& OutCapsuleCentre) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector Probe(GroundPlanePosition.X, GroundPlanePosition.Y, Arena.GetGroundZ());
	const float CheckRadius = CapsuleRadius + ClearancePadding;

	// 1. Map data: the whole footprint must be on playable cells, which keeps spawns off the map edge.
	if (!Arena.IsFootprintPlayable(Probe, CheckRadius))
	{
		return false;
	}

	// 2. Ground: something solid at about the tile height, and level enough to stand on.
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GrantMoneyEnemySpawn), false);
	QueryParams.AddIgnoredActor(Centre);

	FHitResult GroundHit;
	const FVector TraceStart = Probe + FVector(0.0, 0.0, GroundTraceHalfHeight);
	const FVector TraceEnd = Probe - FVector(0.0, 0.0, GroundTraceHalfHeight);
	if (!World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, GroundTraceChannel, QueryParams)
		|| GroundHit.ImpactNormal.Z < MinGroundNormalZ
		|| FMath::Abs(GroundHit.ImpactPoint.Z - Arena.GetGroundZ()) > GroundHeightTolerance)
	{
		return false;
	}

	// 3. Clearance: a capsule a little wider than the enemy's must not overlap walls, props or other enemies.
	// A capsule is never shorter than its radius, so the padded one gets a matching half-height and is
	// raised to keep its bottom exactly where the enemy's bottom will be.
	const float CheckHalfHeight = FMath::Max(CapsuleHalfHeight, CheckRadius);
	const FVector CheckCentre(Probe.X, Probe.Y, GroundHit.ImpactPoint.Z + CheckHalfHeight + GroundClearance);
	if (World->OverlapBlockingTestByChannel(CheckCentre, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(CheckRadius, CheckHalfHeight), QueryParams))
	{
		return false;
	}

	OutCapsuleCentre = FVector(Probe.X, Probe.Y, GroundHit.ImpactPoint.Z + CapsuleHalfHeight + GroundClearance);
	return true;
}

bool AEnemySpawner::FindSpawnLocation(FVector& OutCapsuleCentre)
{
	if (!EnemyClass || !ResolveArena())
	{
		return false;
	}

	const AActor* Centre = FindCentreActor();
	if (!Centre)
	{
		return false;
	}

	float CapsuleRadius = 0.0f;
	float CapsuleHalfHeight = 0.0f;
	EnemyClass->GetDefaultObject<AEnemyBase>()->GetCapsuleComponent()->GetScaledCapsuleSize(CapsuleRadius, CapsuleHalfHeight);

	const FVector CentreLocation = Centre->GetActorLocation();
	const int32 Attempts = FMath::Max(1, MaxPlacementAttempts);

	for (int32 Attempt = 0; Attempt < Attempts; ++Attempt)
	{
		++PlacementAttemptCount;
		const FVector2D Offset = FEnemySpawnPlacement::SampleRingOffset(MinSpawnDistance, MaxSpawnDistance, RandomStream.FRand(), RandomStream.FRand());
		const FVector2D Position(CentreLocation.X + Offset.X, CentreLocation.Y + Offset.Y);

		if (TryPlaceAt(Position, CapsuleRadius, CapsuleHalfHeight, Centre, OutCapsuleCentre))
		{
			return true;
		}
	}

	return false;
}

AEnemyBase* AEnemySpawner::TrySpawnEnemy()
{
	UWorld* World = GetWorld();
	if (!World || !EnemyClass)
	{
		return nullptr;
	}

	PurgeInactiveEnemies();
	if (SpawnedEnemies.Num() >= GetCurrentMaxActiveEnemies())
	{
		return nullptr;
	}

	FVector CapsuleCentre;
	if (!FindSpawnLocation(CapsuleCentre))
	{
		return nullptr;
	}

	// Face the player as it rises.
	const AActor* Centre = FindCentreActor();
	const FVector ToCentre = Centre ? Centre->GetActorLocation() - CapsuleCentre : FVector::ForwardVector;
	const FRotator Facing(0.0, ToCentre.Rotation().Yaw, 0.0);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AEnemyBase* Enemy = World->SpawnActor<AEnemyBase>(EnemyClass, FTransform(Facing, CapsuleCentre), SpawnParams);
	if (!Enemy)
	{
		return nullptr;
	}

	SpawnedEnemies.Add(Enemy);
	Enemy->BeginEmergence();
	return Enemy;
}

FEnemySpawnBatchResult AEnemySpawner::SpawnBatch()
{
	FEnemySpawnBatchResult Result;
	Result.Requested = GetCurrentBatchSize();

	PurgeInactiveEnemies();

	const int32 Cap = GetCurrentMaxActiveEnemies();
	for (int32 Index = 0; Index < Result.Requested; ++Index)
	{
		if (SpawnedEnemies.Num() >= Cap)
		{
			Result.SkippedByCap = Result.Requested - Index;
			break;
		}

		if (TrySpawnEnemy())
		{
			++Result.Spawned;
		}
		else
		{
			++Result.SkippedNoPosition;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("EnemySpawner: %s spawned %d of %d (no valid position: %d, cap: %d). Active %d / %d (difficulty %.0f%%)."),
		*GetName(), Result.Spawned, Result.Requested, Result.SkippedNoPosition, Result.SkippedByCap, GetActiveEnemyCount(), Cap, GetDifficultyAlpha() * 100.0f);

	return Result;
}
