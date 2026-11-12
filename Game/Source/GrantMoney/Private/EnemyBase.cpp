// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyBase.h"

#include "CombatEventsSubsystem.h"
#include "ContactDamageComponent.h"
#include "EnemyEmergenceComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "HAL/IConsoleManager.h"
#include "HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "StatsComponent.h"
#include "XpOrbPickup.h"

namespace
{
	// Designers can override these per enemy on the Stats component.
	constexpr float DefaultEnemyMaxHealth = 30.0f;
	constexpr float DefaultEnemyContactDamage = 10.0f;
	constexpr float DefaultEnemyAttackInterval = 1.0f; // seconds between hits

#if !UE_BUILD_SHIPPING
	// Dev-only way to hurt enemies before weapons exist: "GrantMoney.DamageEnemies 10" in the PIE console.
	// Goes through UGameplayStatics::ApplyDamage, the same path weapons will use.
	FAutoConsoleCommandWithWorldAndArgs GDamageEnemiesCommand(
		TEXT("GrantMoney.DamageEnemies"),
		TEXT("Dev only. Damages every enemy in the world. Usage: GrantMoney.DamageEnemies [Amount=10]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World)
			{
				return;
			}

			const float Amount = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 10.0f;

			// Collect first: an enemy killed here may be destroyed while iterating.
			TArray<AEnemyBase*> Enemies;
			for (TActorIterator<AEnemyBase> It(World); It; ++It)
			{
				Enemies.Add(*It);
			}

			for (AEnemyBase* Enemy : Enemies)
			{
				UGameplayStatics::ApplyDamage(Enemy, Amount, nullptr, nullptr, UDamageType::StaticClass());
			}

			UE_LOG(LogTemp, Log, TEXT("GrantMoney.DamageEnemies: applied %.1f damage to %d enemies."), Amount, Enemies.Num());
		}));
#endif
}

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Placed enemies would otherwise get an AAIController automatically and spawned ones would not.
	// Enemies are driven directly, so both are controller-less and behave the same.
	AutoPossessAI = EAutoPossessAI::Disabled;

	// Face the direction of travel; no controller exists to supply a rotation.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;

	// With no controller, character movement would otherwise not run at all.
	Movement->bRunPhysicsWithNoController = true;

	Stats = CreateDefaultSubobject<UStatsComponent>(TEXT("Stats"));
	Stats->SetBaseStat(EGrantMoneyStat::MaxHealth, DefaultEnemyMaxHealth);
	Stats->SetBaseStat(EGrantMoneyStat::ContactDamage, DefaultEnemyContactDamage);
	Stats->SetBaseStat(EGrantMoneyStat::AttackInterval, DefaultEnemyAttackInterval);
	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	ContactDamage = CreateDefaultSubobject<UContactDamageComponent>(TEXT("ContactDamage"));
	Emergence = CreateDefaultSubobject<UEnemyEmergenceComponent>(TEXT("Emergence"));

	XpOrbClass = AXpOrbPickup::StaticClass();
}

void AEnemyBase::BeginEmergence()
{
	if (bIsEmerging || Health->IsDead() || IsActorBeingDestroyed())
	{
		return;
	}

	bIsEmerging = true;
	Emergence->OnFinished.AddUniqueDynamic(this, &AEnemyBase::HandleEmergenceFinished);
	Emergence->Begin(GetEmergenceVisual());
}

void AEnemyBase::HandleEmergenceFinished(UEnemyEmergenceComponent* FinishedEmergence)
{
	bIsEmerging = false;
}

bool AEnemyBase::CanBeTargeted() const
{
	return !bIsEmerging && !IsActorBeingDestroyed() && !Health->IsDead();
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	Health->OnDeath.AddDynamic(this, &AEnemyBase::HandleHealthDepleted);
}

void AEnemyBase::HandleHealthDepleted(UHealthComponent* DepletedHealth, AActor* DamageCauser)
{
	HandleDeath(DamageCauser);
}

void AEnemyBase::HandleDeath(AActor* Killer)
{
	// Record where the enemy died first, before it stops and is removed, so a drop system can use it.
	DeathTransform = GetActorTransform();

	// Stop chasing and hitting, and stop blocking the player or other enemies.
	SetActorTickEnabled(false);
	SetActorEnableCollision(false);
	GetCharacterMovement()->DisableMovement();

	// A death mid-emergence leaves the body in its normal shape, not frozen flat.
	bIsEmerging = false;
	Emergence->Cancel();

	// Drop before the enemy is removed, at the spot it died.
	SpawnPickupDrops();

	// Report before destroying, so listeners can still read the enemy (location, stats, type).
	if (UWorld* World = GetWorld())
	{
		if (UCombatEventsSubsystem* CombatEvents = World->GetSubsystem<UCombatEventsSubsystem>())
		{
			CombatEvents->NotifyEnemyKilled(this, Killer);
		}
	}

	if (DeathCleanupDelay > 0.0f)
	{
		SetLifeSpan(DeathCleanupDelay);
	}
	else
	{
		Destroy();
	}
}

void AEnemyBase::SpawnPickupDrops()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FTransform SpawnTransform(FRotator::ZeroRotator, DeathTransform.GetLocation());

	if (bDropsXpOrb && XpOrbClass)
	{
		World->SpawnActor<AXpOrbPickup>(XpOrbClass, SpawnTransform, SpawnParams);
	}

	// Each entry rolls on its own, so no bonus drop can take the place of the XP orb.
	for (const FEnemyPickupDrop& Drop : BonusPickupDrops)
	{
		if (Drop.ShouldDrop(FMath::FRand()))
		{
			World->SpawnActor<APickupBase>(Drop.PickupClass, SpawnTransform, SpawnParams);
		}
	}
}

void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Rising out of the ground: no chasing and no hitting until the animation is done.
	if (bIsEmerging)
	{
		return;
	}

	// The player may not exist yet, or may have been replaced, so look again whenever the target is gone.
	if (!IsValid(Target) && !AcquireTarget())
	{
		return;
	}

	MoveTowardTarget();

	// The component checks reach and its own hit interval; most ticks it does nothing.
	ContactDamage->TryApplyContactDamage(Target);
}

AActor* AEnemyBase::FindTarget() const
{
	return UGameplayStatics::GetPlayerPawn(this, 0);
}

bool AEnemyBase::AcquireTarget()
{
	Target = FindTarget();

	if (!IsValid(Target))
	{
		Target = nullptr;

		if (!bLoggedMissingTarget)
		{
			UE_LOG(LogTemp, Warning, TEXT("EnemyBase: %s has no target yet; it will keep looking."), *GetName());
			bLoggedMissingTarget = true;
		}

		return false;
	}

	bLoggedMissingTarget = false;
	UE_LOG(LogTemp, Log, TEXT("EnemyBase: %s is chasing %s."), *GetName(), *Target->GetName());
	return true;
}

FVector AEnemyBase::GetFlatOffsetToTarget() const
{
	if (!IsValid(Target))
	{
		return FVector::ZeroVector;
	}

	// Ground plane only: capsules of different heights put the two actor origins at different Z.
	FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.0;
	return ToTarget;
}

FVector AEnemyBase::GetMoveDirection() const
{
	const FVector ToTarget = GetFlatOffsetToTarget();

	if (ToTarget.Size() <= StopDistance)
	{
		return FVector::ZeroVector;
	}

	return ToTarget.GetSafeNormal();
}

void AEnemyBase::MoveTowardTarget()
{
	// Slow in proportion to the distance left, so the enemy settles at StopDistance. Starting and
	// stopping at full speed on the boundary makes it stutter when the target moves away slowly.
	float Speed = MoveSpeed;
	if (SlowdownDistance > 0.0f)
	{
		const float Remaining = static_cast<float>(GetFlatOffsetToTarget().Size()) - StopDistance;
		Speed *= FMath::Clamp(Remaining / SlowdownDistance, 0.0f, 1.0f);
	}

	GetCharacterMovement()->MaxWalkSpeed = Speed;

	const FVector Direction = GetMoveDirection();
	if (!Direction.IsNearlyZero())
	{
		AddMovementInput(Direction, 1.0f);
	}
}
