// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/SoftObjectPtr.h"
#include "GrantMoneyAudioSubsystem.generated.h"

class UAudioComponent;
class USoundBase;

/**
 * Owns the level's background music and turns it down while something wants the player's attention.
 *
 * Anything that should quiet the music (the pause menu, the level-up screen, a totem that is charging) calls
 * AcquireMusicDuck with itself as the source and ReleaseMusicDuck when it is done. The music is ducked for as
 * long as at least one source holds it, so two totems charging at once do not stack and releasing one does not
 * un-duck the other. A source that is destroyed without releasing is dropped automatically.
 *
 * The game mode starts the music (PlayMusic) and sets the duck volume and fade time from its own properties.
 */
UCLASS()
class GRANTMONEY_API UGrantMoneyAudioSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UGrantMoneyAudioSubsystem* Get(const UObject* WorldContext);

	virtual void Deinitialize() override;

	// Fades out whatever music is playing and starts Music in its place, fading in. An empty slot just fades the
	// music out. The new music starts at the ducked volume if something is holding a duck.
	void PlayMusic(const TSoftObjectPtr<USoundBase>& Music, float FadeSeconds);

	// Quiets the music for as long as Source holds it. Safe to call again with the same source.
	void AcquireMusicDuck(const UObject* Source);

	// Lets Source go. The music returns to full volume once no source holds it. Safe to call without acquiring.
	void ReleaseMusicDuck(const UObject* Source);

	bool IsMusicDucked();

	// The volume the music should be at right now: MusicDuckVolume while ducked, otherwise 1.
	float GetMusicVolume();

	// Music volume while ducked, as a fraction of normal (0.6 = 40% quieter).
	float MusicDuckVolume = 0.6f;

	// Seconds the volume takes to move between normal and ducked.
	float MusicDuckFadeSeconds = 0.4f;

private:
	void ApplyMusicVolume();

	// Plays the current music again when it ends, so a track that is not set to Looping still repeats.
	void HandleMusicFinished(UAudioComponent* Finished);

	void StartMusic(const TSoftObjectPtr<USoundBase>& NewMusic, float FadeSeconds);

	TSet<TWeakObjectPtr<const UObject>> DuckSources;

	// The track that should be playing, and when it last started (real seconds).
	TSoftObjectPtr<USoundBase> CurrentMusic;
	double MusicStartTime = 0.0;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Music;
};
