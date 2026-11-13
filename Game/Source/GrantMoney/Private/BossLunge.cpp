// Fill out your copyright notice in the Description page of Project Settings.

#include "BossLunge.h"

#include "EnemySpawnPlacement.h"

EBossLungeEvent FBossLungeState::Advance(const FBossLungeSettings& Settings, float DeltaSeconds, bool bTargetInRange)
{
	const float Delta = FMath::Max(0.0f, DeltaSeconds);

	switch (Phase)
	{
	case EBossLungePhase::Idle:
		if (bTargetInRange)
		{
			Phase = EBossLungePhase::Windup;
			PhaseElapsed = 0.0f;
			ComboIndex = 0;
			return EBossLungeEvent::WindupStarted;
		}
		return EBossLungeEvent::None;

	case EBossLungePhase::Windup:
		PhaseElapsed += Delta;
		if (PhaseElapsed >= GetWindupSeconds(Settings))
		{
			Phase = EBossLungePhase::Lunging;
			PhaseElapsed = 0.0f;
			ActiveLungeDuration = DurationLimit >= 0.0f ? FMath::Min(Settings.DurationSeconds, DurationLimit) : Settings.DurationSeconds;
			ActiveLungeDuration = FMath::Max(0.0f, ActiveLungeDuration);
			return EBossLungeEvent::LungeStarted;
		}
		return EBossLungeEvent::None;

	case EBossLungePhase::Lunging:
		PhaseElapsed += Delta;
		if (PhaseElapsed >= ActiveLungeDuration)
		{
			DurationLimit = -1.0f;
			PhaseElapsed = 0.0f;

			if (ComboIndex + 1 < FMath::Max(1, Settings.ComboCount))
			{
				++ComboIndex;
				Phase = EBossLungePhase::Windup;
				return EBossLungeEvent::ComboWindupStarted;
			}

			Phase = EBossLungePhase::Cooldown;
			ComboIndex = 0;
			return EBossLungeEvent::LungeEnded;
		}
		return EBossLungeEvent::None;

	case EBossLungePhase::Cooldown:
		PhaseElapsed += Delta;
		if (PhaseElapsed >= Settings.CooldownSeconds)
		{
			Phase = EBossLungePhase::Idle;
			PhaseElapsed = 0.0f;
			return EBossLungeEvent::CooldownEnded;
		}
		return EBossLungeEvent::None;
	}

	return EBossLungeEvent::None;
}

void FBossLungeState::Reset()
{
	Phase = EBossLungePhase::Idle;
	PhaseElapsed = 0.0f;
	DurationLimit = -1.0f;
	ActiveLungeDuration = 0.0f;
	ComboIndex = 0;
}

void FBossLungeState::AbortCombo()
{
	Phase = EBossLungePhase::Cooldown;
	PhaseElapsed = 0.0f;
	DurationLimit = -1.0f;
	ComboIndex = 0;
}

float FBossLungeState::GetWindupSeconds(const FBossLungeSettings& Settings) const
{
	return ComboIndex > 0 ? Settings.ComboWindupSeconds : Settings.WindupSeconds;
}

float FBossLungeState::GetWindupAlpha(const FBossLungeSettings& Settings) const
{
	if (Phase != EBossLungePhase::Windup)
	{
		return 0.0f;
	}

	const float Windup = GetWindupSeconds(Settings);
	return Windup > UE_KINDA_SMALL_NUMBER ? FMath::Clamp(PhaseElapsed / Windup, 0.0f, 1.0f) : 1.0f;
}

float FBossLungeMath::ComputeSafeDistance(const FSpawnArena& Arena, const FVector& Start, const FVector& Direction, float DesiredDistance, float Radius, float StepSize)
{
	FVector Flat(Direction.X, Direction.Y, 0.0);
	if (!Arena.IsValid() || DesiredDistance <= 0.0f || !Flat.Normalize())
	{
		return 0.0f;
	}

	const float Step = FMath::Max(1.0f, StepSize);
	float Safe = 0.0f;

	for (float Travelled = Step; ; Travelled += Step)
	{
		const float Distance = FMath::Min(Travelled, DesiredDistance);
		if (!Arena.IsFootprintPlayable(Start + Flat * Distance, Radius))
		{
			return Safe;
		}

		Safe = Distance;
		if (Distance >= DesiredDistance)
		{
			return Safe;
		}
	}
}

float FBossLungeMath::ComputeSafeDuration(float SafeDistance, float Speed, float MaxDuration)
{
	if (Speed <= UE_KINDA_SMALL_NUMBER || SafeDistance <= 0.0f)
	{
		return 0.0f;
	}

	return FMath::Clamp(SafeDistance / Speed, 0.0f, FMath::Max(0.0f, MaxDuration));
}

bool FBossLungeMath::ComputeInterceptDirection(const FVector& RelativeTargetPosition, const FVector& FlatTargetVelocity, float Speed, float DelaySeconds, FVector& OutDirection)
{
	const FVector RelPos(RelativeTargetPosition.X, RelativeTargetPosition.Y, 0.0);
	const FVector Velocity(FlatTargetVelocity.X, FlatTargetVelocity.Y, 0.0);

	// The fallback (and the answer whenever Speed cannot be used): straight at the target's current position.
	OutDirection = RelPos;
	if (!OutDirection.Normalize() || Speed <= UE_KINDA_SMALL_NUMBER)
	{
		return false;
	}

	// Where the target will be once the telegraph ends and the dash itself actually launches.
	const FVector LaunchRelPos = RelPos + Velocity * FMath::Max(0.0f, DelaySeconds);

	// Solve |LaunchRelPos + Velocity * t| = Speed * t for the earliest positive t: the point along the
	// target's path, measured from the dash's launch, that a Speed-cm/s dash reaches at the same moment
	// the target does.
	const double A = Velocity.SizeSquared() - static_cast<double>(Speed) * Speed;
	const double B = 2.0 * FVector::DotProduct(LaunchRelPos, Velocity);
	const double C = LaunchRelPos.SizeSquared();

	double BestTime = -1.0;
	if (FMath::Abs(A) <= UE_KINDA_SMALL_NUMBER)
	{
		// Target speed equals dash speed: the quadratic term vanishes; solve the remaining linear equation.
		if (FMath::Abs(B) > UE_KINDA_SMALL_NUMBER)
		{
			BestTime = -C / B;
		}
	}
	else
	{
		const double Discriminant = B * B - 4.0 * A * C;
		if (Discriminant >= 0.0)
		{
			const double SqrtDiscriminant = FMath::Sqrt(Discriminant);
			const double Root1 = (-B + SqrtDiscriminant) / (2.0 * A);
			const double Root2 = (-B - SqrtDiscriminant) / (2.0 * A);

			if (Root1 > 0.0 && Root2 > 0.0)
			{
				BestTime = FMath::Min(Root1, Root2);
			}
			else if (Root1 > 0.0)
			{
				BestTime = Root1;
			}
			else if (Root2 > 0.0)
			{
				BestTime = Root2;
			}
		}
	}

	if (BestTime <= UE_KINDA_SMALL_NUMBER)
	{
		// No real, positive-time meeting point: the target cannot be led (for example it is outrunning the
		// dash). Keep the straight-line fallback set above.
		return false;
	}

	const FVector InterceptPoint = LaunchRelPos + Velocity * static_cast<float>(BestTime);
	FVector LeadDirection = InterceptPoint;
	if (!LeadDirection.Normalize())
	{
		return false;
	}

	OutDirection = LeadDirection;
	return true;
}
