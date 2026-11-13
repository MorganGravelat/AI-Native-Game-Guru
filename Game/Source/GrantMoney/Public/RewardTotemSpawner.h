// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnemySpawnPlacement.h"
#include "GameFramework/Actor.h"
#include "RewardTotemSpawner.generated.h"

class ARewardTotem;
class UMaterialInterface;
class UStaticMesh;
class UTemporaryEffect;

/**
 * Scatters reward totems over the level's generated map once it has been built (SCRUM-39), at random
 * positions that are safe to stand at:
 *  1. Map data: the whole footprint (FootprintRadius) is on playable cells, so never on the map edge.
 *  2. Ground: a trace finds solid, level ground at the tile height.
 *  3. Clearance: nothing that blocks pawns (walls, props, the player, other totems) inside the footprint.
 *  4. Spacing: at least MinDistanceFromPlayer from the player's start and MinSpacing from other totems.
 * Positions are shuffled with a random seed per run (or RandomSeed, for a repeatable layout).
 *
 * Each totem's reward is rolled from RandomEffectPool (by default: swiftness, might, frenzy, piercing),
 * dealt like cards so the totems in one level give different buffs, and the totem is tinted with its
 * buff's colour. EffectsOverride, when set, gives every totem the same fixed rewards instead.
 *
 * AGrantMoneyGameMode spawns one automatically unless the level has its own, so placing one in a level
 * is how a level changes the count, spacing or totem type.
 */
UCLASS()
class GRANTMONEY_API ARewardTotemSpawner : public AActor
{
	GENERATED_BODY()

public:
	ARewardTotemSpawner();

	virtual void Tick(float DeltaSeconds) override;

	// Spawns up to TotemCount totems now. Returns how many were placed (fewer if the map has no room).
	// Runs only once per spawner; later calls return 0.
	int32 SpawnTotems();

	// Ground positions (tile height) of every cell whose footprint of Radius is on playable cells.
	static void GatherCandidatePositions(const FSpawnArena& Arena, float Radius, TArray<FVector>& OutPositions);

	// True if Candidate is at least MinDistance (ground distance) from every point in Others.
	static bool IsFarFromAll(const FVector& Candidate, const TArray<FVector>& Others, float MinDistance);

	const TArray<TObjectPtr<ARewardTotem>>& GetSpawnedTotems() const { return SpawnedTotems; }
	bool HasSpawned() const { return bHasSpawned; }

	// Test hooks: a fixed map, a fixed actor to keep away from (instead of the player pawn).
	void SetArenaOverride(const FSpawnArena& InArena);
	void SetAvoidActorOverride(AActor* InActor) { AvoidActorOverride = InActor; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totems")
	TSubclassOf<ARewardTotem> TotemClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totems", meta = (ClampMin = "0"))
	int32 TotemCount = 3;

	// Set on a spawner placed in the level to change every totem it spawns, without code or Blueprints.
	// Empty/null values keep the totem class's own defaults.

	// Mesh for every totem (see ARewardTotem::CustomMesh).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totems|Look")
	TObjectPtr<UStaticMesh> TotemMesh;

	// Material for every totem (see ARewardTotem::CustomMaterial).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totems|Look")
	TObjectPtr<UMaterialInterface> TotemMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totems|Look")
	FVector TotemMeshScale = FVector::OneVector;

	// Fixed rewards for every totem, each with its own duration and strength. When set, the random pool
	// below is not used. Each totem gets its own copy.
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Totems|Reward")
	TArray<TObjectPtr<UTemporaryEffect>> EffectsOverride;

	// Each totem gets one reward picked at random from here. List an entry twice to make it more common.
	// Empty (and no EffectsOverride) = the totem class's own default reward.
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Totems|Reward")
	TArray<TObjectPtr<UTemporaryEffect>> RandomEffectPool;

	// On: picks are dealt from a shuffled deck, so no buff repeats in a level until every one has been used.
	// Off: every pick is independent (repeats are possible).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totems|Reward")
	bool bVaryBuffsAcrossTotems = true;

	// Tints each totem with its random buff's aura colour, so players can tell the buffs apart.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totems|Reward")
	bool bColorTotemByBuff = true;

	// The next pool entry for a totem, or null if the pool is empty. Uses the spawner's random stream.
	const UTemporaryEffect* PickRandomEffect();

	// Clear ground needed around each totem (cm). Covers the pillar plus room to walk around it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement", meta = (ClampMin = "1.0", Units = "cm"))
	float FootprintRadius = 120.0f;

	// Height of the clearance check above the ground (cm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement", meta = (ClampMin = "1.0", Units = "cm"))
	float ClearanceHeight = 200.0f;

	// Keeps totems from appearing on top of the player's start.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement", meta = (ClampMin = "0.0", Units = "cm"))
	float MinDistanceFromPlayer = 800.0f;

	// Keeps totems spread over the map.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement", meta = (ClampMin = "0.0", Units = "cm"))
	float MinSpacing = 1200.0f;

	// 0 = a different layout every run. Any other value repeats the same layout on the same map.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	int32 RandomSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement", meta = (ClampMin = "1.0", Units = "cm"))
	float GroundTraceHalfHeight = 400.0f;

	// How far (cm) the found ground may be from the tile height and still count as the map's floor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement", meta = (ClampMin = "0.0", Units = "cm"))
	float GroundHeightTolerance = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

protected:
	virtual void BeginPlay() override;

private:
	bool ResolveArena();

	// Ground position under Candidate if it passes the ground and clearance checks.
	bool TryPlaceAt(const FVector& Candidate, FVector& OutGround) const;

	AActor* FindAvoidActor() const;

	// Copies the Look and Reward settings above onto a totem before it begins play.
	void ConfigureTotem(ARewardTotem& Totem);

	// Shuffled pool indices still to deal (bVaryBuffsAcrossTotems).
	TArray<int32> PoolDeck;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ARewardTotem>> SpawnedTotems;

	UPROPERTY(Transient)
	TObjectPtr<AActor> AvoidActorOverride;

	FSpawnArena Arena;
	bool bArenaOverridden = false;
	bool bHasSpawned = false;
	FRandomStream RandomStream;
};
