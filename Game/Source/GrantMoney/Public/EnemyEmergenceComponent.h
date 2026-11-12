// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyEmergenceComponent.generated.h"

class UEnemyEmergenceComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGrantMoneyEmergenceFinished, UEnemyEmergenceComponent*, Emergence);

/**
 * Presentation only: grows a visual component from a flattened shape back to its normal shape, with
 * its bottom held at the same height, so it looks like it rises out of the ground. It never touches
 * collision, movement or gameplay; the owner decides what to enable when OnFinished fires.
 *
 * The visual can be any scene component (a static mesh now, a skeletal mesh later). The component
 * records the visual's own relative scale and location when Begin is called and scales relative to
 * those, so replacing the mesh needs no change here. The bottom is read from the visual's local
 * bounds, or taken to be its origin if it has none (a skeletal mesh whose origin is at its feet).
 * Only scale and the vertical offset are animated; the visual should stand upright.
 *
 * Ticks only while emerging. Actor and component ticks stop while the game is paused, so a pause
 * freezes the animation.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class GRANTMONEY_API UEnemyEmergenceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyEmergenceComponent();

	// Seconds from the flattened shape to the normal shape. 0 skips the animation.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emergence", meta = (ClampMin = "0.0", Units = "s"))
	float Duration = 0.8f;

	// The starting shape as a multiple of the visual's normal scale. The default is a wide, flat puddle.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emergence")
	FVector FlattenedScale = FVector(1.25, 1.25, 0.1);

	// Fires once when the visual has reached its normal shape. Not fired by Cancel.
	UPROPERTY(BlueprintAssignable, Category = "Emergence")
	FOnGrantMoneyEmergenceFinished OnFinished;

	// Starts the animation on Visual and shows its first (flattened) frame immediately. With a Duration
	// of 0, or no valid visual, it finishes at once. Does nothing if already emerging.
	void Begin(USceneComponent* Visual);

	// Stops the animation and puts the visual back to its normal shape. Fires no event.
	void Cancel();

	// Moves the animation forward. Tick calls this; tests and tools can call it directly.
	void AdvanceEmergence(float DeltaSeconds);

	UFUNCTION(BlueprintPure, Category = "Emergence")
	bool IsEmerging() const { return bEmerging; }

	// 0 at the start, 1 when finished.
	UFUNCTION(BlueprintPure, Category = "Emergence")
	float GetProgress() const;

	// Scale and location (relative to the visual's parent) of the visual at Progress 0..1.
	// RestBottomLocalZ is the visual's lowest point in its own unscaled space. The scaled bottom stays at
	// the height it has at the normal shape.
	static void ComputePose(const FVector& RestScale, const FVector& RestLocation, float RestBottomLocalZ,
		const FVector& InFlattenedScale, float Progress, FVector& OutScale, FVector& OutLocation);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void ApplyProgress(float Progress);
	void Finish();
	void StopTicking();

	UPROPERTY(Transient)
	TWeakObjectPtr<USceneComponent> VisualComponent;

	FVector RestScale = FVector::OneVector;
	FVector RestLocation = FVector::ZeroVector;
	float RestBottomLocalZ = 0.0f;
	float Elapsed = 0.0f;
	bool bEmerging = false;
};
