// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupBase.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

namespace
{
	// Radius (cm) of the query-only sphere a UPickupComponent scan can find.
	constexpr float PickupCollisionRadius = 12.0f;
}

APickupBase::APickupBase()
{
	// Ticks only while chasing a collector; BeginAttraction turns it on.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// After character movement, so the pickup aims at where the collector ended up this frame.
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(PickupCollisionRadius);
	RootComponent = Collision;

	// Found by object-type scans only; it never blocks or overlaps anything by itself.
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetGenerateOverlapEvents(false);
	Collision->SetCanEverAffectNavigation(false);
	Collision->CanCharacterStepUpOn = ECB_No;

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(RootComponent);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupMesh->SetGenerateOverlapEvents(false);
	PickupMesh->SetCanEverAffectNavigation(false);
}

bool APickupBase::BeginAttraction(AActor* InCollector)
{
	if (bCollected || !IsValid(InCollector) || IsAttracted())
	{
		return false;
	}

	LockedCollector = InCollector;
	CurrentSpeed = BaseSpeed;
	SetActorTickEnabled(true);
	return true;
}

void APickupBase::StopAttraction()
{
	LockedCollector.Reset();
	CurrentSpeed = 0.0f;
	SetActorTickEnabled(false);
}

bool APickupBase::TryCollect(AActor* InCollector)
{
	if (bCollected || !IsValid(InCollector) || IsActorBeingDestroyed())
	{
		return false;
	}

	bCollected = true;
	SetActorTickEnabled(false);

	OnCollected(InCollector);
	Destroy();
	return true;
}

void APickupBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AActor* Target = LockedCollector.Get();
	if (!IsValid(Target))
	{
		StopAttraction();
		return;
	}

	// Speed up toward the top speed (never below the base speed, even if MaxSpeed is set lower).
	const float TopSpeed = FMath::Max(MaxSpeed, BaseSpeed);
	const float NewSpeed = FMath::Min(CurrentSpeed + Acceleration * DeltaSeconds, TopSpeed);

	// Average of the speed at both ends of the frame: the distance covered does not depend on the frame rate.
	const double Step = 0.5 * (CurrentSpeed + NewSpeed) * DeltaSeconds;
	CurrentSpeed = NewSpeed;

	// Aim at the collector's position now, so the pickup follows its turns.
	const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	const double Distance = ToTarget.Size();

	// Arrived, or would pass the collection range this frame. Without the second check a long frame
	// or a fast pickup could jump past the collector and never be collected.
	if (Distance - Step <= CollectionDistance)
	{
		SetActorLocation(GetActorLocation() + ToTarget.GetSafeNormal() * FMath::Min(Step, Distance));
		TryCollect(Target);
		return;
	}

	SetActorLocation(GetActorLocation() + ToTarget / Distance * Step);
}
