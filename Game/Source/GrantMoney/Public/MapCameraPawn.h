// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MapCameraPawn.generated.h"

class USceneComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

UCLASS()
class GRANTMONEY_API AMapCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AMapCameraPawn();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void OnConstruction(const FTransform& Transform) override;

	USpringArmComponent* GetSpringArm() const { return SpringArm; }
	UCameraComponent* GetCamera() const { return Camera; }

	// Lets another system (for example one that reads the active map's size)
	// set the world X/Y limits at runtime without this class knowing about maps.
	void SetMovementBounds(const FVector2D& NewMin, const FVector2D& NewMax);

	// Makes the camera track an actor (for example the player pawn). Pass nullptr
	// to return to free roaming. While following, WASD and bounds are not applied;
	// yaw, pitch and zoom still work.
	void SetFollowTarget(AActor* NewTarget);
	AActor* GetFollowTarget() const { return FollowTarget; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// Applies the tuning properties below to the spring arm and camera.
	void ApplyCameraSettings();

	// Input action handlers.
	void HandleMove(const FInputActionValue& Value);
	void HandleMoveCompleted(const FInputActionValue& Value);
	void HandleTurn(const FInputActionValue& Value);
	void HandleLookUp(const FInputActionValue& Value);
	void HandleZoom(const FInputActionValue& Value);

	// Replaces the manual bounds with the playable area of the level that the
	// TileMapRenderer in this world is showing. Leaves them unchanged on failure.
	void ApplyMapBounds();

	// Keeps a world location inside the movement bounds and at walking height.
	FVector ConstrainLocation(const FVector& Location) const;

	// Enhanced Input assets. Defaults point at /Game/Input/IA_Camera*.
	UPROPERTY(EditAnywhere, Category = "Camera|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Camera|Input")
	TObjectPtr<UInputAction> TurnAction;

	UPROPERTY(EditAnywhere, Category = "Camera|Input")
	TObjectPtr<UInputAction> LookUpAction;

	UPROPERTY(EditAnywhere, Category = "Camera|Input")
	TObjectPtr<UInputAction> ZoomAction;

	// Distance from the pawn to the camera at start, in Unreal units.
	UPROPERTY(EditAnywhere, Category = "Camera|Zoom", meta = (ClampMin = "1.0"))
	float InitialArmLength = 450.0f;

	// Closest the camera may zoom in.
	UPROPERTY(EditAnywhere, Category = "Camera|Zoom", meta = (ClampMin = "1.0"))
	float MinArmLength = 200.0f;

	// Farthest the camera may zoom out.
	UPROPERTY(EditAnywhere, Category = "Camera|Zoom", meta = (ClampMin = "1.0"))
	float MaxArmLength = 1500.0f;

	// Arm length change per mouse-wheel notch, in Unreal units.
	UPROPERTY(EditAnywhere, Category = "Camera|Zoom", meta = (ClampMin = "0.0"))
	float ZoomStep = 100.0f;

	// How quickly the arm eases toward the zoom target. 0 snaps instantly.
	UPROPERTY(EditAnywhere, Category = "Camera|Zoom", meta = (ClampMin = "0.0"))
	float ZoomInterpSpeed = 10.0f;

	// Initial camera pitch in degrees. Negative values look down at the map.
	UPROPERTY(EditAnywhere, Category = "Camera|Pitch", meta = (ClampMin = "-89.0", ClampMax = "89.0"))
	float InitialPitch = -25.0f;

	// Steepest downward pitch (closest to -90 is straight down).
	UPROPERTY(EditAnywhere, Category = "Camera|Pitch", meta = (ClampMin = "-89.0", ClampMax = "89.0"))
	float MinPitch = -85.0f;

	// Shallowest pitch toward the horizon.
	UPROPERTY(EditAnywhere, Category = "Camera|Pitch", meta = (ClampMin = "-89.0", ClampMax = "89.0"))
	float MaxPitch = -5.0f;

	// Degrees of yaw per unit of mouse X movement.
	UPROPERTY(EditAnywhere, Category = "Camera|Look", meta = (ClampMin = "0.0"))
	float TurnSensitivity = 1.0f;

	// Degrees of pitch per unit of mouse Y movement.
	UPROPERTY(EditAnywhere, Category = "Camera|Look", meta = (ClampMin = "0.0"))
	float PitchSensitivity = 1.0f;

	// When false, moving the mouse up tilts the camera up toward the horizon.
	UPROPERTY(EditAnywhere, Category = "Camera|Look")
	bool bInvertPitch = false;

	// Ground-plane movement speed in Unreal units per second.
	UPROPERTY(EditAnywhere, Category = "Camera|Movement", meta = (ClampMin = "0.0"))
	float MoveSpeed = 600.0f;

	// Keeps the pawn at a fixed height above the ground plane, like a walking player.
	UPROPERTY(EditAnywhere, Category = "Camera|Movement")
	bool bLockHeightToGround = true;

	// World Z of the ground plane. The current tile map sits at Z = 0.
	UPROPERTY(EditAnywhere, Category = "Camera|Movement", meta = (EditCondition = "bLockHeightToGround"))
	float GroundZ = 0.0f;

	// Height of the pawn's pivot (the point the arm orbits) above the ground, roughly head height.
	UPROPERTY(EditAnywhere, Category = "Camera|Movement", meta = (ClampMin = "0.0", EditCondition = "bLockHeightToGround"))
	float HeightAboveGround = 100.0f;

	// Actor the camera tracks. Leave empty for free roaming.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Follow")
	TObjectPtr<AActor> FollowTarget;

	// Offset from the target's location to the point the arm orbits.
	UPROPERTY(EditAnywhere, Category = "Camera|Follow")
	FVector FollowOffset = FVector(0.0, 0.0, 50.0);

	// At start, replace the bounds below with the playable area of the map.
	// The manual values are kept as a fallback if no map can be found.
	UPROPERTY(EditAnywhere, Category = "Camera|Movement")
	bool bDeriveBoundsFromMap = true;

	// Unreal units per map tile. Must match the renderer's tile spacing.
	UPROPERTY(EditAnywhere, Category = "Camera|Movement", meta = (ClampMin = "1.0", EditCondition = "bDeriveBoundsFromMap"))
	float MapTileSize = 100.0f;

	// Grows (positive) or shrinks (negative) the map-derived bounds on every side.
	UPROPERTY(EditAnywhere, Category = "Camera|Movement", meta = (EditCondition = "bDeriveBoundsFromMap"))
	float MapBoundsPadding = 0.0f;

	// Limits camera X/Y movement to the rectangle below.
	UPROPERTY(EditAnywhere, Category = "Camera|Movement")
	bool bClampToBounds = true;

	// World-space minimum X/Y. The current 64x64 map at 100 units per tile spans 0..6300.
	UPROPERTY(EditAnywhere, Category = "Camera|Movement", meta = (EditCondition = "bClampToBounds"))
	FVector2D MovementBoundsMin = FVector2D(0.0, 0.0);

	// World-space maximum X/Y.
	UPROPERTY(EditAnywhere, Category = "Camera|Movement", meta = (EditCondition = "bClampToBounds"))
	FVector2D MovementBoundsMax = FVector2D(6300.0, 6300.0);

	// Perspective field of view in degrees.
	UPROPERTY(EditAnywhere, Category = "Camera|Lens", meta = (ClampMin = "5.0", ClampMax = "170.0"))
	float FieldOfView = 90.0f;

	// Smooths camera position changes by lagging behind the pawn.
	UPROPERTY(EditAnywhere, Category = "Camera|Smoothing")
	bool bEnableCameraLag = false;

	UPROPERTY(EditAnywhere, Category = "Camera|Smoothing", meta = (ClampMin = "0.1", EditCondition = "bEnableCameraLag"))
	float CameraLagSpeed = 10.0f;

	// Smooths arm rotation (yaw/pitch) changes.
	UPROPERTY(EditAnywhere, Category = "Camera|Smoothing")
	bool bEnableCameraRotationLag = false;

	UPROPERTY(EditAnywhere, Category = "Camera|Smoothing", meta = (ClampMin = "0.1", EditCondition = "bEnableCameraRotationLag"))
	float CameraRotationLagSpeed = 10.0f;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> Camera;

	// Latest move input (X = right, Y = forward), held between input events.
	FVector2D MoveInput = FVector2D::ZeroVector;

	float CurrentPitch = -25.0f;
	float DesiredArmLength = 450.0f;
};