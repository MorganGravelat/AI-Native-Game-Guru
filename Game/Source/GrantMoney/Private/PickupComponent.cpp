// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupComponent.h"

#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "PickupBase.h"

UPickupComponent::UPickupComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UPickupComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AttractPickupsInRadius();
}

int32 UPickupComponent::AttractPickupsInRadius()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World || !(PickupRadius > 0.0f))
	{
		return 0;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PickupScan), false, Owner);
	World->OverlapMultiByObjectType(
		Overlaps,
		Owner->GetActorLocation(),
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_WorldDynamic),
		FCollisionShape::MakeSphere(PickupRadius),
		QueryParams);

	int32 Started = 0;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		// Other world-dynamic things (projectiles, props) show up too; only pickups matter.
		APickupBase* Pickup = Cast<APickupBase>(Overlap.GetActor());
		if (Pickup && Pickup->BeginAttraction(Owner))
		{
			++Started;
		}
	}

	return Started;
}
