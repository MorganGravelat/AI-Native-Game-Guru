// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RunController.h"

/**
 * Who may open or close which screen, as pure rules. AMapCameraPlayerController asks these before it
 * pauses, unpauses or changes the input mode, so the pause menu, the level-up choice and the Game Over /
 * Victory screens cannot undo each other:
 *  - A finished run (Won or Lost, or a result screen already showing) accepts no pause menu and no
 *    upgrade choice, and nothing may unpause it: only Restart and Quit leave that screen.
 *  - A level-up choice is a forced decision, so the pause menu cannot be opened over it.
 */
struct GRANTMONEY_API FRunUiPolicy
{
	// Won or Lost.
	static bool IsTerminal(EGrantMoneyRunState State);

	// bRunFinished: the run state is terminal or a result screen is showing.
	static bool CanOpenPauseMenu(bool bRunFinished, bool bLevelUpShowing);

	// A level-up choice is refused once the run is finished.
	static bool CanShowLevelUp(bool bRunFinished);

	// The pause menu's Resume may unpause only if the run is still going and no level-up choice needs the pause.
	static bool CanResumeFromPauseMenu(bool bRunFinished, bool bLevelUpShowing);

	// Closing the level-up choice returns to gameplay only if nothing else (a finished run) owns the pause and input.
	static bool ShouldReturnToGameplayAfterLevelUp(bool bRunFinished);
};
