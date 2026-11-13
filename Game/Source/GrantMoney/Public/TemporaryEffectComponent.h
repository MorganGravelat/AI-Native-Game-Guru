// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TemporaryEffectComponent.generated.h"

class UNiagaraComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UTemporaryEffect;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTemporaryEffectEvent, UTemporaryEffect*, Effect);

/** One running effect and the aura that shows it. */
USTRUCT()
struct GRANTMONEY_API FActiveTemporaryEffect
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UTemporaryEffect> Effect;

	// -1 for a permanent effect (Effect->bPermanent): nothing to count down, and GetRemainingSeconds
	// reports it as such instead of a real number.
	float RemainingSeconds = 0.0f;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> AuraLight;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> AuraRing;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> AuraParticles;
};

/**
 * Runs an actor's temporary effects (SCRUM-39): starts each one, counts down its Duration, shows its
 * aura, and removes it when the time is up. Every timed effect still running is removed when this
 * component or its owner goes away, so a buff can never outlive its timer. A bPermanent effect (see
 * UTemporaryEffect) is never counted down or auto-removed by this component; it is removed only by an
 * explicit RemoveEffect/RemoveAllEffects call, or implicitly when the owner is destroyed (a run restart).
 *
 * Nothing needs to add this component ahead of time: FindOrAdd creates it on the target the first time
 * an effect is applied (ARewardTotem does this), so the player character does not change. Ticks only
 * while a timed effect is running (a permanent one alone needs no tick), and pauses with the game, so a
 * buff does not drain on the level-up screen.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class GRANTMONEY_API UTemporaryEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTemporaryEffectComponent();

	// Owner's existing component, or a new registered one. Null if Owner is invalid.
	static UTemporaryEffectComponent* FindOrAdd(AActor* Owner);

	// Starts a copy of Template on the owner. Template itself is not changed. If an effect with the same
	// non-None StackKey is running, a timed one's timer restarts instead and that effect is returned; a
	// permanent one is simply returned unchanged, since it is already applied for good.
	// Returns null if the copy refused to apply (e.g. the owner has no Stats component).
	UFUNCTION(BlueprintCallable, Category = "Effects")
	UTemporaryEffect* ApplyEffect(const UTemporaryEffect* Template);

	// Ends one running effect early. Returns false if it is not running here.
	UFUNCTION(BlueprintCallable, Category = "Effects")
	bool RemoveEffect(UTemporaryEffect* Effect);

	UFUNCTION(BlueprintCallable, Category = "Effects")
	void RemoveAllEffects();

	// Counts timers down by DeltaSeconds and removes expired effects. Permanent effects are skipped
	// entirely: they have no timer to advance and never expire this way. Called by TickComponent.
	void AdvanceEffects(float DeltaSeconds);

	UFUNCTION(BlueprintPure, Category = "Effects")
	int32 GetActiveEffectCount() const { return ActiveEffects.Num(); }

	// Seconds left on Effect, 0 if it is not running here, or -1 if it is running but permanent (no
	// countdown to report).
	UFUNCTION(BlueprintPure, Category = "Effects")
	float GetRemainingSeconds(const UTemporaryEffect* Effect) const;

	const TArray<FActiveTemporaryEffect>& GetActiveEffects() const { return ActiveEffects; }

	UPROPERTY(BlueprintAssignable, Category = "Effects")
	FOnTemporaryEffectEvent OnEffectApplied;

	// Fires when an effect ends for any reason (timer, early removal, owner destroyed).
	UPROPERTY(BlueprintAssignable, Category = "Effects")
	FOnTemporaryEffectEvent OnEffectEnded;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

private:
	void ShowAura(FActiveTemporaryEffect& Active);
	static void HideAura(FActiveTemporaryEffect& Active);

	// Removes ActiveEffects[Index], undoes the effect, hides its aura and broadcasts OnEffectEnded.
	void EndEffectAt(int32 Index);

	// True if any active effect is timed (not bPermanent): that is the only reason to keep ticking.
	void UpdateTickState();

	UPROPERTY(Transient)
	TArray<FActiveTemporaryEffect> ActiveEffects;
};
