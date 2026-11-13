// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "UObject/SoftObjectPtr.h"

class USoundBase;
class UAudioComponent;
class UObject;

/**
 * Small helpers for the sound slots on gameplay classes.
 *
 * A slot is a TSoftObjectPtr<USoundBase> that a class fills with a default path in its constructor, so the
 * sound works without opening a Blueprint, and a Blueprint child can still replace or clear it. Nothing is loaded
 * until the sound first plays, and an empty or missing slot is silently skipped.
 */
namespace GrantMoneyAudio
{
	// A slot default: the sound asset at ObjectPath (for example "/Game/Sound/Enemy/SlimeHit.SlimeHit").
	GRANTMONEY_API TSoftObjectPtr<USoundBase> DefaultSound(const TCHAR* ObjectPath);

	// The loaded sound, or null if the slot is empty, the asset is missing, or there is no audio device.
	GRANTMONEY_API USoundBase* Resolve(const TSoftObjectPtr<USoundBase>& Sound);

	// Not positioned in the world: heard at the same volume wherever the camera is. Plays while the game is paused.
	GRANTMONEY_API void Play2D(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Sound, float Volume = 1.0f);

	// At a spot in the world. Heard at full volume unless the sound asset has attenuation settings.
	GRANTMONEY_API void PlayAtLocation(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Sound, const FVector& Location);

	// A 2D sound that can be faded out later (music). Fades in to TargetVolume. Null if the slot is empty or
	// there is no audio device.
	GRANTMONEY_API UAudioComponent* SpawnMusic(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Sound, float FadeInSeconds, float TargetVolume = 1.0f);
}
