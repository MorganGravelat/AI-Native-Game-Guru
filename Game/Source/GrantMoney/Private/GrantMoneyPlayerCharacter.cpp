// Fill out your copyright notice in the Description page of Project Settings.


#include "GrantMoneyPlayerCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimationAsset.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "GrantMoneyCharacterVisualData.h"
#include "HAL/IConsoleManager.h"
#include "HealthComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "LevelComponent.h"
#include "MapCameraPlayerController.h"
#include "Math/RotationMatrix.h"
#include "PickupComponent.h"
#include "RunController.h"
#include "StarterWeaponComponent.h"
#include "StatsComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "UpgradeComponent.h"

namespace
{
	// Starting max health. Designers can override it on the Stats component.
	constexpr float DefaultMaxHealth = 100.0f;

#if !UE_BUILD_SHIPPING
	// Dev-only way to end the run as a loss without waiting on real combat: "GrantMoney.DamagePlayer 1000"
	// in the PIE console. Goes through UGameplayStatics::ApplyDamage, the same path contact damage uses.
	// An optional delay (seconds) is useful to line the hit up with a specific moment, e.g. well after the
	// run's Starting state so the loss is not silently refused.
	FAutoConsoleCommandWithWorldAndArgs GDamagePlayerCommand(
		TEXT("GrantMoney.DamagePlayer"),
		TEXT("Dev only. Damages the first player's pawn. Usage: GrantMoney.DamagePlayer [Amount=1000] [DelaySeconds=0]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World)
			{
				return;
			}

			const float Amount = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 1000.0f;
			const float DelaySeconds = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 0.0f;

			auto ApplyPlayerDamage = [World, Amount]()
			{
				AActor* Player = UGameplayStatics::GetPlayerPawn(World, 0);
				if (!Player)
				{
					UE_LOG(LogTemp, Warning, TEXT("GrantMoney.DamagePlayer: no player pawn."));
					return;
				}

				UGameplayStatics::ApplyDamage(Player, Amount, nullptr, nullptr, UDamageType::StaticClass());
				UE_LOG(LogTemp, Log, TEXT("GrantMoney.DamagePlayer: applied %.1f damage to %s."), Amount, *Player->GetName());
			};

			if (DelaySeconds > 0.0f)
			{
				FTimerHandle Handle;
				World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda(ApplyPlayerDamage), DelaySeconds, false);
			}
			else
			{
				ApplyPlayerDamage();
			}
		}));
#endif
}

AGrantMoneyPlayerCharacter::AGrantMoneyPlayerCharacter()
{
	// Ticks only to choose between the idle and jog animations.
	PrimaryActorTick.bCanEverTick = true;

	// Movement is relative to the camera, so the controller's rotation must not turn the character.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Face the direction of travel.
	GetCharacterMovement()->bOrientRotationToMovement = true;

	// Combat. Health reads MaxHealth from Stats, so health buffs are stat modifiers.
	Stats = CreateDefaultSubobject<UStatsComponent>(TEXT("Stats"));
	Stats->SetBaseStat(EGrantMoneyStat::MaxHealth, DefaultMaxHealth);
	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));

	// Max-health stat boosts raise only the player's maximum; they do not heal (SCRUM-53).
	Health->bHealOnMaxHealthGain = false;

	// MoveSpeed starts at whatever MaxWalkSpeed the movement component already defaults to, so this
	// does not hardcode (or need to match) the engine's own default; a Move Speed upgrade (SCRUM-36)
	// then raises it via the usual stat modifier path (see HandleStatChanged).
	Stats->SetBaseStat(EGrantMoneyStat::MoveSpeed, GetCharacterMovement()->MaxWalkSpeed);

	// The player starts every run armed. The weapon starts firing when play begins.
	StarterWeapon = CreateDefaultSubobject<UStarterWeaponComponent>(TEXT("StarterWeapon"));

	// XP starts at zero every run and only grows. Orbs add to it through the stat's base value.
	Stats->SetBaseStat(EGrantMoneyStat::Experience, 0.0f);
	Pickups = CreateDefaultSubobject<UPickupComponent>(TEXT("Pickups"));

	// Turns Experience into a level, and that level into upgrade choices (SCRUM-36).
	LevelComponent = CreateDefaultSubobject<ULevelComponent>(TEXT("LevelComponent"));
	UpgradeComponent = CreateDefaultSubobject<UUpgradeComponent>(TEXT("UpgradeComponent"));

	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionFinder(TEXT("/Game/Input/IA_CameraMove.IA_CameraMove"));

	if (MoveActionFinder.Succeeded())
	{
		MoveAction = MoveActionFinder.Object;
	}

	// Optional data-asset override, resolved at BeginPlay. A soft path does not
	// log an error if the asset has not been created yet.
	VisualData = TSoftObjectPtr<UGrantMoneyCharacterVisualData>(
		FSoftObjectPath(TEXT("/Game/Characters/Player/DA_PlayerVisual.DA_PlayerVisual")));

	// Use the inherited character mesh only for rendering. The capsule remains the
	// sole collision shape, and the offset aligns Manny's feet with its base.
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	CharacterMesh->SetRelativeLocation(FVector(0.0, 0.0, -GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
	CharacterMesh->SetRelativeRotation(FRotator(0.0, -90.0, 0.0));
	CharacterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CharacterMesh->SetGenerateOverlapEvents(false);
	CharacterMesh->SetCanEverAffectNavigation(false);

	// Built-in defaults: Manny mesh with a single-node idle/jog animation.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyFinder(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));

	if (MannyFinder.Succeeded())
	{
		CharacterMesh->SetSkeletalMesh(MannyFinder.Object);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("GrantMoneyPlayerCharacter: Manny mesh is not available at /Game/Characters/Mannequins/Meshes/SKM_Manny_Simple."));
	}

	static ConstructorHelpers::FObjectFinder<UAnimationAsset> IdleFinder(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));

	if (IdleFinder.Succeeded())
	{
		IdleAnimation = IdleFinder.Object;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("GrantMoneyPlayerCharacter: Idle animation is not available at /Game/Characters/Mannequins/Anims/Unarmed/MM_Idle."));
	}

	static ConstructorHelpers::FObjectFinder<UAnimationAsset> JogFinder(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd.MF_Unarmed_Jog_Fwd"));

	if (JogFinder.Succeeded())
	{
		JogAnimation = JogFinder.Object;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("GrantMoneyPlayerCharacter: Jog animation is not available at /Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd."));
	}

	CharacterMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
}

void AGrantMoneyPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	Health->OnDeath.AddUniqueDynamic(this, &AGrantMoneyPlayerCharacter::HandleHealthDepleted);
	LevelComponent->OnLevelUp.AddUniqueDynamic(this, &AGrantMoneyPlayerCharacter::HandleLevelGained);
	UpgradeComponent->OnChoiceReady.AddUniqueDynamic(this, &AGrantMoneyPlayerCharacter::HandleUpgradeChoiceReady);
	UpgradeComponent->OnChoicesResolved.AddUniqueDynamic(this, &AGrantMoneyPlayerCharacter::HandleUpgradeChoicesResolved);
	Stats->OnStatChanged.AddUniqueDynamic(this, &AGrantMoneyPlayerCharacter::HandleStatChanged);

	// In case a Blueprint default or save data set MoveSpeed to something other than what the
	// constructor read from the movement component.
	GetCharacterMovement()->MaxWalkSpeed = Stats->GetStat(EGrantMoneyStat::MoveSpeed);

	ApplyVisualData();

	if (bUseBuiltInLocomotion)
	{
		PlayLocomotionAnimation(IdleAnimation);
	}

	// Only the built-in idle/jog logic needs the tick.
	SetActorTickEnabled(bUseBuiltInLocomotion);
}

float AGrantMoneyPlayerCharacter::GetExperience() const
{
	return Stats ? Stats->GetStat(EGrantMoneyStat::Experience) : 0.0f;
}

void AGrantMoneyPlayerCharacter::HandleHealthDepleted(UHealthComponent* DepletedHealth, AActor* DamageCauser)
{
	if (ARunController* Run = ARunController::Find(GetWorld()))
	{
		// No-op (returns false) if the run is already over or was never started; a state can be lost only
		// once, and GrantMoneyGameMode::HandleRunStateChanged reacts to that single transition.
		Run->RequestLoss();
	}
}

void AGrantMoneyPlayerCharacter::HandleLevelGained(int32 NewLevel, int32 PreviousLevel)
{
	UpgradeComponent->NotifyLevelGained();
}

void AGrantMoneyPlayerCharacter::HandleUpgradeChoiceReady()
{
	if (AMapCameraPlayerController* PC = Cast<AMapCameraPlayerController>(GetController()))
	{
		PC->ShowLevelUpChoice(UpgradeComponent);
	}
}

void AGrantMoneyPlayerCharacter::HandleUpgradeChoicesResolved()
{
	if (AMapCameraPlayerController* PC = Cast<AMapCameraPlayerController>(GetController()))
	{
		PC->HideLevelUpChoice();
	}
}

void AGrantMoneyPlayerCharacter::HandleStatChanged(EGrantMoneyStat Stat, float OldValue, float NewValue)
{
	if (Stat == EGrantMoneyStat::MoveSpeed)
	{
		GetCharacterMovement()->MaxWalkSpeed = NewValue;
	}
}

void AGrantMoneyPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bUseBuiltInLocomotion)
	{
		return;
	}

	const float GroundSpeed = GetVelocity().Size2D();
	PlayLocomotionAnimation(GroundSpeed > JogSpeedThreshold ? JogAnimation : IdleAnimation);
}

void AGrantMoneyPlayerCharacter::PlayLocomotionAnimation(UAnimationAsset* Animation)
{
	// Restart only when the desired animation actually changes.
	if (!Animation || Animation == CurrentAnimation)
	{
		return;
	}

	CurrentAnimation = Animation;
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GetMesh()->PlayAnimation(Animation, true);
}

void AGrantMoneyPlayerCharacter::ApplyVisualData()
{
	// No data asset: keep the built-in Manny defaults.
	if (VisualData.IsNull())
	{
		return;
	}

	const UGrantMoneyCharacterVisualData* Data = VisualData.LoadSynchronous();
	if (!Data)
	{
		UE_LOG(LogTemp, Log, TEXT("GrantMoneyPlayerCharacter: %s not found. Using built-in Manny defaults."), *VisualData.ToString());
		return;
	}

	USkeletalMeshComponent* CharacterMesh = GetMesh();

	if (USkeletalMesh* LoadedMesh = Data->SkeletalMesh.LoadSynchronous())
	{
		CharacterMesh->SetSkeletalMesh(LoadedMesh);
	}

	// Feet at the capsule's base, plus any per-asset adjustment.
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	CharacterMesh->SetRelativeLocation(FVector(0.0, 0.0, -HalfHeight) + Data->MeshOffset);
	CharacterMesh->SetRelativeRotation(Data->MeshRotation);
	CharacterMesh->SetRelativeScale3D(Data->MeshScale);

	// An Animation Blueprint or a preview animation replaces the built-in idle/jog logic.
	if (UClass* LoadedAnimClass = Data->AnimClass.LoadSynchronous())
	{
		bUseBuiltInLocomotion = false;
		CharacterMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		CharacterMesh->SetAnimInstanceClass(LoadedAnimClass);
	}
	else if (UAnimationAsset* LoadedAnim = Data->PreviewAnimation.LoadSynchronous())
	{
		bUseBuiltInLocomotion = false;
		CharacterMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		CharacterMesh->PlayAnimation(LoadedAnim, true);
	}
}

void AGrantMoneyPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	UE_LOG(LogTemp, Log, TEXT("GrantMoneyPlayerCharacter: %s possessed by %s."), *GetName(), *GetNameSafe(NewController));
}

void AGrantMoneyPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogTemp, Error, TEXT("GrantMoneyPlayerCharacter: Input component is not an EnhancedInputComponent."));
		return;
	}

	if (!MoveAction)
	{
		UE_LOG(LogTemp, Error, TEXT("GrantMoneyPlayerCharacter: MoveAction (IA_CameraMove) is not set."));
		return;
	}

	// The mapping context is added by the PlayerController.
	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGrantMoneyPlayerCharacter::HandleMove);
}

void AGrantMoneyPlayerCharacter::HandleMove(const FInputActionValue& Value)
{
	// X is right, Y is forward.
	FVector2D Input = Value.Get<FVector2D>();

	// Cap at 1 so diagonals are not faster than straight movement.
	if (Input.SizeSquared() > 1.0)
	{
		Input.Normalize();
	}

	// Use whatever camera is currently the view, without knowing its class.
	FRotator ViewRotation = GetActorRotation();

	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (PlayerController->PlayerCameraManager)
		{
			ViewRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
		}
	}

	// Yaw only, so a pitched camera does not tilt movement off the ground plane.
	const FRotationMatrix YawMatrix(FRotator(0.0, ViewRotation.Yaw, 0.0));

	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), Input.Y);
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), Input.X);
}
