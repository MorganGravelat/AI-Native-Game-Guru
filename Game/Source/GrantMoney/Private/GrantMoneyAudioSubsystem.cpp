// Fill out your copyright notice in the Description page of Project Settings.

#include "GrantMoneyAudioSubsystem.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GrantMoneyAudio.h"
#include "HAL/PlatformTime.h"

UGrantMoneyAudioSubsystem* UGrantMoneyAudioSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UGrantMoneyAudioSubsystem>() : nullptr;
}

void UGrantMoneyAudioSubsystem::Deinitialize()
{
	DuckSources.Reset();
	CurrentMusic.Reset();
	Music = nullptr;
	Super::Deinitialize();
}

void UGrantMoneyAudioSubsystem::PlayMusic(const TSoftObjectPtr<USoundBase>& NewMusic, float FadeSeconds)
{
	if (IsValid(Music))
	{
		// No longer the current music, so its ending does not restart it.
		UAudioComponent* Old = Music;
		Music = nullptr;
		Old->FadeOut(FadeSeconds, 0.0f);
	}

	StartMusic(NewMusic, FadeSeconds);
}

void UGrantMoneyAudioSubsystem::StartMusic(const TSoftObjectPtr<USoundBase>& NewMusic, float FadeSeconds)
{
	CurrentMusic = NewMusic;
	MusicStartTime = FPlatformTime::Seconds();
	Music = GrantMoneyAudio::SpawnMusic(this, NewMusic, FadeSeconds, GetMusicVolume());

	if (Music)
	{
		Music->OnAudioFinishedNative.AddUObject(this, &UGrantMoneyAudioSubsystem::HandleMusicFinished);
	}
}

void UGrantMoneyAudioSubsystem::HandleMusicFinished(UAudioComponent* Finished)
{
	// Only the music that is meant to be playing restarts, not one that was faded out or replaced.
	if (Finished != Music || CurrentMusic.IsNull())
	{
		return;
	}

	// A sound that ends almost at once would otherwise restart forever.
	constexpr double MinPlaySeconds = 1.0;
	if (FPlatformTime::Seconds() - MusicStartTime < MinPlaySeconds)
	{
		Music = nullptr;
		return;
	}

	StartMusic(CurrentMusic, 0.0f);
}

void UGrantMoneyAudioSubsystem::AcquireMusicDuck(const UObject* Source)
{
	if (!Source)
	{
		return;
	}

	const bool bWasDucked = IsMusicDucked();
	DuckSources.Add(Source);

	if (!bWasDucked)
	{
		ApplyMusicVolume();
	}
}

void UGrantMoneyAudioSubsystem::ReleaseMusicDuck(const UObject* Source)
{
	if (DuckSources.Remove(Source) > 0)
	{
		ApplyMusicVolume();
	}
}

bool UGrantMoneyAudioSubsystem::IsMusicDucked()
{
	// A source that was destroyed without releasing must not hold the music down for good.
	for (auto It = DuckSources.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}

	return !DuckSources.IsEmpty();
}

float UGrantMoneyAudioSubsystem::GetMusicVolume()
{
	return IsMusicDucked() ? FMath::Clamp(MusicDuckVolume, 0.0f, 1.0f) : 1.0f;
}

void UGrantMoneyAudioSubsystem::ApplyMusicVolume()
{
	if (IsValid(Music))
	{
		Music->AdjustVolume(MusicDuckFadeSeconds, GetMusicVolume());
	}
}
