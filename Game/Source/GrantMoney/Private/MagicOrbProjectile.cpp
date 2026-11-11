// Fill out your copyright notice in the Description page of Project Settings.


#include "MagicOrbProjectile.h"

#include "AttackHitComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Orb radius in cm. The engine sphere is 100 cm across, so the visual scale is Diameter / 100.
	constexpr float OrbRadius = 10.0f;
	constexpr double EngineSphereDiameter = 100.0;

	// Long frames are split into steps this short, so a fast orb cannot skip over a slime between two frames.
	constexpr float MaxSimulationStep = 1.0f / 60.0f;
	constexpr int32 MaxSimulationSteps = 8;

	// Shortest lifetime accepted, so an orb is never removed before its first move.
	constexpr float MinLifetimeSeconds = 0.05f;
}

AMagicOrbProjectile::AMagicOrbProjectile()
{
	// Movement is done by the movement component; nothing else needs a per-frame update.
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(OrbRadius);
	RootComponent = Collision;

	// Detects pawns by overlap and ignores everything else (walls, ground, other orbs).
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	Collision->SetCanEverAffectNavigation(false);
	Collision->CanCharacterStepUpOn = ECB_No;
	Collision->OnComponentBeginOverlap.AddDynamic(this, &AMagicOrbProjectile::HandleBeginOverlap);

	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
	PlaceholderMesh->SetupAttachment(RootComponent);
	PlaceholderMesh->SetRelativeScale3D(FVector(OrbRadius * 2.0 / EngineSphereDiameter));
	PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderMesh->SetCanEverAffectNavigation(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	if (SphereFinder.Succeeded())
	{
		PlaceholderMesh->SetStaticMesh(SphereFinder.Object);
	}

	// A straight line: no gravity, no bouncing, no homing.
	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->ProjectileGravityScale = 0.0f;
	Movement->bShouldBounce = false;
	Movement->bRotationFollowsVelocity = false;
	Movement->bInitialVelocityInLocalSpace = false;
	Movement->bForceSubStepping = true;
	Movement->MaxSimulationTimeStep = MaxSimulationStep;
	Movement->MaxSimulationIterations = MaxSimulationSteps;

	AttackHit = CreateDefaultSubobject<UAttackHitComponent>(TEXT("AttackHit"));
	AttackHit->FallbackPenetration = 0;
	AttackHit->OnHitsExhausted.AddDynamic(this, &AMagicOrbProjectile::HandleHitsExhausted);
}

void AMagicOrbProjectile::Launch(const FVector& Direction, float Speed, float Damage, float Lifetime, int32 Penetration)
{
	const FVector Heading = Direction.GetSafeNormal();

	Movement->Velocity = Heading * Speed;
	Movement->InitialSpeed = Speed;
	Movement->MaxSpeed = Speed;
	AttackHit->FallbackDamage = Damage;
	AttackHit->FallbackPenetration = FMath::Max(0, Penetration);
	LifetimeSeconds = FMath::Max(Lifetime, MinLifetimeSeconds);

	if (!Heading.IsNearlyZero())
	{
		SetActorRotation(Heading.Rotation());
	}
}

void AMagicOrbProjectile::BeginPlay()
{
	Super::BeginPlay();

	// A missed orb removes itself.
	SetLifeSpan(LifetimeSeconds);
}

void AMagicOrbProjectile::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// The hit rules skip the shooter, things without health and dead enemies, so those are simply flown through.
	AttackHit->TryHit(OtherActor);
}

void AMagicOrbProjectile::HandleHitsExhausted(UAttackHitComponent* UsedAttackHit)
{
	Destroy();
}
