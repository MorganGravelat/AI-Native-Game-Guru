// Fill out your copyright notice in the Description page of Project Settings.

#include "ContactDamageComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "StatsComponent.h"

UContactDamageComponent::UContactDamageComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UContactDamageComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const AActor* Owner = GetOwner())
	{
		Stats = Owner->FindComponentByClass<UStatsComponent>();
	}
}

float UContactDamageComponent::GetDamage() const
{
	return Stats && Stats->HasStat(EGrantMoneyStat::ContactDamage) ? Stats->GetStat(EGrantMoneyStat::ContactDamage) : FallbackDamage;
}

float UContactDamageComponent::GetInterval() const
{
	return Stats && Stats->HasStat(EGrantMoneyStat::AttackInterval) ? Stats->GetStat(EGrantMoneyStat::AttackInterval) : FallbackInterval;
}

bool UContactDamageComponent::IsInReach(const AActor* Target) const
{
	const AActor* Owner = GetOwner();
	if (!Owner || !IsValid(Target))
	{
		return false;
	}

	// Ground plane only, matching AEnemyBase's distance checks.
	FVector Offset = Target->GetActorLocation() - Owner->GetActorLocation();
	Offset.Z = 0.0;

	const float Gap = static_cast<float>(Offset.Size()) - Owner->GetSimpleCollisionRadius() - Target->GetSimpleCollisionRadius();
	return Gap <= Reach;
}

bool UContactDamageComponent::TryApplyContactDamage(AActor* Target)
{
	const UWorld* World = GetWorld();
	if (!World || !IsInReach(Target))
	{
		return false;
	}

	const double Now = World->GetTimeSeconds();
	if (LastHitTime.IsSet() && Now - LastHitTime.GetValue() < GetInterval())
	{
		return false;
	}

	const float Damage = GetDamage();
	if (Damage <= 0.0f)
	{
		return false;
	}

	AActor* Owner = GetOwner();
	UGameplayStatics::ApplyDamage(Target, Damage, Owner->GetInstigatorController(), Owner, UDamageType::StaticClass());

	if (bLogHits)
	{
		UE_LOG(LogTemp, Log, TEXT("ContactDamage: %s hit %s for %.1f at t=%.2f s (next hit possible after %.1f s)."),
			*Owner->GetName(), *Target->GetName(), Damage, Now, GetInterval());
	}

	LastHitTime = Now;
	return true;
}
