// Fill out your copyright notice in the Description page of Project Settings.


#include "XpOrbPickup.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GrantMoneyStatTypes.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "StatsComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// The engine sphere is 100 cm across, so a scale of 0.24 gives a 24 cm orb.
	constexpr double OrbVisualScale = 0.24;
}

AXpOrbPickup::AXpOrbPickup()
{
	PickupMesh->SetRelativeScale3D(FVector(OrbVisualScale));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereFinder.Succeeded())
	{
		PickupMesh->SetStaticMesh(SphereFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowFinder(TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	if (GlowFinder.Succeeded())
	{
		GlowMaterial = GlowFinder.Object;
	}
}

void AXpOrbPickup::BeginPlay()
{
	Super::BeginPlay();

	if (!GlowMaterial)
	{
		return;
	}

	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(GlowMaterial, this);
	Material->SetVectorParameterValue(GlowParameterName, GlowColor);
	PickupMesh->SetMaterial(0, Material);
}

void AXpOrbPickup::OnCollected(AActor* Collector)
{
	UStatsComponent* Stats = Collector ? Collector->FindComponentByClass<UStatsComponent>() : nullptr;
	if (!Stats)
	{
		UE_LOG(LogTemp, Warning, TEXT("XpOrbPickup: %s has no Stats component, so %.1f XP was lost."), *GetNameSafe(Collector), XpValue);
		return;
	}

	// The total lives in the base value, so buffs that modify other stats never alter earned XP.
	const float NewTotal = Stats->GetBaseStat(EGrantMoneyStat::Experience) + XpValue;
	Stats->SetBaseStat(EGrantMoneyStat::Experience, NewTotal);
	UE_LOG(LogTemp, Log, TEXT("XpOrbPickup: %s gained %.1f XP (total %.1f)."), *GetNameSafe(Collector), XpValue, NewTotal);
}
