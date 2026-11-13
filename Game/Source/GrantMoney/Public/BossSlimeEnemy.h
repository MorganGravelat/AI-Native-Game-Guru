// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BossLunge.h"
#include "EnemySpawnPlacement.h"
#include "SlimeEnemy.h"
#include "BossSlimeEnemy.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;

/**
 * The prototype boss: a large, differently coloured slime with one extra behaviour, a telegraphed lunge.
 * Health, contact damage (and its hit cadence), targeting, death, drops, emergence and walking all come from
 * AEnemyBase / ASlimeEnemy; this class only supplies boss defaults and the lunge. Make a Blueprint child to
 * retune any of it, or point ABossEncounter::BossClass at a different class.
 *
 * Lunge (see FBossLungeState for the phases): while the target is within Lunge.TriggerRange the boss stops,
 * tints toward TelegraphColor, swells and draws a red line along the path for Lunge.WindupSeconds, then
 * charges that line at Lunge.Speed for up to Lunge.DurationSeconds. That is one dash of a combo of
 * LungeComboCount: after each dash it stops, re-aims at where the target is now, telegraphs again for
 * LungeComboWindupSeconds and dashes again. After the last dash it chases normally until
 * Lunge.CooldownSeconds have passed. Each dash's direction is fixed when its windup starts, so the player can
 * still step aside; LungeLeadStrength blends that direction between aiming straight at the target (0) and
 * leading it based on its current velocity (1, the full FBossLungeMath::ComputeInterceptDirection solve for
 * where it will be once the dash actually launches) — the default (0.6) leads enough to punish running in a
 * straight line without being unavoidable. A stationary or outrunning target just gets the straight-line aim.
 * Every dash is cut short so that its footprint never leaves the playable map cells (FBossLungeMath::ComputeSafeDistance), a combo ends early
 * if there is no room for the next dash, and if no map is available the boss does not lunge. Contact damage
 * during a lunge is the normal contact damage, so hits still obey AttackInterval.
 */
UCLASS()
class GRANTMONEY_API ABossSlimeEnemy : public ASlimeEnemy
{
	GENERATED_BODY()

public:
	ABossSlimeEnemy();

	virtual void Tick(float DeltaTime) override;

	// Timing, speed and trigger range of the lunge. Editable in the Details panel on a placed or Blueprint boss.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Lunge", meta = (ClampMin = "0.0", Units = "s"))
	float LungeWindupSeconds = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Lunge", meta = (ClampMin = "0.0", Units = "cm/s"))
	float LungeSpeed = 1400.0f;

	// Longest charge. It is shortened when the playable area ends sooner.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Lunge", meta = (ClampMin = "0.0", Units = "s"))
	float LungeDurationSeconds = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Lunge", meta = (ClampMin = "0.0", Units = "s"))
	float LungeCooldownSeconds = 4.0f;

	// Dashes per combo (1 = a single lunge). After each dash the boss re-aims at where you are and dashes again.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Lunge", meta = (ClampMin = "1"))
	int32 LungeComboCount = 6;

	// Telegraph time before every dash after the first one of a combo.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Lunge", meta = (ClampMin = "0.0", Units = "s"))
	float LungeComboWindupSeconds = 0.25f;

	// The windup starts when the target is this close (ground plane, centre to centre).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Lunge", meta = (ClampMin = "0.0", Units = "cm"))
	float LungeTriggerRange = 700.0f;

	// A lunge that could only go this far (because of a map edge) is not started at all.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Lunge", meta = (ClampMin = "0.0", Units = "cm"))
	float MinLungeDistance = 150.0f;

	// Extra radius added to the capsule when checking that a lunge stays on playable cells.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Lunge", meta = (ClampMin = "0.0", Units = "cm"))
	float LungeBoundsPadding = 10.0f;

	// Blends each dash's direction between aiming at the target's exact position (0) and the full computed
	// lead (1, see the class comment). 0.6 leads enough to punish running away in a straight line while
	// still giving a window to juke during the telegraph. Edit this directly to retune the feel.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Lunge", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LungeLeadStrength = 0.6f;

	// Body colour, and the colour it fades to during the windup. Applied to ColorParameterName of
	// BodyMaterialBase; values above 1 glow brighter.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Visual")
	FLinearColor BodyColor = FLinearColor(0.7f, 0.1f, 1.6f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Visual")
	FLinearColor TelegraphColor = FLinearColor(3.0f, 0.1f, 0.1f);

	// The material the body is tinted through (a dynamic copy is made per boss). Defaults to the engine's
	// emissive material, the same one the pickups use. Replace it, and ColorParameterName, to use another.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Visual")
	TObjectPtr<UMaterialInterface> BodyMaterialBase;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Visual")
	FName ColorParameterName = TEXT("EmissiveColor");

	// Max health of this boss, applied at spawn to the MaxHealth stat. Change it on a Blueprint child (or use
	// the GrantMoney.BossHealth console command while testing).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss", meta = (ClampMin = "1.0"))
	float BossMaxHealth = 3000.0f;

	// Uses this arena for lunge bounds instead of the level's TileMapRenderer. For tests and custom setups.
	void SetArenaOverride(const FSpawnArena& InArena);

	// Dev/test hook: bosses spawned from now on get this max health instead of the class default. 0 or less
	// turns it off. The GrantMoney.BossHealth console command sets it.
	static void SetDebugMaxHealthOverride(float MaxHealth);
	static float GetDebugMaxHealthOverride();

	const FBossLungeState& GetLungeState() const { return LungeState; }
	FBossLungeSettings GetLungeSettings() const;

	// Flat unit direction of the current or most recent lunge.
	const FVector& GetLungeDirection() const { return LungeDirection; }

	// Moves the lunge state machine forward. Tick calls this (while alive and not emerging); tests can call it
	// directly after the boss has a target.
	void AdvanceLunge(float DeltaSeconds);

protected:
	virtual void BeginPlay() override;
	virtual FVector GetMoveDirection() const override;
	virtual float GetDesiredMoveSpeed() const override;
	virtual void HandleDeath(AActor* Killer) override;

	// Flat (no vertical part) velocity of the current target, used to lead a dash. Defaults to the target
	// actor's own GetVelocity() (zero for a stationary target); a test double overrides this to supply a
	// velocity directly, without needing real movement.
	virtual FVector GetTargetVelocity() const;

private:
	// Fills Arena from the override or the level's TileMapRenderer. False if no valid arena exists.
	bool ResolveArena();

	// The flat direction toward the target and how far a lunge along it may safely go (leading the target's
	// velocity if bLungeLeadsTarget is on). False if there is no target or no arena.
	bool PlanLunge(const FBossLungeSettings& Settings, FVector& OutDirection, float& OutSafeDistance);

	// Fixes the direction of a dash (when its windup starts), caps its length to the playable area, stops the
	// body and turns it toward the dash.
	void BeginDashWindup(const FVector& Direction, float SafeDistance, const FBossLungeSettings& Settings);

	void ApplyTelegraph(float WindupAlpha);

	FBossLungeState LungeState;
	FVector LungeDirection = FVector::ForwardVector;

	FSpawnArena Arena;
	bool bArenaOverridden = false;
	bool bLoggedNoArena = false;

	FVector BaseMeshScale = FVector::OneVector;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;
};
