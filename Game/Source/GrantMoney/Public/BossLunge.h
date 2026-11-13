// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct FSpawnArena;

enum class EBossLungePhase : uint8
{
	// Ready: waiting for the target to come within trigger range.
	Idle,
	// Standing still and telegraphing. The lunge direction is fixed for the whole windup.
	Windup,
	// Charging in a straight line at LungeSpeed.
	Lunging,
	// Chasing normally again; the next windup cannot start until this has run out.
	Cooldown
};

// What an Advance call changed. At most one transition happens per call.
enum class EBossLungeEvent : uint8
{
	None,
	WindupStarted,
	LungeStarted,
	LungeEnded,
	// A combo dash just ended and the next windup began at once (no cooldown in between). The caller re-aims.
	ComboWindupStarted,
	CooldownEnded
};

// Designer-facing timing of the lunge. Edited on the boss (ABossSlimeEnemy), passed in by value so the
// state machine below stays pure and testable.
struct GRANTMONEY_API FBossLungeSettings
{
	// Telegraph time before the first dash, in seconds. 0 charges immediately.
	float WindupSeconds = 1.0f;

	// Charge speed in cm/s.
	float Speed = 1400.0f;

	// Longest charge, in seconds. A charge that would leave the playable area is shortened (see ComputeSafeDistance).
	float DurationSeconds = 0.6f;

	// Seconds after the last dash of a combo ends before another windup can start.
	float CooldownSeconds = 4.0f;

	// The windup starts when the target is within this ground-plane distance (centre to centre).
	float TriggerRange = 700.0f;

	// Dashes in one combo. 1 is a single lunge. Each dash after the first follows the previous one directly,
	// after ComboWindupSeconds, and is re-aimed at where the target is then.
	int32 ComboCount = 3;

	// Telegraph time before every dash after the first.
	float ComboWindupSeconds = 0.5f;
};

/**
 * The boss lunge as a small state machine: Idle -> Windup -> Lunging -> (Windup -> Lunging ...) -> Cooldown ->
 * Idle. Pure logic with no world access: the boss feeds it game-time deltas and whether the target is in
 * range, and reacts to the returned events. Tests drive it directly with synthetic numbers.
 */
struct GRANTMONEY_API FBossLungeState
{
	EBossLungePhase Phase = EBossLungePhase::Idle;

	// Time spent in the current phase.
	float PhaseElapsed = 0.0f;

	// Cap on the next (or current) charge's duration, from the playable-area check. Negative = no cap.
	float DurationLimit = -1.0f;

	// Length of the charge in progress: the settings' duration, shortened by DurationLimit. Set on entering Lunging.
	float ActiveLungeDuration = 0.0f;

	// Which dash of the combo is winding up or in progress; 0 is the first.
	int32 ComboIndex = 0;

	// Moves time forward and applies at most one phase change.
	EBossLungeEvent Advance(const FBossLungeSettings& Settings, float DeltaSeconds, bool bTargetInRange);

	// Back to Idle with no cap, for example when the boss dies or loses its target.
	void Reset();

	// Ends the combo early (for example no room for the next dash) and starts the cooldown.
	void AbortCombo();

	// Telegraph length of the current dash: WindupSeconds for the first, ComboWindupSeconds after that.
	float GetWindupSeconds(const FBossLungeSettings& Settings) const;

	// 0 at the start of the windup, 1 at its end; 0 outside the Windup phase. For the telegraph visual.
	float GetWindupAlpha(const FBossLungeSettings& Settings) const;

	bool IsLunging() const { return Phase == EBossLungePhase::Lunging; }
	bool IsWindingUp() const { return Phase == EBossLungePhase::Windup; }
};

class GRANTMONEY_API FBossLungeMath
{
public:
	// How far (cm) a charge from Start along the flat unit Direction may go, up to DesiredDistance, while a
	// circle of Radius stays on playable cells of Arena (checked every StepSize cm). 0 if even the first step
	// is not playable. Also 0 for an invalid arena or a zero direction, so a lunge never starts without bounds.
	static float ComputeSafeDistance(const FSpawnArena& Arena, const FVector& Start, const FVector& Direction, float DesiredDistance, float Radius, float StepSize = 25.0f);

	// Charge length in seconds for a safe distance at Speed, never above MaxDuration.
	static float ComputeSafeDuration(float SafeDistance, float Speed, float MaxDuration);

	// The flat unit direction an interceptor at the origin, dashing at Speed, should travel to meet a target
	// currently at RelativeTargetPosition (target minus interceptor, flat) and moving at FlatTargetVelocity,
	// given the interceptor only starts moving DelaySeconds from now (the telegraph) and the target keeps its
	// current velocity the whole time. Pure "lead/intercept" math (solves |R + V t| = Speed * t for the
	// earliest positive t); no world access, so it can be tested with synthetic numbers.
	//
	// Returns false, leaving OutDirection pointing straight at the target's current position, whenever there
	// is no valid lead (a stationary target, Speed <= 0, or a target too fast to ever be met at this Speed).
	static bool ComputeInterceptDirection(const FVector& RelativeTargetPosition, const FVector& FlatTargetVelocity, float Speed, float DelaySeconds, FVector& OutDirection);
};
