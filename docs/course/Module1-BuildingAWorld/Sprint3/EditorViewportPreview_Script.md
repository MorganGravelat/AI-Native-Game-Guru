# VIDEO SCRIPT — Editor Viewport Preview for Procedural Actors
## Module 1 — Building a World (GMS-101)

> **Script status:** Steps 1–8 follow what actually happened in GMS-101: one structured request to the AI, an engine-source check, the code change, and successful Editor and Game builds. Steps 9–10 (looking at the preview in the editor, and PIE) had **not** been performed when this script was written. Record them live, and report whatever actually happens.
>
> **Verified** means a file read, engine-source check, diff or build backs it. **Pending** means not yet run. Do not present a pending step as passing.

# Opening

### SAY

Welcome back to Grant Money. Our map renderer works. Press Play, and the tiles and boundary walls appear. But in the editor, the level is empty. Every time we want to see a change, we press Play, look, stop and repeat.

Today we are going to make the generated map appear directly in the editor viewport, and update live as we change its settings.

Your actor will be different. Maybe it builds dungeon rooms, fences or spawn points. The important idea is not the tile map. It is this: **an actor that generates content must be able to rebuild itself safely, any number of times.** Once that is true, showing it in the editor is the easy part.

# Acceptance Criteria

### SAY

At the end of this part:

- The map appears in the editor without pressing Play.
- Changing a setting in the Details panel updates the viewport immediately, even while dragging a slider.
- Editing many times never leaves duplicate tiles or walls behind.
- The preview is never saved into the level file.
- Play still builds exactly one copy of the map.
- The project still compiles as a packaged game, not just in the editor.

# Engineering Principle

### SAY

Here is the engineering principle:

**Generated output is disposable. Rebuild it; never patch it.**

If your generated components can always be thrown away and recreated from the data, then the editor, Play and future features like regenerating a level can all share one build path. The moment generated output gets saved, copied or patched by hand, you have two sources of truth.

# Prerequisites

### SAY

You need an Unreal 5 C++ actor that already builds its content at runtime and works in Play. Ideally the build logic lives in one function. In Grant Money, that is `ATileMapRenderer::BuildMap`. You also need a clean branch, and the ability to build both the Editor target and the Game target.

---

# Step 1 — Protect the Baseline

### SAY

As always, we start clean so the diff only shows this feature.

### SHOW

Git Bash at the repository root.

### DO

```bash
git status
git checkout -b GMS-XX-editor-preview
```

### AI PROMPT

None.

### VERIFY

`working tree clean`. Grant Money used the branch `GMS-101-Render-to-Unreal-Editor-View`.

---

# Step 2 — Show the Problem

### SAY

Here is our level in the editor. The renderer actor is selected, and there is nothing to see. Now I press Play, and the map appears. That gap is what we are closing.

### SHOW

The editor viewport with `PlayGame1` open and the empty renderer actor selected, then PIE showing the map.

### DO

Select the renderer in the Outliner. Press **Play**, then stop.

### AI PROMPT

None.

### VERIFY

Learners can see that the map only exists at runtime.

---

# Step 3 — Ask AI for a Bounded Solution

### SAY

This is the request used in this milestone. Notice that it doesn't just say "make it work in the editor". It names three requirements: how to run in the editor, how to guard against the wrong world and packaged builds, and how to clean up. That last one is the part people forget.

### SHOW

The AI assistant (Claude Code in Grant Money), running from the repository root.

### DO

Paste the prompt.

### AI PROMPT

```text
I want the visuals this actor generates at Play time to also render in the
Unreal Editor viewport at edit time.

1. Editor Execution: override OnConstruction / PostEditChangeProperty so changes
   update dynamically when tweaking parameters in the Details panel.
2. Environment & World Guarding: check IsGameWorld() vs. WITH_EDITOR / GIsEditor,
   and wrap editor-only code in #if WITH_EDITOR so packaged builds still compile.
3. Lifecycle & Cleanup: clear previous generated components before regenerating,
   so there is no duplication or leak. Property edits should update in real time.

First read the .uproject and the actor's source. Do not assume file names.
Build both the Editor and Game targets, and tell me what you could not verify.
```

The original request in Grant Money also offered `UCLASS(ExecuteInEditor)` as an option. Keep that in mind; we will come back to it.

### VERIFY

The AI reads `GrantMoney.uproject` (version 5.8), `TileMapRenderer.h/.cpp`, `MapBoundaryGenerator.h/.cpp` and `MapArrays.h` before proposing code.

---

# Step 4 — Check the AI's Engine Assumptions

### SAY

Before accepting any code, we check two claims against the engine itself.

The first claim: `UCLASS(ExecuteInEditor)` is **not** a real Unreal specifier. The AI said so, and used `OnConstruction` instead, which is the C++ version of a Blueprint Construction Script. That is a good sign. If an AI ever "adds" a specifier you can't find in the docs, stop and check.

The second claim: in Unreal 5.8, every Details-panel edit reruns the construction script, including slider drags. The AI checked this in the engine source, not from memory.

### SHOW

`Engine/Source/Runtime/Engine/Private/ActorEditor.cpp` in the UE 5.8 install, at `AActor::PostEditChangeProperty`, with the `RerunConstructionScripts()` call highlighted.

### DO

Open the file and search for `RerunConstructionScripts`.

### AI PROMPT

```text
In my engine version, which function reruns OnConstruction when I edit a property
in the Details panel, and does it run during slider drags? Quote the engine file
and function you checked.
```

### VERIFY

The call exists, and nothing filters out interactive changes.

---

# Engineering Discussion

### SAY

Here is an engineering discussion.

The tempting solution is one line: call `BuildMap()` from `OnConstruction` and call it done. Try it, and the first slider drag adds a second full map, then a third, then a fourth.

Why? Components we create with `NewObject` and `AddInstanceComponent` are *instance* components. Unreal assumes we placed them deliberately, so rerunning construction doesn't delete them. And it gets worse. A destroyed component keeps its name until garbage collection, so recreating `BoundaryWalls` with the same name clashes. On top of that, instance components get saved into the level by default.

So the editor hook is the easy part. The real feature is making the build **safely repeatable**. That pays off later too. Any future feature that regenerates the map at runtime gets it for free.

---

# Step 5 — Inspect the Rebuild Design

### SAY

The AI's design has three parts.

**One: clear by tag.** Every generated component gets a tag, `TileMapRenderer.Generated`. `RebuildMap()` destroys everything with that tag, then builds. Why a tag instead of a saved list? Because a tag travels with the component, onto any copy of the actor.

**Two: unique names.** `MakeUniqueObjectName` instead of fixed names, so we never clash with a component that is destroyed but not yet collected.

**Three: transient flags.** `RF_Transient` means not saved into the level. `RF_DuplicateTransient` means not copied into Play. `RF_TextExportTransient` means not included in copy/paste.

### SHOW

`TileMapRenderer.cpp`: `RebuildMap`, `ClearGeneratedComponents`, and the `MarkAsGenerated` helper.

### DO

Scroll through each piece.

### AI PROMPT

```text
Show me exactly which line destroys the previous components before each build,
and which line keeps generated components out of the saved level.
```

### VERIFY

You can point at both lines yourself.

---

# Step 6 — Inspect the World Guard

### SAY

`OnConstruction` doesn't only run in the editor. It also runs when an actor is spawned during gameplay. If we built there too, a runtime-spawned renderer would build twice: once in construction, once in `BeginPlay`.

So `OnConstruction` only builds when `IsEditorPreviewWorld()` is true. That means the editor is running, the world is *not* a game world, this isn't the class default object, and we aren't cooking. `GIsEditor` alone isn't enough, because it is also true during Play-in-Editor.

`BeginPlay` keeps its job. It now calls `RebuildMap()`, so even if something slipped through, it is cleared first.

### SHOW

`OnConstruction` and `IsEditorPreviewWorld` in `TileMapRenderer.cpp`.

### DO

Highlight `World->IsGameWorld()` and `IsRunningCommandlet()`.

### AI PROMPT

None.

### VERIFY

There is exactly one build path per world type.

---

# Step 7 — Inspect the Details-Panel Extras

### SAY

Three small editor conveniences:

- A **Preview In Editor** checkbox. It is wrapped in `WITH_EDITORONLY_DATA`, so it doesn't exist in the shipped game.
- A **Rebuild Map** button. A `UFUNCTION` marked `CallInEditor` becomes a button for free.
- `bRunConstructionScriptOnDrag = false`. The tiles are attached to the actor, so moving it doesn't need a rebuild on every frame of the drag. Unreal still rebuilds once when you let go.

There is also a `PostEditChangeProperty` and `PostEditUndo` fallback. In 5.8 the engine already rebuilds on edits, so a small flag makes sure we never build twice. The fallback only acts if construction didn't run.

### SHOW

`TileMapRenderer.h`, the new public section.

### DO

Point at each `#if WITH_EDITOR` / `#if WITH_EDITORONLY_DATA` block.

### AI PROMPT

None.

### VERIFY

Every editor-only member and override is inside a guard.

---

# Step 8 — Build Both Targets

### SAY

Now we build. Two builds, not one. The Editor build proves the code compiles. The **Game** build proves our guards work, because that is the configuration a packaged game uses. If anything editor-only leaked out, this is where it fails.

Close the editor first. Header changes need a full build.

### SHOW

Git Bash, or the IDE build output.

### DO

```bash
"<UE>/Engine/Build/BatchFiles/Build.bat" GrantMoneyEditor Win64 Development -Project="<repo>/Game/GrantMoney.uproject" -WaitMutex
"<UE>/Engine/Build/BatchFiles/Build.bat" GrantMoney Win64 Development -Project="<repo>/Game/GrantMoney.uproject" -WaitMutex
```

### AI PROMPT

If the Game build fails:

```text
The Editor build succeeds but the Game build fails with this error. Which
editor-only symbol leaked, and what is the smallest guard fix?
```

### VERIFY

`Result: Succeeded` for both. **Verified in Grant Money.** The only warning, C4996, comes from the engine's own instanced-mesh header, not from our code.

---

# Step 9 — See It in the Editor (record live)

### SAY

Now the moment we came for.

### SHOW

The Unreal Editor with `PlayGame1` open.

### DO

1. Open the level. The map should already be there.
2. Select the renderer and drag **Wall Height**. Watch the walls move.
3. Change **Level To Render**.
4. Untick and re-tick **Preview In Editor**.
5. Press `Ctrl+Z` a few times.
6. Drag the actor around the viewport.

### AI PROMPT

If you see duplicates or errors:

```text
After editing a property, I see <describe duplicates / paste Output Log error>.
Here is the actor's component list. Which component is not being cleared, or
which name is reused? Give the smallest fix.
```

### VERIFY

Live updates, no duplicates, the toggle works, undo matches, and moving is smooth. **Pending at time of writing.**

---

# Step 10 — Save, Reload, Play (record live)

### SAY

Last, we prove the preview never leaks into the level file or into Play.

### SHOW

The editor, the Outliner and the Output Log.

### DO

1. Save the level, close it, and reopen it. The map is rebuilt.
2. Press **Play**. Walk into a wall.
3. In the Outliner (while playing), select the renderer and check its components: one group per tile type, and one wall component.

### AI PROMPT

None, unless something fails.

### VERIFY

One copy in Play, walls still block, and no errors in the log. **Pending at time of writing.**

---

# Common Mistakes

### SAY

Here are some common mistakes.

**First, trusting a feature name that sounds right.** `UCLASS(ExecuteInEditor)` was in our original request. It doesn't exist. Unity has `ExecuteInEditMode`, and that's probably where it came from. This actually happened in this milestone.

**Second, forgetting cleanup.** Calling the build from `OnConstruction` without clearing first stacks a new map on every edit.

**Third, fixed component names.** They work once and clash on the second build.

**Fourth, letting the preview get saved.** Then the level loads with an old map, and Play adds another one.

**Fifth, building only the Editor target.** Guards are only proven by a Game or packaged build.

And two small ones from our session: the AI's first draft made a constant private that a helper needed, which was caught in review before compiling. And the AI tried a Python script on a machine without Python. Check your tools.

# Verification Checkpoint

### SAY

Now let's verify our result.

### SHOW

- `git diff --stat`: four source files.
- Both build results.
- The editor viewport, with the map visible and not playing.
- A live slider drag.
- PIE, with one copy and blocking walls.
- The Output Log, filtered for `Error`.

Only claim the editor and PIE checks if you actually ran them on camera.

# AI Review

### SAY

Let's review what AI actually did here.

- **It generated:** the read-through of the actor and helper, the engine-source check, the rebuild design, the code, and both builds.
- **It corrected the request:** it replaced `ExecuteInEditor` with `OnConstruction`, and it flagged the name-clash problem before it happened.
- **Humans decided:** the requirements, including that cleanup and packaged builds mattered, and the Jira scope, GMS-101.
- **What failed:** a Python-based edit, because Python isn't installed, and an access level in the first draft. Both were fixed before compiling.
- **How it was verified:** two successful builds. Looking at the editor and playing are the steps we just recorded.

# Engineering Pause

### SAY

Here is an engineering pause. Consider this question:

*Did we build an editor feature, or a rebuild feature?*

Mostly a rebuild feature. `OnConstruction` is about ten lines. The work went into making the map safe to rebuild: tags, unique names, transient flags and one clear-then-build entry point. That is what lets the editor, Play and a future "regenerate level" button all share the same code.

# Completion Criteria

### SAY

This milestone is complete when:

- both the Editor and Game targets build;
- the map is visible in the editor and follows Details-panel edits live;
- repeated edits leave exactly one set of components;
- save and reload leave no generated components in the level;
- Play shows one copy, and the walls still block;
- the code and documentation are committed under GMS-101.

# Next Milestone

### SAY

Module 2 brings the world to life with a player character. Now that we can see the walls in the editor, we can place a player start, cameras and spawn points against them, without guessing. And because the map can rebuild safely, switching levels at runtime is now a small step, not a rewrite.

---

## Suggested Screenshots

| Filename | Content |
|---|---|
| `editor-preview-01-empty-editor.png` | The editor viewport before the change: renderer selected, nothing visible |
| `editor-preview-02-ai-prompt.png` | The three-requirement prompt in the AI assistant |
| `editor-preview-03-engine-rerun.png` | `ActorEditor.cpp` at `RerunConstructionScripts()` in `AActor::PostEditChangeProperty` |
| `editor-preview-04-rebuild-code.png` | `RebuildMap` / `ClearGeneratedComponents` in `TileMapRenderer.cpp` |
| `editor-preview-05-world-guard.png` | `OnConstruction` and `IsEditorPreviewWorld` |
| `editor-preview-06-build-editor.png` | Editor target `Result: Succeeded` |
| `editor-preview-07-build-game.png` | Game target `Result: Succeeded` |
| `editor-preview-08-details-panel.png` | Details panel showing **Preview In Editor** and **Rebuild Map** (once run) |
| `editor-preview-09-live-preview.png` | The map visible in the editor viewport, not playing (once run) |
| `editor-preview-10-slider-drag.png` | Walls mid-update during a `WallHeight` drag (once run) |
| `editor-preview-11-pie-single-copy.png` | PIE, with the renderer's component list showing one set (once run) |
