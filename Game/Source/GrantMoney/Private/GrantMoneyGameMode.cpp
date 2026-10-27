// Fill out your copyright notice in the Description page of Project Settings.


#include "GrantMoneyGameMode.h"

#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GrantMoneyPlayerCharacter.h"
#include "MapArrays.h"
#include "MapCameraPawn.h"
#include "PlayerSpawnResolver.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "SkyLightingActor.h"
#include "TileMapRenderer.h"

namespace
{
	// Gap between the bottom of the capsule and the tile surface, so the start is not touching the ground slab.
	constexpr float SpawnGroundClearance = 5.0f;
}

AGrantMoneyGameMode::AGrantMoneyGameMode()
{
	PlayerCharacterClass = AGrantMoneyPlayerCharacter::StaticClass();
	CameraPawnClass = AMapCameraPawn::StaticClass();
	SkyLightingClass = ASkyLightingActor::StaticClass();
	DefaultPawnClass = PlayerCharacterClass;
}

void AGrantMoneyGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World || !SkyLightingClass)
	{
		return;
	}

	// A sky lighting actor placed in the level overrides the default one.
	for (TActorIterator<ASkyLightingActor> It(World); It; ++It)
	{
		return;
	}

	// Lights and atmospheres placed by hand would stack with the default sky, so say so.
	if (TActorIterator<ASkyAtmosphere>(World) || TActorIterator<ADirectionalLight>(World) || TActorIterator<ASkyLight>(World))
	{
		UE_LOG(LogTemp, Warning, TEXT("GrantMoneyGameMode: the level already has a sky atmosphere or light; the default sky lighting adds to it."));
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (World->SpawnActor<ASkyLightingActor>(SkyLightingClass, FTransform::Identity, SpawnParams))
	{
		UE_LOG(LogTemp, Log, TEXT("GrantMoneyGameMode: spawned default sky lighting."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GrantMoneyGameMode: default sky lighting could not be spawned."));
	}
}

UClass* AGrantMoneyGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	return PlayerCharacterClass ? PlayerCharacterClass.Get() : Super::GetDefaultPawnClassForController_Implementation(InController);
}

APawn* AGrantMoneyGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	// Keep the start spot's yaw (PlayerStart, or World Settings when the level has none) so the camera faces the same way.
	const double StartYaw = StartSpot ? StartSpot->GetActorRotation().Yaw : 0.0;

	FVector SpawnLocation;
	if (FindSafeSpawnLocation(GetDefaultPawnClassForController(NewPlayer), SpawnLocation))
	{
		return SpawnDefaultPawnAtTransform(NewPlayer, FTransform(FRotator(0.0, StartYaw, 0.0), SpawnLocation));
	}

	if (!StartSpot)
	{
		UE_LOG(LogTemp, Error, TEXT("GrantMoneyGameMode: no safe start and no start spot; %s is not spawned."), *GetNameSafe(NewPlayer));
		return nullptr;
	}

	UE_LOG(LogTemp, Error, TEXT("GrantMoneyGameMode: no safe start on the generated map; falling back to %s."), *GetNameSafe(StartSpot));
	return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
}

bool AGrantMoneyGameMode::FindSafeSpawnLocation(UClass* PawnClass, FVector& OutLocation) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// The level's renderer decides which map is played, so the spawn follows the same map selection.
	ATileMapRenderer* Renderer = nullptr;
	int32 RendererCount = 0;
	for (TActorIterator<ATileMapRenderer> It(World); It; ++It)
	{
		if (!Renderer)
		{
			Renderer = *It;
		}
		++RendererCount;
	}

	if (!Renderer)
	{
		UE_LOG(LogTemp, Error, TEXT("GrantMoneyGameMode: the level has no TileMapRenderer, so there is no generated map to spawn on."));
		return false;
	}

	if (RendererCount > 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("GrantMoneyGameMode: %d TileMapRenderers in the level; spawning on %s."), RendererCount, *GetNameSafe(Renderer));
	}

	FGrantMoneyMapData MapData;
	if (!FMapArrays::GetMap(Renderer->LevelToRender, MapData) || !MapData.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("GrantMoneyGameMode: level %d (from %s) is not a valid registered map."), Renderer->LevelToRender, *GetNameSafe(Renderer));
		return false;
	}

	const TArray<FIntPoint> SafeCells = FPlayerSpawnResolver::FindSafeStartCells(MapData);
	if (SafeCells.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("GrantMoneyGameMode: level %d has no playable cell surrounded by playable cells."), Renderer->LevelToRender);
		return false;
	}

	// Test with the pawn's own capsule; fall back to the default ACharacter capsule size.
	float CapsuleRadius = 34.0f;
	float CapsuleHalfHeight = 88.0f;
	if (const ACharacter* CharacterDefaults = PawnClass ? Cast<ACharacter>(PawnClass->GetDefaultObject()) : nullptr)
	{
		CharacterDefaults->GetCapsuleComponent()->GetScaledCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
	}

	const FCollisionShape Capsule = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);
	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GrantMoneySafeSpawn), false);

	// Map data keeps the start away from walls; the overlap test catches props placed in the level.
	for (int32 Index = 0; Index < SafeCells.Num(); ++Index)
	{
		const FIntPoint& Cell = SafeCells[Index];
		const FVector Location = Renderer->GetCellWorldLocation(Cell) + FVector(0.0, 0.0, CapsuleHalfHeight + SpawnGroundClearance);

		if (World->OverlapBlockingTestByChannel(Location, FQuat::Identity, ECC_Pawn, Capsule, QueryParams))
		{
			continue;
		}

		UE_LOG(LogTemp, Log, TEXT("GrantMoneyGameMode: safe start on level %d at cell (%d, %d), %s (%d blocked candidates skipped)."),
			Renderer->LevelToRender, Cell.X, Cell.Y, *Location.ToString(), Index);
		OutLocation = Location;
		return true;
	}

	UE_LOG(LogTemp, Error, TEXT("GrantMoneyGameMode: all %d safe cells on level %d are blocked."), SafeCells.Num(), Renderer->LevelToRender);
	return false;
}

void AGrantMoneyGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// Possessing the character must not move the view onto the character, which has no camera.
	NewPlayer->bAutoManageActiveCameraTarget = false;

	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	APawn* PlayerPawn = NewPlayer->GetPawn();
	if (!PlayerPawn)
	{
		UE_LOG(LogTemp, Error, TEXT("GrantMoneyGameMode: no pawn was spawned for %s."), *GetNameSafe(NewPlayer));
		return;
	}

	if (!IsValid(CameraPawn) && CameraPawnClass)
	{
		// Start on the character, which may not be at the PlayerStart. Its yaw is the PlayerStart's yaw.
		const FTransform StartTransform = PlayerPawn->GetActorTransform();
		const FTransform SpawnTransform(FRotator(0.0, StartTransform.Rotator().Yaw, 0.0), StartTransform.GetLocation());

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		CameraPawn = GetWorld()->SpawnActor<AMapCameraPawn>(CameraPawnClass, SpawnTransform, SpawnParams);
	}

	if (IsValid(CameraPawn))
	{
		NewPlayer->SetViewTarget(CameraPawn);

		// The Module 1 camera becomes the gameplay camera: same arm, pitch, yaw and zoom, now tracking the player.
		CameraPawn->SetFollowTarget(PlayerPawn);

		UE_LOG(LogTemp, Log, TEXT("GrantMoneyGameMode: camera %s now follows %s."), *GetNameSafe(CameraPawn), *GetNameSafe(PlayerPawn));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GrantMoneyGameMode: camera pawn could not be spawned; falling back to the character's view."));
		NewPlayer->bAutoManageActiveCameraTarget = true;
		NewPlayer->AutoManageActiveCameraTarget(PlayerPawn);
	}
}

