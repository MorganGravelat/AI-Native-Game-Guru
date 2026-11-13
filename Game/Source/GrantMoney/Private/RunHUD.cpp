// Fill out your copyright notice in the Description page of Project Settings.

// NO-TEST: canvas drawing only. The values it shows are tested on ARunController (clock, state label),
// ULevelComponent (level, XP progress), UHealthComponent (player health), AEnemySpawner (active enemy count,
// current cap) and ABossEncounter (arrival message, spawn diagnostic, boss state); the layout is checked by
// eye in PIE.

#include "RunHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "BossEncounter.h"
#include "EnemyBase.h"
#include "EnemySpawner.h"
#include "GrantMoneyPlayerCharacter.h"
#include "HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LevelComponent.h"
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

	// Level and XP progress (SCRUM-36), exposed to the HUD as required.
	if (const AGrantMoneyPlayerCharacter* PlayerCharacter = Cast<AGrantMoneyPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		if (const ULevelComponent* Level = PlayerCharacter->GetLevelComponent())
		{
			Y += DrawCentredLine(FString::Printf(TEXT("Level %d  (%.0f / %.0f XP)"), Level->GetLevel(), Level->GetXpIntoLevel(), Level->GetXpToNextLevel()), Y, 1.0f, FLinearColor::White);
		}

		// Needed to judge the boss's contact damage and hit cadence.
		if (const UHealthComponent* PlayerHealth = PlayerCharacter->GetHealth())
		{
			Y += DrawCentredLine(FString::Printf(TEXT("HP %.0f / %.0f"), PlayerHealth->GetHealth(), PlayerHealth->GetMaxHealth()), Y, 1.0f, FLinearColor(1.0f, 0.4f, 0.4f));
		}
	}

	// Enough to watch the cap and replacement while testing.
	for (TActorIterator<AEnemySpawner> It(GetWorld()); It; ++It)
	{
		Y += DrawCentredLine(FString::Printf(TEXT("Enemies %d / %d"), It->GetActiveEnemyCount(), It->GetCurrentMaxActiveEnemies()), Y, 1.0f, FLinearColor::Gray);
		break;
	}

	DrawBossUi();
}

ABossEncounter* ARunHUD::GetBossEncounter()
{
	if (!BossEncounter.IsValid())
	{
		BossEncounter = ABossEncounter::Find(GetWorld());
	}

	return BossEncounter.Get();
}

void ARunHUD::DrawBossUi()
{
	const ABossEncounter* Encounter = GetBossEncounter();
	if (!Encounter)
	{
		return;
	}

	// A failed or retrying boss spawn must never be silent: the run would otherwise sit in an empty boss phase.
	const FString Diagnostic = Encounter->GetSpawnDiagnostic();
	if (!Diagnostic.IsEmpty())
	{
		const bool bFailed = Encounter->GetState() == EBossEncounterState::Failed;
		DrawCentredLine(Diagnostic, Canvas->SizeY * 0.35f, 1.2f, bFailed ? FLinearColor::Red : FLinearColor(1.0f, 0.6f, 0.1f));
	}

	if (Encounter->IsArrivalMessageVisible())
	{
		DrawCentredLine(TEXT("BOSS ARRIVED"), Canvas->SizeY * 0.2f, 3.0f, FLinearColor(0.8f, 0.2f, 1.0f));
	}

	const AEnemyBase* Boss = Encounter->GetBoss();
	const UHealthComponent* Health = (Encounter->GetState() == EBossEncounterState::Fighting && IsValid(Boss)) ? Boss->GetHealth() : nullptr;
	if (!Health || Health->IsDead())
	{
		return;
	}

	const float BarWidth = Canvas->SizeX * 0.5f;
	const float BarHeight = FMath::Max(14.0f, Canvas->SizeY * 0.025f);
	const float BarX = (Canvas->SizeX - BarWidth) * 0.5f;
	const float BarY = Canvas->SizeY * 0.92f;
	const float Fraction = FMath::Clamp(Health->GetHealthFraction(), 0.0f, 1.0f);

	DrawCentredLine(Encounter->BossDisplayName, BarY - BarHeight * 2.2f, 1.0f, FLinearColor::White);
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f), BarX - 3.0f, BarY - 3.0f, BarWidth + 6.0f, BarHeight + 6.0f);
	DrawRect(FLinearColor(0.25f, 0.0f, 0.0f, 1.0f), BarX, BarY, BarWidth, BarHeight);
	DrawRect(FLinearColor(0.85f, 0.1f, 0.1f, 1.0f), BarX, BarY, BarWidth * Fraction, BarHeight);
}
