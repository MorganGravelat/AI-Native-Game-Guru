// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ContactDamageComponent.generated.h"

class UStatsComponent;

/**
 * Damages a target its owner is touching, at most once per attack interval.
 * "Touching" is measured on the ground plane between the two actors' edges
 * (collision radii), not with overlap events, because enemies stop just short
 * of the player and their capsules never actually meet.
 *
 * Damage and time between hits come from the owner's ContactDamage and
 * AttackInterval stats (UStatsComponent). If the owner has no stats component, or
 * the stat has no base value, FallbackDamage / FallbackInterval are used.
 * Damage is sent through UGameplayStatics::ApplyDamage,
 * so the target only needs something that handles Unreal damage, such as a
 * UHealthComponent. The owner decides when to try (e.g. every tick) and who the target is.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class GRANTMONEY_API UContactDamageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UContactDamageComponent();

	// Damages Target if it is within Reach and GetInterval() has passed since this component's last hit.
	// Returns true if damage was sent.
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool TryApplyContactDamage(AActor* Target);

	// True when the gap between the owner's and Target's edges on the ground plane is at most Reach.
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsInReach(const AActor* Target) const;

	// Damage per hit: the ContactDamage stat, or FallbackDamage.
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetDamage() const;

	// Seconds between hits: the AttackInterval stat, or FallbackInterval.
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetInterval() const;

	// Largest gap between the two actors' edges that still counts as touching.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.0", Units = "cm"))
	float Reach = 40.0f;

	// Seconds between hits when the owner has no AttackInterval stat.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.0", Units = "s"))
	float FallbackInterval = 1.0f;

	// Damage per hit when the owner has no ContactDamage stat.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.0"))
	float FallbackDamage = 10.0f;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UStatsComponent> Stats;

	// World time of the last hit; unset until the first hit.
	TOptional<double> LastHitTime;
};
