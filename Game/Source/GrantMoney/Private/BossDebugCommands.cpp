// Fill out your copyright notice in the Description page of Project Settings.

// NO-TEST: dev-only console command glue (parses arguments and forwards to already-tested calls:
// ARunController::AdvanceRun, UStatsComponent::SetBaseStat, UHealthComponent::Heal,
// UTemporaryEffectComponent::ApplyEffect and ABossSlimeEnemy::SetDebugMaxHealthOverride, which has its
// own test). Compiled out of shipping builds.

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
#include "RewardTotemSpawner.h"
#include "StatsComponent.h"
#include "TemporaryEffect.h"
#include "TemporaryEffectComponent.h"

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

	// "GrantMoney.GiveBuff 15": a timed +50% move speed buff (the same mechanism a Swiftness totem uses),
	// to see the buff list, the toast, and the countdown. A second call while it is active refreshes it
	// instead of stacking (same StackKey rule as the totems).
	FAutoConsoleCommandWithWorldAndArgs GGiveBuffCommand(
		TEXT("GrantMoney.GiveBuff"),
		TEXT("Dev only. Gives the player a temporary +50% move speed buff. Usage: GrantMoney.GiveBuff [Seconds=15]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
		{
			const float Seconds = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 15.0f;
			AGrantMoneyPlayerCharacter* Player = Cast<AGrantMoneyPlayerCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
			if (!Player || Seconds <= 0.0f)
			{
				UE_LOG(LogTemp, Warning, TEXT("GrantMoney.GiveBuff: needs a player pawn and a positive duration."));
				return;
			}

			UStatModifierEffect* SpeedBuff = NewObject<UStatModifierEffect>();
			SpeedBuff->Stat = EGrantMoneyStat::MoveSpeed;
			SpeedBuff->ModifierOp = EGrantMoneyStatModifierOp::Multiplicative;
			SpeedBuff->Magnitude = 0.5f;
			SpeedBuff->Duration = Seconds;
			SpeedBuff->StackKey = TEXT("DebugSpeedSurge");
			SpeedBuff->DisplayName = FText::FromString(TEXT("Speed Surge"));

			UTemporaryEffectComponent* Effects = UTemporaryEffectComponent::FindOrAdd(Player);
			if (!Effects || !Effects->ApplyEffect(SpeedBuff))
			{
				UE_LOG(LogTemp, Warning, TEXT("GrantMoney.GiveBuff: the effect could not be applied."));
				return;
			}

			UE_LOG(LogTemp, Log, TEXT("GrantMoney.GiveBuff: +50%% move speed for %.1f s."), Seconds);
		}));

	// "GrantMoney.GiveFrenzy": the real Frenzy totem reward (permanent, 10% shorter attack interval), to
	// see a permanent buff combine with a permanent Attack Speed upgrade and show in the HUD without a countdown.
	FAutoConsoleCommandWithWorldAndArgs GGiveFrenzyCommand(
		TEXT("GrantMoney.GiveFrenzy"),
		TEXT("Dev only. Gives the player the Frenzy totem reward (permanent, 10% shorter attack interval)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
		{
			AGrantMoneyPlayerCharacter* Player = Cast<AGrantMoneyPlayerCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
			if (!Player)
			{
				UE_LOG(LogTemp, Warning, TEXT("GrantMoney.GiveFrenzy: needs a player pawn."));
				return;
			}

			UStatModifierEffect* Frenzy = NewObject<UStatModifierEffect>();
			Frenzy->Stat = EGrantMoneyStat::AttackInterval;
			Frenzy->ModifierOp = EGrantMoneyStatModifierOp::Multiplicative;
			Frenzy->Magnitude = -0.1f;
			Frenzy->bPermanent = true;
			Frenzy->StackKey = TEXT("DebugFrenzy");
			Frenzy->DisplayName = FText::FromString(TEXT("Frenzy"));

			UTemporaryEffectComponent* Effects = UTemporaryEffectComponent::FindOrAdd(Player);
			if (!Effects || !Effects->ApplyEffect(Frenzy))
			{
				UE_LOG(LogTemp, Warning, TEXT("GrantMoney.GiveFrenzy: the effect could not be applied."));
				return;
			}

			UE_LOG(LogTemp, Log, TEXT("GrantMoney.GiveFrenzy: permanently 10%% shorter attack interval."));
		}));

	// "GrantMoney.GiveTotemBuff [Name]": grants one of the real totem rewards (Swiftness, Might, Frenzy,
	// Piercing) directly, exactly as ARewardTotemSpawner defines it, without needing to find and charge a
	// totem. Omit Name for a random one.
	FAutoConsoleCommandWithWorldAndArgs GGiveTotemBuffCommand(
		TEXT("GrantMoney.GiveTotemBuff"),
		TEXT("Dev only. Gives the player one of the real totem rewards. Usage: GrantMoney.GiveTotemBuff [Swiftness|Might|Frenzy|Piercing]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
		{
			AGrantMoneyPlayerCharacter* Player = Cast<AGrantMoneyPlayerCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
			const ARewardTotemSpawner* Defaults = GetDefault<ARewardTotemSpawner>();
			if (!Player || !Defaults || Defaults->RandomEffectPool.IsEmpty())
			{
				UE_LOG(LogTemp, Warning, TEXT("GrantMoney.GiveTotemBuff: needs a player pawn and a non-empty reward pool."));
				return;
			}

			const UTemporaryEffect* Chosen = nullptr;
			if (Args.Num() > 0)
			{
				const TObjectPtr<UTemporaryEffect>* Found = Defaults->RandomEffectPool.FindByPredicate(
					[&Args](const UTemporaryEffect* Effect) { return Effect && Effect->GetName().Equals(Args[0], ESearchCase::IgnoreCase); });
				Chosen = Found ? *Found : nullptr;
			}
			else
			{
				Chosen = Defaults->RandomEffectPool[FMath::RandRange(0, Defaults->RandomEffectPool.Num() - 1)];
			}

			if (!Chosen)
			{
				UE_LOG(LogTemp, Warning, TEXT("GrantMoney.GiveTotemBuff: no reward named '%s' in the pool."), Args.Num() > 0 ? *Args[0] : TEXT(""));
				return;
			}

			UTemporaryEffectComponent* Effects = UTemporaryEffectComponent::FindOrAdd(Player);
			if (!Effects || !Effects->ApplyEffect(Chosen))
			{
				UE_LOG(LogTemp, Warning, TEXT("GrantMoney.GiveTotemBuff: the effect could not be applied."));
				return;
			}

			UE_LOG(LogTemp, Log, TEXT("GrantMoney.GiveTotemBuff: gave %s."), *Chosen->DisplayName.ToString());
		}));
}

#endif
