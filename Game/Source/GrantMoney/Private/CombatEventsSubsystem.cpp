// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatEventsSubsystem.h"

#include "EnemyBase.h"

void UCombatEventsSubsystem::NotifyEnemyKilled(AEnemyBase* Enemy, AActor* Killer)
{
	if (!Enemy)
	{
		return;
	}

	OnEnemyKilled.Broadcast(Enemy, Killer);
}
