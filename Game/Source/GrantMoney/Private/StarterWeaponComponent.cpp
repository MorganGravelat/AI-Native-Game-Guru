// Fill out your copyright notice in the Description page of Project Settings.


#include "StarterWeaponComponent.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "EnemyBase.h"
#include "GameFramework/Pawn.h"
#include "GrantMoneyAudio.h"
#include "HealthComponent.h"
#include "MagicOrbProjectile.h"
#include "StatsComponent.h"

namespace
{
	// Matches the FireInterval property's minimum, so a bad value cannot create a timer that fires every frame.
	constexpr float MinFireInterval = 0.05f;

	// A weapon value after the owner's stats: the owner's own base value for Stat if it has one,
	// otherwise WeaponValue with the owner's modifiers for Stat (e.g. stat boosts) applied on top.
	// Without a stats component the weapon's value is used unchanged.
	float ApplyOwnerStats(const AActor* Owner, EGrantMoneyStat Stat, float WeaponValue)
	{
		const UStatsComponent* Stats = Owner ? Owner->FindComponentByClass<UStatsComponent>() : nullptr;
		if (!Stats)
		{
			return WeaponValue;
		}

		return Stats->HasStat(Stat) ? Stats->GetStat(Stat) : Stats->GetStatFromBase(Stat, WeaponValue);
	}

	bool IsLivingEnemy(const AEnemyBase* Enemy)
	{
		// CanBeTargeted also rules out enemies that are still rising out of the ground.
		return IsValid(Enemy) && Enemy->CanBeTargeted() && Enemy->GetHealth() != nullptr;
	}
}

UStarterWeaponComponent::UStarterWeaponComponent()
{
	// Driven by a timer, not by a per-frame tick.
	PrimaryComponentTick.bCanEverTick = false;

	ProjectileClass = AMagicOrbProjectile::StaticClass();
	FireSound = GrantMoneyAudio::DefaultSound(TEXT("/Game/Sound/Weapon/FireBolt.FireBolt"));
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
		World->GetTimerManager().ClearTimer(StaggerTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UStarterWeaponComponent::ScheduleNextShot()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(FireTimerHandle, this, &UStarterWeaponComponent::HandleFireTimer, GetEffectiveFireInterval(), false);
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

	Orb->Launch(Direction, ProjectileSpeed, GetEffectiveDamage(), ProjectileLifetime, GetEffectivePenetration(), GetEffectiveExplosionRadius());
	Orb->FinishSpawning(SpawnTransform);

	GrantMoneyAudio::Play2D(this, FireSound);
	UE_LOG(LogTemp, Log, TEXT("StarterWeapon: %s fired an orb at %s."), *GetNameSafe(Owner), *GetNameSafe(Target));
	return Orb;
}

bool UStarterWeaponComponent::TryFire()
{
	// Always exactly one target (unchanged rule); a Projectile Count upgrade affects how many orbs
	// fire at it and how they are paced, never how many different enemies are hit per cycle.
	AActor* Target = SelectTarget();
	if (!Target)
	{
		return false;
	}

	const bool bFiredFirst = FireAt(Target) != nullptr;
	if (bFiredFirst)
	{
		const int32 ShotCount = FMath::Max(1, GetEffectiveProjectileCount());
		FireStaggeredVolley(Target, ShotCount - 1);
	}

	return bFiredFirst;
}

void UStarterWeaponComponent::FireStaggeredVolley(AActor* Target, int32 ExtraShots)
{
	if (ExtraShots <= 0 || !IsValid(Target))
	{
		return;
	}

	if (ProjectileStaggerSeconds <= 0.0f)
	{
		// No stagger: the rest of the burst fires in this same frame.
		for (int32 Index = 0; Index < ExtraShots; ++Index)
		{
			FireAt(Target);
		}
		return;
	}

	PendingStaggerTarget = Target;
	PendingStaggeredShots = ExtraShots;

	if (UWorld* World = GetWorld())
	{
		// Replaces any volley still running from an earlier, shorter FireInterval, rather than
		// overlapping two bursts' timers.
		World->GetTimerManager().SetTimer(StaggerTimerHandle, this, &UStarterWeaponComponent::HandleStaggerTimer, ProjectileStaggerSeconds, true);
	}
}

void UStarterWeaponComponent::HandleStaggerTimer()
{
	// Skipped, not redirected to a different enemy, if the target died or was removed mid-burst:
	// FireAt already refuses an invalid target and simply returns null.
	FireAt(PendingStaggerTarget.Get());

	if (--PendingStaggeredShots <= 0)
	{
		if (const UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(StaggerTimerHandle);
		}
	}
}

float UStarterWeaponComponent::GetEffectiveDamage() const
{
	return ApplyOwnerStats(GetOwner(), EGrantMoneyStat::Damage, Damage);
}

float UStarterWeaponComponent::GetEffectiveFireInterval() const
{
	// Attack speed boosts are negative AttackInterval modifiers. However many are stacked, the
	// interval never drops below the minimum, so a timer can never fire every frame.
	return FMath::Max(ApplyOwnerStats(GetOwner(), EGrantMoneyStat::AttackInterval, FireInterval), MinFireInterval);
}

int32 UStarterWeaponComponent::GetEffectivePenetration() const
{
	const float Value = ApplyOwnerStats(GetOwner(), EGrantMoneyStat::Penetration, static_cast<float>(Penetration));

	// Rounded down, with a small tolerance so a sum like 0.1 * 10 still counts as a whole 1.
	return FMath::Max(0, FMath::FloorToInt32(Value + UE_KINDA_SMALL_NUMBER));
}

int32 UStarterWeaponComponent::GetEffectiveProjectileCount() const
{
	const float Value = ApplyOwnerStats(GetOwner(), EGrantMoneyStat::ProjectileCount, static_cast<float>(ProjectileCount));
	return FMath::Max(1, FMath::FloorToInt32(Value + UE_KINDA_SMALL_NUMBER));
}

float UStarterWeaponComponent::GetEffectiveExplosionRadius() const
{
	return FMath::Max(0.0f, ApplyOwnerStats(GetOwner(), EGrantMoneyStat::ExplosionRadius, ExplosionRadius));
}
