# Master Prompts: Player Visual Model and Animations (Unreal Engine 5.8, C++)

**Purpose:** Prompts you can paste into Claude or another LLM to get the instructions and C++ code for giving the Grant Money player a skeletal mesh (Epic's Manny) with idle/jog animation and a no-recompile swap point.

**Project assumptions:** Repo `AI-Native-Game-Guru`, project `Game/GrantMoney.uproject`, module `GrantMoney`, Windows, C++ gameplay only (no Blueprint gameplay logic). The player is `AGrantMoneyPlayerCharacter` (an `ACharacter`) that already spawns, moves camera-relative with WASD and collides.

> **Always verify what the LLM gives you.** Nothing it writes has been compiled until you build it. Send it the first real compiler error, not the trailing summary.

---

## How to use

1. Paste **Prompt 0 (context)** first in a new chat.
2. Run Prompts 1 to 6 in order. Commit after each stage.
3. Use Prompt 7 only when something doesn't work.
4. Compare every answer to the **Key mistakes** table at the end.

---

## Prompt 0: Context (paste first)

```text
I am working on an Unreal Engine 5.8 C++ project called Grant Money on Windows.
Repo: AI-Native-Game-Guru. Project: Game/GrantMoney.uproject. Module: GrantMoney
(API macro GRANTMONEY_API). Source lives in Game/Source/GrantMoney/Public and /Private.

Rules:
- C++ gameplay only. No Blueprint gameplay logic.
- Do not change the GameMode, PlayerController, pause system, TileMapRenderer,
  MapArrays, or the static map camera.
- Git with Git LFS for .uasset/.umap. One Jira ticket = one branch = one PR.
- I use Unreal's Content Browser to move assets, never Windows Explorer.

Existing state: AGrantMoneyPlayerCharacter (ACharacter) is possessed by Player 0,
moves relative to the camera with Enhanced Input (IA_CameraMove), and has a
capsule collision. The mesh component has no visual asset yet.

Do not claim anything compiled or ran. I will report results. When you write code,
tell me the exact file and where in the file each change goes. When something
fails, ask me for the first real compiler error or Output Log lines.
```

---

## Prompt 1: Plan (no code yet)

```text
Plan the minimum viable player visual asset using Epic's Mannequin (Manny) as a
placeholder, with idle and jog animations, in the project described above.
Do not write the full solution yet. Give me:
1. The staged progression (asset import, mesh in C++, animation, data-asset swap
   point, documentation), with one commit per stage.
2. Which C++ classes are new and which are modified.
3. Pre-flight checks: lighting (my map uses an Unlit tile material), camera angle,
   mesh scale vs 100 cm tiles, Git LFS coverage.
4. What I should record in the asset documentation (skeleton, root bone, units,
   forward axis, sockets, frame rate, import settings, license).
```

**A good answer includes:** staged commits, a warning that a lit material renders black in an unlit level, and the Mannequin being roughly 180 uu tall against 100 uu tiles.

---

## Prompt 2: Import the assets

```text
Give me exact steps to bring Manny and his idle and jog animations into my
GrantMoney project without importing the Third Person template's GameMode,
Blueprints or input assets. Use a scratch Third Person project outside my repo and
Unreal's Migrate. Tell me which assets to select, what the dependency list should
and should not contain, where to point the destination, how to organize folders,
how to fix redirectors, and how to validate the skeleton and play an animation in
the Animation Editor. Keep Epic's asset names unchanged. Do not rename assets.
```

**A good answer includes:** the scratch project outside the repo, selecting only mesh and needed animations, a destination of `Game/Content` with the editor closed, "Fix Up Redirectors in Folder", and not renaming Epic's assets.

**Asset paths used in this project** (confirm in your own Content Browser):

| Asset | Path |
|---|---|
| Mesh | `/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple` |
| Idle | `/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle` |
| Jog | `/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd` |

---

## Prompt 3: Attach the mesh (C++)

```text
In my existing AGrantMoneyPlayerCharacter (ACharacter), add the minimum C++ to show
the Manny mesh at /Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple.
Requirements:
- Load the mesh with ConstructorHelpers::FObjectFinder in the constructor.
- Position the mesh so the feet sit at the capsule bottom (use the capsule half-height,
  not a magic number).
- Rotate it so the Mannequin faces the capsule's forward direction.
- Disable collision, overlap events and navigation on the mesh; the capsule owns collision.
- Log a warning if the asset is missing.
Tell me the exact includes and where each line goes in the .cpp. Do not touch any
other class. Do not name any local variable "Mesh" (ACharacter already has a member
named Mesh). Tell me whether I need a full build or Live Coding.
```

**A good answer includes:** yaw `-90`, with the reason that the Mannequin faces +Y in mesh space; `-GetCapsuleComponent()->GetScaledCapsuleHalfHeight()` for Z; `NoCollision`; and "full build, not Live Coding".

---

## Prompt 4: Idle and jog animation

```text
Extend AGrantMoneyPlayerCharacter so the mesh plays an idle animation when standing
still and a jog animation when moving. My project has NO Animation Blueprint, so
use single-node animation mode driven from C++. Idle: /Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle.
Jog: /Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd.MF_Unarmed_Jog_Fwd.
Requirements:
- UPROPERTY(EditAnywhere) members for the idle and jog animations, plus a ground-speed
  threshold (default 5 cm/s).
- Load the defaults in the constructor with FObjectFinder and log a warning if either fails.
- Choose the animation each Tick from GetVelocity().Size2D(), but restart the loop
  only when the chosen animation actually changes.
- Show every header change and every .cpp change, and tell me which constructor
  line must change to enable ticking.
```

**A good answer includes:** `PrimaryActorTick.bCanEverTick = true`, a `BeginPlay` that starts idle, a `PlayLocomotionAnimation` helper with an early return when the animation is unchanged, warnings on both finders, and a full build (header changed).

---

## Prompt 5: Data-asset swap point

```text
Add a data asset that lets us swap the player's skeletal mesh, animation and
placement without recompiling. Create UGrantMoneyCharacterVisualData, a real UCLASS
deriving from UDataAsset (not a plain C++ class), with: TSoftObjectPtr<USkeletalMesh>,
TSoftClassPtr<UAnimInstance> AnimClass, TSoftObjectPtr<UAnimationAsset> PreviewAnimation,
MeshOffset, MeshRotation (default yaw -90) and MeshScale. In AGrantMoneyPlayerCharacter
add a TSoftObjectPtr to the data asset defaulting to
/Game/Characters/Player/DA_PlayerVisual.DA_PlayerVisual (a soft path, so a missing
asset logs no error) and an ApplyVisualData() called from BeginPlay that overrides the
built-in Manny defaults. If AnimClass or PreviewAnimation is set, switch off the
built-in idle/jog logic and its tick. Show every file, and tell me how to create the
asset in the editor and run a Manny-to-Quinn swap.
```

**A good answer includes:** `UCLASS(BlueprintType)` on a `UDataAsset` subclass, `Engine/DataAsset.h`, soft pointers (visible to the cooker), `LoadSynchronous()`, a local variable not named `Mesh` (use `LoadedMesh`), and a note that the swap works only because Manny and Quinn share a skeleton.

---

## Prompt 6: Verify, Git and documentation

```text
Give me a verification checklist for the player visual in PlayGame (visibility,
feet on the tile plane, facing, scale, lighting, idle/jog switching, camera, pause,
tiles unchanged, Output Log clean) and the exact Git and Git LFS commands to stage
only the Characters content and source files, check that every new .uasset is an LFS
file, and commit with a Jira-linked message. Then outline the task document I should
write: asset source and license, folder map, skeleton, root bone, units, forward axis,
sockets, frame rate, the C++ changes per stage, mistakes and fixes, AI prompts used
with accepted/modified/rejected notes, and verification steps.
```

---

## Prompt 7: Debugging (use when something fails)

Fill in the bracket and paste:

```text
This is not working: [describe exactly what you see, for example "Manny stays the
model after I set Quinn in DA_PlayerVisual" or "idle plays but jog never does"].
Give me temporary UE_LOG debug lines and an ordered list of checks. The code is
silent on failure by design (soft paths fall back to defaults), so each log line
must tell me which exit was taken. Include checks for: a stale build (Live Coding),
the asset's exact name, folder and class, unsaved asset edits, saved Blueprint or
level-instance overrides of EditAnywhere properties, ticking disabled, a null
animation pointer, a skeleton mismatch, and the wrong character class spawning.
Tell me how to read each log result.
```

---

## Key mistakes to check in any LLM answer

| Mistake | Correct approach |
|---|---|
| Local variable named `Mesh` in an `ACharacter` function | Rename (for example `LoadedMesh`). `Mesh` hides an `ACharacter` member and breaks the build. |
| Assuming `ABP_Manny` exists | This project has no Animation Blueprint. Drive idle/jog from C++ in single-node mode. |
| Using Live Coding after adding a `UCLASS`, a header change or a property | Close the editor and do a full Development Editor build. |
| Data asset class declared as a plain C++ class (the editor wizard stub) | It must be a `UCLASS` derived from `UDataAsset`, or it can't be created in the editor. |
| Forgetting to enable ticking | `bCanEverTick = true`. Otherwise idle plays and jog never does. |
| Mesh facing sideways | The Mannequin faces +Y in mesh space, so use yaw `-90`. |
| Feet floating or sunk | Z offset is `-capsule half-height` (88 by default), not a guess. |
| Mesh with collision | Mesh collision off. The capsule is the only collision shape. |
| Renaming Epic's assets | Keep names so Manny/Quinn swaps and Epic's docs stay valid. |
| Migrating the whole template | Migrate only the mesh, skeleton dependencies and needed animations. Check the dependency list. |
| Moving assets in Windows Explorer | Move only in the Content Browser, then Fix Up Redirectors. |
| Wrong asset path or data-asset name | The code looks for exactly `/Game/Characters/Player/DA_PlayerVisual`. A wrong name silently falls back to Manny. |
| Unsaved or overridden values | Save the data asset. Check Blueprint subclasses and level instances for saved overrides. |
| Mesh looks black | The level is probably unlit. Needs a light (shared `.umap`, coordinate) or an unlit material. |
| Different-skeleton mesh with Manny animations | Retarget the animations (IK Rig and IK Retargeter) or set an `AnimClass`. |
| `git add .` after an Unreal session | Stage explicit paths and check `git diff --cached --stat` and `git lfs ls-files`. |
| Claiming success before building | Always build and test, then report results back. |

---

## File map (for reference)

| File | Folder under `Game/Source/GrantMoney/` | Role |
|---|---|---|
| `GrantMoneyPlayerCharacter.h` | `Public/` | Properties, tick, `ApplyVisualData` declaration |
| `GrantMoneyPlayerCharacter.cpp` | `Private/` | Mesh, animation and data-asset logic |
| `GrantMoneyCharacterVisualData.h` | `Public/` | The data asset class |
| `GrantMoneyCharacterVisualData.cpp` | `Private/` | Include only |
| `DA_PlayerVisual.uasset` | `Game/Content/Characters/Player/` | The editable swap point |
