// Fill out your copyright notice in the Description page of Project Settings.


#include "MagicOrbProjectile.h"

#include "AttackHitComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
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

	// The explosion visual never shrinks the mesh, even for a small radius: always a clearly visible grow.
	constexpr float MinExplosionVisualScale = 1.5f;

	// Same parameter name XpOrbPickup/StatBoostPickup already tint on this same engine material.
	const FName ExplosionEmissiveParameterName(TEXT("EmissiveColor"));
}

AMagicOrbProjectile::AMagicOrbProjectile()
{
	// Off until an explosion starts the brief grow-then-vanish visual; nothing else needs a per-frame update.
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(false);

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

	// Same emissive material XpOrbPickup/StatBoostPickup use for their glow; only applied at explosion
	// time (see Explode), so the orb's everyday look is untouched.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowFinder(TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	if (GlowFinder.Succeeded())
	{
		ExplosionGlowMaterial = GlowFinder.Object;
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

void AMagicOrbProjectile::Launch(const FVector& Direction, float Speed, float Damage, float Lifetime, int32 Penetration, float InExplosionRadius)
{
	const FVector Heading = Direction.GetSafeNormal();

	Movement->Velocity = Heading * Speed;
	Movement->InitialSpeed = Speed;
	Movement->MaxSpeed = Speed;
	AttackHit->FallbackDamage = Damage;
	AttackHit->FallbackPenetration = FMath::Max(0, Penetration);
	LifetimeSeconds = FMath::Max(Lifetime, MinLifetimeSeconds);
	ExplosionRadius = FMath::Max(0.0f, InExplosionRadius);

	if (!Heading.IsNearlyZero())
	{
		SetActorRotation(Heading.Rotation());
	}
}

void AMagicOrbProjectile::BeginPlay()
{
	Super::BeginPlay();

	// A missed orb removes itself (or explodes first: see LifeSpanExpired).
	SetLifeSpan(LifetimeSeconds);
}

void AMagicOrbProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bExploding)
	{
		return;
	}

	ExplosionElapsed += DeltaSeconds;
	const float Alpha = ExplosionVisualSeconds > 0.0f ? FMath::Clamp(ExplosionElapsed / ExplosionVisualSeconds, 0.0f, 1.0f) : 1.0f;
	PlaceholderMesh->SetRelativeScale3D(FMath::Lerp(ExplosionStartScale, ExplosionTargetScale, Alpha));

	if (ExplosionMID)
	{
		// Fades the glow from ExplosionColor to black as the burst grows, so it visibly dims out rather
		// than just stopping; this placeholder material is opaque, so "fading" is a dimming emissive,
		// not alpha transparency (see class comment on PlaceholderMesh for the later real-VFX swap).
		ExplosionMID->SetVectorParameterValue(ExplosionEmissiveParameterName, FMath::Lerp(ExplosionColor, FLinearColor::Black, Alpha));
	}

	if (Alpha >= 1.0f)
	{
		bExploding = false;
		SetActorTickEnabled(false);

		if (bDestroyWhenVisualFinishes)
		{
			Destroy();
		}
		else
		{
			// The orb survives this burst (penetration left): back to its normal look, still flying.
			PlaceholderMesh->SetRelativeScale3D(ExplosionStartScale);
			PlaceholderMesh->SetMaterial(0, nullptr);
			ExplosionMID = nullptr;
		}
	}
}

void AMagicOrbProjectile::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// The hit rules skip the shooter, things without health and dead enemies, so those are simply flown through.
	const EGrantMoneyHitResult Result = AttackHit->TryHit(OtherActor);

	// A hit that also exhausts the orb's budget fires OnHitsExhausted (-> HandleHitsExhausted)
	// synchronously inside TryHit, above, before TriggerExplosion ever runs. That is deliberate: this
	// contact is still spent on exploding first, and FinishIfOutOfHits (called below) only stops the
	// orb once the explosion (if any) has already been triggered.
	if (Result == EGrantMoneyHitResult::Hit)
	{
		TriggerExplosion();
	}

	if (bHitsExhausted)
	{
		bHitsExhausted = false;
		FinishIfOutOfHits();
	}
}

void AMagicOrbProjectile::HandleHitsExhausted(UAttackHitComponent* UsedAttackHit)
{
	// Deferred: see HandleBeginOverlap. Removing the orb here, before TriggerExplosion has had a chance
	// to run for this same hit, would destroy it right before (or instead of) its one explosion.
	bHitsExhausted = true;
}

void AMagicOrbProjectile::LifeSpanExpired()
{
	// A total miss (never landed a direct hit) that still carries an unused explosion charge spends it
	// here, wherever the orb ends up, instead of just vanishing. Once explodes, it is always removed
	// (there is no more flying left to do once its lifetime is over).
	if (ExplosionRadius > 0.0f && !bHasExploded)
	{
		TriggerExplosion();
		FinishIfOutOfHits();
		return;
	}

	if (bExploding)
	{
		// A contact-triggered burst from earlier is still animating; let it finish instead of cutting it
		// short by destroying the orb out from under it.
		bDestroyWhenVisualFinishes = true;
		return;
	}

	Super::LifeSpanExpired();
}

void AMagicOrbProjectile::TriggerExplosion()
{
	if (bHasExploded || ExplosionRadius <= 0.0f)
	{
		return;
	}

	bHasExploded = true;

	// Damage everyone in range. TryAreaHit ignores penetration (an explosion is not limited by it) but
	// still skips anyone this orb already hit directly, so nobody is ever damaged twice by one orb.
	if (UWorld* World = GetWorld())
	{
		TArray<FOverlapResult> Overlaps;
		const FCollisionShape Sphere = FCollisionShape::MakeSphere(ExplosionRadius);
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MagicOrbExplosion), false);
		QueryParams.AddIgnoredActor(this);

		World->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity, ECC_Pawn, Sphere, QueryParams);

		for (const FOverlapResult& Overlap : Overlaps)
		{
			AttackHit->TryAreaHit(Overlap.GetActor());
		}
	}

	// Grow to a size that roughly represents the blast radius, then vanish over ExplosionVisualSeconds.
	// Movement and collision are left untouched here: if the orb still has penetration left, it needs to
	// keep flying and hitting more enemies directly; FinishIfOutOfHits is what actually stops it, once
	// (and only once) it is genuinely out of hits or its lifetime is over.
	bExploding = true;
	ExplosionElapsed = 0.0f;
	ExplosionStartScale = PlaceholderMesh->GetRelativeScale3D();
	const float VisualScale = FMath::Max(MinExplosionVisualScale, ExplosionRadius / OrbRadius);
	ExplosionTargetScale = FVector(VisualScale);

	// Tints the burst red (starting at ExplosionColor, faded to black in Tick), on top of the grow.
	if (ExplosionGlowMaterial)
	{
		ExplosionMID = UMaterialInstanceDynamic::Create(ExplosionGlowMaterial, this);
		ExplosionMID->SetVectorParameterValue(ExplosionEmissiveParameterName, ExplosionColor);
		PlaceholderMesh->SetMaterial(0, ExplosionMID);
	}

	SetActorTickEnabled(true);
}

void AMagicOrbProjectile::FinishIfOutOfHits()
{
	Movement->StopMovementImmediately();
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (bExploding)
	{
		// The burst visual is still animating (it was just triggered by this same hit, or an earlier one
		// that finished slower than this hit sequence): let Tick finish it before removing the orb.
		bDestroyWhenVisualFinishes = true;
	}
	else
	{
		Destroy();
	}
}
