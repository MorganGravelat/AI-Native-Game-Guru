// Fill out your copyright notice in the Description page of Project Settings.


#include "GrantMoneyGameMode.h"

#include "TimerManager.h"
#include "Components/CapsuleComponent.h"
#include "EnemySpawner.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GrantMoneyPlayerCharacter.h"
#include "HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MapArrays.h"
#include "MapCameraPawn.h"
#include "MapCameraPlayerController.h"
#include "PlayerSpawnResolver.h"
#include "RewardTotemSpawner.h"
#include "RunHUD.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "SkyLightingActor.h"
#include "TileMapRenderer.h"

namespace
{
	// Gap between the bottom of the capsule and the tile surface, so the start is not touching the ground slab.
	constexpr float SpawnGroundClearance = 5.0f;

	// How often, and for how long, to wait for the map to be built before giving up on starting the run.
	constexpr float RunReadyPollSeconds = 0.1f;
	constexpr int32 RunReadyMaxChecks = 100;
}

AGrantMoneyGameMode::AGrantMoneyGameMode()
{
	PlayerCharacterClass = AGrantMoneyPlayerCharacter::StaticClass();
	CameraPawnClass = AMapCameraPawn::StaticClass();
	SkyLightingClass = ASkyLightingActor::StaticClass();
	RunControllerClass = ARunController::StaticClass();
	EnemySpawnerClass = AEnemySpawner::StaticClass();
	RewardTotemSpawnerClass = ARewardTotemSpawner::StaticClass();
	DefaultPawnClass = PlayerCharacterClass;
	HUDClass = ARunHUD::StaticClass();
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

	// A Blueprint game mode can still save an older HUD class; the run readout must show regardless.
	if (!NewPlayer->GetHUD() || !NewPlayer->GetHUD()->IsA<ARunHUD>())
	{
		NewPlayer->ClientSetHUD(ARunHUD::StaticClass());
	}

	PrepareRun();
}

void AGrantMoneyGameMode::PrepareRun()
{
	UWorld* World = GetWorld();
	if (!World || IsValid(RunController))
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// A controller placed in the level wins, so a level can set its own duration.
	for (TActorIterator<ARunController> It(World); It; ++It)
	{
		RunController = *It;
		break;
	}

	if (!IsValid(RunController) && RunControllerClass)
	{
		RunController = World->SpawnActor<ARunController>(RunControllerClass, FTransform::Identity, SpawnParams);
	}

	if (!IsValid(RunController))
	{
		UE_LOG(LogTemp, Error, TEXT("GrantMoneyGameMode: no run controller could be created, so the run will not start."));
		return;
	}

	RunController->OnStateChanged.AddUniqueDynamic(this, &AGrantMoneyGameMode::HandleRunStateChanged);

	// The same for the spawner. The spawner finds the run controller by itself.
	bool bHasSpawner = false;
	for (TActorIterator<AEnemySpawner> It(World); It; ++It)
	{
		bHasSpawner = true;
		break;
	}

	if (!bHasSpawner && EnemySpawnerClass)
	{
		World->SpawnActor<AEnemySpawner>(EnemySpawnerClass, FTransform::Identity, SpawnParams);
	}

	// And the reward totems: the spawner waits for the map to be built, then places them once.
	bool bHasTotemSpawner = false;
	for (TActorIterator<ARewardTotemSpawner> It(World); It; ++It)
	{
		bHasTotemSpawner = true;
		break;
	}

	if (!bHasTotemSpawner && RewardTotemSpawnerClass)
	{
		World->SpawnActor<ARewardTotemSpawner>(RewardTotemSpawnerClass, FTransform::Identity, SpawnParams);
	}

	// Wait for the map to be built (the TileMapRenderer builds it at BeginPlay) before the clock starts.
	RunReadyChecks = 0;
	World->GetTimerManager().SetTimer(RunReadyTimer, this, &AGrantMoneyGameMode::CheckRunReady, RunReadyPollSeconds, true);
	CheckRunReady();
}

bool AGrantMoneyGameMode::IsRunReady(FString& OutReason) const
{
	const UWorld* World = GetWorld();
	const ATileMapRenderer* Renderer = nullptr;
	for (TActorIterator<ATileMapRenderer> It(World); It; ++It)
	{
		Renderer = *It;
		break;
	}

	if (!Renderer)
	{
		OutReason = TEXT("the level has no TileMapRenderer");
		return false;
	}

	if (!Renderer->IsMapReady())
	{
		OutReason = TEXT("the map has not been built");
		return false;
	}

	if (!IsValid(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		OutReason = TEXT("the player has no pawn");
		return false;
	}

	return true;
}

void AGrantMoneyGameMode::CheckRunReady()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!IsValid(RunController) || RunController->HasStarted())
	{
		World->GetTimerManager().ClearTimer(RunReadyTimer);
		return;
	}

	FString Reason;
	if (IsRunReady(Reason))
	{
		World->GetTimerManager().ClearTimer(RunReadyTimer);

		// Lets RequestWin refuse victory once this player is dead, so a simultaneous boss kill and
		// player death always resolves to Lost (see ARunController's class comment).
		if (AActor* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			RunController->SetPlayerHealthComponent(PlayerPawn->FindComponentByClass<UHealthComponent>());
		}

		RunController->StartRun();
		return;
	}

	if (++RunReadyChecks >= RunReadyMaxChecks)
	{
		World->GetTimerManager().ClearTimer(RunReadyTimer);
		UE_LOG(LogTemp, Error, TEXT("GrantMoneyGameMode: the run was not started: %s after %.0f s."), *Reason, RunReadyChecks * RunReadyPollSeconds);
	}
}

void AGrantMoneyGameMode::HandleRunStateChanged(EGrantMoneyRunState OldState, EGrantMoneyRunState NewState)
{
	if (NewState != EGrantMoneyRunState::Lost)
	{
		// Won has no screen yet: there is no boss to reach it, so there is nothing to verify today.
		return;
	}

	AMapCameraPlayerController* PC = Cast<AMapCameraPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("GrantMoneyGameMode: no MapCameraPlayerController to show the Game Over screen on."));
		return;
	}

	const AGrantMoneyPlayerCharacter* PlayerCharacter = Cast<AGrantMoneyPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	const float Experience = PlayerCharacter ? PlayerCharacter->GetExperience() : 0.0f;

	UE_LOG(LogTemp, Log, TEXT("GrantMoneyGameMode: run lost at %.1f s survived, %.0f XP; showing the Game Over screen."),
		RunController->GetElapsedSeconds(), Experience);

	PC->ShowGameOver(RunController->GetElapsedSeconds(), Experience);
}

