// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GrantMoneyStatTypes.h"
#include "RunController.h"
#include "Types/SlateEnums.h"
#include "GameplayHudWidget.generated.h"
class ABossEncounter;
class AEnemySpawner;
class AGrantMoneyPlayerCharacter;
class UBorder;
class UCanvasPanel;
class UProgressBar;
class UTextBlock;
class UVerticalBox;
class UWidget;
class UHealthComponent;

/**
 * The gameplay HUD: one readable overlay that replaces the old canvas-drawn placeholder (ARunHUD owns it).
 * It is built entirely in C++ from basic panels, bars and text, so it needs no Blueprint asset.
 *
 * Shows, from the existing gameplay classes:
 *  - top left: level, player health bar (current / max), XP progress bar, then the active temporary
 *    buffs with their remaining time and the upgrades taken so far with their stacks;
 *  - top centre: the survival clock, the run phase (with "(paused)") and the enemy count;
 *  - top right: the score (see FRunHudFormat::ComputeScore: XP earned and time survived);
 *  - during the boss fight: the "Boss Arrived" message, any spawn problem and a boss health bar at the bottom.
 *
 * Values are pushed by events and read once when binding, never recomputed every frame:
 * UHealthComponent::OnHealthChanged and UStatsComponent::OnStatChanged (max health, XP), ULevelComponent::OnLevelUp,
 * UUpgradeComponent::OnUpgradeChosen, UTemporaryBuffComponent::OnBuffsChanged and ARunController::OnTimeChanged /
 * OnStateChanged. Only what has no event is read in the tick: a buff's counting-down time, the pause flag, the
 * boss (it appears and disappears on its own schedule) and the enemy count.
 *
 * Binding follows the sources, not the other way round: the tick notices a new or missing player pawn (death,
 * respawn, possession change) or run controller and rebinds, removing every old subscription first, so a replaced
 * player leaves no stale listener and one widget never listens twice. The widget ignores the mouse
 * entirely (hit-test invisible), so it can never take focus or clicks from the pause, level-up or result screens.
 */
UCLASS()
class GRANTMONEY_API UGameplayHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Starts listening to Run (and unsubscribes from the previous one). Null just clears. Refreshes the
	// clock, phase and score at once. Safe to call repeatedly.
	void BindRun(ARunController* InRun);

	// Starts listening to Player's components (and unsubscribes from the previous player). Null just clears.
	// Refreshes health, XP, score, buffs and upgrades at once. Safe to call repeatedly.
	void BindPlayer(AGrantMoneyPlayerCharacter* InPlayer);

	ARunController* GetBoundRun() const { return Run.Get(); }
	AGrantMoneyPlayerCharacter* GetBoundPlayer() const { return Player.Get(); }

	// What each readout currently says; the layout is checked by eye, these let tests check the values.
	FString GetHealthText() const;
	FString GetXpText() const;
	FString GetLevelText() const;
	FString GetTimerText() const;
	FString GetPhaseText() const;
	FString GetScoreText() const;
	float GetHealthPercent() const;
	float GetXpPercent() const;
	int32 GetVisibleBuffRowCount() const { return VisibleBuffRows; }
	int32 GetVisibleUpgradeRowCount() const { return VisibleUpgradeRows; }
	FString GetBuffRowText(int32 Index) const;
	FString GetUpgradeRowText(int32 Index) const;

	// Refreshes everything from the bound sources.
	void RefreshAll();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	// Creates the widget tree once. Called from every entry point so it exists however the widget was made.
	void EnsureLayout();

	UTextBlock* MakeText(int32 Size, const FLinearColor& Color, bool bBold, ETextJustify::Type Justification);
	UBorder* MakePanel(UWidget* Content);

	// Builds a progress bar with its label centred on top, returns the wrapper to place in a box.
	UWidget* MakeBar(float Width, float Height, const FLinearColor& Fill, TObjectPtr<UProgressBar>& OutBar, TObjectPtr<UTextBlock>& OutLabel, int32 LabelSize);

	void UnbindRun();
	void UnbindPlayer();

	// Finds a new player pawn or run controller when the bound one is gone or replaced.
	void ResolveSources(float DeltaTime);

	void RefreshHealth();
	void RefreshXp();
	void RefreshScore();
	void RefreshClock();
	void RefreshPhase();
	void RefreshBuffs();
	void RefreshUpgrades();
	void RefreshBoss();
	void RefreshEnemyCount();

	// Shows exactly Count rows in Box (adding rows as needed, collapsing the rest).
	void EnsureRows(UVerticalBox* Box, TArray<TObjectPtr<UTextBlock>>& Rows, int32 Count);

	UFUNCTION()
	void HandleHealthChanged(UHealthComponent* HealthComponent, float NewHealth, float Delta);

	UFUNCTION()
	void HandleStatChanged(EGrantMoneyStat Stat, float OldValue, float NewValue);

	UFUNCTION()
	void HandleLevelUp(int32 NewLevel, int32 PreviousLevel);

	UFUNCTION()
	void HandleUpgradeChosen();

	UFUNCTION()
	void HandleBuffsChanged();

	UFUNCTION()
	void HandleRunTimeChanged(float ElapsedSeconds, float RemainingSeconds);

	UFUNCTION()
	void HandleRunStateChanged(EGrantMoneyRunState OldState, EGrantMoneyRunState NewState);

	void SetTextIfChanged(UTextBlock* Text, const FString& Value) const;

	bool bLayoutBuilt = false;
	bool bWasPaused = false;

	// Real seconds until the tick looks again for a run, boss encounter or spawner that does not exist yet.
	float SourceSearchCooldown = 0.0f;

	TWeakObjectPtr<ARunController> Run;
	TWeakObjectPtr<AGrantMoneyPlayerCharacter> Player;
	TWeakObjectPtr<ABossEncounter> Encounter;
	TWeakObjectPtr<AEnemySpawner> Spawner;

	int32 VisibleBuffRows = 0;
	int32 VisibleUpgradeRows = 0;

	// Widgets (owned by the widget tree; held here only to update them).
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LevelText;
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HealthLabel;
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> XpBar;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> XpLabel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TimerText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PhaseText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EnemyCountText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ScoreText;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> BuffPanel;
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> BuffBox;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> BuffRows;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> UpgradePanel;
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> UpgradeBox;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> UpgradeRows;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> BossPanel;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BossNameText;
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> BossBar;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BossLabel;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BossMessageText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BossDiagnosticText;
};
