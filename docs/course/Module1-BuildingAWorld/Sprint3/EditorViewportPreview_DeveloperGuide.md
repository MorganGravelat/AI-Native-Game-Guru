# Editor Viewport Preview for Procedural Actors

> **Worked example:** GrantMoney, Module 1 (GMS-101), Unreal Engine 5.8. The actor is `ATileMapRenderer`, which builds a TileGen map and its boundary walls.
>
> **Evidence levels used in this guide:**
> - **Performed** means it actually happened during the milestone: a file read, an engine-source check, a diff or a build.
> - **Pending** means it has not been run yet. Both build targets compile, but nobody has opened the editor to look at the preview yet.

---

## 1. Objective

At the end of this guide you will have a procedural C++ actor that:

- draws its generated content in the editor viewport, without pressing **Play**;
- rebuilds immediately when you change its properties in the Details panel, including during slider drags;
- never duplicates its components, however many times it rebuilds;
- never saves its generated content into the level file;
- still builds normally at runtime, exactly once;
- compiles in a packaged (non-editor) build.

In GrantMoney, the actor is a tile-map renderer. The method works for any actor that creates components from data: a dungeon builder, a fence spline, a crowd spawner or a track generator.

## 2. Why This System Exists

### The engineering problem

Before this milestone, `ATileMapRenderer` built everything in `BeginPlay`. In the editor it was an invisible root component, so level design worked like this:

1. Change `LevelToRender` or `WallHeight`.
2. Press **Play**.
3. Look.
4. Stop and repeat.

That loop is slow, and it hides problems. You can't place a camera, a light or a spawn point against walls you can't see.

### Why `OnConstruction`

Unreal already has a hook that runs whenever an actor is placed, loaded, moved or edited in the editor: the construction script. In C++ it is the virtual `AActor::OnConstruction`. In UE 5.8, `AActor::PostEditChangeProperty` reruns the construction script on **every** property change, including interactive ones such as slider drags. This was checked in `Engine/Source/Runtime/Engine/Private/ActorEditor.cpp`.

That means the editor preview needs no custom refresh machinery. You put the build call in `OnConstruction` and Unreal calls it at the right times.

### The real difficulty: rebuilding safely

Running `BuildMap()` once in `BeginPlay` is easy. Running it hundreds of times in the editor exposes three problems:

1. **Duplicates.** Components created with `NewObject` + `AddInstanceComponent` are *instance* components. Rerunning the construction script does not remove them, so each rebuild adds another full copy.
2. **Name clashes.** A destroyed component keeps its name until garbage collection runs. Creating a new component with the same fixed name (`TileGroup_3`, `BoundaryWalls`) in the same actor clashes with it.
3. **Saved output.** Instance components are saved into the `.umap` by default. The level would then load with a stale copy, and `BeginPlay` would add a second one.

Most of this milestone is about these three problems, not about the editor hook.

### Alternatives considered

| Alternative | Decision |
|---|---|
| `UCLASS(ExecuteInEditor)` | Not a real Unreal class specifier. `OnConstruction` is the C++ equivalent of a Blueprint Construction Script. |
| Move the logic into a Blueprint Construction Script | Rejected. The code is in C++ and a Blueprint copy would split the source of truth. |
| Keep a cached `TArray` of created components | Rejected. A cached list is lost on copies of the actor (PIE, duplicates), so leftovers would go unnoticed. A **component tag** travels with each component. |
| Mark components as `CreationMethod = UserConstructionScript` so the engine deletes them | Deferred. It works, but the engine then also tries to restore per-component instance data, which is hard to reason about for instanced meshes. |
| A dedicated editor module (`UnrealEd` dependency) | Not needed. Everything fits in the runtime module behind `WITH_EDITOR`. Adding `UnrealEd` to a runtime module breaks packaged builds. |
| Incremental rebuild (only walls when wall settings change) | Deferred. Full rebuilds are fast enough for a 64 × 64 map. |

## 3. Prerequisites

- An Unreal Engine 5.x C++ project. GrantMoney uses 5.8 (`"EngineAssociation": "5.8"` in `Game/GrantMoney.uproject`).
- A C++ actor that already builds its content at runtime and works in PIE. In GrantMoney, that is `ATileMapRenderer` (TileGen World Renderer and Map Boundary milestones).
- Build logic that lives in one function (`BuildMap()` here), not spread across `BeginPlay`.
- A clean working tree on a feature branch. GrantMoney used `GMS-101-Render-to-Unreal-Editor-View`.
- The ability to build both the **Editor** target and the **Game** target. The second build is how you prove the editor-only code is guarded.

## 4. Architecture Overview

```text
                 Editor world                      Game world (PIE / packaged)
                 ────────────                      ───────────────────────────
Place / load / move / edit actor                   BeginPlay
        ↓                                               ↓
AActor::PostEditChangeProperty / PostEditMove          RebuildMap()
        ↓                                               │
RerunConstructionScripts()                              │
        ↓                                               │
OnConstruction()  ── IsEditorPreviewWorld()? ──no──> return (gameplay owns it)
        │ yes                                           │
        ↓                                               ↓
RebuildMap() ───────────────────────────────────────────┘
        ↓
ClearGeneratedComponents()   destroy every component tagged "TileMapRenderer.Generated"
        ↓
BuildMap()                   MapArrays → HISM tile groups + FMapBoundaryGenerator walls
        ↓
New components: unique name + tag + RF_Transient | RF_DuplicateTransient | RF_TextExportTransient
```

### Responsibility boundaries

| Piece | Responsibility |
|---|---|
| `OnConstruction` | *When* to build in the editor. It doesn't know how to build. |
| `BeginPlay` | *When* to build in the game. |
| `IsEditorPreviewWorld()` | *Whether* this world should get a preview: editor running, not a game world, not the class default object, not a commandlet such as cooking. |
| `RebuildMap()` | Clear, then build. The only entry point both worlds use. |
| `ClearGeneratedComponents()` | Find generated components by tag and destroy them. |
| `BuildMap()` / `FMapBoundaryGenerator` | *How* to build. Unchanged except for naming, tagging and flags. |
| `MapArrays` | The data. Not touched. |

### What each object flag does

| Flag | Effect |
|---|---|
| `RF_Transient` | Not saved into the `.umap`. |
| `RF_DuplicateTransient` | Not copied when the world is duplicated for PIE, or when the actor is duplicated. |
| `RF_TextExportTransient` | Not included in copy/paste text. |

The tag is the safety net. If a generated component ever does reach a copy of the actor, `ClearGeneratedComponents()` still finds it before the next build.

## 5. AI-Assisted Planning

### AI Prompt

This is the prompt actually used in this milestone, slightly shortened:

```text
For this Unreal Engine C++ project where custom rendering, procedural mesh
generation, or dynamic visual elements are generated when pressing Play (PIE).
I want to modify this code so that the visuals also render directly inside the
Unreal Engine Editor viewport at edit time, allowing me to preview the level
layout without launching the game.

1. Editor Execution: mark the class with UCLASS(ExecuteInEditor) or override
   OnConstruction / PostEditChangeProperty so changes update dynamically when
   tweaking parameters in the Details panel.
2. Environment & World Guarding: check GetWorld()->IsGameWorld() vs.
   WITH_EDITOR / GIsEditor. Wrap editor-only code in #if WITH_EDITOR to prevent
   cook/build errors in packaged non-editor builds.
3. Lifecycle & Cleanup: clean up previous generated components before
   re-generating to avoid duplication or memory leaks. Property edits should
   trigger an update in real time.
```

It worked because the AI was running inside the repository and could read the source and the engine install. A more reusable version is:

```text
I have an Unreal Engine 5 C++ actor that generates components at runtime in
BeginPlay. I want the same content to appear in the editor viewport and update
live when I edit its properties.

1. First read the .uproject (engine version) and the actor's .h/.cpp, plus any
   helper that creates components for it. Do not assume class or file names.
2. Before writing code, tell me which engine function reruns the construction
   script on property edits in my engine version, and whether it runs during
   slider drags. If you can read the engine source, check it.
3. Build the preview from OnConstruction, in editor (non-game) worlds only.
   Keep BeginPlay as the runtime path. Never build twice at runtime.
4. Make rebuilding safe: clear old generated components first, avoid name
   clashes with destroyed components, and make sure generated components are
   never saved into the level or duplicated into PIE.
5. Put editor-only code behind WITH_EDITOR / WITH_EDITORONLY_DATA. Do not add
   editor-only module dependencies to a runtime module.
6. Keep the diff minimal. Do not change the data interface or generated files.
7. Build both the Editor target and the Game target if you can. Say plainly
   what you did not verify (opening the editor, PIE, packaged run).
```

### Expected AI Output

- An explanation of when `OnConstruction` runs in your engine version.
- An `OnConstruction` override guarded by a world check.
- A clear-then-build function used by both `OnConstruction` and `BeginPlay`.
- A cleanup strategy: tags, a tracked list, or engine-managed construction components. Ask why it chose that one.
- A naming strategy that avoids clashes.
- Object flags or another mechanism that keeps generated output out of saves and PIE.
- Results from both builds, and a list of what was not run.

### Human Review

Before accepting the output, check:

- **Invented specifiers.** `UCLASS(ExecuteInEditor)` does not exist. If the AI "adds" it, the code won't compile, or UHT will reject it.
- **Double building.** Does a runtime-spawned actor build in both `OnConstruction` and `BeginPlay`? It must not. In GrantMoney, `OnConstruction` returns early in game worlds.
- **Cleanup really runs.** Find the line that destroys old components before every build.
- **Names.** Search for fixed `NewObject(..., TEXT("SomeName"))` names in anything that can now run more than once.
- **Guards.** Every override that exists only in editor builds (`PostEditChangeProperty`, `PostEditUndo`) and every editor-only member (`bRunConstructionScriptOnDrag`) must be inside the matching `#if`. Only the Game build proves this.
- **Build.cs.** It should not gain `UnrealEd`.

## 6. Step-by-Step Implementation

### Step 1: Start from a clean branch
**Where:** Git Bash
**What:**
```bash
git status
git checkout -b GMS-XX-editor-preview
```
**Why:** The diff should show only preview changes.
**Expect:** `nothing to commit, working tree clean`.

### Step 2: Confirm the engine version and how edits rerun construction
**Where:** Text editor, and the engine install
**What:**
1. Read `EngineAssociation` in your `.uproject` (`Game/GrantMoney.uproject`: `"5.8"`).
2. Open `Engine/Source/Runtime/Engine/Private/ActorEditor.cpp` in your engine install and find `AActor::PostEditChangeProperty`.
3. Check that it calls `RerunConstructionScripts()` without filtering out interactive changes.

**Why:** If your engine version skips interactive edits, slider drags won't update live, and you would need extra code in `PostEditChangeProperty`.
**Expect:** In UE 5.8, the rerun happens for every edit (except `ActorLabel`). `PostEditMove` only reruns during a drag when `bRunConstructionScriptOnDrag` is true.

### Step 3: Declare the editor entry points
**Where:** IDE, the actor's header (`Game/Source/GrantMoney/Public/TileMapRenderer.h`)
**What:** Add to the `public:` section:

```cpp
#if WITH_EDITORONLY_DATA
    // Draws the map in the editor viewport while editing the level.
    // The preview is never saved; gameplay always builds a fresh copy.
    UPROPERTY(EditAnywhere, Category = "Map")
    bool bPreviewInEditor = true;
#endif

    // Clears and rebuilds the generated tiles and walls.
    // Shows up as a "Rebuild Map" button in the Details panel.
    UFUNCTION(CallInEditor, Category = "Map")
    void RebuildMap();

    virtual void OnConstruction(const FTransform& Transform) override;

    static const FName GeneratedComponentTag;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent
    ) override;

    virtual void PostEditUndo() override;
#endif
```

And to `private:`:

```cpp
    void ClearGeneratedComponents();

#if WITH_EDITOR
    bool IsEditorPreviewWorld() const;
    bool bConstructedDuringEdit = false;
#endif
```

**Why:** `CallInEditor` gives you a manual escape hatch. `WITH_EDITORONLY_DATA` removes the checkbox from packaged builds.
**Expect:** After building, the Details panel shows **Preview In Editor** and a **Rebuild Map** button under *Map*.

### Step 4: Guard the world
**Where:** IDE, the actor's `.cpp`
**What:**

```cpp
#if WITH_EDITOR
bool ATileMapRenderer::IsEditorPreviewWorld() const
{
    if (!GIsEditor || IsTemplate() || IsRunningCommandlet())
    {
        return false;
    }

    const UWorld* World = GetWorld();

    return World && !World->IsGameWorld();
}
#endif
```

**Why:** `GIsEditor` is also true during PIE, so the `IsGameWorld()` check is what separates the editor from PIE. `IsRunningCommandlet()` keeps cooking and resaving from building previews nobody will see.

### Step 5: Build from `OnConstruction`
**Where:** IDE, the actor's `.cpp`
**What:**

```cpp
void ATileMapRenderer::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

#if WITH_EDITOR
    if (!IsEditorPreviewWorld())
    {
        return;
    }

    bConstructedDuringEdit = true;

    if (bPreviewInEditor)
    {
        RebuildMap();
    }
    else
    {
        ClearGeneratedComponents();
    }
#endif
}
```

And change `BeginPlay` from `BuildMap();` to `RebuildMap();`.

**Why:** Game worlds are left to `BeginPlay`. An actor spawned at runtime also runs `OnConstruction`, and would otherwise build twice.

### Step 6: Clear by tag
**Where:** IDE, the actor's `.cpp`
**What:**

```cpp
void ATileMapRenderer::RebuildMap()
{
    ClearGeneratedComponents();
    BuildMap();
}

void ATileMapRenderer::ClearGeneratedComponents()
{
    TArray<UActorComponent*> Generated;

    for (UActorComponent* Component : GetComponents())
    {
        if (Component && Component->ComponentHasTag(GeneratedComponentTag))
        {
            Generated.Add(Component);
        }
    }

    for (UActorComponent* Component : Generated)
    {
        RemoveInstanceComponent(Component);
        Component->DestroyComponent();
    }
}
```

**Why:** Collect first, then destroy. Destroying while iterating the actor's own component set would modify it mid-loop.

### Step 7: Make every generated component disposable
**Where:** IDE, wherever components are created (`TileMapRenderer.cpp` and `MapBoundaryGenerator.cpp`)
**What:** For each `NewObject<...Component>`:

1. Generate a unique name:
   ```cpp
   const FName ComponentName = MakeUniqueObjectName(
       this,
       UHierarchicalInstancedStaticMeshComponent::StaticClass(),
       *FString::Printf(TEXT("TileGroup_%d"), TileId)
   );
   ```
2. Right after `NewObject`, set the flags and the tag:
   ```cpp
   Component->SetFlags(RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
   Component->ComponentTags.AddUnique(ATileMapRenderer::GeneratedComponentTag);
   ```
3. Mark any `UMaterialInstanceDynamic` you create as `RF_Transient` too.

For a helper that creates components on someone else's behalf (`FMapBoundaryGenerator`), add an optional `FName GeneratedTag = NAME_None` parameter instead of making the helper depend on the actor class. Existing callers keep compiling.

**Why:** Unique names avoid the clash with destroyed components. The flags keep the output out of saves, PIE and copy/paste. The tag lets cleanup find it.

### Step 8: Don't rebuild on every frame of a drag
**Where:** IDE, the actor's constructor
**What:**
```cpp
#if WITH_EDITORONLY_DATA
    bRunConstructionScriptOnDrag = false;
#endif
```
**Why:** The generated components are attached to the root, so they already follow the actor. The engine still reruns construction once when the drag ends.

### Step 9: Add edit and undo fallbacks
**Where:** IDE, the actor's `.cpp`
**What:** In `PostEditChangeProperty` and `PostEditUndo`, set `bConstructedDuringEdit = false`, then call `Super`. Afterwards, rebuild only if `OnConstruction` did **not** run. For property edits, also check that the property is one that affects the map (`GET_MEMBER_NAME_CHECKED`).
**Why:** In UE 5.8, `Super::PostEditChangeProperty` normally reruns construction already, so this fallback usually does nothing. The flag keeps it from building twice. Undo restores property values, and the fallback makes sure the preview matches them.

### Step 10: Build both targets, editor closed
**Where:** Git Bash or IDE, with the Unreal Editor **closed**
**What:**
```bash
"<UE>/Engine/Build/BatchFiles/Build.bat" GrantMoneyEditor Win64 Development -Project="<repo>/Game/GrantMoney.uproject" -WaitMutex
"<UE>/Engine/Build/BatchFiles/Build.bat" GrantMoney Win64 Development -Project="<repo>/Game/GrantMoney.uproject" -WaitMutex
```
**Why:** The Editor build proves the code compiles. The Game build proves the editor-only code is guarded. Header changes need a full build; Live Coding can't apply them.
**Expect:** `Result: Succeeded` for both. **Performed in GrantMoney.** The only warning was C4996 from the engine's own HISM header.

### Step 11: Look at it in the editor (pending in GrantMoney)
**Where:** Unreal Editor
**What:**
1. Open the level containing the renderer (`PlayGame1` in GrantMoney).
2. Select the renderer actor in the Outliner.
3. Change `LevelToRender`, then drag `WallHeight`.
4. Untick **Preview In Editor**, then tick it again.
5. Press `Ctrl+Z` a few times.
6. Save the level, close it, and reopen it.
7. Press **Play**.

**Expect:**
- The map appears without Play.
- The preview follows every edit live.
- The map disappears and returns with the checkbox.
- Undo matches the restored values.
- After reopening, the map is rebuilt.
- In PIE there is exactly one copy, and the walls still block the player.

## 7. AI Prompts by Stage

### Prompt A: Inspect the build path
```text
Read <Actor>.h/.cpp and every helper it calls to create components. List each
component it creates, its name, how it is attached and registered, and when the
creating function runs. Do not change anything yet.
```

### Prompt B: Confirm engine behavior
```text
In Unreal Engine <version>, when exactly does AActor::OnConstruction run in the
editor? Does a Details-panel slider drag rerun it? Does moving the actor? If you
can read the engine source in my install, quote the function and file you checked.
```

### Prompt C: Implement a safe rebuild
```text
Add RebuildMap() that destroys all previously generated components and then calls
the existing build function. Generated components must have unique names, a
component tag, and RF_Transient | RF_DuplicateTransient | RF_TextExportTransient.
Show the full diff. Do not change the data interface.
```

### Prompt D: Wire the editor hooks
```text
Call RebuildMap() from OnConstruction only in non-game editor worlds (not the
CDO, not commandlets). Keep BeginPlay as the runtime path. Put editor-only
overrides and members behind WITH_EDITOR / WITH_EDITORONLY_DATA.
```

### Prompt E: Prove the guards
```text
Build the Editor target and the Game (non-editor) target. If the Game target fails,
show the error and the smallest guard fix. Tell me which checks still need a human
in the editor.
```

### Prompt F: Debug duplicates or clashes (if they appear)
```text
After editing a property, the Outliner shows <N> copies of <component>, or the
log shows <paste error>. Here is the component list from the Details panel and the
Output Log. Find which component is not being cleared or which name is reused,
and give the smallest fix.
```

## 8. Common Mistakes

The first three items actually happened during this milestone. The rest were **prevented by design**: the AI identified them before writing code. None of them was observed in GrantMoney.

### Asking for a specifier that doesn't exist (observed)
- **Symptom:** The plan calls for `UCLASS(ExecuteInEditor)`.
- **Likely Cause:** It sounds like a real Unreal feature. Unity has `[ExecuteInEditMode]`, and Unreal has `CallInEditor` for functions.
- **Fix:** Use `OnConstruction` for actors.
- **Lesson:** Ask the AI to confirm that an engine feature exists before building on it.

### Helper can't see a private member (observed, caught before compiling)
- **Symptom:** The first draft put `GeneratedComponentTag` in `private:` while a free helper function in the `.cpp` used it.
- **Fix:** Moved it to `public:`.
- **Lesson:** Review access levels when a helper lives outside the class.

### Tooling assumption (observed)
- **Symptom:** The AI tried to edit files with a Python script, and the shell reported that Python was not found.
- **Fix:** It used the editor tools instead.
- **Lesson:** Don't assume scripting tools exist on a Windows game-dev machine.

### Duplicated components after each edit (prevented)
- **Symptom:** Every slider move adds another full set of tiles. The Outliner's component list grows, and the frame rate drops.
- **Likely Cause:** No cleanup before building, or cleanup that only knows about construction-script components.
- **Fix:** `ClearGeneratedComponents()` by tag at the start of every rebuild.

### Name clash on the second build (prevented)
- **Symptom:** A crash or an error about an object name already in use, on the second build.
- **Likely Cause:** A fixed name such as `TEXT("BoundaryWalls")` and an old, destroyed component that has not been garbage-collected yet.
- **Fix:** `MakeUniqueObjectName`.

### Two copies in PIE, or a stale map after loading (prevented)
- **Symptom:** Walls are doubled in PIE, or an old map shows after changing `LevelToRender` and reloading.
- **Likely Cause:** The preview was saved into the `.umap` or duplicated into PIE.
- **Fix:** Transient flags, plus clearing by tag in `BeginPlay`.

### Packaged build fails (prevented)
- **Symptom:** Errors about `PostEditChangeProperty`, `FPropertyChangedEvent` or `bRunConstructionScriptOnDrag` in a Game or cook build.
- **Fix:** Wrap them in `WITH_EDITOR` / `WITH_EDITORONLY_DATA`. Build the Game target to confirm.

## 9. Verification

| Test | Action | Expected Result | Status |
|------|--------|-----------------|--------|
| Engine behavior check | Read `AActor::PostEditChangeProperty` in UE 5.8 | Reruns construction on every edit | **Performed** |
| Editor build | `Build.bat GrantMoneyEditor Win64 Development` | Succeeds | **Performed: succeeded** |
| Non-editor build | `Build.bat GrantMoney Win64 Development` | Succeeds | **Performed: succeeded** |
| Preview appears | Open `PlayGame1` in the editor | Tiles and walls visible without Play | **Pending** (editor) |
| Live edits | Drag `WallHeight`, change `LevelToRender` | Viewport updates during the drag | **Pending** (editor) |
| No duplicates | Edit 10+ times, then inspect the actor's components | One HISM per tile type plus one wall component | **Pending** (editor) |
| Toggle | Untick and re-tick **Preview In Editor** | The map clears and returns | **Pending** (editor) |
| Undo | `Ctrl+Z` after edits | The preview matches the restored values | **Pending** (editor) |
| Not saved | Save, close and reopen the level | Map rebuilt on open; no errors in the log | **Pending** (editor) |
| Move | Drag the actor | Content follows; one rebuild on release | **Pending** (editor) |
| PIE | Press Play | Exactly one copy; walls block the player | **Pending** (editor) |
| Packaged run | Package Win64 and play | Same as PIE | **Pending** (packaged) |

## 10. Known Limitations

- **Log noise:** `BuildMap` logs at `Warning` level on each build, so slider drags flood the Output Log.
- **Full rebuilds only:** every change rebuilds all tiles and walls, including texture lookups. That's fine for 64 × 64, but not measured for larger maps.
- **Synchronous loading:** textures load with `LoadObject` on the editor thread.
- **Previews everywhere:** the preview also builds in other non-game worlds, such as a Blueprint subclass's preview viewport.
- **Untested setups:** World Partition and level instances.
- **Undo history:** after an undo, the actor's saved instance-component list may briefly hold references to already-destroyed preview components. They are garbage and harmless, but this was not observed either way.
- **No runtime switching:** changing `LevelToRender` during gameplay is still unsupported. `RebuildMap()` makes it possible later.

## 11. Extension Ideas

- **Dungeon or room builders:** preview room layouts from seed data before playing.
- **Spline tools:** fences, rails or race-track barriers that rebuild as you edit the spline.
- **Population tools:** previewing foliage, props or spawn markers, with a "Rebuild" button and a seed property.
- **Runtime regeneration:** the same safe `RebuildMap()` path supports switching maps in-game, or roguelike floor changes.
- **Editor-only debug drawing:** add a `WITH_EDITOR` overlay (cell IDs, invalid cells) using the same world guard.

## 12. Source-Control Checkpoint

| Item | Value |
|---|---|
| Jira ticket | `GMS-101` (replace with your own ticket) |
| Branch | `GMS-101-Render-to-Unreal-Editor-View` (general form: `GMS-XX-editor-preview`) |
| Changed code files | `Game/Source/GrantMoney/Public/TileMapRenderer.h`, `Game/Source/GrantMoney/Private/TileMapRenderer.cpp`, `Game/Source/GrantMoney/Public/MapBoundaryGenerator.h`, `Game/Source/GrantMoney/Private/MapBoundaryGenerator.cpp` |
| Docs | `docs/course/Module1-BuildingAWorld/Sprint3/EditorViewportPreview_*.md` |
| PR title | `GMS-101: Preview generated tile map and boundaries in the editor viewport` |

```bash
git add Game/Source/GrantMoney/Public/TileMapRenderer.h
git add Game/Source/GrantMoney/Private/TileMapRenderer.cpp
git add Game/Source/GrantMoney/Public/MapBoundaryGenerator.h
git add Game/Source/GrantMoney/Private/MapBoundaryGenerator.cpp
git add docs/course/Module1-BuildingAWorld/Sprint3/EditorViewportPreview_ChangeReport.md
git add docs/course/Module1-BuildingAWorld/Sprint3/EditorViewportPreview_DeveloperGuide.md
git add docs/course/Module1-BuildingAWorld/Sprint3/EditorViewportPreview_Script.md
git diff --cached --stat
git commit -m "GMS-101: preview generated tile map and boundaries in the editor viewport"
```

Verification checklist before merging:

- [x] Editor build succeeds
- [x] Non-editor Game build succeeds
- [ ] Preview appears in the editor and updates live
- [ ] No duplicated components after repeated edits
- [ ] Save/reload leaves no generated components in the level
- [ ] PIE shows one copy; walls still block the player
