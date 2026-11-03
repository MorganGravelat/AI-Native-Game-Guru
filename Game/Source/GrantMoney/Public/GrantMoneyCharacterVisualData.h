#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GrantMoneyCharacterVisualData.generated.h"

class USkeletalMesh;
class UAnimationAsset;
class UAnimInstance;

UCLASS(BlueprintType)
class GRANTMONEY_API UGrantMoneyCharacterVisualData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Visual")
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	// Animation Blueprint class. Wins over PreviewAnimation.
	UPROPERTY(EditAnywhere, Category = "Animation")
	TSoftClassPtr<UAnimInstance> AnimClass;

	// Used only when AnimClass is empty. Loops one animation.
	UPROPERTY(EditAnywhere, Category = "Animation")
	TSoftObjectPtr<UAnimationAsset> PreviewAnimation;

	// Added on top of the "feet at capsule bottom" offset.
	UPROPERTY(EditAnywhere, Category = "Placement")
	FVector MeshOffset = FVector::ZeroVector;

	// The Mannequin faces +Y in mesh space; yaw -90 aligns it with +X.
	UPROPERTY(EditAnywhere, Category = "Placement")
	FRotator MeshRotation = FRotator(0.0, -90.0, 0.0);

	UPROPERTY(EditAnywhere, Category = "Placement")
	FVector MeshScale = FVector::OneVector;
};