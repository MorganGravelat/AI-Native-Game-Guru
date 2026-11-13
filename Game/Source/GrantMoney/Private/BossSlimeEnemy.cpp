// Fill out your copyright notice in the Description page of Project Settings.

#include "BossSlimeEnemy.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ContactDamageComponent.h"
#include "DrawDebugHelpers.h"
#include "EnemyEmergenceComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GrantMoneyAudio.h"
#include "HealthComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "StatsComponent.h"
#include "TileMapRenderer.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float BossCapsuleRadius = 90.0f;

	// The engine sphere is 100 cm across, so 1.8 gives a 180 cm boss, three times the size of a slime.
	constexpr double BossPlaceholderScale = 1.8;

	constexpr float BossContactDamage = 20.0f;
	constexpr float BossAttackInterval = 1.5f;

	// 0 = use the class default. Set by SetDebugMaxHealthOverride.
	float GDebugMaxHealthOverride = 0.0f;
}

ABossSlimeEnemy::ABossSlimeEnemy()
{
	// Heavier and slower than a slime; the lunge is its threat.
	MoveSpeed = 170.0f;
	StopDistance = 150.0f;
	SlowdownDistance = 80.0f;

	Stats->SetBaseStat(EGrantMoneyStat::MaxHealth, BossMaxHealth);
	Stats->SetBaseStat(EGrantMoneyStat::ContactDamage, BossContactDamage);
	Stats->SetBaseStat(EGrantMoneyStat::AttackInterval, BossAttackInterval);

	// Same damage path as every enemy; the larger body just needs a longer reach to count as touching.
	// Hits are logged so the cadence can be read in the Output Log while testing.
	ContactDamage->Reach = 60.0f;
	ContactDamage->bLogHits = true;

	// No rare stat boosts from the boss; it still leaves the guaranteed XP orb.
	BonusPickupDrops.Reset();

	DeathSound = GrantMoneyAudio::DefaultSound(TEXT("/Game/Sound/Enemy/SlimeBossDying.SlimeBossDying"));
	LungeWindupSound = GrantMoneyAudio::DefaultSound(TEXT("/Game/Sound/Enemy/SlimeBossDashWindup.SlimeBossDashWindup"));
	LungeSound = GrantMoneyAudio::DefaultSound(TEXT("/Game/Sound/Enemy/SlimeBossDashSound.SlimeBossDashSound"));

	GetCapsuleComponent()->InitCapsuleSize(BossCapsuleRadius, BossCapsuleRadius);
	PlaceholderMesh->SetRelativeScale3D(FVector(BossPlaceholderScale));

	Emergence->Duration = 1.5f;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BodyMaterialFinder(TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	if (BodyMaterialFinder.Succeeded())
	{
		BodyMaterialBase = BodyMaterialFinder.Object;
	}
}

void ABossSlimeEnemy::BeginPlay()
{
	Super::BeginPlay();

	// The editable BossMaxHealth, unless the debug override is on. Raising max health also heals by the gain
	// (UHealthComponent::bHealOnMaxHealthGain) and the explicit Heal covers a lowered value, so it starts full.
	const float StartingMaxHealth = GDebugMaxHealthOverride > 0.0f ? GDebugMaxHealthOverride : BossMaxHealth;
	Stats->SetBaseStat(EGrantMoneyStat::MaxHealth, StartingMaxHealth);
	Health->Heal(StartingMaxHealth);

	if (BodyMaterialBase)
	{
		BodyMaterial = UMaterialInstanceDynamic::Create(BodyMaterialBase, this);
		PlaceholderMesh->SetMaterial(0, BodyMaterial);
	}
	else
	{
		BodyMaterial = PlaceholderMesh->CreateAndSetMaterialInstanceDynamic(0);
	}

	FLinearColor ExistingColor;
	if (!BodyMaterial || !BodyMaterial->GetVectorParameterValue(FHashedMaterialParameterInfo(ColorParameterName), ExistingColor))
	{
		UE_LOG(LogTemp, Warning, TEXT("BossSlimeEnemy: %s has no material with a '%s' colour parameter, so the boss tint and telegraph colour will not show."), *GetName(), *ColorParameterName.ToString());
	}

	ApplyTelegraph(0.0f);
}

FBossLungeSettings ABossSlimeEnemy::GetLungeSettings() const
{
	FBossLungeSettings Settings;
	Settings.WindupSeconds = LungeWindupSeconds;
	Settings.Speed = LungeSpeed;
	Settings.DurationSeconds = LungeDurationSeconds;
	Settings.CooldownSeconds = LungeCooldownSeconds;
	Settings.TriggerRange = LungeTriggerRange;
	Settings.ComboCount = LungeComboCount;
	Settings.ComboWindupSeconds = LungeComboWindupSeconds;
	return Settings;
}

void ABossSlimeEnemy::SetDebugMaxHealthOverride(float MaxHealth)
{
	GDebugMaxHealthOverride = MaxHealth > 0.0f ? MaxHealth : 0.0f;
}

float ABossSlimeEnemy::GetDebugMaxHealthOverride()
{
	return GDebugMaxHealthOverride;
}

void ABossSlimeEnemy::SetArenaOverride(const FSpawnArena& InArena)
{
	Arena = InArena;
	bArenaOverridden = true;
}

void ABossSlimeEnemy::Tick(float DeltaTime)
{
	if (!IsEmerging() && !GetHealth()->IsDead())
	{
		AdvanceLunge(DeltaTime);
	}

	Super::Tick(DeltaTime);
}

bool ABossSlimeEnemy::ResolveArena()
{
	if (bArenaOverridden)
	{
		return Arena.IsValid();
	}

	const ATileMapRenderer* Renderer = nullptr;
	for (TActorIterator<ATileMapRenderer> It(GetWorld()); It; ++It)
	{
		Renderer = *It;
		break;
	}

	if (!Renderer || !Renderer->IsMapReady() || !FSpawnArena::FromRenderer(Renderer, Arena))
	{
		if (!bLoggedNoArena)
		{
			UE_LOG(LogTemp, Warning, TEXT("BossSlimeEnemy: %s has no built map to keep its lunge inside, so it will not lunge."), *GetName());
			bLoggedNoArena = true;
		}

		return false;
	}

	return true;
}

bool ABossSlimeEnemy::PlanLunge(const FBossLungeSettings& Settings, FVector& OutDirection, float& OutSafeDistance)
{
	if (!ResolveArena())
	{
		return false;
	}

	const FVector Offset = GetFlatOffsetToTarget();
	FVector Direction = Offset;
	if (!Direction.Normalize())
	{
		return false;
	}

	// ComboIndex is 0 for the first dash (planned from Idle) and already counted up for the dashes after it.
	const float LeadStrength = LungeState.ComboIndex == 0 ? FirstDashLeadStrength : LungeLeadStrength;

	if (LeadStrength > UE_KINDA_SMALL_NUMBER)
	{
		const FVector TargetVelocity = GetTargetVelocity();
		if (!TargetVelocity.IsNearlyZero())
		{
			// Lead by how long the target keeps moving before this dash actually launches: the windup (or the
			// shorter combo windup for dash 2+) that is just about to start.
			const float DelaySeconds = LungeState.GetWindupSeconds(Settings);
			FVector LeadDirection;
			if (FBossLungeMath::ComputeInterceptDirection(Offset, TargetVelocity, LungeSpeed, DelaySeconds, LeadDirection))
			{
				// Blend rather than use the full lead outright, so a lower LungeLeadStrength still leaves a
				// window to juke during the telegraph instead of making the dash unavoidable.
				const FVector Blended = FMath::Lerp(Direction, LeadDirection, FMath::Clamp(LeadStrength, 0.0f, 1.0f));
				FVector Normalized = Blended;
				if (Normalized.Normalize())
				{
					Direction = Normalized;
				}
			}
		}
	}

	const float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius() + LungeBoundsPadding;
	OutDirection = Direction;
	OutSafeDistance = FBossLungeMath::ComputeSafeDistance(Arena, GetActorLocation(), Direction, LungeSpeed * LungeDurationSeconds, Radius);
	return true;
}

FVector ABossSlimeEnemy::GetTargetVelocity() const
{
	const AActor* TargetActor = GetTarget();
	if (!IsValid(TargetActor))
	{
		return FVector::ZeroVector;
	}

	FVector Velocity = TargetActor->GetVelocity();
	Velocity.Z = 0.0;
	return Velocity;
}

void ABossSlimeEnemy::BeginDashWindup(const FVector& Direction, float SafeDistance, const FBossLungeSettings& Settings)
{
	LungeDirection = Direction;
	LungeState.DurationLimit = FBossLungeMath::ComputeSafeDuration(SafeDistance, Settings.Speed, Settings.DurationSeconds);
	SetActorRotation(FRotator(0.0, LungeDirection.Rotation().Yaw, 0.0));

	// Plant its feet so the telegraph is a real pause, not a slide carried over from the previous dash.
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->Velocity = FVector(0.0, 0.0, Movement->Velocity.Z);
}

void ABossSlimeEnemy::AdvanceLunge(float DeltaSeconds)
{
	if (!IsValid(GetTarget()) && !AcquireTarget())
	{
		LungeState.Reset();
		ApplyTelegraph(0.0f);
		return;
	}

	const FBossLungeSettings Settings = GetLungeSettings();

	bool bReadyToLunge = false;
	FVector PlannedDirection = FVector::ForwardVector;
	float PlannedSafeDistance = 0.0f;
	if (LungeState.Phase == EBossLungePhase::Idle && GetFlatOffsetToTarget().Size() <= Settings.TriggerRange)
	{
		// A lunge that could not go anywhere useful (map edge in the way) is never started, so there is no empty windup.
		bReadyToLunge = PlanLunge(Settings, PlannedDirection, PlannedSafeDistance) && PlannedSafeDistance >= MinLungeDistance;
	}

	switch (LungeState.Advance(Settings, DeltaSeconds, bReadyToLunge))
	{
	case EBossLungeEvent::WindupStarted:
		BeginDashWindup(PlannedDirection, PlannedSafeDistance, Settings);
		GrantMoneyAudio::PlayAtLocation(this, LungeWindupSound, GetActorLocation());
		UE_LOG(LogTemp, Log, TEXT("BossSlimeEnemy: %s winds up a lunge combo of %d (%.1f s, safe distance %.0f cm)."), *GetName(), Settings.ComboCount, Settings.WindupSeconds, PlannedSafeDistance);
		break;

	case EBossLungeEvent::ComboWindupStarted:
	{
		// Re-aim at where the target is now. No room (or no direction) ends the combo.
		if (PlanLunge(Settings, PlannedDirection, PlannedSafeDistance) && PlannedSafeDistance >= MinLungeDistance)
		{
			BeginDashWindup(PlannedDirection, PlannedSafeDistance, Settings);
			GrantMoneyAudio::PlayAtLocation(this, LungeWindupSound, GetActorLocation());
			UE_LOG(LogTemp, Log, TEXT("BossSlimeEnemy: %s re-aims for dash %d of %d."), *GetName(), LungeState.ComboIndex + 1, Settings.ComboCount);
		}
		else
		{
			LungeState.AbortCombo();
			ApplyTelegraph(0.0f);
			UE_LOG(LogTemp, Log, TEXT("BossSlimeEnemy: %s ends its combo early: no room for the next dash."), *GetName());
		}
		break;
	}

	case EBossLungeEvent::LungeStarted:
		GrantMoneyAudio::PlayAtLocation(this, LungeSound, GetActorLocation());
		UE_LOG(LogTemp, Log, TEXT("BossSlimeEnemy: %s lunges for %.2f s at %.0f cm/s."), *GetName(), LungeState.ActiveLungeDuration, Settings.Speed);
		break;

	case EBossLungeEvent::LungeEnded:
		ApplyTelegraph(0.0f);
		break;

	default:
		break;
	}

	if (LungeState.IsWindingUp())
	{
		ApplyTelegraph(LungeState.GetWindupAlpha(Settings));

#if ENABLE_DRAW_DEBUG
		const float PathLength = Settings.Speed * FMath::Min(Settings.DurationSeconds, LungeState.DurationLimit >= 0.0f ? LungeState.DurationLimit : Settings.DurationSeconds);
		const FVector PathStart = GetActorLocation() - FVector(0.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 5.0);
		DrawDebugLine(GetWorld(), PathStart, PathStart + LungeDirection * PathLength, FColor::Red, false, -1.0f, 0, 14.0f);
#endif
	}
	else if (LungeState.IsLunging())
	{
		ApplyTelegraph(1.0f);

		// Set directly rather than ramped through acceleration, so the charge starts at full speed.
		UCharacterMovementComponent* Movement = GetCharacterMovement();
		Movement->Velocity = FVector(LungeDirection.X * Settings.Speed, LungeDirection.Y * Settings.Speed, Movement->Velocity.Z);
	}
}

FVector ABossSlimeEnemy::GetMoveDirection() const
{
	switch (LungeState.Phase)
	{
	case EBossLungePhase::Windup:
		return FVector::ZeroVector;
	case EBossLungePhase::Lunging:
		return LungeDirection;
	default:
		return Super::GetMoveDirection();
	}
}

float ABossSlimeEnemy::GetDesiredMoveSpeed() const
{
	switch (LungeState.Phase)
	{
	case EBossLungePhase::Windup:
		return 0.0f;
	case EBossLungePhase::Lunging:
		return LungeSpeed;
	default:
		return Super::GetDesiredMoveSpeed();
	}
}

void ABossSlimeEnemy::HandleDeath(AActor* Killer)
{
	LungeState.Reset();
	ApplyTelegraph(0.0f);
	Super::HandleDeath(Killer);
}

bool ABossSlimeEnemy::GetSquishOverride(float& OutSwing) const
{
	switch (LungeState.Phase)
	{
	case EBossLungePhase::Windup:
		// Sinks lower the closer the dash gets.
		OutSwing = -WindupSquash * LungeState.GetWindupAlpha(GetLungeSettings());
		return true;
	case EBossLungePhase::Lunging:
		OutSwing = LungeStretch;
		return true;
	default:
		return false;
	}
}

void ABossSlimeEnemy::ApplyTelegraph(float WindupAlpha)
{
	const float Alpha = FMath::Clamp(WindupAlpha, 0.0f, 1.0f);

	// The body shape during the windup and the charge comes from GetSquishOverride, so only the colour is set here.
	if (BodyMaterial)
	{
		BodyMaterial->SetVectorParameterValue(ColorParameterName, FMath::Lerp(BodyColor, TelegraphColor, Alpha));
	}
}
