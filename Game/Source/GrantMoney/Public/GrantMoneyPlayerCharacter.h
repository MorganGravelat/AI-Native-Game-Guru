// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GrantMoneyStatTypes.h"
#include "GrantMoneyPlayerCharacter.generated.h"

class UAnimationAsset;
class UInputAction;
class UGrantMoneyCharacterVisualData;
class UHealthComponent;
class ULevelComponent;
class UPickupComponent;
class UStarterWeaponComponent;
class UStatsComponent;
class UTemporaryBuffComponent;
class UUpgradeComponent;
struct FInputActionValue;

/**
 * The player's playable character. Uses the standard ACharacter capsule and
 * CharacterMovementComponent. Moves relative to the active camera's yaw.
 * Looks like Epic's Manny (SKM_Manny_Simple) and switches between an idle and
 * a jog animation based on ground speed. A UGrantMoneyCharacterVisualData asset
 * can override the mesh, animation and placement without recompiling. Health
 * comes from a UHealthComponent backed by a UStatsComponent. It starts every run
 * with a UStarterWeaponComponent that fires at nearby enemies by itself, and a UPickupComponent that
 * draws in nearby pickups such as XP orbs. XP is the Experience stat, turned into a level and upgrade
 * choices by ULevelComponent and UUpgradeComponent (SCRUM-36): see HandleLevelGained,
 * HandleUpgradeChoiceReady and HandleUpgradeChoicesResolved, which route those events to the local
 * AMapCameraPlayerController to pause the game and show the choice. The view comes from
 * the map camera, not this class. Reaching zero health ends the run as a loss (SCRUM-44): see
 * HandleHealthDepleted, which asks the level's ARunController to transition to Lost. The MoveSpeed stat
 * drives GetCharacterMovement()->MaxWalkSpeed directly (see HandleStatChanged), since the movement
 * component reads its own property every tick rather than asking Stats for it.
 */
UCLASS()
class GRANTMONEY_API AGrantMoneyPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGrantMoneyPlayerCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UStatsComponent* GetStats() const { return Stats; }
	UHealthComponent* GetHealth() const { return Health; }
	UStarterWeaponComponent* GetStarterWeapon() const { return StarterWeapon; }
	UPickupComponent* GetPickups() const { return Pickups; }
	ULevelComponent* GetLevelComponent() const { return LevelComponent; }
	UUpgradeComponent* GetUpgradeComponent() const { return UpgradeComponent; }
	UTemporaryBuffComponent* GetTemporaryBuffs() const { return TemporaryBuffs; }
	// Running XP total (the Experience stat). Orbs add to it when they arrive.
	float GetExperience() const;
protected:
	// Max health and future player stats. Buffs and upgrades add modifiers here.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UStatsComponent> Stats;

	// Current health; reads MaxHealth from Stats.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UHealthComponent> Health;

	// The weapon the player starts with. Fires automatically; tune damage, interval, range and speed on it.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UStarterWeaponComponent> StarterWeapon;

	// Detects pickups within its radius and draws them in. Tune the radius on it.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UPickupComponent> Pickups;

	// Turns the Experience stat into a level and progress toward the next one (SCRUM-36).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	TObjectPtr<ULevelComponent> LevelComponent;

	// Queues level-ups into upgrade choices and applies the one picked (SCRUM-36).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	TObjectPtr<UUpgradeComponent> UpgradeComponent;

	// Timed stat buffs (they remove themselves); the HUD lists them with their remaining time.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	TObjectPtr<UTemporaryBuffComponent> TemporaryBuffs;

	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	void HandleMove(const FInputActionValue& Value);

	// Optional override of the built-in Manny defaults. Defaults to
	// /Game/Characters/Player/DA_PlayerVisual; if that asset doesn't exist the
	// Manny defaults set in the constructor are used.
	UPROPERTY(EditAnywhere, Category = "Visual")
	TSoftObjectPtr<UGrantMoneyCharacterVisualData> VisualData;

	// Played while standing still.
	UPROPERTY(EditAnywhere, Category = "Animation")
	TObjectPtr<UAnimationAsset> IdleAnimation;

	// Played while moving.
	UPROPERTY(EditAnywhere, Category = "Animation")
	TObjectPtr<UAnimationAsset> JogAnimation;

	// Ground speed (cm/s) above which the jog animation plays.
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (ClampMin = "0"))
	float JogSpeedThreshold = 5.0f;

	// Defaults to /Game/Input/IA_CameraMove (Axis2D: X right, Y forward). Must be
	// mapped in the mapping context the PlayerController adds.
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

private:
	void ApplyVisualData();
	void PlayLocomotionAnimation(UAnimationAsset* Animation);

	// Bound to Health->OnDeath. Ends the run as a loss; does nothing if no ARunController is found
	// (for example a test world with no run set up) or the run cannot be lost from its current state.
	UFUNCTION()
	void HandleHealthDepleted(UHealthComponent* DepletedHealth, AActor* DamageCauser);

	// Bound to LevelComponent->OnLevelUp. Forwards to UpgradeComponent->NotifyLevelGained.
	UFUNCTION()
	void HandleLevelGained(int32 NewLevel, int32 PreviousLevel);

	// Bound to UpgradeComponent->OnChoiceReady. Tells the local player controller to pause and show (or
	// refresh, if already showing) the level-up choice screen.
	UFUNCTION()
	void HandleUpgradeChoiceReady();

	// Bound to UpgradeComponent->OnChoicesResolved. Tells the local player controller to hide the
	// choice screen and unpause.
	UFUNCTION()
	void HandleUpgradeChoicesResolved();

	// Bound to Stats->OnStatChanged. Keeps GetCharacterMovement()->MaxWalkSpeed in sync with the
	// MoveSpeed stat, so a Move Speed upgrade (or any other MoveSpeed modifier) takes effect immediately.
	UFUNCTION()
	void HandleStatChanged(EGrantMoneyStat Stat, float OldValue, float NewValue);

	// The animation currently playing from the built-in idle/jog logic.
	UPROPERTY(Transient)
	TObjectPtr<UAnimationAsset> CurrentAnimation;

	// False when a data asset supplies its own animation setup.
	bool bUseBuiltInLocomotion = true;
};
