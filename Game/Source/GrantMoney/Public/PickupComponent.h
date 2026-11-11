// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PickupComponent.generated.h"

/**
 * Lets its owner (the player) collect pickups. Every frame it looks for any APickupBase within
 * PickupRadius of the owner and tells each one to lock onto the owner. From then on the pickup flies
 * in and applies its own effect (see APickupBase::OnCollected), so a new pickup type needs no change here.
 *
 * Detection is a sphere scan around the owner's location, so the radius can be tuned while playing.
 * Pickups already chasing the owner (or collected) ignore repeated requests.
 */
UCLASS(ClassGroup = (Pickups), meta = (BlueprintSpawnableComponent))
class GRANTMONEY_API UPickupComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPickupComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Finds pickups within PickupRadius and starts attracting any that are not already chasing someone.
	// Returns how many started chasing during this call. Runs every tick; callable directly too.
	int32 AttractPickupsInRadius();

	// Radius (cm) around the owner inside which pickups notice the owner and fly toward it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup", meta = (ClampMin = "0.0", Units = "cm"))
	float PickupRadius = 300.0f;
};
