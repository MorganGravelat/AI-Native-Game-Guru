// Fill out your copyright notice in the Description page of Project Settings.

// NO-TEST (layout only): the widget tree and its colours are checked by eye in the Editor. What it shows is
// tested: the formulas and text in FRunHudFormat, the buff clock in UTemporaryBuffComponent, the upgrade
// history in UUpgradeComponent, and the binding / rebinding behaviour in GameplayHudWidgetTests.cpp.

#include "GameplayHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "BossEncounter.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EnemyBase.h"
#include "EnemySpawner.h"
#include "EngineUtils.h"
#include "GrantMoneyPlayerCharacter.h"
#include "HealthComponent.h"
#include "LevelComponent.h"
#include "RunHudFormat.h"
#include "RunUiPolicy.h"
#include "StatsComponent.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "TemporaryBuffComponent.h"
#include "UpgradeComponent.h"

namespace
{
	const FLinearColor PanelColor(0.02f, 0.02f, 0.05f, 0.78f);
	const FLinearColor BarBackColor(0.03f, 0.03f, 0.05f, 0.95f);
	const FLinearColor LabelGrey(0.72f, 0.74f, 0.80f, 1.0f);
	const FLinearColor Gold(1.0f, 0.82f, 0.25f, 1.0f);
	const FLinearColor HealthRed(0.80f, 0.14f, 0.14f, 1.0f);
	const FLinearColor XpBlue(0.20f, 0.60f, 1.0f, 1.0f);
	const FLinearColor BuffGreen(0.55f, 1.0f, 0.60f, 1.0f);
	const FLinearColor BossPurple(0.80f, 0.30f, 1.0f, 1.0f);

	FLinearColor ColorForState(EGrantMoneyRunState State)
	{
		switch (State)
		{
		case EGrantMoneyRunState::Starting:  return FLinearColor::White;
		case EGrantMoneyRunState::Active:    return FLinearColor(0.35f, 1.0f, 0.45f, 1.0f);
		case EGrantMoneyRunState::BossPhase: return FLinearColor(1.0f, 0.30f, 0.30f, 1.0f);
		case EGrantMoneyRunState::Won:       return Gold;
		case EGrantMoneyRunState::Lost:      return FLinearColor(1.0f, 0.45f, 0.15f, 1.0f);
		}

		return FLinearColor::White;
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------------------------------------------

void UGameplayHudWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EnsureLayout();
}

TSharedRef<SWidget> UGameplayHudWidget::RebuildWidget()
{
	EnsureLayout();
	return Super::RebuildWidget();
}

UTextBlock* UGameplayHudWidget::MakeText(int32 Size, const FLinearColor& Color, bool bBold, ETextJustify::Type Justification)
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Text->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size));
	Text->SetColorAndOpacity(FSlateColor(Color));
	Text->SetShadowOffset(FVector2D(1.5f, 1.5f));
	Text->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.95f));
	Text->SetJustification(Justification);
	return Text;
}

UBorder* UGameplayHudWidget::MakePanel(UWidget* Content)
{
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Panel->SetBrush(FSlateColorBrush(PanelColor));
	Panel->SetPadding(FMargin(14.0f, 10.0f));
	Panel->SetContent(Content);
	return Panel;
}

UWidget* UGameplayHudWidget::MakeBar(float Width, float Height, const FLinearColor& Fill, TObjectPtr<UProgressBar>& OutBar, TObjectPtr<UTextBlock>& OutLabel, int32 LabelSize)
{
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Size->SetWidthOverride(Width);
	Size->SetHeightOverride(Height);

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	Size->AddChild(Overlay);

	OutBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
	FProgressBarStyle Style = OutBar->GetWidgetStyle();
	Style.SetBackgroundImage(FSlateColorBrush(BarBackColor));
	Style.SetFillImage(FSlateColorBrush(FLinearColor::White));
	OutBar->SetWidgetStyle(Style);
	OutBar->SetFillColorAndOpacity(Fill);
	OutBar->SetPercent(0.0f);

	UOverlaySlot* BarSlot = Cast<UOverlaySlot>(Overlay->AddChild(OutBar));
	BarSlot->SetHorizontalAlignment(HAlign_Fill);
	BarSlot->SetVerticalAlignment(VAlign_Fill);

	OutLabel = MakeText(LabelSize, FLinearColor::White, true, ETextJustify::Center);
	UOverlaySlot* LabelSlot = Cast<UOverlaySlot>(Overlay->AddChild(OutLabel));
	LabelSlot->SetHorizontalAlignment(HAlign_Center);
	LabelSlot->SetVerticalAlignment(VAlign_Center);

	return Size;
}

void UGameplayHudWidget::EnsureLayout()
{
	if (bLayoutBuilt)
	{
		return;
	}

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	bLayoutBuilt = true;

	// The HUD only displays: it must never take clicks or keyboard focus from the screens above it.
	SetVisibility(ESlateVisibility::HitTestInvisible);

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Root;

	auto Place = [Root](UWidget* Widget, const FAnchors& Anchors, const FVector2D& Alignment, const FVector2D& Position)
	{
		UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Widget);
		Slot->SetAnchors(Anchors);
		Slot->SetAlignment(Alignment);
		Slot->SetPosition(Position);
		Slot->SetAutoSize(true);
	};

	auto AddTo = [](UVerticalBox* Box, UWidget* Widget, EHorizontalAlignment HAlign, float TopPadding)
	{
		UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Widget);
		Slot->SetHorizontalAlignment(HAlign);
		Slot->SetPadding(FMargin(0.0f, TopPadding, 0.0f, 0.0f));
	};

	// Top left: player status, buffs, upgrades.
	UVerticalBox* LeftColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	UVerticalBox* PlayerBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	LevelText = MakeText(24, Gold, true, ETextJustify::Left);
	AddTo(PlayerBox, LevelText, HAlign_Left, 0.0f);
	AddTo(PlayerBox, MakeBar(340.0f, 30.0f, HealthRed, HealthBar, HealthLabel, 18), HAlign_Left, 6.0f);
	AddTo(PlayerBox, MakeBar(340.0f, 22.0f, XpBlue, XpBar, XpLabel, 14), HAlign_Left, 6.0f);
	AddTo(LeftColumn, MakePanel(PlayerBox), HAlign_Left, 0.0f);

	UVerticalBox* BuffInner = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UTextBlock* BuffTitle = MakeText(13, LabelGrey, true, ETextJustify::Left);
	BuffTitle->SetText(FText::FromString(TEXT("BUFFS")));
	AddTo(BuffInner, BuffTitle, HAlign_Left, 0.0f);
	BuffBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	AddTo(BuffInner, BuffBox, HAlign_Left, 2.0f);
	BuffPanel = MakePanel(BuffInner);
	BuffPanel->SetVisibility(ESlateVisibility::Collapsed);
	AddTo(LeftColumn, BuffPanel, HAlign_Left, 8.0f);

	UVerticalBox* UpgradeInner = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UTextBlock* UpgradeTitle = MakeText(13, LabelGrey, true, ETextJustify::Left);
	UpgradeTitle->SetText(FText::FromString(TEXT("UPGRADES")));
	AddTo(UpgradeInner, UpgradeTitle, HAlign_Left, 0.0f);
	UpgradeBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	AddTo(UpgradeInner, UpgradeBox, HAlign_Left, 2.0f);
	UpgradePanel = MakePanel(UpgradeInner);
	UpgradePanel->SetVisibility(ESlateVisibility::Collapsed);
	AddTo(LeftColumn, UpgradePanel, HAlign_Left, 8.0f);

	Place(LeftColumn, FAnchors(0.0f, 0.0f), FVector2D(0.0f, 0.0f), FVector2D(24.0f, 24.0f));

	// Top centre: clock, phase, enemy count.
	UVerticalBox* CentreBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	TimerText = MakeText(54, FLinearColor::White, true, ETextJustify::Center);
	AddTo(CentreBox, TimerText, HAlign_Center, 0.0f);
	PhaseText = MakeText(22, FLinearColor::White, true, ETextJustify::Center);
	AddTo(CentreBox, PhaseText, HAlign_Center, 0.0f);
	EnemyCountText = MakeText(14, LabelGrey, false, ETextJustify::Center);
	EnemyCountText->SetVisibility(ESlateVisibility::Collapsed);
	AddTo(CentreBox, EnemyCountText, HAlign_Center, 2.0f);
	Place(MakePanel(CentreBox), FAnchors(0.5f, 0.0f), FVector2D(0.5f, 0.0f), FVector2D(0.0f, 16.0f));

	// Top right: score.
	UVerticalBox* ScoreBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UTextBlock* ScoreTitle = MakeText(13, LabelGrey, true, ETextJustify::Right);
	ScoreTitle->SetText(FText::FromString(TEXT("SCORE")));
	AddTo(ScoreBox, ScoreTitle, HAlign_Right, 0.0f);
	ScoreText = MakeText(40, Gold, true, ETextJustify::Right);
	AddTo(ScoreBox, ScoreText, HAlign_Right, 0.0f);
	Place(MakePanel(ScoreBox), FAnchors(1.0f, 0.0f), FVector2D(1.0f, 0.0f), FVector2D(-24.0f, 24.0f));

	// Bottom centre: boss health.
	UVerticalBox* BossBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	BossNameText = MakeText(24, FLinearColor::White, true, ETextJustify::Center);
	AddTo(BossBox, BossNameText, HAlign_Center, 0.0f);
	AddTo(BossBox, MakeBar(760.0f, 32.0f, HealthRed, BossBar, BossLabel, 18), HAlign_Center, 6.0f);
	BossPanel = MakePanel(BossBox);
	BossPanel->SetVisibility(ESlateVisibility::Collapsed);
	Place(BossPanel, FAnchors(0.5f, 1.0f), FVector2D(0.5f, 1.0f), FVector2D(0.0f, -36.0f));

	// Upper middle: boss arrival message and any spawn problem.
	UVerticalBox* MessageBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	BossMessageText = MakeText(60, BossPurple, true, ETextJustify::Center);
	BossMessageText->SetText(FText::FromString(TEXT("BOSS ARRIVED")));
	BossMessageText->SetVisibility(ESlateVisibility::Collapsed);
	AddTo(MessageBox, BossMessageText, HAlign_Center, 0.0f);
	BossDiagnosticText = MakeText(24, FLinearColor(1.0f, 0.6f, 0.1f, 1.0f), true, ETextJustify::Center);
	BossDiagnosticText->SetVisibility(ESlateVisibility::Collapsed);
	AddTo(MessageBox, BossDiagnosticText, HAlign_Center, 6.0f);
	Place(MessageBox, FAnchors(0.5f, 0.28f), FVector2D(0.5f, 0.5f), FVector2D(0.0f, 0.0f));

	RefreshAll();
}

// ---------------------------------------------------------------------------------------------------------------
// Binding
// ---------------------------------------------------------------------------------------------------------------

void UGameplayHudWidget::UnbindRun()
{
	if (ARunController* Bound = Run.Get())
	{
		Bound->OnTimeChanged.RemoveAll(this);
		Bound->OnStateChanged.RemoveAll(this);
	}

	Run.Reset();
}

void UGameplayHudWidget::UnbindPlayer()
{
	if (AGrantMoneyPlayerCharacter* Bound = Player.Get())
	{
		if (UHealthComponent* Health = Bound->GetHealth()) { Health->OnHealthChanged.RemoveAll(this); }
		if (UStatsComponent* Stats = Bound->GetStats()) { Stats->OnStatChanged.RemoveAll(this); }
		if (ULevelComponent* Level = Bound->GetLevelComponent()) { Level->OnLevelUp.RemoveAll(this); }
		if (UUpgradeComponent* Upgrades = Bound->GetUpgradeComponent()) { Upgrades->OnUpgradeChosen.RemoveAll(this); }
		if (UTemporaryBuffComponent* Buffs = Bound->GetTemporaryBuffs()) { Buffs->OnBuffsChanged.RemoveAll(this); }
	}

	Player.Reset();
}

void UGameplayHudWidget::BindRun(ARunController* InRun)
{
	EnsureLayout();
	UnbindRun();

	Run = InRun;
	if (InRun)
	{
		InRun->OnTimeChanged.AddUniqueDynamic(this, &UGameplayHudWidget::HandleRunTimeChanged);
		InRun->OnStateChanged.AddUniqueDynamic(this, &UGameplayHudWidget::HandleRunStateChanged);
	}

	RefreshClock();
	RefreshPhase();
	RefreshScore();
}

void UGameplayHudWidget::BindPlayer(AGrantMoneyPlayerCharacter* InPlayer)
{
	EnsureLayout();
	UnbindPlayer();

	Player = InPlayer;
	if (InPlayer)
	{
		if (UHealthComponent* Health = InPlayer->GetHealth()) { Health->OnHealthChanged.AddUniqueDynamic(this, &UGameplayHudWidget::HandleHealthChanged); }
		if (UStatsComponent* Stats = InPlayer->GetStats()) { Stats->OnStatChanged.AddUniqueDynamic(this, &UGameplayHudWidget::HandleStatChanged); }
		if (ULevelComponent* Level = InPlayer->GetLevelComponent()) { Level->OnLevelUp.AddUniqueDynamic(this, &UGameplayHudWidget::HandleLevelUp); }
		if (UUpgradeComponent* Upgrades = InPlayer->GetUpgradeComponent()) { Upgrades->OnUpgradeChosen.AddUniqueDynamic(this, &UGameplayHudWidget::HandleUpgradeChosen); }
		if (UTemporaryBuffComponent* Buffs = InPlayer->GetTemporaryBuffs()) { Buffs->OnBuffsChanged.AddUniqueDynamic(this, &UGameplayHudWidget::HandleBuffsChanged); }
	}

	RefreshHealth();
	RefreshXp();
	RefreshScore();
	RefreshBuffs();
	RefreshUpgrades();
}

void UGameplayHudWidget::NativeDestruct()
{
	UnbindRun();
	UnbindPlayer();
	Super::NativeDestruct();
}

void UGameplayHudWidget::ResolveSources(float DeltaTime)
{
	// Cheap, so checked every frame: a respawn or possession change gives a different pawn.
	AGrantMoneyPlayerCharacter* CurrentPlayer = Cast<AGrantMoneyPlayerCharacter>(GetOwningPlayerPawn());
	if (CurrentPlayer != Player.Get())
	{
		BindPlayer(CurrentPlayer);
	}

	// Searching the level is not, so a missing run, encounter or spawner is looked for twice a second.
	SourceSearchCooldown -= DeltaTime;
	if (SourceSearchCooldown > 0.0f)
	{
		return;
	}

	SourceSearchCooldown = 0.5f;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!Run.IsValid())
	{
		if (ARunController* Found = ARunController::Find(World))
		{
			BindRun(Found);
		}
	}

	if (!Encounter.IsValid())
	{
		Encounter = ABossEncounter::Find(World);
	}

	if (!Spawner.IsValid())
	{
		for (TActorIterator<AEnemySpawner> It(World); It; ++It)
		{
			Spawner = *It;
			break;
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Event handlers
// ---------------------------------------------------------------------------------------------------------------

void UGameplayHudWidget::HandleHealthChanged(UHealthComponent* HealthComponent, float NewHealth, float Delta)
{
	RefreshHealth();
}

void UGameplayHudWidget::HandleStatChanged(EGrantMoneyStat Stat, float OldValue, float NewValue)
{
	if (Stat == EGrantMoneyStat::MaxHealth)
	{
		RefreshHealth();
	}
	else if (Stat == EGrantMoneyStat::Experience)
	{
		RefreshXp();
		RefreshScore();
	}
}

void UGameplayHudWidget::HandleLevelUp(int32 NewLevel, int32 PreviousLevel)
{
	RefreshXp();
}

void UGameplayHudWidget::HandleUpgradeChosen()
{
	RefreshUpgrades();
}

void UGameplayHudWidget::HandleBuffsChanged()
{
	RefreshBuffs();
}

void UGameplayHudWidget::HandleRunTimeChanged(float ElapsedSeconds, float RemainingSeconds)
{
	RefreshClock();
	RefreshScore();
}

void UGameplayHudWidget::HandleRunStateChanged(EGrantMoneyRunState OldState, EGrantMoneyRunState NewState)
{
	RefreshPhase();
	RefreshClock();
	RefreshScore();
}

// ---------------------------------------------------------------------------------------------------------------
// Refresh
// ---------------------------------------------------------------------------------------------------------------

void UGameplayHudWidget::SetTextIfChanged(UTextBlock* Text, const FString& Value) const
{
	if (Text && Text->GetText().ToString() != Value)
	{
		Text->SetText(FText::FromString(Value));
	}
}

void UGameplayHudWidget::RefreshAll()
{
	RefreshHealth();
	RefreshXp();
	RefreshScore();
	RefreshClock();
	RefreshPhase();
	RefreshBuffs();
	RefreshUpgrades();
	RefreshBoss();
	RefreshEnemyCount();
}

void UGameplayHudWidget::RefreshHealth()
{
	const AGrantMoneyPlayerCharacter* Bound = Player.Get();
	const UHealthComponent* Health = Bound ? Bound->GetHealth() : nullptr;

	SetTextIfChanged(HealthLabel, Health ? FString::Printf(TEXT("HP  %s"), *FRunHudFormat::FormatHealth(Health->GetHealth(), Health->GetMaxHealth())) : FString(TEXT("HP  --")));
	if (HealthBar)
	{
		HealthBar->SetPercent(Health ? FRunHudFormat::SafeFraction(Health->GetHealth(), Health->GetMaxHealth()) : 0.0f);
	}
}

void UGameplayHudWidget::RefreshXp()
{
	const AGrantMoneyPlayerCharacter* Bound = Player.Get();
	const ULevelComponent* Level = Bound ? Bound->GetLevelComponent() : nullptr;

	SetTextIfChanged(LevelText, FRunHudFormat::FormatLevel(Level ? Level->GetLevel() : 1));
	SetTextIfChanged(XpLabel, Level ? FRunHudFormat::FormatXp(Level->GetXpIntoLevel(), Level->GetXpToNextLevel()) : FString(TEXT("-- XP")));
	if (XpBar)
	{
		XpBar->SetPercent(Level ? Level->GetProgressFraction() : 0.0f);
	}
}

void UGameplayHudWidget::RefreshScore()
{
	const AGrantMoneyPlayerCharacter* Bound = Player.Get();
	const ARunController* Controller = Run.Get();
	const int32 Score = FRunHudFormat::ComputeScore(Bound ? Bound->GetExperience() : 0.0f, Controller ? Controller->GetElapsedSeconds() : 0.0f);
	SetTextIfChanged(ScoreText, FRunHudFormat::FormatScore(Score));
}

void UGameplayHudWidget::RefreshClock()
{
	const ARunController* Controller = Run.Get();
	if (!Controller)
	{
		SetTextIfChanged(TimerText, TEXT("--:--"));
		return;
	}

	const float Remaining = Controller->GetRemainingSeconds();
	SetTextIfChanged(TimerText, ARunController::FormatClock(Remaining));

	// The last ten seconds of the survival phase turn the clock red.
	const bool bUrgent = Controller->GetState() == EGrantMoneyRunState::Active && Remaining <= 10.0f;
	TimerText->SetColorAndOpacity(FSlateColor(bUrgent ? FLinearColor(1.0f, 0.30f, 0.25f, 1.0f) : FLinearColor::White));
}

void UGameplayHudWidget::RefreshPhase()
{
	const ARunController* Controller = Run.Get();
	if (!Controller)
	{
		SetTextIfChanged(PhaseText, TEXT(""));
		return;
	}

	const UWorld* World = GetWorld();
	const EGrantMoneyRunState State = Controller->GetState();
	SetTextIfChanged(PhaseText, FRunHudFormat::FormatPhase(State, World && World->IsPaused()));
	PhaseText->SetColorAndOpacity(FSlateColor(ColorForState(State)));
}

void UGameplayHudWidget::EnsureRows(UVerticalBox* Box, TArray<TObjectPtr<UTextBlock>>& Rows, int32 Count)
{
	while (Rows.Num() < Count)
	{
		UTextBlock* Row = MakeText(18, FLinearColor::White, false, ETextJustify::Left);
		UVerticalBoxSlot* RowSlot = Box->AddChildToVerticalBox(Row);
		RowSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
		Rows.Add(Row);
	}

	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		Rows[Index]->SetVisibility(Index < Count ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UGameplayHudWidget::RefreshBuffs()
{
	const AGrantMoneyPlayerCharacter* Bound = Player.Get();
	const UTemporaryBuffComponent* Buffs = Bound ? Bound->GetTemporaryBuffs() : nullptr;
	static const TArray<FGrantMoneyActiveBuff> NoBuffs;
	const TArray<FGrantMoneyActiveBuff>& Active = Buffs ? Buffs->GetActiveBuffs() : NoBuffs;

	VisibleBuffRows = Active.Num();
	EnsureRows(BuffBox, BuffRows, VisibleBuffRows);
	BuffPanel->SetVisibility(VisibleBuffRows > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	for (int32 Index = 0; Index < Active.Num(); ++Index)
	{
		const FGrantMoneyActiveBuff& Buff = Active[Index];
		const FText Name = Buff.DisplayName.IsEmpty() ? FText::FromName(Buff.Id) : Buff.DisplayName;
		BuffRows[Index]->SetColorAndOpacity(FSlateColor(BuffGreen));
		SetTextIfChanged(BuffRows[Index], FRunHudFormat::FormatBuffLine(Name, Buff.RemainingSeconds));
	}
}

void UGameplayHudWidget::RefreshUpgrades()
{
	const AGrantMoneyPlayerCharacter* Bound = Player.Get();
	const UUpgradeComponent* Upgrades = Bound ? Bound->GetUpgradeComponent() : nullptr;
	const TArray<FGrantMoneyPickedUpgrade> Picked = Upgrades ? Upgrades->GetPickedUpgrades() : TArray<FGrantMoneyPickedUpgrade>();

	VisibleUpgradeRows = Picked.Num();
	EnsureRows(UpgradeBox, UpgradeRows, VisibleUpgradeRows);
	UpgradePanel->SetVisibility(VisibleUpgradeRows > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	for (int32 Index = 0; Index < Picked.Num(); ++Index)
	{
		SetTextIfChanged(UpgradeRows[Index], FRunHudFormat::FormatUpgradeLine(Picked[Index].DisplayName, Picked[Index].Stacks, Picked[Index].MaxStacks));
	}
}

void UGameplayHudWidget::RefreshBoss()
{
	const ARunController* Controller = Run.Get();
	const bool bRunOver = Controller && FRunUiPolicy::IsTerminal(Controller->GetState());
	const ABossEncounter* Boss = bRunOver ? nullptr : Encounter.Get();

	// The arrival message and a failed or retrying spawn must never be silent, so both have their own line.
	BossMessageText->SetVisibility(Boss && Boss->IsArrivalMessageVisible() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	const FString Diagnostic = Boss ? Boss->GetSpawnDiagnostic() : FString();
	BossDiagnosticText->SetVisibility(Diagnostic.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	if (!Diagnostic.IsEmpty())
	{
		SetTextIfChanged(BossDiagnosticText, Diagnostic);
		const bool bFailed = Boss->GetState() == EBossEncounterState::Failed;
		BossDiagnosticText->SetColorAndOpacity(FSlateColor(bFailed ? FLinearColor::Red : FLinearColor(1.0f, 0.6f, 0.1f, 1.0f)));
	}

	const AEnemyBase* BossActor = (Boss && Boss->GetState() == EBossEncounterState::Fighting) ? Boss->GetBoss() : nullptr;
	const UHealthComponent* Health = IsValid(BossActor) ? BossActor->GetHealth() : nullptr;
	if (!Health || Health->IsDead())
	{
		BossPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	BossPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	SetTextIfChanged(BossNameText, Boss->BossDisplayName);
	SetTextIfChanged(BossLabel, FRunHudFormat::FormatHealth(Health->GetHealth(), Health->GetMaxHealth()));
	BossBar->SetPercent(FRunHudFormat::SafeFraction(Health->GetHealth(), Health->GetMaxHealth()));
}

void UGameplayHudWidget::RefreshEnemyCount()
{
	const AEnemySpawner* Source = Spawner.Get();
	EnemyCountText->SetVisibility(Source ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (Source)
	{
		SetTextIfChanged(EnemyCountText, FString::Printf(TEXT("Enemies %d / %d"), Source->GetActiveEnemyCount(), Source->GetCurrentMaxActiveEnemies()));
	}
}

void UGameplayHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	ResolveSources(InDeltaTime);

	// Pausing has no event: the phase line says "(paused)" while the world is paused.
	const UWorld* World = GetWorld();
	const bool bPaused = World && World->IsPaused();
	if (bPaused != bWasPaused)
	{
		bWasPaused = bPaused;
		RefreshPhase();
	}

	// A buff's remaining time counts down continuously; the rest of the HUD only changes on events.
	if (VisibleBuffRows > 0)
	{
		RefreshBuffs();
	}

	RefreshBoss();
	RefreshEnemyCount();
}

// ---------------------------------------------------------------------------------------------------------------
// Readouts (for tests)
// ---------------------------------------------------------------------------------------------------------------

namespace
{
	FString TextOf(const UTextBlock* Text)
	{
		return Text ? Text->GetText().ToString() : FString();
	}
}

FString UGameplayHudWidget::GetHealthText() const { return TextOf(HealthLabel); }
FString UGameplayHudWidget::GetXpText() const { return TextOf(XpLabel); }
FString UGameplayHudWidget::GetLevelText() const { return TextOf(LevelText); }
FString UGameplayHudWidget::GetTimerText() const { return TextOf(TimerText); }
FString UGameplayHudWidget::GetPhaseText() const { return TextOf(PhaseText); }
FString UGameplayHudWidget::GetScoreText() const { return TextOf(ScoreText); }
float UGameplayHudWidget::GetHealthPercent() const { return HealthBar ? HealthBar->GetPercent() : 0.0f; }
float UGameplayHudWidget::GetXpPercent() const { return XpBar ? XpBar->GetPercent() : 0.0f; }
FString UGameplayHudWidget::GetBuffRowText(int32 Index) const { return BuffRows.IsValidIndex(Index) ? TextOf(BuffRows[Index]) : FString(); }
FString UGameplayHudWidget::GetUpgradeRowText(int32 Index) const { return UpgradeRows.IsValidIndex(Index) ? TextOf(UpgradeRows[Index]) : FString(); }
