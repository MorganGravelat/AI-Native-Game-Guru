// Fill out your copyright notice in the Description page of Project Settings.

#include "EnemyEmergenceComponent.h"

#include "Components/SceneComponent.h"

UEnemyEmergenceComponent::UEnemyEmergenceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

float UEnemyEmergenceComponent::GetProgress() const
{
	if (!bEmerging)
	{
		return 1.0f;
	}

	return Duration > UE_KINDA_SMALL_NUMBER ? FMath::Clamp(Elapsed / Duration, 0.0f, 1.0f) : 1.0f;
}

void UEnemyEmergenceComponent::ComputePose(const FVector& InRestScale, const FVector& InRestLocation, float InRestBottomLocalZ,
	const FVector& InFlattenedScale, float Progress, FVector& OutScale, FVector& OutLocation)
{
	const double Eased = FMath::SmoothStep(0.0, 1.0, FMath::Clamp(static_cast<double>(Progress), 0.0, 1.0));

	OutScale = InRestScale * FMath::Lerp(InFlattenedScale, FVector::OneVector, Eased);

	// The scaled bottom is RelativeZ + ScaleZ * BottomLocalZ. Hold it at its normal-shape height.
	const double RestBottomZ = InRestLocation.Z + InRestScale.Z * InRestBottomLocalZ;
	OutLocation = InRestLocation;
	OutLocation.Z = RestBottomZ - OutScale.Z * InRestBottomLocalZ;
}

void UEnemyEmergenceComponent::Begin(USceneComponent* Visual)
{
	if (bEmerging)
	{
		return;
	}

	VisualComponent = Visual;
	Elapsed = 0.0f;

	if (!IsValid(Visual) || Duration <= UE_KINDA_SMALL_NUMBER)
	{
		// Nothing to animate, so there is no pose to restore either.
		bEmerging = false;
		OnFinished.Broadcast(this);
		return;
	}

	RestScale = Visual->GetRelativeScale3D();
	RestLocation = Visual->GetRelativeLocation();

	// Identity transform: the visual's bounds in its own space, before its component scale.
	const FBoxSphereBounds LocalBounds = Visual->CalcBounds(FTransform::Identity);
	RestBottomLocalZ = static_cast<float>(LocalBounds.Origin.Z - LocalBounds.BoxExtent.Z);

	bEmerging = true;
	ApplyProgress(0.0f);
	SetComponentTickEnabled(true);
}

void UEnemyEmergenceComponent::Cancel()
{
	if (!bEmerging)
	{
		return;
	}

	ApplyProgress(1.0f);
	bEmerging = false;
	StopTicking();
}

void UEnemyEmergenceComponent::AdvanceEmergence(float DeltaSeconds)
{
	if (!bEmerging)
	{
		return;
	}

	if (!VisualComponent.IsValid())
	{
		Finish();
		return;
	}

	Elapsed += FMath::Max(0.0f, DeltaSeconds);

	if (Elapsed >= Duration)
	{
		Finish();
		return;
	}

	ApplyProgress(GetProgress());
}

void UEnemyEmergenceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AdvanceEmergence(DeltaTime);
}

void UEnemyEmergenceComponent::ApplyProgress(float Progress)
{
	USceneComponent* Visual = VisualComponent.Get();
	if (!Visual)
	{
		return;
	}

	FVector Scale;
	FVector Location;
	ComputePose(RestScale, RestLocation, RestBottomLocalZ, FlattenedScale, Progress, Scale, Location);
	Visual->SetRelativeScale3D(Scale);
	Visual->SetRelativeLocation(Location);
}

void UEnemyEmergenceComponent::Finish()
{
	ApplyProgress(1.0f);
	bEmerging = false;
	StopTicking();
	OnFinished.Broadcast(this);
}

void UEnemyEmergenceComponent::StopTicking()
{
	SetComponentTickEnabled(false);
}
