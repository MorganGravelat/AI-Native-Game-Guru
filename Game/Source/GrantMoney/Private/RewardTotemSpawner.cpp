// Fill out your copyright notice in the Description page of Project Settings.

#include "RewardTotemSpawner.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "RewardTotem.h"
#include "TemporaryEffect.h"
#include "TileMapRenderer.h"

namespace
{
	// A surface this steep or steeper is not ground.
	constexpr double MinGroundNormalZ = 0.7;

	// The clearance box starts this far above the ground, so the ground itself never counts as blocking.
	constexpr double ClearanceLift = 5.0;

	// Every default buff lasts this long.
	constexpr float DefaultBuffSeconds = 8.0f;
}

ARewardTotemSpawner::ARewardTotemSpawner()
{
	PrimaryActorTick.bCanEverTick = true;
	SetCanBeDamaged(false);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	TotemClass = ARewardTotem::StaticClass();

	// The default random pool. Each has its own stack key, so two different buffs stack and a repeat refreshes.
	auto AddBuff = [this](const TCHAR* Name, EGrantMoneyStat Stat, EGrantMoneyStatModifierOp Op, float Magnitude, const FText& DisplayName, const FLinearColor& Color)
	{
		UStatModifierEffect* Buff = CreateDefaultSubobject<UStatModifierEffect>(Name);
		Buff->Stat = Stat;
		Buff->ModifierOp = Op;
		Buff->Magnitude = Magnitude;
		Buff->Duration = DefaultBuffSeconds;
		Buff->StackKey = FName(*FString::Printf(TEXT("Totem.%s"), Name));
		Buff->DisplayName = DisplayName;
		Buff->AuraColor = Color;
		RandomEffectPool.Add(Buff);
	};

	AddBuff(TEXT("Swiftness"), EGrantMoneyStat::MoveSpeed, EGrantMoneyStatModifierOp::Multiplicative, 0.5f,
		NSLOCTEXT("GrantMoney", "TotemSwiftness", "Swiftness (+50% move speed)"), FLinearColor(0.1f, 1.0f, 0.3f));
	AddBuff(TEXT("Might"), EGrantMoneyStat::Damage, EGrantMoneyStatModifierOp::Multiplicative, 0.5f,
		NSLOCTEXT("GrantMoney", "TotemMight", "Might (+50% damage)"), FLinearColor(1.0f, 0.15f, 0.1f));
	AddBuff(TEXT("Frenzy"), EGrantMoneyStat::AttackInterval, EGrantMoneyStatModifierOp::Multiplicative, -0.3f,
		NSLOCTEXT("GrantMoney", "TotemFrenzy", "Frenzy (30% shorter attack interval)"), FLinearColor(1.0f, 0.8f, 0.1f));
	AddBuff(TEXT("Piercing"), EGrantMoneyStat::Penetration, EGrantMoneyStatModifierOp::Additive, 2.0f,
		NSLOCTEXT("GrantMoney", "TotemPiercing", "Piercing (+2 penetration)"), FLinearColor(0.2f, 0.5f, 1.0f));
}

void ARewardTotemSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (RandomSeed != 0)
	{
		RandomStream.Initialize(RandomSeed);
	}
	else
	{
		RandomStream.GenerateNewSeed();
	}
}

void ARewardTotemSpawner::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Waits for the renderer to build the map (its ground slabs are what the trace hits), then runs once.
	// By the first tick every actor placed in the level, and the player, has begun play.
	if (!bHasSpawned && ResolveArena())
	{
		SpawnTotems();
	}
}

void ARewardTotemSpawner::SetArenaOverride(const FSpawnArena& InArena)
{
	Arena = InArena;
	bArenaOverridden = true;
}

bool ARewardTotemSpawner::ResolveArena()
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

	return Renderer && Renderer->IsMapReady() && FSpawnArena::FromRenderer(Renderer, Arena);
}

AActor* ARewardTotemSpawner::FindAvoidActor() const
{
	return AvoidActorOverride ? AvoidActorOverride.Get() : UGameplayStatics::GetPlayerPawn(this, 0);
}

void ARewardTotemSpawner::GatherCandidatePositions(const FSpawnArena& Arena, float Radius, TArray<FVector>& OutPositions)
{
	OutPositions.Reset();
	if (!Arena.IsValid())
	{
		return;
	}

	for (int32 X = 0; X < Arena.Map.Width; ++X)
	{
		for (int32 Y = 0; Y < Arena.Map.Height; ++Y)
		{
			if (!Arena.IsPlayableCell(FIntPoint(X, Y)))
			{
				continue;
			}

			const FVector Position = Arena.MapToWorld.TransformPosition(FVector(X * Arena.CellSize, Y * Arena.CellSize, 0.0));
			if (Arena.IsFootprintPlayable(Position, Radius))
			{
				OutPositions.Add(Position);
			}
		}
	}
}

bool ARewardTotemSpawner::IsFarFromAll(const FVector& Candidate, const TArray<FVector>& Others, float MinDistance)
{
	const double MinSquared = FMath::Square(static_cast<double>(FMath::Max(0.0f, MinDistance)));
	for (const FVector& Other : Others)
	{
		if (FVector::DistSquared2D(Candidate, Other) < MinSquared)
		{
			return false;
		}
	}
	return true;
}

bool ARewardTotemSpawner::TryPlaceAt(const FVector& Candidate, FVector& OutGround) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GrantMoneyTotemSpawn), false);

	FHitResult GroundHit;
	const FVector TraceStart = Candidate + FVector(0.0, 0.0, GroundTraceHalfHeight);
	const FVector TraceEnd = Candidate - FVector(0.0, 0.0, GroundTraceHalfHeight);
	if (!World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, GroundTraceChannel, QueryParams)
		|| GroundHit.ImpactNormal.Z < MinGroundNormalZ
		|| FMath::Abs(GroundHit.ImpactPoint.Z - Arena.GetGroundZ()) > GroundHeightTolerance)
	{
		return false;
	}

	// Anything that would stop a pawn (walls, props with collision, the player, other totems) rules it out.
	const FVector BoxCentre(Candidate.X, Candidate.Y, GroundHit.ImpactPoint.Z + ClearanceLift + ClearanceHeight * 0.5);
	const FCollisionShape Box = FCollisionShape::MakeBox(FVector(FootprintRadius, FootprintRadius, ClearanceHeight * 0.5));
	if (World->OverlapBlockingTestByChannel(BoxCentre, FQuat::Identity, ECC_Pawn, Box, QueryParams))
	{
		return false;
	}

	OutGround = FVector(Candidate.X, Candidate.Y, GroundHit.ImpactPoint.Z);
	return true;
}

const UTemporaryEffect* ARewardTotemSpawner::PickRandomEffect()
{
	if (RandomEffectPool.Num() == 0)
	{
		return nullptr;
	}

	if (!bVaryBuffsAcrossTotems)
	{
		return RandomEffectPool[RandomStream.RandRange(0, RandomEffectPool.Num() - 1)];
	}

	if (PoolDeck.Num() == 0)
	{
		for (int32 Index = 0; Index < RandomEffectPool.Num(); ++Index)
		{
			PoolDeck.Add(Index);
		}
		for (int32 Index = PoolDeck.Num() - 1; Index > 0; --Index)
		{
			PoolDeck.Swap(Index, RandomStream.RandRange(0, Index));
		}
	}

	return RandomEffectPool[PoolDeck.Pop()];
}

void ARewardTotemSpawner::ConfigureTotem(ARewardTotem& Totem)
{
	if (TotemMesh)
	{
		Totem.CustomMesh = TotemMesh;
		Totem.CustomMeshScale = TotemMeshScale;
	}

	if (TotemMaterial)
	{
		Totem.CustomMaterial = TotemMaterial;
	}

	if (EffectsOverride.Num() > 0)
	{
		Totem.Effects.Reset();
		for (const UTemporaryEffect* Effect : EffectsOverride)
		{
			if (Effect)
			{
				Totem.Effects.Add(DuplicateObject<UTemporaryEffect>(Effect, &Totem));
			}
		}
		return;
	}

	if (const UTemporaryEffect* Picked = PickRandomEffect())
	{
		Totem.Effects.Reset();
		Totem.Effects.Add(DuplicateObject<UTemporaryEffect>(Picked, &Totem));
		if (bColorTotemByBuff)
		{
			Totem.ReadyColor = Picked->AuraColor;
		}
		UE_LOG(LogTemp, Log, TEXT("RewardTotemSpawner: %s gives %s."), *Totem.GetName(), *Picked->DisplayName.ToString());
	}
}

int32 ARewardTotemSpawner::SpawnTotems()
{
	if (bHasSpawned)
	{
		return 0;
	}

	bHasSpawned = true;
	SetActorTickEnabled(false);

	UWorld* World = GetWorld();
	if (!World || !TotemClass || TotemCount <= 0 || !Arena.IsValid())
	{
		return 0;
	}

	TArray<FVector> Candidates;
	GatherCandidatePositions(Arena, FootprintRadius, Candidates);

	// Fisher-Yates with the spawner's stream, so a fixed seed gives a fixed layout.
	for (int32 Index = Candidates.Num() - 1; Index > 0; --Index)
	{
		Candidates.Swap(Index, RandomStream.RandRange(0, Index));
	}

	TArray<FVector> AvoidPoints;
	if (const AActor* AvoidActor = FindAvoidActor())
	{
		AvoidPoints.Add(AvoidActor->GetActorLocation());
	}

	TArray<FVector> Placed;
	for (const FVector& Candidate : Candidates)
	{
		if (Placed.Num() >= TotemCount)
		{
			break;
		}

		FVector Ground;
		if (!IsFarFromAll(Candidate, AvoidPoints, MinDistanceFromPlayer) || !IsFarFromAll(Candidate, Placed, MinSpacing) || !TryPlaceAt(Candidate, Ground))
		{
			continue;
		}

		// AlwaysSpawn: the checks above already proved the spot is clear.
		const FRotator Yaw(0.0, RandomStream.FRandRange(0.0f, 360.0f), 0.0);
		const FTransform Transform(Yaw, Ground);
		if (ARewardTotem* Totem = World->SpawnActorDeferred<ARewardTotem>(TotemClass, Transform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
		{
			ConfigureTotem(*Totem);
			Totem->FinishSpawning(Transform);
			SpawnedTotems.Add(Totem);
			Placed.Add(Ground);
		}
	}

	if (Placed.Num() < TotemCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("RewardTotemSpawner: placed %d of %d totems; the map has no more clear spots with this spacing."), Placed.Num(), TotemCount);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("RewardTotemSpawner: placed %d totems."), Placed.Num());
	}

	return Placed.Num();
}
