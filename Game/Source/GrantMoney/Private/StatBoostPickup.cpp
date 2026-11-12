// Fill out your copyright notice in the Description page of Project Settings.


#include "StatBoostPickup.h"

#include "Components/MaterialBillboardComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "StatsComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// The engine sphere is 100 cm across, so 0.32 gives a 32 cm sphere: a little bigger than an XP orb.
	constexpr double StatBoostPlaceholderScale = 0.32;

	// Paper2D's masked, unlit sprite material: draws a texture as-is and cuts out transparent pixels.
	const TCHAR* DefaultIconMaterialPath = TEXT("/Paper2D/MaskedUnlitSpriteMaterial.MaskedUnlitSpriteMaterial");

	FString StatBoostStatName(EGrantMoneyStat Stat)
	{
		return StaticEnum<EGrantMoneyStat>()->GetNameStringByValue(static_cast<int64>(Stat));
	}
}

AStatBoostPickup::AStatBoostPickup()
{
	PickupMesh->SetRelativeScale3D(FVector(StatBoostPlaceholderScale));

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

	// A soft reference: nothing is loaded (and nothing is logged) unless an Icon is actually used.
	IconMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(DefaultIconMaterialPath));

	// Empty until BeginPlay adds the icon. A material billboard always faces the camera.
	IconBillboard = CreateDefaultSubobject<UMaterialBillboardComponent>(TEXT("IconBillboard"));
	IconBillboard->SetupAttachment(RootComponent);
	IconBillboard->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	IconBillboard->SetGenerateOverlapEvents(false);
	IconBillboard->SetCanEverAffectNavigation(false);
}

void AStatBoostPickup::BeginPlay()
{
	Super::BeginPlay();

	if (ShowIcon())
	{
		PickupMesh->SetVisibility(false);
		return;
	}

	ApplyGlow();
}

bool AStatBoostPickup::ShowIcon()
{
	if (!Icon || !IconBillboard)
	{
		return false;
	}

	UMaterialInterface* BaseMaterial = IconMaterial.LoadSynchronous();
	if (!BaseMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("StatBoostPickup: %s could not load IconMaterial '%s', so it shows the sphere instead of its icon."),
			*GetName(), *IconMaterial.ToString());
		return false;
	}

	UMaterialInstanceDynamic* IconInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	IconInstance->SetTextureParameterValue(IconTextureParameterName, Icon);

	IconBillboard->SetElements({});
	IconBillboard->AddElement(IconInstance, nullptr, false, IconSize, IconSize, nullptr);
	return true;
}

bool AStatBoostPickup::IsShowingIcon() const
{
	return IconBillboard && IconBillboard->Elements.Num() > 0;
}

void AStatBoostPickup::ApplyGlow()
{
	if (!GlowMaterial)
	{
		return;
	}

	UMaterialInstanceDynamic* Glow = UMaterialInstanceDynamic::Create(GlowMaterial, this);
	Glow->SetVectorParameterValue(GlowParameterName, GlowColor);
	PickupMesh->SetMaterial(0, Glow);
}

bool AStatBoostPickup::ApplyBoost(AActor* Collector)
{
	UStatsComponent* Stats = IsValid(Collector) ? Collector->FindComponentByClass<UStatsComponent>() : nullptr;
	if (!Stats)
	{
		UE_LOG(LogTemp, Warning, TEXT("StatBoostPickup: %s has no Stats component, so the %s boost was lost."), *GetNameSafe(Collector), *StatBoostStatName(Stat));
		return false;
	}

	if (FMath::IsNearlyZero(Amount))
	{
		return false;
	}

	// Permanent: the handle is not kept, so nothing ever removes this modifier during the run.
	Stats->AddModifier(Stat, ModifierOp, Amount);

	const bool bPercent = ModifierOp == EGrantMoneyStatModifierOp::Multiplicative;
	UE_LOG(LogTemp, Log, TEXT("StatBoostPickup: %s gained %+.2f%s %s (permanent)."), *GetNameSafe(Collector),
		bPercent ? Amount * 100.0f : Amount, bPercent ? TEXT("%") : TEXT(""), *StatBoostStatName(Stat));
	return true;
}

void AStatBoostPickup::OnCollected(AActor* Collector)
{
	if (ApplyBoost(Collector))
	{
		PlayPickupFeedback(Collector);
	}
}

void AStatBoostPickup::PlayPickupFeedback(AActor* Collector)
{
	if (PickupSound)
	{
		UGameplayStatics::PlaySound2D(this, PickupSound, PickupSoundVolume);
	}

	if (PickupEffect && Collector && Collector->GetRootComponent())
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(PickupEffect, Collector->GetRootComponent(), NAME_None,
			FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
	}
}

// Ready-made boosts. Each one only changes defaults; a Blueprint subclass can change any of them.

AMaxHealthBoostPickup::AMaxHealthBoostPickup()
{
	Stat = EGrantMoneyStat::MaxHealth;
	ModifierOp = EGrantMoneyStatModifierOp::Additive;
	Amount = 10.0f;
	GlowColor = FLinearColor(3.0f, 0.15f, 0.15f);
}

ADamageBoostPickup::ADamageBoostPickup()
{
	Stat = EGrantMoneyStat::Damage;
	ModifierOp = EGrantMoneyStatModifierOp::Additive;
	Amount = 2.0f;
	GlowColor = FLinearColor(3.0f, 1.0f, 0.1f);
}

AAttackSpeedBoostPickup::AAttackSpeedBoostPickup()
{
	Stat = EGrantMoneyStat::AttackInterval;
	ModifierOp = EGrantMoneyStatModifierOp::Multiplicative;
	Amount = -0.05f;
	GlowColor = FLinearColor(3.0f, 3.0f, 0.2f);
}

APenetrationBoostPickup::APenetrationBoostPickup()
{
	Stat = EGrantMoneyStat::Penetration;
	ModifierOp = EGrantMoneyStatModifierOp::Additive;
	Amount = 1.0f;
	GlowColor = FLinearColor(0.3f, 0.8f, 3.0f);
}

AContactDamageBoostPickup::AContactDamageBoostPickup()
{
	Stat = EGrantMoneyStat::ContactDamage;
	ModifierOp = EGrantMoneyStatModifierOp::Additive;
	Amount = 2.0f;
	GlowColor = FLinearColor(2.0f, 0.2f, 3.0f);
}
