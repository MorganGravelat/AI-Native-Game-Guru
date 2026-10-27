// Fill out your copyright notice in the Description page of Project Settings.


#include "SlimeEnemy.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
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
