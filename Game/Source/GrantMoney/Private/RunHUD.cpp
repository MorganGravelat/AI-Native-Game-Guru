// Fill out your copyright notice in the Description page of Project Settings.

// NO-TEST: canvas drawing only. The values it shows are tested on ARunController (clock, state label)
// and AEnemySpawner (active enemy count, current cap); the layout is checked by eye in PIE.

#include "RunHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EnemySpawner.h"
#include "RunController.h"

namespace
{
	FLinearColor ColorForState(EGrantMoneyRunState State)
	{
		switch (State)
		{
		case EGrantMoneyRunState::Starting:  return FLinearColor::White;
		case EGrantMoneyRunState::Active:    return FLinearColor::Green;
		case EGrantMoneyRunState::BossPhase: return FLinearColor::Red;
		case EGrantMoneyRunState::Won:       return FLinearColor::Yellow;
		case EGrantMoneyRunState::Lost:      return FLinearColor(1.0f, 0.4f, 0.1f);
		}

		return FLinearColor::White;
	}
}

ARunController* ARunHUD::GetRunController()
{
	if (!RunController.IsValid())
	{
		RunController = ARunController::Find(GetWorld());
	}

	return RunController.Get();
}

float ARunHUD::DrawCentredLine(const FString& Text, float Y, float Scale, const FLinearColor& Color)
{
	UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr;
	float Width = 0.0f;
	float Height = 0.0f;
	GetTextSize(Text, Width, Height, Font, Scale);
	DrawText(Text, Color, (Canvas->SizeX - Width) * 0.5f, Y, Font, Scale);
	return Height;
}

void ARunHUD::DrawHUD()
{
	Super::DrawHUD();

	const ARunController* Run = GetRunController();
	if (!Run || !Canvas)
	{
		return;
	}

	const UWorld* World = GetWorld();
	const EGrantMoneyRunState State = Run->GetState();

	float Y = Canvas->SizeY * 0.03f;
	Y += DrawCentredLine(ARunController::FormatClock(Run->GetRemainingSeconds()), Y, 2.0f, FLinearColor::White);

	FString StateLine = ARunController::GetStateLabel(State);
	if (World && World->IsPaused())
	{
		StateLine += TEXT(" (paused)");
	}
	Y += DrawCentredLine(StateLine, Y, 1.2f, ColorForState(State));

	// Enough to watch the cap and replacement while testing.
	for (TActorIterator<AEnemySpawner> It(GetWorld()); It; ++It)
	{
		Y += DrawCentredLine(FString::Printf(TEXT("Enemies %d / %d"), It->GetActiveEnemyCount(), It->GetCurrentMaxActiveEnemies()), Y, 1.0f, FLinearColor::Gray);
		break;
	}
}
