#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GrantMoneyCharacterVisualData.generated.h"

class USkeletalMesh;
class UAnimationAsset;
class UAnimInstance;

/**
 * Data-only description of how the player looks and animates. Swapping the
 * mesh or animations means editing an instance of this asset, not C++.
 * AGrantMoneyPlayerCharacter loads /Game/Characters/Player/DA_PlayerVisual
 * (if it exists) and applies it over its built-in Manny defaults.
 */
UCLASS(BlueprintType)
class GRANTMONEY_API UGrantMoneyCharacterVisualData : public UDataAsset
{
	GENERATED_BODY()

public:
	// Soft references so the cooker can see them (unlike plain string paths).
	UPROPERTY(EditAnywhere, Category = "Visual")
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	// Animation Blueprint class (e.g. ABP_Manny). Wins over PreviewAnimation.
	UPROPERTY(EditAnywhere, Category = "Animation")
	TSoftClassPtr<UAnimInstance> AnimClass;

	// Used only when AnimClass is empty. Loops a single animation (e.g. MM_Idle).
	UPROPERTY(EditAnywhere, Category = "Animation")
	TSoftObjectPtr<UAnimationAsset> PreviewAnimation;

	// Added on top of the "feet at capsule bottom" offset.
	UPROPERTY(EditAnywhere, Category = "Placement")
	FVector MeshOffset = FVector::ZeroVector;

	// The Mannequin faces +Y in mesh space; yaw -90 aligns it with the capsule's +X.
	UPROPERTY(EditAnywhere, Category = "Placement")
	FRotator MeshRotation = FRotator(0.0, -90.0, 0.0);

	UPROPERTY(EditAnywhere, Category = "Placement")
	FVector MeshScale = FVector::OneVector;
};
