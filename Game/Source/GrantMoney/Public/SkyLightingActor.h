// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkyLightingActor.generated.h"

class UDirectionalLightComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;

/**
 * Procedural skybox and global lighting: sky atmosphere, one sun that drives it, and a
 * real-time-captured sky light for ambient fill.
 * AGrantMoneyGameMode spawns one into every level that does not already contain one,
 * so a designer can still drop one into a level to override the defaults.
 */
UCLASS()
class GRANTMONEY_API ASkyLightingActor : public AActor
{
	GENERATED_BODY()

public:
	ASkyLightingActor();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sky")
	TObjectPtr<USkyAtmosphereComponent> SkyAtmosphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sky")
	TObjectPtr<UDirectionalLightComponent> Sun;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sky")
	TObjectPtr<USkyLightComponent> SkyLight;
};
