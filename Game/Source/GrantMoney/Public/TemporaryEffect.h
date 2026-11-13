// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrantMoneyStatTypes.h"
#include "UObject/Object.h"
#include "TemporaryEffect.generated.h"

class UNiagaraSystem;
class UStatsComponent;

/**
 * The reusable contract for a timed (or, with bPermanent, untimed) buff or debuff (SCRUM-39). An effect
 * only says what it does to its target while it is active; UTemporaryEffectComponent owns the timer (if
 * any), the visible aura and the clean-up, so a new effect type needs nothing from the player, its
 * movement or the camera.
 *
 * To add a new type, derive from this class (C++ or Blueprint) and implement ApplyEffect and
 * RemoveEffect. RemoveEffect must undo everything ApplyEffect did. UStatModifierEffect is the
 * ready-made type: one removable UStatsComponent modifier, so base stats are never written.
 *
 * Effects are configured as templates (for example in ARewardTotem::Effects). A template is never
 * applied itself: UTemporaryEffectComponent::ApplyEffect duplicates it, so each activation is
 * independent and a template can be reused any number of times.
 */
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced, CollapseCategories)
class GRANTMONEY_API UTemporaryEffect : public UObject
{
	GENERATED_BODY()

public:
	// Starts the effect on Target. Returns false (and leaves Target untouched) if the effect is already
	// applied, Target is invalid, or ApplyEffect refuses it.
	bool Apply(AActor* Target);

	// Ends the effect. Safe to call any number of times, and after the target has been destroyed.
	void Remove();

	UFUNCTION(BlueprintPure, Category = "Effect")
	bool IsApplied() const { return bApplied; }

	UFUNCTION(BlueprintPure, Category = "Effect")
	AActor* GetTarget() const { return AppliedTarget.Get(); }

	// How long one application lasts, in seconds. Ignored when bPermanent is set.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect", meta = (ClampMin = "0.05", Units = "s", EditCondition = "!bPermanent"))
	float Duration = 8.0f;

	// On: this effect is never removed by a timer (Duration is ignored) and has no countdown to show;
	// UTemporaryEffectComponent keeps it applied until RemoveEffect/RemoveAllEffects is called explicitly
	// (for example, the player character being destroyed on a run restart). Small, permanent totem
	// rewards use this so a run's choices compound instead of fading; a genuinely temporary buff (a
	// totem could still offer one, or a future timed pickup) leaves this off.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	bool bPermanent = false;

	// Applying an effect while one with the same key is active restarts that one's timer instead of
	// stacking a second copy (or, for a bPermanent effect, is simply a no-op: it is already applied for
	// good). None = every application stacks.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	FName StackKey;

	// Shown in logs (and later in UI).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	FText DisplayName;

	// While active, the target glows: a coloured light and a ring at its feet.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Visual")
	bool bShowAura = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Visual")
	FLinearColor AuraColor = FLinearColor(0.1f, 1.0f, 0.4f);

	// Radius (cm) of the ring drawn at the target's feet.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Visual", meta = (ClampMin = "1.0", Units = "cm"))
	float AuraRadius = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Visual", meta = (ClampMin = "0.0"))
	float AuraLightIntensity = 6000.0f;

	// Optional Niagara effect attached to the target while the effect lasts.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Visual")
	TObjectPtr<UNiagaraSystem> AuraEffect;

protected:
	// What the effect does. Return false to refuse (for example, the target lacks a needed component).
	UFUNCTION(BlueprintNativeEvent, Category = "Effect")
	bool ApplyEffect(AActor* Target);

	// Undo everything ApplyEffect did. Target may be null if it was destroyed first.
	UFUNCTION(BlueprintNativeEvent, Category = "Effect")
	void RemoveEffect(AActor* Target);

	virtual bool ApplyEffect_Implementation(AActor* Target) { return false; }
	virtual void RemoveEffect_Implementation(AActor* Target) {}

private:
	bool bApplied = false;
	TWeakObjectPtr<AActor> AppliedTarget;
};

/**
 * A temporary change to one stat: a single UStatsComponent modifier, added on apply and removed by its
 * handle on remove. Because it never touches the stat's base value, expiry always restores the exact
 * value the stat would have without it, whatever else changed in between (upgrades, other buffs).
 * Example: MoveSpeed, Multiplicative, 0.5 = +50% move speed (the player applies MoveSpeed to its
 * movement component itself, so movement code does not change).
 */
UCLASS(BlueprintType, meta = (DisplayName = "Stat Modifier Effect"))
class GRANTMONEY_API UStatModifierEffect : public UTemporaryEffect
{
	GENERATED_BODY()

public:
	UStatModifierEffect();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Stat")
	EGrantMoneyStat Stat = EGrantMoneyStat::MoveSpeed;

	// Additive = flat amount. Multiplicative = fraction of the stat (0.5 = +50%, -0.25 = -25%).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Stat", meta = (DisplayName = "Modifier Type"))
	EGrantMoneyStatModifierOp ModifierOp = EGrantMoneyStatModifierOp::Multiplicative;

	// How strong the effect is. Zero is refused.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Stat")
	float Magnitude = 0.5f;

	// The live modifier, valid only while applied.
	FGrantMoneyStatModifierHandle GetModifierHandle() const { return ModifierHandle; }

protected:
	virtual bool ApplyEffect_Implementation(AActor* Target) override;
	virtual void RemoveEffect_Implementation(AActor* Target) override;

private:
	TWeakObjectPtr<UStatsComponent> ModifiedStats;
	FGrantMoneyStatModifierHandle ModifierHandle;
};
