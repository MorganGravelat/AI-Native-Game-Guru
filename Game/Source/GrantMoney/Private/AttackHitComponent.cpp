// Fill out your copyright notice in the Description page of Project Settings.

#include "AttackHitComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "StatsComponent.h"

UAttackHitComponent::UAttackHitComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAttackHitComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const AActor* Owner = GetOwner())
	{
		Stats = Owner->FindComponentByClass<UStatsComponent>();
	}
}

float UAttackHitComponent::GetDamage() const
{
	return Stats && Stats->HasStat(EGrantMoneyStat::Damage) ? Stats->GetStat(EGrantMoneyStat::Damage) : FallbackDamage;
}

int32 UAttackHitComponent::GetPenetration() const
{
	const int32 Penetration = Stats && Stats->HasStat(EGrantMoneyStat::Penetration)
		? FMath::FloorToInt32(Stats->GetStat(EGrantMoneyStat::Penetration))
		: FallbackPenetration;
	return FMath::Max(0, Penetration);
}

int32 UAttackHitComponent::GetRemainingHits() const
{
	return FMath::Max(0, 1 + GetPenetration() - HitActors.Num());
}

bool UAttackHitComponent::HasHit(const AActor* Target) const
{
	return Target && HitActors.Contains(TWeakObjectPtr<const AActor>(Target));
}

bool UAttackHitComponent::IsValidTarget(const AActor* Target) const
{
	const AActor* Owner = GetOwner();
	if (!IsValid(Target) || Target->IsActorBeingDestroyed() || !Owner)
	{
		return false;
	}

	// Never the attack itself, the weapon that made it, or the player who fired it.
	if (Target == Owner || Target == Owner->GetOwner() || Target == Owner->GetInstigator())
	{
		return false;
	}

	// Only things that can be hurt and are still alive. Walls, props and dying enemies do not use up penetration.
	const UHealthComponent* TargetHealth = Target->FindComponentByClass<UHealthComponent>();
	return TargetHealth && !TargetHealth->IsDead();
}

bool UAttackHitComponent::CanHit(const AActor* Target) const
{
	return IsValidTarget(Target) && !HasHit(Target) && !IsExhausted();
}

EGrantMoneyHitResult UAttackHitComponent::TryHit(AActor* Target)
{
	if (!IsValidTarget(Target))
	{
		return EGrantMoneyHitResult::NotATarget;
	}

	if (HasHit(Target))
	{
		return EGrantMoneyHitResult::AlreadyHit;
	}

	if (IsExhausted())
	{
		return EGrantMoneyHitResult::Exhausted;
	}

	// Record before applying damage: damage can kill and destroy the target, and a listener could call TryHit again.
	HitActors.Add(Target);

	AActor* Owner = GetOwner();
	const float Damage = GetDamage();
	UGameplayStatics::ApplyDamage(Target, Damage, Owner->GetInstigatorController(), Owner, UDamageType::StaticClass());

	OnTargetHit.Broadcast(Target, Damage);

	if (IsExhausted())
	{
		OnHitsExhausted.Broadcast(this);
	}

	return EGrantMoneyHitResult::Hit;
}

void UAttackHitComponent::ResetHits()
{
	HitActors.Reset();
}
