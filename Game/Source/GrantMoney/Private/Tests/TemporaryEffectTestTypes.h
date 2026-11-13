// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TemporaryEffect.h"
#include "TemporaryEffectTestTypes.generated.h"

/**
 * Test helper only. A new kind of temporary reward that touches no stat: it just counts how often it is
 * applied and removed. It exists to show that a new effect type plugs into UTemporaryEffectComponent and
 * ARewardTotem with no change to them or to the player.
 */
UCLASS(NotBlueprintable, HideDropdown)
class UTestCountingEffect : public UTemporaryEffect
{
	GENERATED_BODY()

public:
	// Counted across all copies, because the component applies a duplicate. Tests reset them first.
	static inline int32 Applies = 0;
	static inline int32 Removes = 0;

	static void ResetCounts() { Applies = 0; Removes = 0; }

	// Copied with the template, so a test can make the applied copy refuse.
	UPROPERTY()
	bool bRefuse = false;

protected:
	virtual bool ApplyEffect_Implementation(AActor* Target) override
	{
		if (bRefuse)
		{
			return false;
		}
		++Applies;
		return true;
	}

	virtual void RemoveEffect_Implementation(AActor* Target) override
	{
		++Removes;
	}
};
