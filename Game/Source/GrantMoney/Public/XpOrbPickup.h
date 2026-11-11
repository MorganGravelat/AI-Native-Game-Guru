// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PickupBase.h"
#include "XpOrbPickup.generated.h"

class UMaterialInterface;

/**
 * Small glowing green orb dropped by enemies. It flies to the player when they come close (all of
 * that is APickupBase) and on arrival adds XpValue to the collector's Experience stat, a running total.
 * It works for any collector that has a UStatsComponent. Leveling, UI and upgrades are separate work.
 *
 * The look is a placeholder: an engine sphere with an emissive material. To use a real asset, set
 * PickupMesh's mesh and GlowMaterial on a Blueprint subclass; gameplay does not depend on either.
 */
UCLASS()
class GRANTMONEY_API AXpOrbPickup : public APickupBase
{
	GENERATED_BODY()

public:
	AXpOrbPickup();

	float GetXpValue() const { return XpValue; }

	// XP added to the collector's total when this orb arrives.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|XP", meta = (ClampMin = "0.0"))
	float XpValue = 5.0f;

protected:
	virtual void BeginPlay() override;
	virtual void OnCollected(AActor* Collector) override;

	// Material for the orb. Replace it to change the look; BeginPlay tints it with GlowColor if it
	// has a vector parameter named GlowParameterName.
	UPROPERTY(EditAnywhere, Category = "Pickup|Visual")
	TObjectPtr<UMaterialInterface> GlowMaterial;

	// Emissive tint; values above 1 glow brighter.
	UPROPERTY(EditAnywhere, Category = "Pickup|Visual")
	FLinearColor GlowColor = FLinearColor(0.2f, 3.0f, 0.3f);

	// Name of the colour parameter on GlowMaterial. The engine's emissive material calls it EmissiveColor.
	UPROPERTY(EditAnywhere, Category = "Pickup|Visual")
	FName GlowParameterName = TEXT("EmissiveColor");
};
