// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// Test helper only. Included by the pickup and enemy-drop tests.

#include "CoreMinimal.h"
#include "EnemyBase.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HealthComponent.h"
#include "Misc/AutomationTest.h"
#include "PickupBase.h"
#include "PickupComponent.h"
#include "SlimeEnemy.h"
#include "StatsComponent.h"
#include "Tests/AutomationCommon.h"
#include "Tests/PickupTestTypes.h"
#include "XpOrbPickup.h"

namespace PickupTest
{
    // An empty game world with play started and a stand-in player: a plain character at the origin
    // with a Stats component and a UPickupComponent. The collector is frozen (no movement, no gravity),
    // so a test moves it by hand to simulate walking or sprinting.
    struct FScene
    {
        FTestWorldWrapper WorldWrapper;
        ACharacter* Collector = nullptr;
        UStatsComponent* Stats = nullptr;
        UPickupComponent* Pickups = nullptr;

        bool Create(FAutomationTestBase& Test, float PickupRadius = 300.0f)
        {
            ATestBuffPickup::TotalCollections = 0;
            ATestBuffPickup::LastCollector.Reset();

            if (!WorldWrapper.CreateTestWorld(EWorldType::Game) || !WorldWrapper.BeginPlayInTestWorld())
            {
                WorldWrapper.ForwardErrorMessages(&Test);
                return false;
            }

            Collector = SpawnFrozen<ACharacter>(FVector::ZeroVector);
            if (!Test.TestNotNull(TEXT("Collector spawned"), Collector))
            {
                return false;
            }

            Stats = NewObject<UStatsComponent>(Collector);
            Stats->RegisterComponent();
            Stats->SetBaseStat(EGrantMoneyStat::Experience, 0.0f);

            Pickups = NewObject<UPickupComponent>(Collector);
            Pickups->PickupRadius = PickupRadius;
            Pickups->RegisterComponent();
            return true;
        }

        UWorld* World() const { return WorldWrapper.GetTestWorld(); }

        template <typename T>
        T* SpawnFrozen(const FVector& Location)
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            T* Character = World()->SpawnActor<T>(T::StaticClass(), FTransform(Location), Params);
            if (Character)
            {
                Character->GetCharacterMovement()->DisableMovement();
            }
            return Character;
        }

        ASlimeEnemy* SpawnSlime(const FVector& Location) { return SpawnFrozen<ASlimeEnemy>(Location); }

        template <typename T = AXpOrbPickup>
        T* SpawnPickup(const FVector& Location)
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            return World()->SpawnActor<T>(T::StaticClass(), FTransform(Location), Params);
        }

        // Advances the world in small steps.
        void Step(float Seconds, float StepSeconds = 0.02f)
        {
            for (float Elapsed = 0.0f; Elapsed < Seconds - UE_KINDA_SMALL_NUMBER; Elapsed += StepSeconds)
            {
                WorldWrapper.TickTestWorld(StepSeconds);
            }
        }

        // Pickups of type T still in the world.
        template <typename T = APickupBase>
        int32 CountPickups() const
        {
            int32 Count = 0;
            for (TActorIterator<T> It(World()); It; ++It)
            {
                Count += It->IsActorBeingDestroyed() ? 0 : 1;
            }
            return Count;
        }

        template <typename T = APickupBase>
        T* FirstPickup() const
        {
            for (TActorIterator<T> It(World()); It; ++It)
            {
                if (!It->IsActorBeingDestroyed())
                {
                    return *It;
                }
            }
            return nullptr;
        }

        float Experience() const { return Stats->GetStat(EGrantMoneyStat::Experience); }
    };
}
