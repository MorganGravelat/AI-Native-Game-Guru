# Recording Instructions: Player Visual Model and Animations

**Audience:** The teammate producing instructional videos and screenshots for this process.
**Goal:** A viewer can replicate the player visual (Manny mesh, idle/jog animation, and the swap data asset) from a clean clone.

---

## 0. Before you record

- Start from a **clean clone on a fresh ticket branch** that does not yet contain the visual work. It must have the working movement and collision, but **no** `Game/Content/Characters/` folder and no `SetSkeletalMesh` call in `GrantMoneyPlayerCharacter.cpp`.
- Keep the finished version of the work on a separate branch (`<finished-branch>`) as a reference.
- Record at 1080p. Use a large font in the code editor. Keep personal paths, tokens and credentials off screen.
- Close Unreal before every branch switch. Do a **full build** (`Development Editor | Win64`) after any header or new-class change. Do not use Live Coding.
- Dry-run each stage once with the editor closed and these notes open.
- Commit after each stage so every video segment has a matching checkpoint.
- Name screenshots with the prefixes below and store them under an `images/` folder next to the task document.

**Runtime total:** about 20 minutes, in segments of 3 to 4 minutes.

---

## Stage 0: Intro and baseline (about 1.5 min)

**Say:** what the ticket is, the acceptance criteria, and that the placeholder is Epic's Mannequin.

**Do**
```bash
cd /c/Dev/AI-Native-Game-Guru
git branch --show-current
git status
git log --oneline -5
```
Confirm a clean tree and no `Content/Characters` folder. Press Play to show the baseline.

**Screenshots**
- `00-jira-ticket.png`: the ticket and acceptance criteria.
- `00-before-capsule.png`: `PlayGame` with no visible character (the capsule only, via the console command `show Collision`).

---

## Stage A: Import the assets (about 4 min)

**Do in the editor**
1. In Epic Games Launcher, use the same engine version as the team. Create a Games, Third Person project in `C:\Dev\Sandbox\ThirdPersonScratch`, outside the repo.
2. In the scratch project, open `Content/Characters/Mannequins/` and select these assets (add `SKM_Quinn_Simple` for the swap demo later):
   - `Meshes/SKM_Manny_Simple`
   - `Anims/Unarmed/MM_Idle`
   - `Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd`
3. Right-click, then **Asset Actions, Migrate**. Read the dependency list on camera. It should contain the skeleton, physics asset, materials and textures, and **nothing from `/Game/ThirdPerson/`**.
4. Close GrantMoney. Set the destination to `C:\Dev\AI-Native-Game-Guru\Game\Content`.
5. Open GrantMoney. Right-click `Content/Characters`, then **Fix Up Redirectors in Folder**. Do not rename Epic's assets.
6. Open `SKM_Manny_Simple` and `MM_Idle`. Play the animation in the Animation Editor. Say aloud (and note for the doc) the skeleton name, root bone, units (cm), forward axis (+Y in mesh space), sockets such as `hand_r`, animation frame rate, and mesh height (about 180 uu).

**Commit**
```bash
git status
git lfs ls-files
git add Game/Content/Characters/
git diff --cached --stat
git commit -m "GMS-XXX: migrate Manny mesh and animations"
```

**Screenshots**
- `A1-migrate-dependencies.png`: the Migrate dependency list.
- `A2-content-folders.png`: the folder tree under `Content/Characters/`.
- `A3-skeleton-bones.png`: the skeleton tree with the root bone and sockets.
- `A4-idle-preview.png`: `MM_Idle` playing in the Animation Editor.
- `A5-lfs-ls-files.png`: `git lfs ls-files` showing the new `.uasset` files.

**Mention:** the asset source and license (Epic's Mannequin may be used in Unreal Engine projects).

---

## Stage B: Attach the mesh in C++ (about 3 min)

**File:** `Game/Source/GrantMoney/Private/GrantMoneyPlayerCharacter.cpp`

**Add these includes** if missing:
```cpp
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/ConstructorHelpers.h"
```

**Add this to the constructor:**
```cpp
USkeletalMeshComponent* CharacterMesh = GetMesh();
CharacterMesh->SetRelativeLocation(FVector(0.0, 0.0, -GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
CharacterMesh->SetRelativeRotation(FRotator(0.0, -90.0, 0.0));
CharacterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
CharacterMesh->SetGenerateOverlapEvents(false);
CharacterMesh->SetCanEverAffectNavigation(false);

static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyFinder(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));

if (MannyFinder.Succeeded())
{
	CharacterMesh->SetSkeletalMesh(MannyFinder.Object);
}
else
{
	UE_LOG(LogTemp, Warning, TEXT("GrantMoneyPlayerCharacter: Manny mesh is not available at /Game/Characters/Mannequins/Meshes/SKM_Manny_Simple."));
}
```

**Explain on camera**
- Z offset is minus the capsule half-height, so the feet sit at the capsule base.
- Yaw -90 is needed because the Mannequin faces +Y in mesh space.
- The mesh has no collision. The capsule owns collision.
- The local variable is named `CharacterMesh`, not `Mesh`, because `ACharacter` already has a member named `Mesh`.

**Build and test:** close the editor, build, open `PlayGame`, press Play. Manny should appear, static, sliding as WASD moves. Check scale, feet on the tile plane, facing, the Output Log, and that camera, pause and tiles are unchanged. Note whether he looks black (unlit level).

**Commit**
```bash
git add Game/Source/
git diff --cached --stat
git commit -m "GMS-XXX: attach Manny mesh to player character"
```

**Screenshots**
- `B1-mesh-code.png`: the code added in the constructor.
- `B2-manny-in-play.png`: Manny standing on the tiles.
- `B3-feet-closeup.png`: feet on the tile plane, with the capsule shown via `show Collision`.
- `B4-output-log-clean.png`: the Output Log with no warnings.

---

## Stage C: Idle and jog animation (about 4 min)

**File 1:** `Game/Source/GrantMoney/Public/GrantMoneyPlayerCharacter.h`

Add the forward declaration near the others:
```cpp
class UAnimationAsset;
```

In `public:` add:
```cpp
virtual void Tick(float DeltaSeconds) override;
```

In `protected:` add:
```cpp
virtual void BeginPlay() override;

// Played while standing still.
UPROPERTY(EditAnywhere, Category = "Animation")
TObjectPtr<UAnimationAsset> IdleAnimation;

// Played while moving.
UPROPERTY(EditAnywhere, Category = "Animation")
TObjectPtr<UAnimationAsset> JogAnimation;

// Ground speed (cm/s) above which the jog animation plays.
UPROPERTY(EditAnywhere, Category = "Animation", meta = (ClampMin = "0"))
float JogSpeedThreshold = 5.0f;
```

In `private:` add:
```cpp
void PlayLocomotionAnimation(UAnimationAsset* Animation);

// The animation currently playing from the idle/jog logic.
UPROPERTY(Transient)
TObjectPtr<UAnimationAsset> CurrentAnimation;
```

**File 2:** `Game/Source/GrantMoney/Private/GrantMoneyPlayerCharacter.cpp`

Add the include:
```cpp
#include "Animation/AnimationAsset.h"
```

In the constructor, **change** `PrimaryActorTick.bCanEverTick` from `false` to `true`. Then add, after the mesh lookup:
```cpp
static ConstructorHelpers::FObjectFinder<UAnimationAsset> IdleFinder(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));

if (IdleFinder.Succeeded())
{
	IdleAnimation = IdleFinder.Object;
}
else
{
	UE_LOG(LogTemp, Warning, TEXT("GrantMoneyPlayerCharacter: Idle animation is not available at /Game/Characters/Mannequins/Anims/Unarmed/MM_Idle."));
}

static ConstructorHelpers::FObjectFinder<UAnimationAsset> JogFinder(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd.MF_Unarmed_Jog_Fwd"));

if (JogFinder.Succeeded())
{
	JogAnimation = JogFinder.Object;
}
else
{
	UE_LOG(LogTemp, Warning, TEXT("GrantMoneyPlayerCharacter: Jog animation is not available at /Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd."));
}

CharacterMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
```

Add the new functions:
```cpp
void AGrantMoneyPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	PlayLocomotionAnimation(IdleAnimation);
}

void AGrantMoneyPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float GroundSpeed = GetVelocity().Size2D();
	PlayLocomotionAnimation(GroundSpeed > JogSpeedThreshold ? JogAnimation : IdleAnimation);
}

void AGrantMoneyPlayerCharacter::PlayLocomotionAnimation(UAnimationAsset* Animation)
{
	// Restart only when the desired animation actually changes.
	if (!Animation || Animation == CurrentAnimation)
	{
		return;
	}

	CurrentAnimation = Animation;
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GetMesh()->PlayAnimation(Animation, true);
}
```

**Explain on camera**
- There is no Animation Blueprint in this project, so animation is single-node and driven from C++.
- The tick chooses between idle and jog. It restarts a loop only when the choice changes, so the animation isn't reset every frame.
- Ticking must be on (`bCanEverTick = true`). If left false, idle plays and jog never does.

**Build and test:** header changed, so close the editor and do a full build. Then confirm Manny idles when still, jogs when WASD is held, and returns to idle when released.

**Commit**
```bash
git add Game/Source/
git diff --cached --stat
git commit -m "GMS-XXX: add idle and jog animation to player character"
```

**Screenshots**
- `C1-details-animation.png`: Idle and Jog properties in the Details panel (press F8 to eject during Play, then select the character).
- `C2-idle-play.png`: Manny idling.
- `C3-jog-play.png`: Manny jogging.

---

## Stage D: Visual data asset, the swap point (about 4 min)

**Explain first:** the mesh, animation and placement are hard-coded, so any change needs a rebuild. A data asset moves them to the editor. It also uses soft references, which the cooker can see.

**File 1 (replace the editor stub):** `Game/Source/GrantMoney/Public/GrantMoneyCharacterVisualData.h`
```cpp
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
```

**File 2:** `Game/Source/GrantMoney/Private/GrantMoneyCharacterVisualData.cpp`
```cpp
#include "GrantMoneyCharacterVisualData.h"
```

**File 3 (header edits):** `GrantMoneyPlayerCharacter.h`

Add near the other forward declarations:
```cpp
class UGrantMoneyCharacterVisualData;
```
In `protected:` add:
```cpp
// Optional override of the built-in Manny defaults. If the asset doesn't
// exist, the defaults set in the constructor are used.
UPROPERTY(EditAnywhere, Category = "Visual")
TSoftObjectPtr<UGrantMoneyCharacterVisualData> VisualData;
```
In `private:` add:
```cpp
void ApplyVisualData();

// False when a data asset supplies its own animation setup.
bool bUseBuiltInLocomotion = true;
```

**File 4 (source edits):** `GrantMoneyPlayerCharacter.cpp`

Add the include:
```cpp
#include "GrantMoneyCharacterVisualData.h"
```
In the constructor, after the `MoveAction` lookup, add the soft path (it logs no error if the asset isn't created yet):
```cpp
VisualData = TSoftObjectPtr<UGrantMoneyCharacterVisualData>(
	FSoftObjectPath(TEXT("/Game/Characters/Player/DA_PlayerVisual.DA_PlayerVisual")));
```
Replace `BeginPlay`:
```cpp
void AGrantMoneyPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	ApplyVisualData();

	if (bUseBuiltInLocomotion)
	{
		PlayLocomotionAnimation(IdleAnimation);
	}

	// Only the built-in idle/jog logic needs the tick.
	SetActorTickEnabled(bUseBuiltInLocomotion);
}
```
At the top of `Tick`, after `Super::Tick`, add:
```cpp
if (!bUseBuiltInLocomotion)
{
	return;
}
```
Add the new function:
```cpp
void AGrantMoneyPlayerCharacter::ApplyVisualData()
{
	// No data asset: keep the built-in Manny defaults.
	if (VisualData.IsNull())
	{
		return;
	}

	const UGrantMoneyCharacterVisualData* Data = VisualData.LoadSynchronous();
	if (!Data)
	{
		UE_LOG(LogTemp, Log, TEXT("GrantMoneyPlayerCharacter: %s not found. Using built-in Manny defaults."), *VisualData.ToString());
		return;
	}

	USkeletalMeshComponent* CharacterMesh = GetMesh();

	if (USkeletalMesh* LoadedMesh = Data->SkeletalMesh.LoadSynchronous())
	{
		CharacterMesh->SetSkeletalMesh(LoadedMesh);
	}

	// Feet at the capsule's base, plus any per-asset adjustment.
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	CharacterMesh->SetRelativeLocation(FVector(0.0, 0.0, -HalfHeight) + Data->MeshOffset);
	CharacterMesh->SetRelativeRotation(Data->MeshRotation);
	CharacterMesh->SetRelativeScale3D(Data->MeshScale);

	// An Animation Blueprint or a preview animation replaces the built-in idle/jog logic.
	if (UClass* LoadedAnimClass = Data->AnimClass.LoadSynchronous())
	{
		bUseBuiltInLocomotion = false;
		CharacterMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		CharacterMesh->SetAnimInstanceClass(LoadedAnimClass);
	}
	else if (UAnimationAsset* LoadedAnim = Data->PreviewAnimation.LoadSynchronous())
	{
		bUseBuiltInLocomotion = false;
		CharacterMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		CharacterMesh->PlayAnimation(LoadedAnim, true);
	}
}
```

**Editor steps**
1. Close the editor and do a full build (new `UCLASS`).
2. Create the folder `Content/Characters/Player/`. Right-click inside it, **Miscellaneous, Data Asset**, pick `GrantMoneyCharacterVisualData`. Name it exactly `DA_PlayerVisual`. The name and folder must match the path in the constructor.
3. Open it. Set **Skeletal Mesh** to `SKM_Manny_Simple`. Leave Anim Class and Preview Animation empty. Save.
4. Press Play. Manny should behave exactly as before.
5. **Swap demo (the key moment):** change Skeletal Mesh to `SKM_Quinn_Simple`, save, press Play **with no rebuild**. Quinn appears and still switches idle and jog. Change it back.
6. Say aloud that the swap works only because Manny and Quinn share a skeleton.

**Commit**
```bash
git add Game/Source/ Game/Content/Characters/
git diff --cached --stat
git lfs ls-files
git commit -m "GMS-XXX: add visual data asset for mesh and animation swap"
```

**Screenshots**
- `D1-data-asset-details.png`: `DA_PlayerVisual` with fields filled in.
- `D2-swap-quinn.png`: Quinn in Play.
- `D3-swap-back-manny.png`: Manny again.
- `D4-lfs-data-asset.png`: `DA_PlayerVisual.uasset` in `git lfs ls-files`.

---

## Stage E: Verify, document, open the PR (about 2 min)

**Do:** run the final checks, then record these.
- [ ] Manny visible, not black. Feet on the tile plane. Facing the walking direction.
- [ ] Idle when still, jog when moving.
- [ ] Camera, pause and tiles unchanged. Output Log clean.
- [ ] `git lfs ls-files` lists everything under `Content/Characters/`.

Write the task doc under `docs/course/Module2-.../Sprint1/`, with images in a nearby `images/` folder. It needs:
- Asset source and license, and the folder map.
- Skeleton, root bone, units, forward axis, sockets, frame rate (from Stage A).
- Import and migrate settings, and the C++ changes per stage.
- Verification steps, Jira key, branch, PR and commit checkpoints.
- The AI prompts used, with accepted, modified or rejected notes.

Then open the PR from the ticket branch and move Jira to In Review.

**Screenshots**
- `E1-final-play.png`: the final in-game view.
- `E2-git-log.png`: `git log --oneline` showing the stage commits.
- `E3-pr.png`: the open PR.

---

## Mistakes to capture on camera

These are real problems that came up during development. Recording them honestly is required by the documentation standard, and it saves viewers hours.

| Mistake | Where | What to show |
|---|---|---|
| Build error: "declaration of Mesh hides a class member" | Stage B or D | A local variable named `Mesh` collides with an `ACharacter` member. Rename it (`LoadedMesh`). |
| Assuming `ABP_Manny` existed | Stage C | The log said the Animation Blueprint was not available. The project has none, so animation is driven from C++. |
| Idle plays but jog never does | Stage C | `bCanEverTick` was still `false`, or the jog asset path was wrong. Show the Details panel with Jog Animation set. |
| Swapping to Quinn does nothing | Stage D | A wrong asset name or folder, an unsaved asset, or a saved override. The fallback is silent by design. Show the exact path check. |
| Editor shows old behavior after code edits | Any | Live Coding can't handle header or new-class changes. Close the editor and build fully. |

**Optional debug lines for a "swap does nothing" segment:** add `UE_LOG(LogTemp, Warning, TEXT("[VisualDebug] ..."))` lines at each exit of `ApplyVisualData()` (null path, failed load, empty mesh field, mesh before and after). Remove them before committing.

---

## Delivery checklist for the video producer

- [ ] One video segment per stage (0, A, B, C, D, E), each linked to its commit.
- [ ] All screenshots named as above and stored in the task doc's `images/` folder.
- [ ] The scratch Third Person project steps recorded, including the Migrate dependency list.
- [ ] The Quinn swap shown with **no rebuild**.
- [ ] The mistakes table recorded, not edited out.
- [ ] No credentials, tokens or personal paths visible.
- [ ] Final `git lfs ls-files`, `git log --oneline` and PR shown.
