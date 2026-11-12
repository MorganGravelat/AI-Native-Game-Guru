// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GrantMoneyStatTypes.h"
#include "PickupBase.h"
#include "StatBoostPickup.generated.h"

class UMaterialBillboardComponent;
class UMaterialInterface;
class UNiagaraSystem;
class USoundBase;
class UTexture2D;

/**
 * A rare drop that permanently boosts one of the collector's stats (SCRUM-53).
 *
 * It uses the existing pickup pipeline unchanged: APickupBase flies it to the player and calls
 * OnCollected once, and the pickup is then removed. It is consumed instantly; there is no inventory.
 * The boost is one permanent UStatsComponent modifier (never removed), so the systems that already
 * read stats react on their own:
 *  - MaxHealth      -> UHealthComponent (the player's max health rises; current health does not)
 *  - Damage         -> UStarterWeaponComponent, damage per magic orb
 *  - AttackInterval -> UStarterWeaponComponent, time between shots (use a negative Amount to attack faster)
 *  - Penetration    -> UStarterWeaponComponent, extra enemies each orb passes through
 *  - ContactDamage  -> UContactDamageComponent (the player does not have one yet; see AContactDamageBoostPickup)
 *
 * Everything is editable in the Details panel of a Blueprint subclass (or of a placed instance):
 * the stat, Flat vs Percent, the amount, the 2D icon, the sound and the effect. The C++ subclasses
 * below are ready-made defaults for each stat. Drop chances live on the enemy, in
 * AEnemyBase::BonusPickupDrops (slimes add these with a rare chance; see ASlimeEnemy).
 *
 * Visual: with an Icon set, the pickup shows that 2D image on a billboard that always faces the
 * camera, and hides the placeholder sphere. Without an Icon it is a glowing sphere tinted GlowColor.
 */
UCLASS()
class GRANTMONEY_API AStatBoostPickup : public APickupBase
{
	GENERATED_BODY()

public:
	AStatBoostPickup();

	// Adds this pickup's permanent modifier to Collector's UStatsComponent. Returns false and changes
	// nothing if Collector has no stats component or Amount is zero. Called once by OnCollected.
	bool ApplyBoost(AActor* Collector);

	// True once BeginPlay has put Icon on the camera-facing billboard (and hidden the sphere).
	bool IsShowingIcon() const;

	// The stat this pickup raises.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Stat Boost")
	EGrantMoneyStat Stat = EGrantMoneyStat::Damage;

	// Additive = flat amount (+10 max health). Multiplicative = percent of the stat as a fraction
	// (0.1 = +10%, -0.05 = -5%). Percent boosts on the same stat add together before being applied.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Stat Boost", meta = (DisplayName = "Modifier Type"))
	EGrantMoneyStatModifierOp ModifierOp = EGrantMoneyStatModifierOp::Additive;

	// How much one pickup adds. For AttackInterval, lower is better, so use a negative value to attack faster.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Stat Boost")
	float Amount = 1.0f;

	// 2D image shown on a billboard that always faces the camera. Leave empty for the glowing sphere.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Visual")
	TObjectPtr<UTexture2D> Icon;

	// Width and height (cm) of the icon billboard in the world.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Visual", meta = (ClampMin = "1.0", Units = "cm"))
	float IconSize = 60.0f;

	// Material that draws Icon. It needs a texture parameter named IconTextureParameterName. The default
	// is Paper2D's masked unlit sprite material, so a PNG with transparency shows only its shape.
	UPROPERTY(EditAnywhere, Category = "Pickup|Visual")
	TSoftObjectPtr<UMaterialInterface> IconMaterial;

	// Texture parameter on IconMaterial that receives Icon. Paper2D's sprite materials call it SpriteTexture.
	UPROPERTY(EditAnywhere, Category = "Pickup|Visual")
	FName IconTextureParameterName = TEXT("SpriteTexture");

	// Material for the placeholder sphere (used when there is no Icon). Tinted with GlowColor.
	UPROPERTY(EditAnywhere, Category = "Pickup|Visual")
	TObjectPtr<UMaterialInterface> GlowMaterial;

	// Emissive tint of the placeholder sphere; values above 1 glow brighter.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Visual")
	FLinearColor GlowColor = FLinearColor(3.0f, 3.0f, 3.0f);

	// Name of the colour parameter on GlowMaterial. The engine's emissive material calls it EmissiveColor.
	UPROPERTY(EditAnywhere, Category = "Pickup|Visual")
	FName GlowParameterName = TEXT("EmissiveColor");

	// Played once when the boost is applied (2D, so it is heard at full volume wherever the camera is).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Feedback")
	TObjectPtr<USoundBase> PickupSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Feedback", meta = (ClampMin = "0.0"))
	float PickupSoundVolume = 1.0f;

	// Optional Niagara effect spawned on the collector when the boost is applied. It follows the collector.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Feedback")
	TObjectPtr<UNiagaraSystem> PickupEffect;

protected:
	virtual void BeginPlay() override;
	virtual void OnCollected(AActor* Collector) override;

	// Sound and effect for a boost that was just applied. Override to add more feedback.
	virtual void PlayPickupFeedback(AActor* Collector);

	// Shows Icon when one is set. Visual only, no collision.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UMaterialBillboardComponent> IconBillboard;

private:
	// Puts Icon on IconBillboard. Returns false if there is no Icon or IconMaterial cannot be loaded.
	bool ShowIcon();

	// Tints the placeholder sphere with GlowColor.
	void ApplyGlow();
};

/** +10 max health per pickup. Raises only the maximum: the player's current health is unchanged. */
UCLASS()
class GRANTMONEY_API AMaxHealthBoostPickup : public AStatBoostPickup
{
	GENERATED_BODY()

public:
	AMaxHealthBoostPickup();
};

/** +2 damage per magic orb, per pickup. */
UCLASS()
class GRANTMONEY_API ADamageBoostPickup : public AStatBoostPickup
{
	GENERATED_BODY()

public:
	ADamageBoostPickup();
};

/** Attack interval -5% per pickup (the weapon fires more often). Stacks additively; never below 0.05 s. */
UCLASS()
class GRANTMONEY_API AAttackSpeedBoostPickup : public AStatBoostPickup
{
	GENERATED_BODY()

public:
	AAttackSpeedBoostPickup();
};

/** +1 penetration per pickup: each magic orb can pass through one more enemy. */
UCLASS()
class GRANTMONEY_API APenetrationBoostPickup : public AStatBoostPickup
{
	GENERATED_BODY()

public:
	APenetrationBoostPickup();
};

/**
 * +2 contact damage per pickup. Note: the player has no UContactDamageComponent today, so this stat
 * changes but nothing reads it on the player yet. It is not in the slime's default drop list for that
 * reason. Add it to BonusPickupDrops once the player deals contact damage.
 */
UCLASS()
class GRANTMONEY_API AContactDamageBoostPickup : public AStatBoostPickup
{
	GENERATED_BODY()

public:
	AContactDamageBoostPickup();
};
