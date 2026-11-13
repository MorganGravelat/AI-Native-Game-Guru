// Fill out your copyright notice in the Description page of Project Settings.

// NO-TEST: life-cycle glue for the HUD widget (create on BeginPlay, remove on EndPlay). The widget's own
// behaviour is tested in GameplayHudWidgetTests.cpp; the on-screen result is checked in the Editor.

#include "RunHUD.h"

#include "GameplayHudWidget.h"
#include "GameFramework/PlayerController.h"

ARunHUD::ARunHUD()
{
	GameplayHudClass = UGameplayHudWidget::StaticClass();
}

void ARunHUD::BeginPlay()
{
	Super::BeginPlay();

	// BeginPlay runs once per actor, but guard anyway: a second widget would double every readout.
	if (IsValid(GameplayHud) || !GameplayHudClass)
	{
		return;
	}

	APlayerController* LocalPlayer = GetOwningPlayerController();
	if (!LocalPlayer || !LocalPlayer->IsLocalController())
	{
		return;
	}

	GameplayHud = CreateWidget<UGameplayHudWidget>(LocalPlayer, GameplayHudClass);
	if (GameplayHud)
	{
		// Layer 0: every screen that takes input (pause 50, level-up 60, result 70) is drawn above it.
		GameplayHud->AddToViewport(0);
	}
}

void ARunHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(GameplayHud))
	{
		GameplayHud->RemoveFromParent();
	}

	GameplayHud = nullptr;
	Super::EndPlay(EndPlayReason);
}
