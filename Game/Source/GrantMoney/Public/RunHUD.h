// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RunHUD.generated.h"

class ARunController;

/**
 * Minimal on-screen readout of the run, drawn straight onto the canvas so it needs no widget assets:
 * the countdown clock, the run state and the spawner's active-enemy count (against its current,
 * difficulty-ramped cap). There is no score line: the character's level is the run's progress readout
 * once it exists.
 * Temporary: the real HUD replaces it, reading the same ARunController / AEnemySpawner getters and events.
 * The game mode makes it the player's HUD class.
 */
UCLASS()
class GRANTMONEY_API ARunHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	// Finds the level's run controller. Looked up again whenever it is missing, because the
	// game mode may create it after the HUD.
	ARunController* GetRunController();

	// Draws Text centred on the screen at Y. Returns the height used, so lines can be stacked.
	float DrawCentredLine(const FString& Text, float Y, float Scale, const FLinearColor& Color);

	TWeakObjectPtr<ARunController> RunController;
};
