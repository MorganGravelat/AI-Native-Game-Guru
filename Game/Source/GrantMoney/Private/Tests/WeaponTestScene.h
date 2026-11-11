// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// Test helper only. Included by the weapon and projectile tests.

#include "CoreMinimal.h"
#include "AttackHitComponent.h"
#include "CombatEventsSubsystem.h"
#include "EnemyBase.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "HealthComponent.h"
#include "MagicOrbProjectile.h"
#include "Misc/AutomationTest.h"
#include "SlimeEnemy.h"
#include "StarterWeaponComponent.h"
#include "Tests/AutomationCommon.h"
#include "Tests/CombatTestListener.h"

namespace WeaponTest
{
    // An empty game world with play started, a stand-in shooter (a plain character at the origin,
    // standing in for the player) with a UStarterWeaponComponent, and a listener on OnEnemyKilled.
    // Slimes and the shooter are frozen (no movement, no gravity) so positions stay where the test puts
    // them. Nothing is possessed, so slimes have no player to chase and stay still.
    struct FScene
    {
        FTestWorldWrapper WorldWrapper;
        ACharacter* Shooter = nullptr;
        UStarterWeaponComponent* Weapon = nullptr;
        UCombatTestListener* Listener = nullptr;

        bool Create(FAutomationTestBase& Test, float WeaponFireInterval = 1.0f)
        {
            if (!WorldWrapper.CreateTestWorld(EWorldType::Game) || !WorldWrapper.BeginPlayInTestWorld())
            {
                WorldWrapper.ForwardErrorMessages(&Test);
                return false;
            }

            Shooter = SpawnFrozenCharacter<ACharacter>(FVector::ZeroVector);
            if (!Test.TestNotNull(TEXT("Shooter spawned"), Shooter))
            {
                return false;
            }

            // The interval is set before the weapon starts, so a test that fires by hand can keep the
            // timer from firing too, by passing a very long interval.
            Weapon = NewObject<UStarterWeaponComponent>(Shooter);
            Weapon->FireInterval = WeaponFireInterval;
            Weapon->RegisterComponent();

            UCombatEventsSubsystem* CombatEvents = World()->GetSubsystem<UCombatEventsSubsystem>();
            if (!Test.TestNotNull(TEXT("Combat events subsystem exists"), CombatEvents))
            {
                return false;
            }

            Listener = NewObject<UCombatTestListener>();
            CombatEvents->OnEnemyKilled.AddDynamic(Listener, &UCombatTestListener::OnEnemyKilled);
            return true;
        }

        UWorld* World() const { return WorldWrapper.GetTestWorld(); }

        template <typename T>
        T* SpawnFrozenCharacter(const FVector& Location)
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

        ASlimeEnemy* SpawnSlime(const FVector& Location) { return SpawnFrozenCharacter<ASlimeEnemy>(Location); }

        // Advances the world in small steps.
        void Step(float Seconds, float StepSeconds = 0.02f)
        {
            for (float Elapsed = 0.0f; Elapsed < Seconds - UE_KINDA_SMALL_NUMBER; Elapsed += StepSeconds)
            {
                WorldWrapper.TickTestWorld(StepSeconds);
            }
        }

        // Orbs still in the world.
        int32 CountOrbs() const
        {
            int32 Count = 0;
            for (TActorIterator<AMagicOrbProjectile> It(World()); It; ++It)
            {
                Count += It->IsActorBeingDestroyed() ? 0 : 1;
            }
            return Count;
        }

        AMagicOrbProjectile* FirstOrb() const
        {
            for (TActorIterator<AMagicOrbProjectile> It(World()); It; ++It)
            {
                if (!It->IsActorBeingDestroyed())
                {
                    return *It;
                }
            }
            return nullptr;
        }
    };

    inline float HealthOf(const AEnemyBase* Enemy)
    {
        return Enemy->GetHealth()->GetHealth();
    }

    // The orb's flight velocity (direction and speed), straight from its movement component.
    inline FVector VelocityOf(const AMagicOrbProjectile* Orb)
    {
        return Orb->FindComponentByClass<UProjectileMovementComponent>()->Velocity;
    }
}
