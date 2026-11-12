// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrantMoneyStatTypes.h"
#include "HealthComponent.generated.h"

class AController;
class UDamageType;
class UHealthComponent;
class UStatsComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGrantMoneyHealthChanged, UHealthComponent*, HealthComponent, float, NewHealth, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGrantMoneyDeath, UHealthComponent*, HealthComponent, AActor*, DamageCauser);

/**
 * Current health for any actor that can be hurt (player, enemies, later bosses).
 * Max health is not stored here: it is the MaxHealth stat from a UStatsComponent
 * on the same actor, so health buffs work through stat modifiers. Without a
 * UStatsComponent, FallbackMaxHealth is used.
 *
 * Receives damage from Unreal's damage pipeline: anything that calls
 * UGameplayStatics::ApplyDamage (or AActor::TakeDamage) on the owner reaches
 * ApplyDamage, so damage dealers never need to know this class.
 *
 * Only tracks health and fires events. It does not destroy the owner, play
 * effects or award XP; listeners to OnDeath do that.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class GRANTMONEY_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	// Lowers health by Amount, at most down to zero. Returns the health actually removed.
	// Ignored (returns 0) when dead or when Amount is not positive.
	UFUNCTION(BlueprintCallable, Category = "Health")
	float ApplyDamage(float Amount, AActor* DamageCauser = nullptr);

	// Raises health by Amount, at most up to max health. Returns the health actually added.
	// Ignored (returns 0) when dead or when Amount is not positive.
	UFUNCTION(BlueprintCallable, Category = "Health")
	float Heal(float Amount);

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const;

	// Health from 0 to 1, for health bars. 0 when max health is 0.
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthFraction() const;

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return bIsDead; }

	// Fires after every change to current health, including max-health changes that move it.
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnGrantMoneyHealthChanged OnHealthChanged;

	// Fires once, when health first reaches zero. Fires after OnHealthChanged.
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnGrantMoneyDeath OnDeath;

	// When max health rises (e.g. a max-health stat boost), also heal by the amount gained.
	// Off means a boost raises only the ceiling and current health stays where it was.
	// A loss of max health always just clamps current health, whatever this is set to.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	bool bHealOnMaxHealthGain = true;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Max health when the owner has no UStatsComponent.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0.0"))
	float FallbackMaxHealth = 100.0f;

private:
	// Bound to the owner's OnTakeAnyDamage.
	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
	void HandleStatChanged(EGrantMoneyStat Stat, float OldValue, float NewValue);

	void SetHealth(float NewHealth);

	UPROPERTY(Transient)
	TObjectPtr<UStatsComponent> Stats;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Health")
	float CurrentHealth = 0.0f;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Health")
	bool bIsDead = false;
};
