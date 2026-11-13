// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// Test helper only. Included by the temporary effect and reward totem tests.

#include "CoreMinimal.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "StatsComponent.h"
#include "Tests/AutomationCommon.h"
#include "Tests/TemporaryEffectTestTypes.h"

namespace EffectTest
{
	// The player's base move speed in these tests.
	constexpr float BaseMoveSpeed = 600.0f;

	// An empty game world with play started and a stand-in player: a frozen plain character with a
	// Stats component (MoveSpeed 600). Tests move it by hand.
	struct FScene
	{
		FTestWorldWrapper WorldWrapper;
		ACharacter* Player = nullptr;
		UStatsComponent* Stats = nullptr;

		bool Create(FAutomationTestBase& Test, const FVector& PlayerLocation = FVector(0.0, 0.0, 100.0))
		{
			UTestCountingEffect::ResetCounts();

			if (!WorldWrapper.CreateTestWorld(EWorldType::Game) || !WorldWrapper.BeginPlayInTestWorld())
			{
				WorldWrapper.ForwardErrorMessages(&Test);
				return false;
			}

			Player = SpawnCharacter(PlayerLocation);
			if (!Test.TestNotNull(TEXT("Player spawned"), Player))
			{
				return false;
			}

			Stats = NewObject<UStatsComponent>(Player);
			Stats->RegisterComponent();
			Stats->SetBaseStat(EGrantMoneyStat::MoveSpeed, BaseMoveSpeed);
			return true;
		}

		UWorld* World() const { return WorldWrapper.GetTestWorld(); }

		// Pauses or resumes the world the way the pause menu and the level-up choice screen do: a player
		// state holds the pause (see MapCameraPlayerController::OpenPauseMenu / ShowLevelUpChoice).
		void SetPaused(bool bPaused)
		{
			World()->GetWorldSettings()->SetPauserPlayerState(bPaused ? World()->SpawnActor<APlayerState>() : nullptr);
		}

		ACharacter* SpawnCharacter(const FVector& Location)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ACharacter* Character = World()->SpawnActor<ACharacter>(ACharacter::StaticClass(), FTransform(Location), Params);
			if (Character)
			{
				Character->GetCharacterMovement()->DisableMovement();
			}
			return Character;
		}

		// Advances the world in small steps.
		void Step(float Seconds, float StepSeconds = 0.02f)
		{
			for (float Elapsed = 0.0f; Elapsed < Seconds - UE_KINDA_SMALL_NUMBER; Elapsed += StepSeconds)
			{
				WorldWrapper.TickTestWorld(StepSeconds);
			}
		}

		float MoveSpeed() const { return Stats->GetStat(EGrantMoneyStat::MoveSpeed); }

		int32 AuraLightCount() const
		{
			TArray<UPointLightComponent*> Lights;
			Player->GetComponents(Lights);
			return Lights.Num();
		}
	};
}
