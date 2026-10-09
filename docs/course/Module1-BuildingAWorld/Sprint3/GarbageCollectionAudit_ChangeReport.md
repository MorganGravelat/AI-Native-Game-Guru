# OUTPUT A — Milestone Change Report
## Garbage Collection Safety Audit (GMS-79, Module 1 Verification)

> **Status:** The audit and the single code change are complete and committed (`977b4ab`) on `GMS-79-Module-1-Verification`, and the branch is pushed. The change has **not** been compiled or run. Nothing below is described as passing unless it was actually performed.

## Milestone Summary

**Before:** Module 1 had a working world renderer (`ATileMapRenderer`), a procedural boundary generator (`FMapBoundaryGenerator`) and a free-roaming camera (`AMapCameraPawn` + `AMapCameraPlayerController`). Nobody had checked the module as a whole for Unreal garbage collection (GC) safety. `ATileMapRenderer` still declared four GC-tracked members as raw `UObject*` pointers, while the newer camera code used `TObjectPtr<>`.

**After:**

- Every UObject reference in the `GrantMoney` game module has been reviewed for GC safety.
- No object that is still in use can be collected, and nothing was found to leak.
- `ATileMapRenderer`'s four `UPROPERTY()` members now use `TObjectPtr<>`, so the whole module follows one convention.

**Why it matters architecturally:**

- Raw `UObject*` members marked `UPROPERTY()` are still kept alive by the collector, so the old code was **not** broken.
- In UE 5.x, `TObjectPtr<>` is the recommended form for `UPROPERTY` object members. It adds a write barrier that incremental reachability analysis (GC work spread across frames) relies on. Raw pointers bypass that barrier.
- Doing this audit at the end of Module 1, before player characters, enemies and spawning arrive in Module 2, keeps a small problem from multiplying.

The original request to the AI was:

> I need you to verify there are no garbage collection issues and if so fix them.

## Files Created

| File | Purpose |
|------|---------|
| `docs/course/Module1-BuildingAWorld/Sprint3/GarbageCollectionAudit_ChangeReport.md` | This report. |
| `docs/course/Module1-BuildingAWorld/Sprint3/GarbageCollectionAudit_DeveloperGuide.md` | Reusable guide for running a GC audit with AI. |
| `docs/course/Module1-BuildingAWorld/Sprint3/GarbageCollectionAudit_Script.md` | Instructor recording script. |

No source files were created.

## Files Modified

| File | What Changed | Why |
|------|--------------|-----|
| `Game/Source/GrantMoney/Public/TileMapRenderer.h` | `SceneRoot`, `TilePlaneMesh`, `BoundaryCubeMesh` and `TileBaseMaterial` changed from `USceneComponent*` / `UStaticMesh*` / `UMaterialInterface*` to `TObjectPtr<...>`. The `UPROPERTY()` specifiers are unchanged. | Puts the members behind the UE5 GC write barrier and matches the rest of the module. |

`Private/TileMapRenderer.cpp` did **not** need changes. `TObjectPtr<T>` converts to `T*` automatically wherever the code passes these values on (`SetupAttachment`, `SetStaticMesh`, `UMaterialInstanceDynamic::Create`, `FMapBoundaryGenerator::GenerateBoundary`).

## Files Not Changed

| Item | Why it was preserved |
|------|----------------------|
| `Game/Source/GrantMoney/Private/TileMapRenderer.cpp` | Runtime HISM components are kept alive by `AddInstanceComponent`, `RegisterComponent` and attachment to `SceneRoot`. Dynamic materials are kept alive by the component's material slot, and textures by the material parameters. No change needed. |
| `Game/Source/GrantMoney/Public/MapCameraPawn.h` / `Private/MapCameraPawn.cpp` | All object members are already `UPROPERTY` `TObjectPtr`. `FollowTarget` is checked with `IsValid()` before every use. Enhanced Input bindings are bound to the pawn and cleaned up with it. |
| `Game/Source/GrantMoney/Public/MapCameraPlayerController.h` / `.cpp` | The mapping context is already a `UPROPERTY` `TObjectPtr`. |
| `Game/Source/GrantMoney/Public/MapBoundaryGenerator.h` / `.cpp` | A static helper that holds no UObject state. The wall component it creates is owned by the actor passed in. |
| `Game/Source/GrantMoney/Public/MapArrays.h` / `.cpp` | Plain static C++ arrays and strings; no UObjects. |
| `Game/Source/GrantMoney/Private/Maps/Map1_*.cpp/.h` (TileGen exports) | Generated data; no UObjects. |
| `Game/Plugins/VisualStudioTools/` | Third-party plugin; outside the audit scope. |
| `Game/Content/` (all `.uasset` / `.umap`) | No assets were touched. |
| `Game/GrantMoney.uproject`, `Game/Config/*.ini` | No GC settings were changed. |

## Verification Status

| Check | Result |
|---|---|
| Every header and source file in `Game/Source/GrantMoney` read and reviewed | **Performed** (static review by AI) |
| Search for `TWeakObjectPtr`, `AddToRoot`, `FGCObject`, `SetTimer`, `AddDynamic`, `AddUObject`, `AddLambda` in `Game/Source` | **Performed**: no matches |
| Engine version confirmed from `GrantMoney.uproject` | **Performed**: `"EngineAssociation": "5.8"` |
| `git diff` of the change reviewed | **Performed**: four type changes only |
| C++ build (`GrantMoneyEditor Win64 Development`) | **Not run** |
| PIE: tiles, walls and camera still work | **Not run** |
| GC stress run (`gc.CollectGarbageEveryFrame 1` in PIE) | **Not run** |
| Packaged build | **Not run** |

## Problems Found and Corrected

| Problem | Correction |
|---|---|
| Raw `UObject*` members in `TileMapRenderer.h`. These were GC-safe but did not follow the UE5 convention. | Converted to `TObjectPtr<>` (commit `977b4ab`). |
| The GitHub CLI (`gh`) was not installed, so the AI could not open the pull request. | The branch was pushed, and the developer was given a compare link and a PR description to paste. |
| The developer could not find the `.uproject` file. | It is at `Game/GrantMoney.uproject`, inside the `Game/` subfolder rather than at the repository root. |

## Git Staging Recommendation

The code change is **already committed and pushed** as:

```text
977b4ab Use TObjectPtr for TileMapRenderer UPROPERTY members
```

That message does not follow the `GMS-XX:` format. Leave it as is, or reword it before merging if your team requires the format.

Only the documentation remains to stage:

```bash
git status
```

```bash
git add docs/course/Module1-BuildingAWorld/Sprint3/GarbageCollectionAudit_ChangeReport.md
git add docs/course/Module1-BuildingAWorld/Sprint3/GarbageCollectionAudit_DeveloperGuide.md
git add docs/course/Module1-BuildingAWorld/Sprint3/GarbageCollectionAudit_Script.md
```

```bash
git diff --cached --stat
git diff --cached
```

Suggested commit:

```text
GMS-79: document garbage collection safety audit for Module 1
```

Do not commit, push or merge automatically.
