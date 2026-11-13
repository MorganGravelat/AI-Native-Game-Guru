// Fill out your copyright notice in the Description page of Project Settings.


#include "SlimeEnemy.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GrantMoneyAudio.h"
#include "StatBoostPickup.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// The engine sphere is 100 cm across, so a scale of 0.6 gives a 60 cm slime.
	constexpr double PlaceholderScale = 0.6;
	constexpr float SlimeCapsuleRadius = 30.0f;

	// Below this ground speed (cm/s) a slime counts as standing still, so it is not squishing for nothing.
	constexpr float MinSquishSpeed = 10.0f;

	// How fast the body eases into and out of a shape a subclass asks for (see GetSquishOverride).
	constexpr float OverrideBlendSpeed = 12.0f;
}

ASlimeEnemy::ASlimeEnemy()
{
	// Slime defaults for values AEnemyBase already handles.
	MoveSpeed = 250.0f;
	StopDistance = 80.0f;

	// Every slime leaves an XP orb behind.
	bDropsXpOrb = true;

	HitSound = GrantMoneyAudio::DefaultSound(TEXT("/Game/Sound/Enemy/SlimeHit.SlimeHit"));
	DeathSound = GrantMoneyAudio::DefaultSound(TEXT("/Game/Sound/Enemy/SlimeDeath.SlimeDeath"));

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

USceneComponent* ASlimeEnemy::GetEmergenceVisual() const
{
	return PlaceholderMesh;
}

void ASlimeEnemy::ComputeSquishPose(const FVector& BaseScale, const FVector& BaseLocation, float BottomLocalZ,
	float Phase, float Amount, float Amplitude, FVector& OutScale, FVector& OutLocation)
{
	const float Swing = static_cast<float>(FMath::Sin(static_cast<double>(Phase))) * Amount * Amplitude;
	ComputeSwingPose(BaseScale, BaseLocation, BottomLocalZ, Swing, OutScale, OutLocation);
}

void ASlimeEnemy::ComputeSwingPose(const FVector& BaseScale, const FVector& BaseLocation, float BottomLocalZ,
	float Swing, FVector& OutScale, FVector& OutLocation)
{
	// Taller means slightly thinner (and flatter means slightly wider) so the body keeps about the same bulk.
	OutScale = BaseScale * FVector(1.0 - Swing * 0.5, 1.0 - Swing * 0.5, 1.0 + Swing);

	// Same bottom-holding rule as the emergence pose: the lowest point of the mesh does not move.
	const double RestBottomZ = BaseLocation.Z + BaseScale.Z * BottomLocalZ;
	OutLocation = BaseLocation;
	OutLocation.Z = RestBottomZ - OutScale.Z * BottomLocalZ;
}

void ASlimeEnemy::BeginPlay()
{
	Super::BeginPlay();

	// Read here, after the Blueprint's mesh, scale and offset are final and before emergence changes them.
	BaseMeshScale = PlaceholderMesh->GetRelativeScale3D();
	BaseMeshLocation = PlaceholderMesh->GetRelativeLocation();
	const FBoxSphereBounds LocalBounds = PlaceholderMesh->CalcBounds(FTransform::Identity);
	BaseMeshBottomLocalZ = static_cast<float>(LocalBounds.Origin.Z - LocalBounds.BoxExtent.Z);

	SquishPhase = FMath::FRand() * UE_TWO_PI;
}

void ASlimeEnemy::HandleDeath(AActor* Killer)
{
	// A corpse that stays for a moment should not freeze mid-squish. Emergence restores its own pose.
	if (!IsEmerging() && !bMeshAtRest)
	{
		PlaceholderMesh->SetRelativeTransform(FTransform(PlaceholderMesh->GetRelativeRotation(), BaseMeshLocation, BaseMeshScale));
		bMeshAtRest = true;
	}

	Super::HandleDeath(Killer);
}

void ASlimeEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateSquish(DeltaTime);
}

void ASlimeEnemy::UpdateSquish(float DeltaTime)
{
	// Emergence owns the mesh pose while it plays.
	if (!bSquishWhileMoving || IsEmerging())
	{
		return;
	}

	const bool bMoving = GetVelocity().SizeSquared2D() > FMath::Square(MinSquishSpeed);
	SquishAlpha = FMath::FInterpTo(SquishAlpha, bMoving ? 1.0f : 0.0f, DeltaTime, SquishBlendSpeed);

	float OverrideTarget = 0.0f;
	const bool bOverriding = GetSquishOverride(OverrideTarget);
	OverrideWeight = FMath::FInterpTo(OverrideWeight, bOverriding ? 1.0f : 0.0f, DeltaTime, OverrideBlendSpeed);
	if (bOverriding)
	{
		OverrideSwing = FMath::FInterpTo(OverrideSwing, OverrideTarget, DeltaTime, OverrideBlendSpeed);
	}

	if (!bMoving && !bOverriding && SquishAlpha < 0.01f && OverrideWeight < 0.01f)
	{
		SquishAlpha = 0.0f;
		OverrideWeight = 0.0f;
		OverrideSwing = 0.0f;

		if (!bMeshAtRest)
		{
			PlaceholderMesh->SetRelativeTransform(FTransform(PlaceholderMesh->GetRelativeRotation(), BaseMeshLocation, BaseMeshScale));
			bMeshAtRest = true;
		}

		return;
	}

	SquishPhase = FMath::Fmod(SquishPhase + UE_TWO_PI * SquishFrequency * DeltaTime, static_cast<float>(UE_TWO_PI));

	const float WaveSwing = FMath::Sin(SquishPhase) * SquishAmount * SquishAlpha;
	const float Swing = FMath::Lerp(WaveSwing, OverrideSwing, OverrideWeight);

	FVector Scale;
	FVector Location;
	ComputeSwingPose(BaseMeshScale, BaseMeshLocation, BaseMeshBottomLocalZ, Swing, Scale, Location);

	// One transform update per frame rather than a scale and a location update.
	PlaceholderMesh->SetRelativeTransform(FTransform(PlaceholderMesh->GetRelativeRotation(), Location, Scale));
	bMeshAtRest = false;
}
