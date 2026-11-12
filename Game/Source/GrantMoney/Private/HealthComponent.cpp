// Fill out your copyright notice in the Description page of Project Settings.

#include "HealthComponent.h"

#include "GameFramework/Actor.h"
#include "StatsComponent.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		Stats = Owner->FindComponentByClass<UStatsComponent>();
		Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
	}

	if (Stats)
	{
		Stats->OnStatChanged.AddDynamic(this, &UHealthComponent::HandleStatChanged);
	}

	// Start at full health without broadcasting; listeners can read the starting value.
	CurrentHealth = GetMaxHealth();
	bIsDead = false;
}

void UHealthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.RemoveDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
	}

	if (Stats)
	{
		Stats->OnStatChanged.RemoveDynamic(this, &UHealthComponent::HandleStatChanged);
	}

	Super::EndPlay(EndPlayReason);
}

float UHealthComponent::GetMaxHealth() const
{
	return Stats ? Stats->GetStat(EGrantMoneyStat::MaxHealth) : FallbackMaxHealth;
}

float UHealthComponent::GetHealthFraction() const
{
	const float MaxHealth = GetMaxHealth();
	return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
}

float UHealthComponent::ApplyDamage(float Amount, AActor* DamageCauser)
{
	if (bIsDead || !(Amount > 0.0f))
	{
		return 0.0f;
	}

	const float OldHealth = CurrentHealth;
	SetHealth(CurrentHealth - Amount);

	if (CurrentHealth <= 0.0f && !bIsDead)
	{
		bIsDead = true;
		UE_LOG(LogTemp, Log, TEXT("HealthComponent: %s died (damage causer: %s)."), *GetNameSafe(GetOwner()), *GetNameSafe(DamageCauser));
		OnDeath.Broadcast(this, DamageCauser);
	}

	return OldHealth - CurrentHealth;
}

void UHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	ApplyDamage(Damage, DamageCauser);
}

float UHealthComponent::Heal(float Amount)
{
	if (bIsDead || !(Amount > 0.0f))
	{
		return 0.0f;
	}

	const float OldHealth = CurrentHealth;
	SetHealth(CurrentHealth + Amount);
	return CurrentHealth - OldHealth;
}

void UHealthComponent::HandleStatChanged(EGrantMoneyStat Stat, float OldValue, float NewValue)
{
	if (Stat != EGrantMoneyStat::MaxHealth || bIsDead)
	{
		return;
	}

	// By default a max-health buff also heals by the amount gained, so picking up an upgrade
	// never leaves the owner "less full". With bHealOnMaxHealthGain off, only the ceiling rises.
	// A loss only clamps current health.
	const float Gained = bHealOnMaxHealthGain ? FMath::Max(0.0f, NewValue - OldValue) : 0.0f;
	SetHealth(CurrentHealth + Gained);

	// A max-health loss can only kill if max health itself drops to zero.
	if (CurrentHealth <= 0.0f)
	{
		bIsDead = true;
		OnDeath.Broadcast(this, nullptr);
	}
}

void UHealthComponent::SetHealth(float NewHealth)
{
	const float Clamped = FMath::Clamp(NewHealth, 0.0f, GetMaxHealth());
	const float Delta = Clamped - CurrentHealth;
	CurrentHealth = Clamped;

	if (!FMath::IsNearlyZero(Delta))
	{
		OnHealthChanged.Broadcast(this, CurrentHealth, Delta);
	}
}
