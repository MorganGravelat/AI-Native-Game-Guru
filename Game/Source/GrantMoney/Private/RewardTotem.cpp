// Fill out your copyright notice in the Description page of Project Settings.

#include "RewardTotem.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GrantMoneyAudioSubsystem.h"
#include "GrantMoneyPlayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "TemporaryEffect.h"
#include "TemporaryEffectComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Engine basic shapes are 100 cm on every side, centred on their origin.
	constexpr double ShapeSize = 100.0;

	constexpr double PillarDiameter = 60.0;
	constexpr double PillarHeight = 180.0;
	constexpr double DiscThickness = 2.0;
	constexpr double BarThickness = 4.0;
	constexpr double BarTall = 14.0;

	// How bright the range disc is compared with the pillar.
	constexpr float DiscBrightness = 0.35f;

	// Both GlowMaterial (EmissiveMeshMaterial) and BarMaterial (BasicShapeMaterial) expose their colour as
	// a vector parameter named "Color", not "EmissiveColor" as this file used to assume. The wrong name
	// was a silent no-op (UMaterialInstanceDynamic does not warn on an unknown parameter), so every glow
	// here used to keep its master material's flat default look instead of being tinted. Confirmed
	// against the materials' actual exposed parameters.
	const FName GlowColorParameterName(TEXT("Color"));

	void DisableCollision(UStaticMeshComponent* Mesh)
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->SetCastShadow(false);
	}
}

ARewardTotem::ARewardTotem()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = false;
	SetCanBeDamaged(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowFinder(TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BarFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	UStaticMesh* Cylinder = CylinderFinder.Succeeded() ? CylinderFinder.Object : nullptr;
	UStaticMesh* Cube = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
	GlowMaterial = GlowFinder.Succeeded() ? GlowFinder.Object : nullptr;
	BarMaterial = BarFinder.Succeeded() ? BarFinder.Object : nullptr;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;

	TotemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TotemMesh"));
	TotemMesh->SetupAttachment(SceneRoot);
	TotemMesh->SetStaticMesh(Cylinder);
	TotemMesh->SetRelativeScale3D(FVector(PillarDiameter / ShapeSize, PillarDiameter / ShapeSize, PillarHeight / ShapeSize));
	TotemMesh->SetRelativeLocation(FVector(0.0, 0.0, PillarHeight * 0.5));
	TotemMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	TotemMesh->SetGenerateOverlapEvents(false);

	RangeDisc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RangeDisc"));
	RangeDisc->SetupAttachment(SceneRoot);
	RangeDisc->SetStaticMesh(Cylinder);
	DisableCollision(RangeDisc);

	BarPivot = CreateDefaultSubobject<USceneComponent>(TEXT("BarPivot"));
	BarPivot->SetupAttachment(SceneRoot);

	BarBackground = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarBackground"));
	BarBackground->SetupAttachment(BarPivot);
	BarBackground->SetStaticMesh(Cube);
	DisableCollision(BarBackground);

	BarFill = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarFill"));
	BarFill->SetupAttachment(BarPivot);
	BarFill->SetStaticMesh(Cube);
	DisableCollision(BarFill);

	ActivatorClass = AGrantMoneyPlayerCharacter::StaticClass();

	// The default reward: a small, permanent +10% move speed. Two totems of the same StackKey are a
	// no-op (already permanent), not a second application.
	UStatModifierEffect* SpeedBoost = CreateDefaultSubobject<UStatModifierEffect>(TEXT("MoveSpeedBoost"));
	SpeedBoost->Stat = EGrantMoneyStat::MoveSpeed;
	SpeedBoost->ModifierOp = EGrantMoneyStatModifierOp::Multiplicative;
	SpeedBoost->Magnitude = 0.1f;
	SpeedBoost->bPermanent = true;

	// A permanent reward would otherwise glow on the player for the rest of the run.
	SpeedBoost->bShowAura = false;
	SpeedBoost->StackKey = TEXT("Totem.MoveSpeed");
	SpeedBoost->DisplayName = NSLOCTEXT("GrantMoney", "SwiftTotemBuff", "Swiftness");
	Effects.Add(SpeedBoost);
}

void ARewardTotem::BeginPlay()
{
	Super::BeginPlay();

	const double DiscDiameter = 2.0 * ActivationRadius / ShapeSize;
	RangeDisc->SetRelativeScale3D(FVector(DiscDiameter, DiscDiameter, DiscThickness / ShapeSize));
	RangeDisc->SetRelativeLocation(FVector(0.0, 0.0, DiscThickness));

	BarPivot->SetRelativeLocation(FVector(0.0, 0.0, BarHeight));
	BarBackground->SetRelativeScale3D(FVector(BarThickness / ShapeSize, BarLength / ShapeSize, BarTall / ShapeSize));
	MakeBarSegment(BarBackground, BarBackgroundColor);
	MakeBarSegment(BarFill, BarColor);

	// A custom totem keeps its own materials; only the default pillar is tinted.
	const bool bCustomLook = ApplyCustomLook();
	TotemGlow = (bTintTotemMesh && !bCustomLook) ? MakeGlow(TotemMesh, ReadyColor) : nullptr;
	DiscGlow = MakeGlow(RangeDisc, ReadyColor * DiscBrightness);

	SetState(ERewardTotemState::Ready);
	UpdateBar();
}

bool ARewardTotem::ApplyCustomLook()
{
	if (CustomMesh)
	{
		TotemMesh->SetStaticMesh(CustomMesh);
		TotemMesh->SetRelativeScale3D(CustomMeshScale);

		// Sit the mesh's lowest point on the totem's base (the ground), whatever its pivot.
		const double BottomZ = CustomMesh->GetBoundingBox().Min.Z * CustomMeshScale.Z;
		TotemMesh->SetRelativeLocation(FVector(0.0, 0.0, -BottomZ));
	}

	if (CustomMaterial)
	{
		for (int32 Slot = 0; Slot < FMath::Max(1, TotemMesh->GetNumMaterials()); ++Slot)
		{
			TotemMesh->SetMaterial(Slot, CustomMaterial);
		}
	}

	return CustomMesh != nullptr || CustomMaterial != nullptr;
}

void ARewardTotem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AdvanceCharge(DeltaSeconds);

	if (IsBarVisible())
	{
		FaceBarToCamera();
	}
}

void ARewardTotem::AdvanceCharge(float DeltaSeconds)
{
	if (State == ERewardTotemState::Spent)
	{
		return;
	}

	if (State == ERewardTotemState::Cooldown)
	{
		CooldownRemaining -= DeltaSeconds;
		if (CooldownRemaining <= 0.0f)
		{
			SetState(ERewardTotemState::Ready);
		}
		return;
	}

	APawn* Activator = FindActivator();
	bFilling = Activator != nullptr;
	const float Step = DeltaSeconds / FMath::Max(ChargeSeconds, UE_KINDA_SMALL_NUMBER);
	ChargeProgress = FMath::Clamp(ChargeProgress + (Activator ? Step : -Step * DrainRate), 0.0f, 1.0f);
	UpdateBar();

	if (Activator && ChargeProgress >= 1.0f && Activate(Activator) == 0)
	{
		// Nothing could be applied, so the totem stays usable; start the bar again.
		ChargeProgress = 0.0f;
		UpdateBar();
	}
}

APawn* ARewardTotem::FindActivator() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector Centre = GetActorLocation();
	APawn* Closest = nullptr;
	double ClosestDistanceSquared = FMath::Square(static_cast<double>(ActivationRadius));

	for (TActorIterator<APawn> It(World, ActivatorClass ? ActivatorClass.Get() : APawn::StaticClass()); It; ++It)
	{
		APawn* Pawn = *It;
		if (!IsValid(Pawn) || Pawn->IsActorBeingDestroyed())
		{
			continue;
		}

		const double DistanceSquared = FVector::DistSquared2D(Centre, Pawn->GetActorLocation());
		if (DistanceSquared <= ClosestDistanceSquared)
		{
			Closest = Pawn;
			ClosestDistanceSquared = DistanceSquared;
		}
	}

	return Closest;
}

int32 ARewardTotem::Activate(AActor* Activator)
{
	if (State != ERewardTotemState::Ready || !IsValid(Activator))
	{
		return 0;
	}

	UTemporaryEffectComponent* EffectRunner = UTemporaryEffectComponent::FindOrAdd(Activator);
	int32 Applied = 0;
	for (const UTemporaryEffect* Template : Effects)
	{
		Applied += (EffectRunner && EffectRunner->ApplyEffect(Template)) ? 1 : 0;
	}

	if (Applied == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("RewardTotem: %s could not give %s any of its %d effects, so it was not used up."),
			*GetName(), *GetNameSafe(Activator), Effects.Num());
		return 0;
	}

	UE_LOG(LogTemp, Log, TEXT("RewardTotem: %s activated by %s (%d effects)."), *GetName(), *GetNameSafe(Activator), Applied);

	if (ActivationSound)
	{
		UGameplayStatics::PlaySound2D(this, ActivationSound);
	}

	if (ActivationEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ActivationEffect, GetActorLocation());
	}

	ChargeProgress = 0.0f;
	bFilling = false;
	if (bSingleUse)
	{
		SetState(ERewardTotemState::Spent);
	}
	else
	{
		CooldownRemaining = CooldownSeconds;
		SetState(ERewardTotemState::Cooldown);
	}
	UpdateBar();

	OnActivated.Broadcast(this, Activator);
	return Applied;
}

bool ARewardTotem::IsBarVisible() const
{
	return BarPivot && BarPivot->IsVisible();
}

void ARewardTotem::SetState(ERewardTotemState NewState)
{
	State = NewState;
	ApplyColors();

	// A spent totem has nothing left to do.
	SetActorTickEnabled(State != ERewardTotemState::Spent);
}

float ARewardTotem::ComputeChargeSoundTime(float Progress, float ChargeSeconds)
{
	return FMath::Clamp(Progress, 0.0f, 1.0f) * FMath::Max(ChargeSeconds, 0.0f);
}

void ARewardTotem::UpdateChargeSound()
{
	const bool bCharging = State == ERewardTotemState::Ready && bFilling && ChargeProgress < 1.0f;
	if (bCharging == bHoldingChargeFeedback)
	{
		return;
	}

	// Only the moment charging starts or stops matters. A sound shorter than the charge simply ends rather
	// than being started over every frame.
	bHoldingChargeFeedback = bCharging;

	if (UGrantMoneyAudioSubsystem* Audio = UGrantMoneyAudioSubsystem::Get(this))
	{
		if (bCharging)
		{
			Audio->AcquireMusicDuck(this);
		}
		else
		{
			Audio->ReleaseMusicDuck(this);
		}
	}

	if (!ChargeSound)
	{
		return;
	}

	if (bCharging)
	{
		// Picks the sound up at the point matching the bar, so stepping back on after a partial drain continues
		// from there, and a fully drained bar starts it from the beginning.
		ChargeAudio = UGameplayStatics::SpawnSoundAtLocation(this, ChargeSound, GetActorLocation(), FRotator::ZeroRotator,
			1.0f, 1.0f, ComputeChargeSoundTime(ChargeProgress, ChargeSeconds));
	}
	else
	{
		if (IsValid(ChargeAudio))
		{
			ChargeAudio->FadeOut(0.1f, 0.0f);
		}

		ChargeAudio = nullptr;
	}
}

void ARewardTotem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Stops the music duck and the charge sound if the totem goes away mid-charge.
	bFilling = false;
	UpdateChargeSound();

	Super::EndPlay(EndPlayReason);
}

void ARewardTotem::UpdateBar()
{
	UpdateChargeSound();

	const bool bShow = State == ERewardTotemState::Ready && ChargeProgress > 0.0f;
	BarPivot->SetVisibility(bShow, true);
	if (!bShow)
	{
		return;
	}

	// The fill grows from the left end of the bar, slightly in front of the background.
	const double Filled = BarLength * ChargeProgress;
	BarFill->SetRelativeScale3D(FVector((BarThickness + 1.0) / ShapeSize, FMath::Max(Filled, 0.1) / ShapeSize, (BarTall - 4.0) / ShapeSize));
	BarFill->SetRelativeLocation(FVector(1.0, -BarLength * 0.5 + Filled * 0.5, 0.0));
}

void ARewardTotem::FaceBarToCamera()
{
	const APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!Controller || !Controller->PlayerCameraManager)
	{
		return;
	}

	const FVector ToCamera = Controller->PlayerCameraManager->GetCameraLocation() - BarPivot->GetComponentLocation();
	BarPivot->SetWorldRotation(FRotator(0.0, ToCamera.Rotation().Yaw, 0.0));
}

void ARewardTotem::ApplyColors()
{
	const FLinearColor Color = State == ERewardTotemState::Spent ? SpentColor
		: State == ERewardTotemState::Cooldown ? FLinearColor::LerpUsingHSV(SpentColor, ReadyColor, 0.3f)
		: ReadyColor;

	if (TotemGlow)
	{
		TotemGlow->SetVectorParameterValue(GlowColorParameterName, Color);
	}

	if (DiscGlow)
	{
		DiscGlow->SetVectorParameterValue(GlowColorParameterName, Color * DiscBrightness);
	}

	RangeDisc->SetVisibility(State != ERewardTotemState::Spent);
}

UMaterialInstanceDynamic* ARewardTotem::MakeGlow(UStaticMeshComponent* Mesh, const FLinearColor& Color)
{
	if (!GlowMaterial || !Mesh)
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* Glow = UMaterialInstanceDynamic::Create(GlowMaterial, this);
	Glow->SetVectorParameterValue(GlowColorParameterName, Color);
	Mesh->SetMaterial(0, Glow);
	return Glow;
}

UMaterialInstanceDynamic* ARewardTotem::MakeBarSegment(UStaticMeshComponent* Mesh, const FLinearColor& Color)
{
	if (!BarMaterial || !Mesh)
	{
		return nullptr;
	}

	// BasicShapeMaterial ("Color", Opaque, Lit): unlike GlowMaterial it actually occludes, so the bar's
	// two segments read as solid blocks instead of both show-through glows over whatever is behind them.
	UMaterialInstanceDynamic* Segment = UMaterialInstanceDynamic::Create(BarMaterial, this);
	Segment->SetVectorParameterValue(GlowColorParameterName, Color);
	Mesh->SetMaterial(0, Segment);
	return Segment;
}
