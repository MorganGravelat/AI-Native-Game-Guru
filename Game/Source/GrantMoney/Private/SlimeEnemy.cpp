// Fill out your copyright notice in the Description page of Project Settings.


#include "SlimeEnemy.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "StatBoostPickup.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// The engine sphere is 100 cm across, so a scale of 0.6 gives a 60 cm slime.
	constexpr double PlaceholderScale = 0.6;
	constexpr float SlimeCapsuleRadius = 30.0f;
}

ASlimeEnemy::ASlimeEnemy()
{
	// Slime defaults for values AEnemyBase already handles.
	MoveSpeed = 250.0f;
	StopDistance = 80.0f;

	// Every slime leaves an XP orb behind.
	bDropsXpOrb = true;

	// Rare permanent stat boosts, each rolled on its own (SCRUM-53). Change the chances, or swap a class
	// for a Blueprint child with an icon and sound, in BonusPickupDrops in the Details panel.
	// Contact damage is left out until the player has a contact attack that reads that stat.
	for (const TSubclassOf<APickupBase> BoostClass : {
		TSubclassOf<APickupBase>(AMaxHealthBoostPickup::StaticClass()),
		TSubclassOf<APickupBase>(ADamageBoostPickup::StaticClass()),
		TSubclassOf<APickupBase>(AAttackSpeedBoostPickup::StaticClass()),
		TSubclassOf<APickupBase>(APenetrationBoostPickup::StaticClass()) })
	{
		FEnemyPickupDrop BoostDrop;
		BoostDrop.PickupClass = BoostClass;
		BoostDrop.DropChance = DefaultStatBoostDropChance;
		BonusPickupDrops.Add(BoostDrop);
	}

	// A round body: the capsule is as tall as it is wide.
	GetCapsuleComponent()->InitCapsuleSize(SlimeCapsuleRadius, SlimeCapsuleRadius);

	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
	PlaceholderMesh->SetupAttachment(RootComponent);
	PlaceholderMesh->SetRelativeScale3D(FVector(PlaceholderScale));
	PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderMesh->SetCanEverAffectNavigation(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	if (SphereFinder.Succeeded())
	{
		PlaceholderMesh->SetStaticMesh(SphereFinder.Object);
	}
}
