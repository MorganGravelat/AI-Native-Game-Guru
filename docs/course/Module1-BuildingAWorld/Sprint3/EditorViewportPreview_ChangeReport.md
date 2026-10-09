# OUTPUT A — Milestone Change Report
## Editor Viewport Preview of the Generated Map (GMS-101)

> **Status:** The code change is complete but **not committed**. It is on `GMS-101-Render-to-Unreal-Editor-View`. Both the editor target and the packaged (non-editor) game target **compile**. Nobody has opened the editor to look at the preview yet. Nothing below is described as passing unless it was actually performed.

## Milestone Summary

**Before:** `ATileMapRenderer` built the ground tiles and boundary walls only in `BeginPlay`. In the editor, the actor was an empty root component. To see the level layout you had to press **Play**.

**After:**

- The tiles and walls appear in the editor viewport as soon as the actor is placed or the level is opened.
- Changing `LevelToRender`, `WallHeight`, `WallThickness`, `BoundaryMaterial` or `bGenerateBoundaries` in the Details panel rebuilds the preview immediately, including while a slider is being dragged.
- A **Preview In Editor** checkbox turns the preview off, and a **Rebuild Map** button forces a rebuild.
- Every rebuild clears the previous components first, so nothing piles up.
- The preview is never saved into the `.umap`. Gameplay still builds its own fresh copy in `BeginPlay`.

**Why it matters architecturally:**

- The same `BuildMap()` code now serves two callers, the editor (`OnConstruction`) and gameplay (`BeginPlay`). There is still one source of truth for how a map is built.
- Generated components are now treated as **disposable output**: tagged, transient and always rebuilt from `MapArrays`. That makes the renderer safe to rebuild any number of times, which the GC audit (GMS-79) had flagged as a known limitation.
- Editor-only code is behind `WITH_EDITOR` / `WITH_EDITORONLY_DATA`, and a packaged build was compiled to prove it.

The original request to the AI listed three requirements: editor execution (`OnConstruction` / `PostEditChangeProperty`), world guarding (`IsGameWorld`, `WITH_EDITOR`), and lifecycle cleanup (no duplicates or leaks, real-time updates).

## Files Created

| File | Purpose |
|------|---------|
| `docs/course/Module1-BuildingAWorld/Sprint3/EditorViewportPreview_ChangeReport.md` | This report. |
| `docs/course/Module1-BuildingAWorld/Sprint3/EditorViewportPreview_DeveloperGuide.md` | Reusable guide for adding an editor preview to a procedural actor. |
| `docs/course/Module1-BuildingAWorld/Sprint3/EditorViewportPreview_Script.md` | Instructor recording script. |

No source files were created.

## Files Modified

| File | What Changed | Why |
|------|--------------|-----|
| `Game/Source/GrantMoney/Public/TileMapRenderer.h` | Added the `bPreviewInEditor` property (editor-only), the `RebuildMap()` `CallInEditor` function, the `OnConstruction` override, `PostEditChangeProperty` / `PostEditUndo` overrides (`WITH_EDITOR`), the public `GeneratedComponentTag`, and the private `ClearGeneratedComponents()`, `IsEditorPreviewWorld()` and `bConstructedDuringEdit`. | Declares the editor entry points, cleanup and guards. |
| `Game/Source/GrantMoney/Private/TileMapRenderer.cpp` | `OnConstruction` builds the preview in editor worlds only. `BeginPlay` now calls `RebuildMap()`. `ClearGeneratedComponents()` destroys components by tag. Tile groups get unique names (`MakeUniqueObjectName`) and are marked transient and tagged. The tile material instances are marked transient. `bRunConstructionScriptOnDrag = false`. | Editor preview, real-time updates and cleanup without duplicates. |
| `Game/Source/GrantMoney/Public/MapBoundaryGenerator.h` | New optional last parameter `FName GeneratedTag = NAME_None` on `GenerateBoundary`. | Lets the owner tag the wall component so it can be cleared. Existing calls still compile. |
| `Game/Source/GrantMoney/Private/MapBoundaryGenerator.cpp` | The wall component gets a unique name. When `GeneratedTag` is set, it is tagged and marked transient. | Same cleanup and naming rules as the tiles. |

## Files Not Changed

| Item | Why it was preserved |
|------|----------------------|
| `Game/Source/GrantMoney/Public/MapArrays.h` / `Private/MapArrays.cpp` | The data interface is unchanged; the preview reads it exactly like gameplay does. |
| `Game/Source/GrantMoney/Private/Maps/Map1_*` (TileGen exports) | Generated data; not touched. |
| `Game/Source/GrantMoney/GrantMoney.Build.cs` | No new module dependencies were needed. `UnrealEd` was deliberately **not** added, because that would break the packaged build. |
| Camera classes (`MapCameraPawn`, `MapCameraPlayerController`), `GrantMoneyGameMode` | Unrelated to map rendering. |
| `M_Tile`, tile textures, all `Game/Content` assets and maps | The preview uses the same assets as gameplay. No asset needs re-saving for this milestone. |
| `Game/Config/*.ini`, `Game/GrantMoney.uproject` | No settings or plugins were needed. |

## Verification Status

| Check | Result |
|---|---|
| Engine version confirmed from `GrantMoney.uproject` | **Performed**: `"EngineAssociation": "5.8"` |
| Engine source checked: `AActor::PostEditChangeProperty` reruns construction on every edit, including interactive (slider) edits; `bRunConstructionScriptOnDrag` is editor-only data | **Performed** (read `Engine/Source/Runtime/Engine/Private/ActorEditor.cpp` and `Classes/GameFramework/Actor.h` in the UE 5.8 install) |
| Editor build: `Build.bat GrantMoneyEditor Win64 Development` | **Performed: succeeded** |
| Game (non-editor) build: `Build.bat GrantMoney Win64 Development` | **Performed: succeeded**. This proves the `WITH_EDITOR` guards compile out cleanly. |
| Build warnings | One warning, C4996 (`BuildComponentInstanceData` deprecated), from the engine's own `HierarchicalInstancedStaticMeshComponent.h`, not from project code. |
| Preview appears in the editor viewport | **Not run** |
| Details-panel edits update the preview in real time | **Not run** |
| No duplicated components after repeated edits | **Not run** |
| Undo/redo restores the matching preview | **Not run** |
| Save, close and reopen the level: preview rebuilds and the `.umap` holds no generated components | **Not run** |
| PIE: one copy of tiles and walls, collision still works | **Not run** |
| Packaged build runs | **Not run** (compiled only) |

The builds ran against identical source in the developer's original checkout of this branch, before the session moved to this working tree.

## Problems Found and Corrected

| Problem | Correction |
|---|---|
| The request suggested `UCLASS(ExecuteInEditor)`. That is not a real Unreal class specifier. | Used `OnConstruction`, the C++ equivalent of a Blueprint Construction Script. |
| The tile groups and wall used fixed names (`TileGroup_<id>`, `BoundaryWalls`). A destroyed component keeps its name until garbage collection, so a second build would clash. | Names now come from `MakeUniqueObjectName`. |
| The AI's first edit attempt used a Python script, but Python is not installed on the machine. | Edits were made with the editor tools instead. |
| First draft: `GeneratedComponentTag` was `private`, but a helper function outside the class needed it. | Moved to `public` before compiling. |
| First draft included `Misc/CoreMisc.h` for `IsRunningCommandlet`. | Replaced with `CoreGlobals.h`, which declares it. |

## Git Staging Recommendation

```bash
git status
```

```bash
git add Game/Source/GrantMoney/Public/TileMapRenderer.h
git add Game/Source/GrantMoney/Private/TileMapRenderer.cpp
git add Game/Source/GrantMoney/Public/MapBoundaryGenerator.h
git add Game/Source/GrantMoney/Private/MapBoundaryGenerator.cpp
git add docs/course/Module1-BuildingAWorld/Sprint3/EditorViewportPreview_ChangeReport.md
git add docs/course/Module1-BuildingAWorld/Sprint3/EditorViewportPreview_DeveloperGuide.md
git add docs/course/Module1-BuildingAWorld/Sprint3/EditorViewportPreview_Script.md
```

```bash
git diff --cached --stat
git diff --cached
```

Suggested commit:

```text
GMS-101: preview generated tile map and boundaries in the editor viewport
```

Do not commit, push or merge automatically.
