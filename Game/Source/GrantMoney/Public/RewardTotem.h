// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RewardTotem.generated.h"

class APawn;
class ARewardTotem;
class UAudioComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UNiagaraSystem;
class USoundBase;
class UStaticMesh;
class UStaticMeshComponent;
class UTemporaryEffect;

UENUM(BlueprintType)
enum class ERewardTotemState : uint8
{
	// Waiting for the player, or charging while the player stands in range.
	Ready,
	// Used, and will be Ready again after CooldownSeconds (only when bSingleUse is off).
	Cooldown,
	// Used for good.
	Spent,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRewardTotemActivated, ARewardTotem*, Totem, AActor*, Activator);

/**
 * A world reward (SCRUM-39). While the player stands within ActivationRadius, a bar above the totem fills
 * over ChargeSeconds (it drains again if they leave). When the bar is full the totem gives the player a
 * copy of every effect in Effects, through the player's UTemporaryEffectComponent.
 *
 * The rewards are data: Effects holds any UTemporaryEffect subclasses, each with its own duration and
 * strength, edited inline in the Details panel. The default is a small, permanent +10% move speed. A new
 * reward type is a new UTemporaryEffect subclass; neither this class nor the player changes.
 *
 * Built from engine shapes, so it needs no assets: a pillar, a glowing disc showing the activation range,
 * and a bar that turns to face the camera. ARewardTotemSpawner places totems on the map at level start.
 */
UCLASS(Blueprintable)
class GRANTMONEY_API ARewardTotem : public AActor
{
	GENERATED_BODY()

public:
	ARewardTotem();

	virtual void Tick(float DeltaSeconds) override;

	// Fills or drains the bar for DeltaSeconds and activates when it is full. Called by Tick.
	void AdvanceCharge(float DeltaSeconds);

	// Gives Activator the rewards now, regardless of charge. Returns how many effects were applied;
	// if none could be (e.g. Activator has no Stats component), the totem is not used up.
	UFUNCTION(BlueprintCallable, Category = "Totem")
	int32 Activate(AActor* Activator);

	// The closest ActivatorClass pawn within ActivationRadius (ground distance), or null.
	APawn* FindActivator() const;

	// 0 = empty, 1 = full.
	UFUNCTION(BlueprintPure, Category = "Totem")
	float GetChargeProgress() const { return ChargeProgress; }

	UFUNCTION(BlueprintPure, Category = "Totem")
	ERewardTotemState GetState() const { return State; }

	// The rewards. Each is copied onto the activator; these templates are never changed.
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Totem|Reward")
	TArray<TObjectPtr<UTemporaryEffect>> Effects;

	// Who can charge the totem. Defaults to the player character, so enemies standing nearby do nothing.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totem|Activation")
	TSubclassOf<APawn> ActivatorClass;

	// How close (cm, ground distance from the totem's centre) the player must stand. The glowing disc shows it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totem|Activation", meta = (ClampMin = "1.0", Units = "cm"))
	float ActivationRadius = 250.0f;

	// Seconds of standing in range to fill the bar.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totem|Activation", meta = (ClampMin = "0.05", Units = "s"))
	float ChargeSeconds = 3.0f;

	// How fast the bar empties when nobody is in range, relative to filling. 1 = same speed, 0 = keeps its progress.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totem|Activation", meta = (ClampMin = "0.0"))
	float DrainRate = 1.0f;

	// On: one use per run. Off: usable again after CooldownSeconds.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totem|Activation")
	bool bSingleUse = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totem|Activation", meta = (ClampMin = "0.0", Units = "s", EditCondition = "!bSingleUse"))
	float CooldownSeconds = 30.0f;

	// Replaces the default pillar. Its bottom is placed on the ground. Null = the glowing pillar.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totem|Visual")
	TObjectPtr<UStaticMesh> CustomMesh;

	// Put on every material slot of the totem. Null = CustomMesh keeps its own materials.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totem|Visual")
	TObjectPtr<UMaterialInterface> CustomMaterial;

	// Scale of CustomMesh.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totem|Visual")
	FVector CustomMeshScale = FVector::OneVector;

	// On: the default pillar glows ReadyColor/SpentColor, replacing its material. Ignored (no tint) when
	// CustomMesh or CustomMaterial is set, so your own look is kept. The bar and range disc always glow.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totem|Visual")
	bool bTintTotemMesh = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totem|Visual")
	FLinearColor ReadyColor = FLinearColor(0.2f, 1.5f, 0.6f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totem|Visual")
	FLinearColor SpentColor = FLinearColor(0.05f, 0.05f, 0.05f);

	// Colour of the filled part of the bar. BarMaterial is Opaque and Lit, not an HDR glow, so this is a
	// plain 0..1 colour: a bold, clearly-visible red against BarBackgroundColor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totem|Visual")
	FLinearColor BarColor = FLinearColor(0.85f, 0.05f, 0.05f);

	// Colour of the unfilled part of the bar, behind BarColor. Default is white, so the red fill reads
	// clearly against it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totem|Visual")
	FLinearColor BarBackgroundColor = FLinearColor::White;

	// Height (cm) of the bar's centre above the totem's base.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totem|Visual", meta = (Units = "cm"))
	float BarHeight = 230.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totem|Visual", meta = (ClampMin = "1.0", Units = "cm"))
	float BarLength = 120.0f;

	// Played once on activation (2D, full volume wherever the camera is).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totem|Feedback")
	TObjectPtr<USoundBase> ActivationSound;

	// The sound of the bar charging, which should be as long as ChargeSeconds (3 s by default): the sound's
	// position follows the bar. It plays while the player stands in range. When the player steps off it stops, and
	// the bar drains; when they step back on it starts again at the point that matches where the bar now is (the
	// bar at 1 s of 3 resumes the sound from its 1 s mark), and a bar that drained to empty restarts the sound from the
	// beginning. Needs no Looping. Empty for silence.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totem|Feedback")
	TObjectPtr<USoundBase> ChargeSound;

	// Seconds into ChargeSound that match a bar fill (0 to 1): the fill times ChargeSeconds.
	static float ComputeChargeSoundTime(float Progress, float ChargeSeconds);

	// Optional Niagara burst at the totem on activation.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Totem|Feedback")
	TObjectPtr<UNiagaraSystem> ActivationEffect;

	UPROPERTY(BlueprintAssignable, Category = "Totem")
	FOnRewardTotemActivated OnActivated;

	// True while the charge bar is drawn (hidden when empty and once spent).
	bool IsBarVisible() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Base of the totem, at ground level.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Totem")
	TObjectPtr<USceneComponent> SceneRoot;

	// The pillar. Blocks pawns, so nothing walks through it.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Totem")
	TObjectPtr<UStaticMeshComponent> TotemMesh;

	// Flat glowing disc on the ground showing ActivationRadius. No collision.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Totem")
	TObjectPtr<UStaticMeshComponent> RangeDisc;

	// Turned to face the camera; holds the bar.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Totem")
	TObjectPtr<USceneComponent> BarPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Totem")
	TObjectPtr<UStaticMeshComponent> BarBackground;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Totem")
	TObjectPtr<UStaticMeshComponent> BarFill;

	UPROPERTY(EditAnywhere, Category = "Totem|Visual")
	TObjectPtr<UMaterialInterface> GlowMaterial;

	// Separate material for the bar's two meshes. GlowMaterial (EmissiveMeshMaterial) is Additive blend,
	// meant for bright overlay glows: it never occludes what is behind it, so two Additive segments next
	// to each other both show through to the sky/background instead of reading as solid blocks, which is
	// why the bar used to look like a washed-out white blob rather than a two-tone progress bar. This one
	// (BasicShapeMaterial, the engine's own "give a basic shape a flat colour" material) is Opaque, so the
	// fill and background actually occlude each other and whatever is behind the totem.
	UPROPERTY(EditAnywhere, Category = "Totem|Visual")
	TObjectPtr<UMaterialInterface> BarMaterial;

private:
	// Swaps in CustomMesh/CustomMaterial. Returns true if the totem now has a custom look.
	bool ApplyCustomLook();
	void SetState(ERewardTotemState NewState);
	void UpdateBar();
	void UpdateChargeSound();
	void FaceBarToCamera();
	void ApplyColors();

	// Tints Mesh with Color using GlowMaterial (the additive pillar/disc glow).
	UMaterialInstanceDynamic* MakeGlow(UStaticMeshComponent* Mesh, const FLinearColor& Color);

	// Tints Mesh with Color using BarMaterial (opaque, for the two bar segments).
	UMaterialInstanceDynamic* MakeBarSegment(UStaticMeshComponent* Mesh, const FLinearColor& Color);

	float ChargeProgress = 0.0f;
	float CooldownRemaining = 0.0f;
	ERewardTotemState State = ERewardTotemState::Ready;

	// True while the player is in range and the bar is filling. Drives the charge sound and the music duck.
	bool bFilling = false;

	// True while the charge sound and the music duck are held for a filling bar.
	bool bHoldingChargeFeedback = false;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> TotemGlow;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DiscGlow;

	// The playing charge sound, if any. Spawned when charging starts and fades out when it stops.
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ChargeAudio;
};
