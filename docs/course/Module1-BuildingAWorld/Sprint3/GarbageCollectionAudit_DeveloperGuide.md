# Garbage Collection Safety Audit

> **Worked example:** GrantMoney, Module 1 (GMS-79). The audit covered the `GrantMoney` game module on Unreal Engine 5.8.
>
> **Evidence levels used in this guide:**
> - **Performed** means it actually happened during the milestone: a file read, a search, a diff or a commit.
> - **Pending** means it has not been run yet. The code change in this milestone has **not** been compiled or play-tested.

---

## 1. Objective

At the end of this guide you will have:

- a written inventory of every `UObject` reference in your game module, and how each one is kept alive;
- a decision for each reference: safe, needs a fix, or out of scope;
- the fixes applied as a small, reviewable diff;
- a verification plan that actually exercises the garbage collector.

In GrantMoney, the audit found **no** GC bugs. It found one convention gap: four raw `UObject*` members in `TileMapRenderer.h`, which were converted to `TObjectPtr<>`.

The method matters more than the result. The same audit applies to any Unreal C++ project, at any milestone.

## 2. Why This System Exists

### The engineering problem

Unreal manages `UObject` memory with a garbage collector. The collector only knows an object is in use if it can reach it through references it understands:

- `UPROPERTY()` members (raw pointer, `TObjectPtr`, or inside `TArray` / `TMap` / `TSet`);
- the object's Outer and the engine's own ownership lists (for example an actor's registered components);
- objects added to the root set (`AddToRoot`), or references reported by an `FGCObject`.

A pointer stored anywhere else (a plain C++ member, a static, a lambda capture, a non-UObject struct) is invisible to the collector. That gives you two failure modes:

1. **Premature collection.** The object is destroyed while code still points at it. The result is a crash, often minutes later and far from the cause.
2. **Unintended retention.** A strong reference keeps something alive after gameplay is done with it, such as a destroyed actor or a whole level.

Both bugs are intermittent. GC timing depends on memory pressure and frame timing, so they rarely show up in a short play session.

### Why at this point in the project

At the end of Module 1, the GrantMoney code base is small: six classes, about 1,300 lines outside the generated map data. Module 2 adds characters, enemies and spawning, and with them many more runtime-created objects. Auditing now is cheap. Auditing after an intermittent crash is expensive.

### Why an AI-assisted static review

GC rules are mechanical and pattern-based, which makes them a good fit for an AI reviewer. The AI can read every file and apply the same checklist consistently. The engineer keeps two jobs:

- deciding which findings are real defects and which are style;
- verifying the fix in the running engine.

### Alternatives deferred

| Alternative | Why not now |
|---|---|
| Rewrite `FollowTarget` as `TWeakObjectPtr` | Already guarded with `IsValid()`. Changing the type of a Blueprint-exposed property is a design decision, not a GC fix. |
| Runtime memory profiling (Unreal Insights, `memreport`) | Valuable later. For a code base this small, a static review finds the structural problems first. |
| Auditing the third-party `VisualStudioTools` plugin | It isn't our code, and it's editor tooling. |

## 3. Prerequisites

- An Unreal Engine 5.x C++ project that builds. GrantMoney uses 5.8 (`"EngineAssociation": "5.8"` in `Game/GrantMoney.uproject`).
- A clean working tree on a feature branch. GrantMoney used `GMS-79-Module-1-Verification`.
- An AI assistant that can read the repository files. In GrantMoney this was Claude Code, run from the repository root.
- Basic familiarity with `UPROPERTY`, `UCLASS` and `NewObject`.

## 4. Architecture Overview

The audit follows every way a `UObject` can be reached:

```text
Engine root set / World / Level
        ↓
Actors in the level (ATileMapRenderer, AMapCameraPawn, AMapCameraPlayerController)
        ↓
UPROPERTY members + OwnedComponents + InstanceComponents + AttachChildren
        ↓
Runtime-created objects (HISM components, dynamic materials)
        ↓
Their own UPROPERTYs (material slots → MID → texture parameters)
```

Anything that is not on one of these chains is either plain C++ data (fine) or a hidden reference (a bug).

The GrantMoney module splits into three groups:

| Group | Classes | GC relevance |
|---|---|---|
| Plain data | `FMapArrays`, `FGrantMoneyMapData`, `Map1_*` exports | None: static arrays and strings |
| Stateless helper | `FMapBoundaryGenerator` | Creates a component and hands ownership to the actor passed in |
| UObjects | `ATileMapRenderer`, `AMapCameraPawn`, `AMapCameraPlayerController`, `AGrantMoneyGameMode` | The whole audit focuses here |

## 5. AI-Assisted Planning

### AI Prompt

This is the prompt actually used in this milestone:

```text
I need you to verify there are no garbage collection issues and if so fix them.
```

It worked because the AI was running inside the repository and could read the files. A more reusable version, suitable for any project or tool, is:

```text
You are reviewing an Unreal Engine 5 C++ game module for garbage collection safety.

1. First read the .uproject to confirm the engine version, then read every header
   and source file under Source/<Module>. Do not assume file or class names.
2. For every UObject pointer (members, locals that outlive a frame, statics,
   lambda captures, delegates, timers, containers), state how the collector
   reaches it: UPROPERTY, component ownership, Outer, root set, FGCObject,
   or "not reachable".
3. Classify each finding as: real defect (premature collection or leak),
   convention / modernization, or safe.
4. Only fix real defects and clearly justified convention issues. Keep the diff
   minimal and do not change behavior, Blueprint-exposed property types,
   generated files, or third-party plugins.
5. Show the diff. List what you checked and found safe.
6. Say plainly what you could not verify (for example compiling or running PIE).
   Do not claim a build or test passed unless you ran it.
```

### Expected AI Output

- A file-by-file list of references and how each is kept alive.
- A short list of findings. In GrantMoney there was one: raw `UPROPERTY` pointers in `TileMapRenderer.h`.
- A minimal diff. In GrantMoney it was four type changes.
- An explicit statement of what was not verified.

Be suspicious of an answer that finds many "critical" GC bugs in code where every pointer is a `UPROPERTY`. That usually means the AI is misreading style issues as defects.

### Human Review

Before accepting the output, check:

- **Severity:** a raw `UPROPERTY() UFoo*` is *not* a GC bug; the collector still sees it. The fix is a modernization, and the commit message should say so.
- **Ownership claims:** confirm that runtime components really are registered. In GrantMoney, `TileMapRenderer.cpp` calls `AddInstanceComponent` and `RegisterComponent` for every `NewObject<UHierarchicalInstancedStaticMeshComponent>`.
- **Engine version:** `TObjectPtr` guidance applies to UE5. Confirm the version from the `.uproject` instead of trusting the AI's assumption.
- **Scope:** the diff should not touch generated data, plugins or Blueprint-exposed types.

## 6. Step-by-Step Implementation

### Step 1: Start from a clean branch
**Where:** Git Bash
**What:**
```bash
git status
git checkout -b GMS-XX-gc-audit
```
**Why:** A clean tree means the audit diff contains only audit changes.
**Expect:** `nothing to commit, working tree clean`.

### Step 2: Confirm the engine version
**Where:** Text editor or Git Bash
**What:** Open your `.uproject`. In GrantMoney it lives in a subfolder: `Game/GrantMoney.uproject`. Read `EngineAssociation`.
**Why:** The pointer guidance is version-specific.
**Expect:** a version string such as `"5.8"`.

### Step 3: List the files in scope
**Where:** Git Bash
**What:**
```bash
git ls-files Game/Source
```
**Why:** You will review every file, and the list shows which ones are plain data and which are UObjects.
**Expect:** GrantMoney lists 6 headers, 6 `.cpp` files in `Private/`, generated map files in `Private/Maps/`, and build/target files.

### Step 4: Search for hidden-reference patterns
**Where:** Git Bash
**What:**
```bash
grep -rn "TWeakObjectPtr\|AddToRoot\|FGCObject\|SetTimer\|AddDynamic\|AddUObject\|AddLambda" Game/Source
```
**Why:** These are the places where lifetime is managed by hand and bugs tend to hide.
**Expect:** In GrantMoney, no matches. This module uses none of these patterns.

### Step 5: Ask the AI for the audit
**Where:** Your AI assistant, run from the repository root
**What:** Paste the prompt from Section 5.
**Why:** The AI applies one checklist consistently to every file.
**Expect:** A per-file inventory and a small number of findings.

### Step 6: Review each finding against the rules
**Where:** Your IDE
**What:** For each reported pointer, open the declaration and confirm how it is reached. In GrantMoney:

| Reference | How it is reached | Verdict |
|---|---|---|
| `ATileMapRenderer::SceneRoot`, `TilePlaneMesh`, `BoundaryCubeMesh`, `TileBaseMaterial` | `UPROPERTY()` raw pointers | Safe; convert to `TObjectPtr` |
| `TileGroup_<id>` HISM components | `AddInstanceComponent` + `RegisterComponent` + attached to `SceneRoot` | Safe |
| `BoundaryWalls` HISM component | Same, done by `FMapBoundaryGenerator` | Safe |
| `UMaterialInstanceDynamic` per tile type | Outer = renderer; referenced from the component's material slot | Safe |
| `UTexture2D` from `LoadObject` | Referenced by the material instance's texture parameter | Safe |
| Local `TMap<int32, UHierarchicalInstancedStaticMeshComponent*>` | Used only inside one `BuildMap()` call; the components are already owned | Safe |
| `AMapCameraPawn` components and input actions | `UPROPERTY` `TObjectPtr` | Safe |
| `AMapCameraPawn::FollowTarget` | `UPROPERTY` `TObjectPtr`, checked with `IsValid()` before use | Safe |
| `AMapCameraPlayerController::MapCameraMappingContext` | `UPROPERTY` `TObjectPtr` | Safe |
| `ConstructorHelpers::FObjectFinder` statics | Standard engine pattern for constructor asset loading | Safe |

**Why:** You, not the AI, own the verdict.

### Step 7: Apply the minimal fix
**Where:** IDE
**What:** In `Game/Source/GrantMoney/Public/TileMapRenderer.h`:

```cpp
// Before
UPROPERTY()
USceneComponent* SceneRoot;

// After
UPROPERTY()
TObjectPtr<USceneComponent> SceneRoot;
```

Do the same for `TilePlaneMesh`, `BoundaryCubeMesh` (`TObjectPtr<UStaticMesh>`) and `TileBaseMaterial` (`TObjectPtr<UMaterialInterface>`).

**Why:** `TObjectPtr` is the UE5 standard for `UPROPERTY` object members and supports the GC write barrier.
**Expect:** No `.cpp` changes are needed, because `TObjectPtr<T>` converts to `T*` automatically.

### Step 8: Review the diff
**Where:** Git Bash
**What:**
```bash
git diff
```
**Expect:** exactly four changed lines in one header.

### Step 9: Build (pending in GrantMoney)
**Where:** Visual Studio or Rider, with the Unreal Editor **closed**
**What:** Build `GrantMoneyEditor Win64 Development`.
**Why:** Header changes require a full rebuild, and Live Coding cannot apply them.
**Expect:** A successful build with no new warnings.

### Step 10: Run a GC stress test in PIE (pending in GrantMoney)
**Where:** Unreal Editor
**What:**
1. Open the play level (`PlayGame1` in GrantMoney) and press **Play**.
2. Press the backtick (`` ` ``) to open the console.
3. Run `gc.CollectGarbageEveryFrame 1`.
4. Move the camera around for a minute, then run `obj list Class=HierarchicalInstancedStaticMeshComponent`.
5. Run `gc.CollectGarbageEveryFrame 0` before stopping.

**Why:** Collecting every frame turns rare timing-dependent GC bugs into immediate, reproducible ones.
**Expect:** Tiles and walls stay visible, there are no crashes or `Error` lines in the Output Log, and the HISM components are listed (one per tile type, plus `BoundaryWalls`).

### Step 11: Commit
**Where:** Git Bash
**What:**
```bash
git add Game/Source/GrantMoney/Public/TileMapRenderer.h
git commit -m "GMS-XX: use TObjectPtr for TileMapRenderer UPROPERTY members"
```

## 7. AI Prompts by Stage

### Prompt A: Inventory references
```text
Read every file under Source/<Module>. For each UObject pointer, list file, line,
declaration, and how the garbage collector reaches it. Do not suggest changes yet.
```

### Prompt B: Classify
```text
For the inventory above, classify each entry as: real defect (explain the exact
scenario where the object is collected too early or never freed), convention issue,
or safe. Be conservative: a UPROPERTY raw pointer is not a defect.
```

### Prompt C: Minimal fix
```text
Apply only the fixes for real defects and the convention issues I approve below.
Do not change Blueprint-exposed property types, generated files, or plugins.
Show the full diff.
```

### Prompt D: Verification plan
```text
Give me a PIE test plan that would expose garbage collection bugs in these classes,
using engine console commands. Tell me exactly what output indicates a problem.
```

### Prompt E: Debug a GC crash (if one appears)
```text
Here is the crash call stack and the last 50 lines of the Output Log. Identify which
UObject was accessed after collection, which reference failed to keep it alive, and
the smallest fix. Do not guess file names; ask me for the file if you need it.
```

## 8. Common Mistakes

The first two items actually happened during this milestone. The rest are general cautions, not observed GrantMoney bugs.

### Can't find the `.uproject` (observed)
- **Symptom:** The developer could not locate the project file to open or rebuild.
- **Likely Cause:** In GrantMoney the Unreal project lives in a `Game/` subfolder (`Game/GrantMoney.uproject`), not at the repository root, and it is named after the game, not the repo.
- **Fix:** Open `Game/GrantMoney.uproject`. Right-click it and choose **Generate Visual Studio project files** if you need IDE files.
- **Lesson:** Write the project file's location in the README of a course repository.

### AI cannot open the PR (observed)
- **Symptom:** The AI pushed the branch but could not create the pull request.
- **Likely Cause:** The GitHub CLI (`gh`) was not installed on the machine.
- **Fix:** Use the GitHub compare page, or install `gh` and run `gh auth login`.
- **Lesson:** Check that your tooling is available before handing a workflow to AI.

### Treating style as a crash bug (general caution)
- **Symptom:** A review calls raw `UPROPERTY` pointers "dangling" or "unsafe".
- **Likely Cause:** Confusing UE5 convention with GC reachability.
- **Fix:** Convert them, but label the change as modernization.
- **Lesson:** Accurate severity keeps reviews credible.

### Editing headers with the editor open (general caution)
- **Symptom:** The build fails, or Live Coding refuses the change.
- **Fix:** Close the Unreal Editor and build from the IDE.
- **Lesson:** This matches the GMS-76 camera milestone experience.

## 9. Verification

| Test | Action | Expected Result | Status |
|------|--------|-----------------|--------|
| Static review | AI read every file in `Game/Source/GrantMoney` | Inventory with verdicts | **Performed** |
| Hidden-reference search | `grep` from Step 4 | No unmanaged patterns | **Performed**: no matches |
| Diff review | `git diff` | Four type changes, one file | **Performed** |
| Editor build | Build `GrantMoneyEditor` | Succeeds | **Pending** |
| PIE smoke test | Play `PlayGame1` | Tiles, walls, camera unchanged | **Pending** (editor-only) |
| GC stress | `gc.CollectGarbageEveryFrame 1` for 60 s | No crash, no errors | **Pending** (editor-only) |
| Packaged build | Package Win64 and play | Same as PIE | **Pending** (packaged) |

## 10. Known Limitations

- The audit was a static review; no runtime memory profiling was done.
- The third-party `VisualStudioTools` plugin was not audited.
- `ATileMapRenderer::BuildMap` creates components with fixed names (`TileGroup_<id>`, `BoundaryWalls`). That is safe for the current single call from `BeginPlay`, but rebuilding the map at runtime would need cleanup first. This is a design note, not a GC defect.
- `FollowTarget` is a strong reference. It is correctly guarded, but it is not a weak pointer.
- The change is not compiled or tested yet (see Section 9).

## 11. Extension Ideas

- Repeat this audit at the end of every module. It is cheapest when the code base is small.
- Add a recurring checklist item to PR reviews: "every UObject pointer member is a `UPROPERTY` `TObjectPtr` or a documented `TWeakObjectPtr`".
- For spawning systems (enemies, projectiles, pickups), extend the audit to object pools and timers, which are where GC bugs usually appear.
- Run `gc.CollectGarbageEveryFrame 1` as a standard step in every milestone's PIE verification.

## 12. Source-Control Checkpoint

| Item | Value |
|---|---|
| Jira ticket | `GMS-79` (replace with your own ticket) |
| Branch | `GMS-79-Module-1-Verification` (general form: `GMS-XX-gc-audit`) |
| Code commit | `977b4ab Use TObjectPtr for TileMapRenderer UPROPERTY members` (already pushed) |
| Changed code file | `Game/Source/GrantMoney/Public/TileMapRenderer.h` |
| Docs | `docs/course/Module1-BuildingAWorld/Sprint3/GarbageCollectionAudit_*.md` |
| PR title | `GMS-79: Garbage collection safety audit (TObjectPtr for TileMapRenderer)` |

Staging the docs:

```bash
git add docs/course/Module1-BuildingAWorld/Sprint3/GarbageCollectionAudit_ChangeReport.md
git add docs/course/Module1-BuildingAWorld/Sprint3/GarbageCollectionAudit_DeveloperGuide.md
git add docs/course/Module1-BuildingAWorld/Sprint3/GarbageCollectionAudit_Script.md
git diff --cached --stat
git commit -m "GMS-79: document garbage collection safety audit for Module 1"
```

Verification checklist before merging:

- [ ] Editor build succeeds
- [ ] PIE: tiles, walls and camera behave as before
- [ ] GC stress run (`gc.CollectGarbageEveryFrame 1`) shows no crash or errors
- [ ] PR description says the change is a modernization, not a crash fix
