// Fill out your copyright notice in the Description page of Project Settings.


#include "StarterWeaponComponent.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "EnemyBase.h"
#include "GameFramework/Pawn.h"
#include "HealthComponent.h"
#include "MagicOrbProjectile.h"

namespace
{
	// Matches the FireInterval property's minimum, so a bad value cannot create a timer that fires every frame.
	constexpr float MinFireInterval = 0.05f;

	bool IsLivingEnemy(const AEnemyBase* Enemy)
	{
		const UHealthComponent* Health = IsValid(Enemy) ? Enemy->GetHealth() : nullptr;
		return Health && !Health->IsDead() && !Enemy->IsActorBeingDestroyed();
	}
}

UStarterWeaponComponent::UStarterWeaponComponent()
{
	// Driven by a timer, not by a per-frame tick.
	PrimaryComponentTick.bCanEverTick = false;

	ProjectileClass = AMagicOrbProjectile::StaticClass();
}

void UStarterWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	ScheduleNextShot();
}

void UStarterWeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The weapon or its owner is going away: no shot may fire after this.
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FireTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UStarterWeaponComponent::ScheduleNextShot()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(FireTimerHandle, this, &UStarterWeaponComponent::HandleFireTimer, FMath::Max(FireInterval, MinFireInterval), false);
	}
}

void UStarterWeaponComponent::HandleFireTimer()
{
	TryFire();

	if (IsValid(this))
	{
		ScheduleNextShot();
	}
}

AActor* UStarterWeaponComponent::SelectTarget() const
{
	const AActor* Owner = GetOwner();
	const UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return nullptr;
	}

	const FVector OwnerLocation = Owner->GetActorLocation();
	const double RangeSquared = FMath::Square(static_cast<double>(TargetRange));

	AEnemyBase* Nearest = nullptr;
	double NearestSquared = RangeSquared;

	for (TActorIterator<AEnemyBase> It(World); It; ++It)
	{
		AEnemyBase* Enemy = *It;
		if (!IsLivingEnemy(Enemy))
		{
			continue;
		}

		const double DistanceSquared = FVector::DistSquared2D(OwnerLocation, Enemy->GetActorLocation());
		if (DistanceSquared <= NearestSquared)
		{
			Nearest = Enemy;
			NearestSquared = DistanceSquared;
		}
	}

	return Nearest;
}

AMagicOrbProjectile* UStarterWeaponComponent::FireAt(AActor* Target)
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!IsValid(Target) || !Owner || !World || !ProjectileClass)
	{
		return nullptr;
	}

	// Aim at the middle of what the target collides with (its capsule, for a character), not its feet or origin.
	FVector AimPoint;
	FVector AimExtent;
	Target->GetActorBounds(true, AimPoint, AimExtent);

	const FVector Start = Owner->GetActorLocation();
	const FVector Direction = (AimPoint - Start).GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		return nullptr;
	}

	const FTransform SpawnTransform(Direction.Rotation(), Start);
	AMagicOrbProjectile* Orb = World->SpawnActorDeferred<AMagicOrbProjectile>(
		ProjectileClass, SpawnTransform, Owner, Cast<APawn>(Owner), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Orb)
	{
		return nullptr;
	}

	Orb->Launch(Direction, ProjectileSpeed, Damage, ProjectileLifetime);
	Orb->FinishSpawning(SpawnTransform);

	UE_LOG(LogTemp, Log, TEXT("StarterWeapon: %s fired an orb at %s."), *GetNameSafe(Owner), *GetNameSafe(Target));
	return Orb;
}

bool UStarterWeaponComponent::TryFire()
{
	AActor* Target = SelectTarget();
	if (!Target)
	{
		return false;
	}

	return FireAt(Target) != nullptr;
}
