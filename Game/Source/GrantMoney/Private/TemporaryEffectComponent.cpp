// Fill out your copyright notice in the Description page of Project Settings.

#include "TemporaryEffectComponent.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TemporaryEffect.h"

namespace
{
	const TCHAR* AuraRingMeshPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* AuraRingMaterialPath = TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial");

	// The engine cylinder is 100 cm wide and 100 cm tall.
	constexpr double EngineShapeSize = 100.0;
	constexpr double AuraRingThickness = 2.0;

	// Emissive values above 1 glow; the ring should read clearly from the map camera.
	constexpr float AuraRingGlow = 4.0f;

	// /Engine/EngineMaterials/EmissiveMeshMaterial's one vector parameter is named "Color", not
	// "EmissiveColor" (verified against the material's actual exposed parameters; see the matching
	// comment and fix in RewardTotem.cpp, which used the same wrong name).
	const FName AuraColorParameterName(TEXT("Color"));
}

UTemporaryEffectComponent::UTemporaryEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bTickEvenWhenPaused = false;
}

UTemporaryEffectComponent* UTemporaryEffectComponent::FindOrAdd(AActor* Owner)
{
	if (!IsValid(Owner))
	{
		return nullptr;
	}

	if (UTemporaryEffectComponent* Existing = Owner->FindComponentByClass<UTemporaryEffectComponent>())
	{
		return Existing;
	}

	UTemporaryEffectComponent* Created = NewObject<UTemporaryEffectComponent>(Owner);
	Owner->AddInstanceComponent(Created);
	Created->RegisterComponent();
	return Created;
}

UTemporaryEffect* UTemporaryEffectComponent::ApplyEffect(const UTemporaryEffect* Template)
{
	AActor* Owner = GetOwner();
	if (!Template || !IsValid(Owner))
	{
		return nullptr;
	}

	if (!Template->StackKey.IsNone())
	{
		for (FActiveTemporaryEffect& Active : ActiveEffects)
		{
			if (Active.Effect && Active.Effect->StackKey == Template->StackKey)
			{
				// A permanent match is already applied for good: nothing to refresh, and no new copy to add.
				if (!Active.Effect->bPermanent)
				{
					Active.RemainingSeconds = FMath::Max(Active.RemainingSeconds, Template->Duration);
				}
				return Active.Effect;
			}
		}
	}

	// A private copy: the template (often a default subobject on a totem) is never applied or changed.
	UTemporaryEffect* Effect = DuplicateObject<UTemporaryEffect>(Template, this);
	if (!Effect || !Effect->Apply(Owner))
	{
		return nullptr;
	}

	FActiveTemporaryEffect& Active = ActiveEffects.AddDefaulted_GetRef();
	Active.Effect = Effect;
	Active.RemainingSeconds = Effect->bPermanent ? -1.0f : FMath::Max(Effect->Duration, UE_KINDA_SMALL_NUMBER);
	ShowAura(Active);
	UpdateTickState();

	if (Effect->bPermanent)
	{
		UE_LOG(LogTemp, Log, TEXT("TemporaryEffect: %s permanently gained %s."), *GetNameSafe(Owner), *Effect->DisplayName.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("TemporaryEffect: %s gained %s for %.1f s."), *GetNameSafe(Owner), *Effect->DisplayName.ToString(), Active.RemainingSeconds);
	}
	OnEffectApplied.Broadcast(Effect);
	return Effect;
}

bool UTemporaryEffectComponent::RemoveEffect(UTemporaryEffect* Effect)
{
	const int32 Index = ActiveEffects.IndexOfByPredicate([Effect](const FActiveTemporaryEffect& Active) { return Active.Effect.Get() == Effect; });
	if (!Effect || Index == INDEX_NONE)
	{
		return false;
	}

	EndEffectAt(Index);
	return true;
}

void UTemporaryEffectComponent::RemoveAllEffects()
{
	while (ActiveEffects.Num() > 0)
	{
		EndEffectAt(ActiveEffects.Num() - 1);
	}
}

void UTemporaryEffectComponent::AdvanceEffects(float DeltaSeconds)
{
	for (int32 Index = ActiveEffects.Num() - 1; Index >= 0; --Index)
	{
		if (!ActiveEffects.IsValidIndex(Index))
		{
			continue; // An OnEffectEnded listener removed more than one effect.
		}

		// A permanent effect has no timer: it stays until explicitly removed.
		if (!ActiveEffects[Index].Effect || ActiveEffects[Index].Effect->bPermanent)
		{
			continue;
		}

		ActiveEffects[Index].RemainingSeconds -= DeltaSeconds;
		if (ActiveEffects[Index].RemainingSeconds <= 0.0f)
		{
			EndEffectAt(Index);
		}
	}
}

float UTemporaryEffectComponent::GetRemainingSeconds(const UTemporaryEffect* Effect) const
{
	const FActiveTemporaryEffect* Found = ActiveEffects.FindByPredicate([Effect](const FActiveTemporaryEffect& Active) { return Active.Effect.Get() == Effect; });
	if (!Found)
	{
		return 0.0f;
	}

	return (Found->Effect && Found->Effect->bPermanent) ? -1.0f : FMath::Max(0.0f, Found->RemainingSeconds);
}

void UTemporaryEffectComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AdvanceEffects(DeltaTime);
}

void UTemporaryEffectComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveAllEffects();
	Super::EndPlay(EndPlayReason);
}

void UTemporaryEffectComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	RemoveAllEffects();
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UTemporaryEffectComponent::EndEffectAt(int32 Index)
{
	// Taken out of the list first, so listeners and RemoveEffect see a consistent state.
	FActiveTemporaryEffect Ended = ActiveEffects[Index];
	ActiveEffects.RemoveAt(Index);

	HideAura(Ended);
	UpdateTickState();

	if (Ended.Effect)
	{
		Ended.Effect->Remove();
		UE_LOG(LogTemp, Log, TEXT("TemporaryEffect: %s lost %s."), *GetNameSafe(GetOwner()), *Ended.Effect->DisplayName.ToString());
		OnEffectEnded.Broadcast(Ended.Effect);
	}
}

void UTemporaryEffectComponent::UpdateTickState()
{
	// A permanent effect has nothing to count down, so its presence alone is no reason to keep ticking.
	const bool bAnyTimed = ActiveEffects.ContainsByPredicate([](const FActiveTemporaryEffect& Active) { return Active.Effect && !Active.Effect->bPermanent; });
	SetComponentTickEnabled(bAnyTimed);
}

void UTemporaryEffectComponent::ShowAura(FActiveTemporaryEffect& Active)
{
	AActor* Owner = GetOwner();
	USceneComponent* Root = Owner ? Owner->GetRootComponent() : nullptr;
	if (!Active.Effect || !Active.Effect->bShowAura || !Root)
	{
		return;
	}

	const UTemporaryEffect& Effect = *Active.Effect;

	// For a character the root is the capsule centre; its half-height puts the ring at the feet.
	const double FeetOffset = Root->Bounds.BoxExtent.Z > 0.0 ? Root->Bounds.BoxExtent.Z : 0.0;

	UPointLightComponent* Light = NewObject<UPointLightComponent>(Owner);
	Light->SetupAttachment(Root);
	Light->SetLightColor(Effect.AuraColor);
	Light->SetIntensity(Effect.AuraLightIntensity);
	Light->SetAttenuationRadius(Effect.AuraRadius * 4.0f);
	Light->SetCastShadows(false);
	Light->RegisterComponent();
	Active.AuraLight = Light;

	UStaticMesh* RingMesh = LoadObject<UStaticMesh>(nullptr, AuraRingMeshPath);
	if (RingMesh)
	{
		UStaticMeshComponent* Ring = NewObject<UStaticMeshComponent>(Owner);
		Ring->SetupAttachment(Root);
		Ring->SetStaticMesh(RingMesh);
		Ring->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Ring->SetGenerateOverlapEvents(false);
		Ring->SetCanEverAffectNavigation(false);
		Ring->SetCastShadow(false);
		const double Diameter = 2.0 * Effect.AuraRadius / EngineShapeSize;
		Ring->SetRelativeScale3D(FVector(Diameter, Diameter, AuraRingThickness / EngineShapeSize));
		Ring->SetRelativeLocation(FVector(0.0, 0.0, -FeetOffset + AuraRingThickness));
		if (UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, AuraRingMaterialPath))
		{
			UMaterialInstanceDynamic* Tint = UMaterialInstanceDynamic::Create(Glow, Ring);
			Tint->SetVectorParameterValue(AuraColorParameterName, Effect.AuraColor * AuraRingGlow);
			Ring->SetMaterial(0, Tint);
		}
		Ring->RegisterComponent();
		Active.AuraRing = Ring;
	}

	if (Effect.AuraEffect)
	{
		Active.AuraParticles = UNiagaraFunctionLibrary::SpawnSystemAttached(Effect.AuraEffect, Root, NAME_None,
			FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false);
	}
}

void UTemporaryEffectComponent::HideAura(FActiveTemporaryEffect& Active)
{
	if (Active.AuraLight)
	{
		Active.AuraLight->DestroyComponent();
		Active.AuraLight = nullptr;
	}

	if (Active.AuraRing)
	{
		Active.AuraRing->DestroyComponent();
		Active.AuraRing = nullptr;
	}

	if (Active.AuraParticles)
	{
		Active.AuraParticles->DestroyComponent();
		Active.AuraParticles = nullptr;
	}
}
