// Fill out your copyright notice in the Description page of Project Settings.

// NO-TEST: pure UI glue (widget construction, text formatting, button bindings); no branching logic of
// its own to exercise headlessly. UUpgradeComponent (what it reads from and reports back to) is fully
// tested. Covered by PIE verification, like GameOverWidget.cpp.

#include "LevelUpWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "UpgradeComponent.h"

void ULevelUpWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // AddUnique so a re-constructed widget never double-binds.
    if (Option1Button) { Option1Button->OnClicked.AddUniqueDynamic(this, &ULevelUpWidget::HandleOption1Clicked); }
    if (Option2Button) { Option2Button->OnClicked.AddUniqueDynamic(this, &ULevelUpWidget::HandleOption2Clicked); }
    if (Option3Button) { Option3Button->OnClicked.AddUniqueDynamic(this, &ULevelUpWidget::HandleOption3Clicked); }
}

void ULevelUpWidget::SetUpgradeComponent(UUpgradeComponent* InUpgrades)
{
    Upgrades = InUpgrades;
}

void ULevelUpWidget::RefreshOptions()
{
    const UUpgradeComponent* Comp = Upgrades.Get();
    static const TArray<FGrantMoneyUpgradeDefinition> Empty;
    const TArray<FGrantMoneyUpgradeDefinition>& Options = Comp ? Comp->GetCurrentOptions() : Empty;

    UTextBlock* Texts[3] = { Option1Text, Option2Text, Option3Text };
    UButton* Buttons[3] = { Option1Button, Option2Button, Option3Button };

    for (int32 Index = 0; Index < 3; ++Index)
    {
        const bool bHasOption = Options.IsValidIndex(Index);

        if (Buttons[Index])
        {
            Buttons[Index]->SetVisibility(bHasOption ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        }

        if (Texts[Index] && bHasOption)
        {
            const FGrantMoneyUpgradeDefinition& Option = Options[Index];
            Texts[Index]->SetText(FText::Format(NSLOCTEXT("GrantMoneyUpgrades", "OptionFormat", "{0}\n{1}"), Option.DisplayName, Option.Description));
        }
    }
}

void ULevelUpWidget::HandleOption1Clicked() { ChooseIndex(0); }
void ULevelUpWidget::HandleOption2Clicked() { ChooseIndex(1); }
void ULevelUpWidget::HandleOption3Clicked() { ChooseIndex(2); }

void ULevelUpWidget::ChooseIndex(int32 Index)
{
    if (UUpgradeComponent* Comp = Upgrades.Get())
    {
        Comp->ChooseUpgrade(Index);
    }
}
