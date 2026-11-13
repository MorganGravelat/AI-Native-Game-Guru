// Fill out your copyright notice in the Description page of Project Settings.

#include "GrantMoneyAudio.h"

#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace GrantMoneyAudio
{
	TSoftObjectPtr<USoundBase> DefaultSound(const TCHAR* ObjectPath)
	{
		return TSoftObjectPtr<USoundBase>(FSoftObjectPath(ObjectPath));
	}

	USoundBase* Resolve(const TSoftObjectPtr<USoundBase>& Sound)
	{
		// No audio device (a dedicated server, or the headless test run): nothing would play, so nothing is loaded.
		if (Sound.IsNull() || !GEngine || !GEngine->UseSound())
		{
			return nullptr;
		}

		return Sound.LoadSynchronous();
	}

	void Play2D(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Sound, float Volume)
	{
		if (USoundBase* Loaded = Resolve(Sound))
		{
			UGameplayStatics::PlaySound2D(WorldContext, Loaded, Volume);
		}
	}

	void PlayAtLocation(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Sound, const FVector& Location)
	{
		if (USoundBase* Loaded = Resolve(Sound))
		{
			UGameplayStatics::PlaySoundAtLocation(WorldContext, Loaded, Location);
		}
	}

	UAudioComponent* SpawnMusic(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Sound, float FadeInSeconds, float TargetVolume)
	{
		USoundBase* Loaded = Resolve(Sound);
		if (!Loaded)
		{
			return nullptr;
		}

		UAudioComponent* Music = UGameplayStatics::SpawnSound2D(WorldContext, Loaded, 1.0f, 1.0f, 0.0f, nullptr, false, true);
		if (Music)
		{
			// Keeps playing while the game is paused, so the pause menu and level-up screen can duck it instead of silencing it.
			Music->SetUISound(true);

			if (FadeInSeconds > 0.0f)
			{
				Music->FadeIn(FadeInSeconds, TargetVolume);
			}
			else
			{
				Music->SetVolumeMultiplier(TargetVolume);
			}
		}

		return Music;
	}
}
