// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyBase.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Placed enemies would otherwise get an AAIController automatically and spawned ones would not.
	// Enemies are driven directly, so both are controller-less and behave the same.
	AutoPossessAI = EAutoPossessAI::Disabled;

	// Face the direction of travel; no controller exists to supply a rotation.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;

	// With no controller, character movement would otherwise not run at all.
	Movement->bRunPhysicsWithNoController = true;
}

void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// The player may not exist yet, or may have been replaced, so look again whenever the target is gone.
	if (!IsValid(Target) && !AcquireTarget())
	{
		return;
	}

	MoveTowardTarget();
}

AActor* AEnemyBase::FindTarget() const
{
	return UGameplayStatics::GetPlayerPawn(this, 0);
}

bool AEnemyBase::AcquireTarget()
{
	Target = FindTarget();

	if (!IsValid(Target))
	{
		Target = nullptr;

		if (!bLoggedMissingTarget)
		{
			UE_LOG(LogTemp, Warning, TEXT("EnemyBase: %s has no target yet; it will keep looking."), *GetName());
			bLoggedMissingTarget = true;
		}

		return false;
	}

	bLoggedMissingTarget = false;
	UE_LOG(LogTemp, Log, TEXT("EnemyBase: %s is chasing %s."), *GetName(), *Target->GetName());
	return true;
}

FVector AEnemyBase::GetFlatOffsetToTarget() const
{
	if (!IsValid(Target))
	{
		return FVector::ZeroVector;
	}

	// Ground plane only: capsules of different heights put the two actor origins at different Z.
	FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.0;
	return ToTarget;
}

FVector AEnemyBase::GetMoveDirection() const
{
	const FVector ToTarget = GetFlatOffsetToTarget();

	if (ToTarget.Size() <= StopDistance)
	{
		return FVector::ZeroVector;
	}

	return ToTarget.GetSafeNormal();
}

void AEnemyBase::MoveTowardTarget()
{
	// Slow in proportion to the distance left, so the enemy settles at StopDistance. Starting and
	// stopping at full speed on the boundary makes it stutter when the target moves away slowly.
	float Speed = MoveSpeed;
	if (SlowdownDistance > 0.0f)
	{
		const float Remaining = static_cast<float>(GetFlatOffsetToTarget().Size()) - StopDistance;
		Speed *= FMath::Clamp(Remaining / SlowdownDistance, 0.0f, 1.0f);
	}

	GetCharacterMovement()->MaxWalkSpeed = Speed;

	const FVector Direction = GetMoveDirection();
	if (!Direction.IsNearlyZero())
	{
		AddMovementInput(Direction, 1.0f);
	}
}
