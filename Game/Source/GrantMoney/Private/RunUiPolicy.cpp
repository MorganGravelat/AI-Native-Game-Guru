// Fill out your copyright notice in the Description page of Project Settings.

#include "RunUiPolicy.h"

bool FRunUiPolicy::IsTerminal(EGrantMoneyRunState State)
{
	return State == EGrantMoneyRunState::Won || State == EGrantMoneyRunState::Lost;
}

bool FRunUiPolicy::CanOpenPauseMenu(bool bRunFinished, bool bLevelUpShowing)
{
	return !bRunFinished && !bLevelUpShowing;
}

bool FRunUiPolicy::CanShowLevelUp(bool bRunFinished)
{
	return !bRunFinished;
}

bool FRunUiPolicy::CanResumeFromPauseMenu(bool bRunFinished, bool bLevelUpShowing)
{
	return !bRunFinished && !bLevelUpShowing;
}

bool FRunUiPolicy::ShouldReturnToGameplayAfterLevelUp(bool bRunFinished)
{
	return !bRunFinished;
}
