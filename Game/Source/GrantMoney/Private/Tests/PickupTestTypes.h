// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrantMoneyStatTypes.h"
#include "PickupBase.h"
#include "StatsComponent.h"
#include "PickupTestTypes.generated.h"

/**
 * Test helper only. A stand-in for a future power-up: it counts its collections and applies a
 * stat modifier through the same OnCollected hook the XP orb uses. It exists to show that a new
 * pickup type needs nothing from UPickupComponent or the player.
 */
UCLASS(NotBlueprintable, HideDropdown)
class ATestBuffPickup : public APickupBase
{
	GENERATED_BODY()

public:
	// Counted across all instances, because a collected pickup is destroyed. Tests reset it first.
	static inline int32 TotalCollections = 0;
	static inline TWeakObjectPtr<AActor> LastCollector;

	// Added to the collector's Damage stat as a flat modifier when collected.
	float DamageBonus = 5.0f;

protected:
	virtual void OnCollected(AActor* Collector) override
	{
		++TotalCollections;
		LastCollector = Collector;

		if (UStatsComponent* Stats = Collector->FindComponentByClass<UStatsComponent>())
		{
			Stats->AddModifier(EGrantMoneyStat::Damage, EGrantMoneyStatModifierOp::Additive, DamageBonus);
		}
	}
};
