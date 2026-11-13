// Fill out your copyright notice in the Description page of Project Settings.

// NO-TEST: dev-only console command glue (parses arguments and forwards to already-tested calls:
// ARunController::AdvanceRun, UStatsComponent::SetBaseStat, UHealthComponent::Heal,
// UTemporaryBuffComponent::AddBuff and ABossSlimeEnemy::SetDebugMaxHealthOverride, which has its own test).
// Compiled out of shipping builds.

#if !UE_BUILD_SHIPPING

#include "BossEncounter.h"
#include "BossSlimeEnemy.h"
#include "EnemyBase.h"
#include "Engine/World.h"
#include "GrantMoneyPlayerCharacter.h"
#include "HAL/IConsoleManager.h"
#include "HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "RunController.h"
#include "StatsComponent.h"
#include "TemporaryBuffComponent.h"

namespace
{
	// Sets MaxHealth to Value and fills health, the same way a max-health boost would.
	void SetMaxHealthAndFill(AActor* Actor, float Value)
	{
		UStatsComponent* Stats = Actor ? Actor->FindComponentByClass<UStatsComponent>() : nullptr;
		UHealthComponent* Health = Actor ? Actor->FindComponentByClass<UHealthComponent>() : nullptr;
		if (!Stats || !Health)
		{
			return;
		}

		Stats->SetBaseStat(EGrantMoneyStat::MaxHealth, Value);
		Health->Heal(Value);
	}

	// "GrantMoney.SkipToBoss": ends the survival clock now so the boss phase starts right away.
	FAutoConsoleCommandWithWorldAndArgs GSkipToBossCommand(
		TEXT("GrantMoney.SkipToBoss"),
		TEXT("Dev only. Jumps the run straight to the boss phase (the survival timer reads 0 and the boss spawns)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
		{
			ARunController* Run = ARunController::Find(World);
			if (!Run || !Run->HasStarted())
			{
				UE_LOG(LogTemp, Warning, TEXT("GrantMoney.SkipToBoss: the run has not started yet."));
				return;
			}

			// The first call finishes the Starting delay (if any); the second runs the clock out.
			Run->AdvanceRun(Run->StartDelaySeconds);
			Run->AdvanceRun(Run->RunDurationSeconds);
			UE_LOG(LogTemp, Log, TEXT("GrantMoney.SkipToBoss: run is now %s."), *ARunController::GetStateLabel(Run->GetState()));
		}));

	// "GrantMoney.BossHealth 5000": the boss's max health, for this boss and every boss spawned afterwards.
	FAutoConsoleCommandWithWorldAndArgs GBossHealthCommand(
		TEXT("GrantMoney.BossHealth"),
		TEXT("Dev only. Sets the boss's max health and fills it, now and for later spawns. Usage: GrantMoney.BossHealth <Amount>  (0 = back to the class default)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
		{
			const float Amount = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 0.0f;
			ABossSlimeEnemy::SetDebugMaxHealthOverride(Amount);

			if (Amount <= 0.0f)
			{
				UE_LOG(LogTemp, Log, TEXT("GrantMoney.BossHealth: override cleared; bosses spawned from now on use the class default."));
				return;
			}

			const ABossEncounter* Encounter = ABossEncounter::Find(World);
			if (AEnemyBase* Boss = Encounter ? Encounter->GetBoss() : nullptr)
			{
				SetMaxHealthAndFill(Boss, Amount);
			}

			UE_LOG(LogTemp, Log, TEXT("GrantMoney.BossHealth: boss max health is %.0f (current boss and later spawns)."), Amount);
		}));

	// "GrantMoney.PlayerHealth 100000": a near-invulnerable player to watch a long fight, or a tiny one to die quickly.
	FAutoConsoleCommandWithWorldAndArgs GPlayerHealthCommand(
		TEXT("GrantMoney.PlayerHealth"),
		TEXT("Dev only. Sets the player's max health and fills it. Usage: GrantMoney.PlayerHealth <Amount>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
		{
			const float Amount = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 0.0f;
			AActor* Player = UGameplayStatics::GetPlayerPawn(World, 0);
			if (Amount <= 0.0f || !Player)
			{
				UE_LOG(LogTemp, Warning, TEXT("GrantMoney.PlayerHealth: needs a positive amount and a player pawn."));
				return;
			}

			SetMaxHealthAndFill(Player, Amount);
			UE_LOG(LogTemp, Log, TEXT("GrantMoney.PlayerHealth: player max health is now %.0f."), Amount);
		}));

	// "GrantMoney.GiveXp 25": adds XP the way an orb does, to see the XP bar, level, score and level-up choice.
	FAutoConsoleCommandWithWorldAndArgs GGiveXpCommand(
		TEXT("GrantMoney.GiveXp"),
		TEXT("Dev only. Adds experience to the player. Usage: GrantMoney.GiveXp <Amount>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
		{
			const float Amount = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 0.0f;
			const AGrantMoneyPlayerCharacter* Player = Cast<AGrantMoneyPlayerCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
			UStatsComponent* Stats = Player ? Player->GetStats() : nullptr;
			if (Amount <= 0.0f || !Stats)
			{
				UE_LOG(LogTemp, Warning, TEXT("GrantMoney.GiveXp: needs a positive amount and a player pawn."));
				return;
			}

			Stats->SetBaseStat(EGrantMoneyStat::Experience, Stats->GetBaseStat(EGrantMoneyStat::Experience) + Amount);
			UE_LOG(LogTemp, Log, TEXT("GrantMoney.GiveXp: +%.0f XP (total %.0f)."), Amount, Stats->GetBaseStat(EGrantMoneyStat::Experience));
		}));

	// "GrantMoney.GiveBuff 15": a timed +50% move speed buff, to see the buff list and its countdown.
	// A second call while it is active refreshes it instead of stacking.
	FAutoConsoleCommandWithWorldAndArgs GGiveBuffCommand(
		TEXT("GrantMoney.GiveBuff"),
		TEXT("Dev only. Gives the player a temporary +50% move speed buff. Usage: GrantMoney.GiveBuff [Seconds=15]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
		{
			const float Seconds = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 15.0f;
			const AGrantMoneyPlayerCharacter* Player = Cast<AGrantMoneyPlayerCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
			UTemporaryBuffComponent* Buffs = Player ? Player->GetTemporaryBuffs() : nullptr;
			if (!Buffs || !Buffs->AddBuff(TEXT("DebugSpeedSurge"), FText::FromString(TEXT("Speed Surge")), EGrantMoneyStat::MoveSpeed, EGrantMoneyStatModifierOp::Multiplicative, 0.5f, Seconds))
			{
				UE_LOG(LogTemp, Warning, TEXT("GrantMoney.GiveBuff: needs a player pawn and a positive duration."));
				return;
			}

			UE_LOG(LogTemp, Log, TEXT("GrantMoney.GiveBuff: +50%% move speed for %.1f s."), Seconds);
		}));
}

#endif
