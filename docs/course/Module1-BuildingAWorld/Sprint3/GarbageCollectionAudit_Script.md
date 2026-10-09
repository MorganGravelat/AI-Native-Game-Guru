# VIDEO SCRIPT — Garbage Collection Safety Audit
## Module 1 — Building a World (Verification)

> **Script status:** Steps 1–6 follow what actually happened in GMS-79: one prompt to the AI, a static review of the whole game module, one four-line fix, a commit and a push. Steps 7–8 (build and GC stress test) had **not** been performed when this script was written. Record them live, and report whatever actually happens.
>
> **Verified** means a file read, search, diff or commit backs it. **Pending** means not yet run. Do not present a pending step as passing.

# Opening

### SAY

Welcome back to Grant Money. Module 1 is functionally done. We have a generated tile world, procedural boundary walls and a free-roaming camera.

Before we add characters and enemies in Module 2, we are going to check that this code is safe with Unreal's garbage collector.

The exact classes do not matter. Your game will have different actors and components. The important idea is that an AI can apply a mechanical safety checklist to every file, while you stay in charge of severity and verification.

# Acceptance Criteria

### SAY

At the end of this part:

- Every UObject reference in our game module has been inventoried.
- Each one has a verdict: safe, needs a fix, or out of scope.
- Real defects are fixed with a minimal diff.
- Convention issues are labelled as convention issues, not crash bugs.
- We have built the project and run a garbage collection stress test, and we report what actually happened.

# Engineering Principle

### SAY

Here is the engineering principle:

**Audit while the system is small.**

Garbage collection bugs are intermittent. They depend on memory pressure and timing, so a five-minute play session rarely shows them. Reviewing six classes takes minutes. Hunting a random crash across sixty classes takes days.

# Prerequisites

### SAY

You need a C++ Unreal 5 project that builds, a clean branch, and an AI assistant that can read your repository. In Grant Money the project file is `Game/GrantMoney.uproject`. Note that it is inside the `Game` folder, not at the repository root.

---

# Step 1 — Protect the Baseline

### SAY

As always, we start clean so the diff shows only audit changes.

### SHOW

Git Bash at the repository root.

### DO

```bash
git status
git checkout -b GMS-XX-gc-audit
```

### AI PROMPT

None.

### VERIFY

`working tree clean`. Grant Money used the branch `GMS-79-Module-1-Verification`.

---

# Step 2 — Explain What the Collector Can See

### SAY

Unreal's garbage collector only keeps an object alive if it can reach it. It can follow `UPROPERTY` members, the components an actor owns, an object's Outer, and the root set. A pointer anywhere else is invisible to it. That leads to two failure modes: the object is deleted while we still use it, or it is kept alive forever.

### SHOW

`Game/Source/GrantMoney/Public/TileMapRenderer.h`, the private members section.

### DO

Point at `UPROPERTY()` above `SceneRoot`.

### AI PROMPT

None.

### VERIFY

Learners can name at least two ways an object stays reachable.

---

# Step 3 — Ask AI for the Audit

### SAY

Here is the exact prompt used in this milestone. It was a single sentence, because the AI was already running inside the repository and could read every file.

### SHOW

The AI assistant (Claude Code in Grant Money).

### DO

Paste the prompt.

### AI PROMPT

```text
I need you to verify there are no garbage collection issues and if so fix them.
```

For a different tool, or a less context-aware assistant, use the structured version from the developer guide, Section 5. It asks the AI to confirm the engine version, inventory every pointer, classify findings, keep the diff minimal, and state what it could not verify.

### VERIFY

The AI reads the `.uproject` (version 5.8 here) and every file in `Game/Source/GrantMoney` before proposing anything.

---

# Step 4 — Inspect the AI's Findings

### SAY

The AI reported one finding. `ATileMapRenderer` declared four members as raw pointers: `SceneRoot`, `TilePlaneMesh`, `BoundaryCubeMesh` and `TileBaseMaterial`. Every other class already used `TObjectPtr`.

It also listed what it checked and found safe: the runtime tile and wall components, the dynamic materials, the textures, the camera's follow target and input actions, and the controller's mapping context. A search for timers, delegates, `AddToRoot` and weak pointers found nothing.

### SHOW

The AI's response, then `TileMapRenderer.cpp`, scrolled to `AddInstanceComponent` and `RegisterComponent`.

### DO

Pick one "safe" claim and check it yourself in the source.

### AI PROMPT

```text
For the tile group components created in BuildMap, show me the exact lines that make
them reachable by the garbage collector.
```

### VERIFY

The lines exist where the AI said they are.

---

# Engineering Discussion

### SAY

Here is an engineering discussion.

Were those raw pointers a bug? **No.** They were marked `UPROPERTY`, so the collector already saw them. The tempting move is to announce "fixed four GC bugs". That would be wrong, and it would teach the team to distrust reviews.

What `TObjectPtr` adds in Unreal 5 is a write barrier, which incremental garbage collection relies on, plus consistency with the rest of the module. That is a modernization. We fix it because it is cheap, and we describe it honestly.

---

# Step 5 — Apply the Minimal Fix

### SAY

The fix is four type changes in one header. The `.cpp` does not change, because `TObjectPtr` converts to a raw pointer automatically.

### SHOW

`git diff`.

### DO

```cpp
UPROPERTY()
TObjectPtr<USceneComponent> SceneRoot;

UPROPERTY()
TObjectPtr<UStaticMesh> TilePlaneMesh;

UPROPERTY()
TObjectPtr<UStaticMesh> BoundaryCubeMesh;

UPROPERTY()
TObjectPtr<UMaterialInterface> TileBaseMaterial;
```

### AI PROMPT

None; the AI applied this in step 3.

### VERIFY

`git diff` shows one file and four changed lines.

---

# Step 6 — Commit and Push

### SAY

We commit the code change on its own, so it can be reviewed and reverted independently.

### SHOW

Git Bash.

### DO

```bash
git add Game/Source/GrantMoney/Public/TileMapRenderer.h
git commit -m "GMS-XX: use TObjectPtr for TileMapRenderer UPROPERTY members"
git push origin GMS-XX-gc-audit
```

In Grant Money this was commit `977b4ab`. The AI could not open the pull request because the GitHub CLI was not installed, so we opened it from the GitHub compare page instead.

### VERIFY

The branch is visible on GitHub.

---

# Step 7 — Build (record live)

### SAY

A static review is not proof. Now we build.

### SHOW

Visual Studio or Rider, with the Unreal Editor closed.

### DO

Build `GrantMoneyEditor Win64 Development`.

### AI PROMPT

If the build fails:

```text
Here is the full compiler error. Explain which line causes it and the smallest fix.
Do not change anything outside TileMapRenderer.h unless the error requires it.
```

### VERIFY

The build succeeds. **Pending at time of writing.**

---

# Step 8 — Stress the Garbage Collector (record live)

### SAY

Collecting garbage every frame turns rare timing bugs into immediate ones.

### SHOW

The Unreal Editor playing `PlayGame1`, with the console and the Output Log visible.

### DO

1. Press **Play**.
2. Press the backtick key and run `gc.CollectGarbageEveryFrame 1`.
3. Move, turn and zoom for one minute.
4. Run `obj list Class=HierarchicalInstancedStaticMeshComponent`.
5. Run `gc.CollectGarbageEveryFrame 0`, then stop.

### AI PROMPT

If something crashes:

```text
Here is the call stack and the last 50 Output Log lines. Which UObject was used after
collection, which reference failed to keep it alive, and what is the smallest fix?
```

### VERIFY

No crash, tiles and walls remain visible, and the HISM components are listed. **Pending at time of writing.**

---

# Common Mistakes

### SAY

Here are some common mistakes.

**First, losing the project file.** In Grant Money, the `.uproject` is in the `Game` folder and named `GrantMoney`, not after the repository. This actually happened during this milestone.

**Second, assuming the AI can finish the workflow.** Our AI pushed the branch but could not open the PR, because the GitHub CLI was missing. Check your tools first.

**Third, overstating severity.** A raw `UPROPERTY` pointer is a style issue, not a crash.

**Fourth, building header changes with the editor open.** Close the editor first.

# Verification Checkpoint

### SAY

Now let's verify our result.

### SHOW

- The diff: one file, four lines.
- The compiler result.
- PIE with `gc.CollectGarbageEveryFrame 1`.
- The Output Log, filtered for `Error`.
- The `obj list` output.

Only claim the build and PIE checks if you actually ran them on camera.

# AI Review

### SAY

Let's review what AI actually did here.

- **It generated:** a full read of the module, a reference inventory, one finding, a four-line fix, a commit and a push.
- **Humans decided:** to commit on the existing branch, and to open the PR by hand when the CLI was missing.
- **What failed:** nothing in the code. The PR step failed because of tooling.
- **How it was verified:** by file inspection, search and diff review. Compiling and running were not done by the AI; they are the steps we just recorded.

# Engineering Pause

### SAY

Here is an engineering pause. Consider this question:

*If an audit finds no bugs, was it worth doing?*

Yes. "No defects found" is a result, as long as it comes with a list of what was checked. That list is the baseline for Module 2. When we add spawning and pooling, we will know which patterns are new and need checking.

# Completion Criteria

### SAY

This milestone is complete when:

- every UObject reference in the module has a verdict;
- the `TObjectPtr` change is committed and pushed;
- the editor build succeeds;
- the GC stress test runs without crashes or errors;
- the documentation is committed.

# Next Milestone

### SAY

Module 2 brings the world to life with a player character, movement and collision. The camera already has a `FollowTarget` hook waiting for it. Every new actor we spawn will go through the same checklist we used today.

---

## Suggested Screenshots

| Filename | Content |
|---|---|
| `gc-audit-01-uproject-version.png` | `GrantMoney.uproject` showing `"EngineAssociation": "5.8"` |
| `gc-audit-02-ai-prompt.png` | The audit prompt in the AI assistant |
| `gc-audit-03-ai-findings.png` | The AI's finding and its list of safe items |
| `gc-audit-04-header-before.png` | `TileMapRenderer.h` with raw pointers |
| `gc-audit-05-git-diff.png` | The four-line diff |
| `gc-audit-06-build-success.png` | IDE build output (once run) |
| `gc-audit-07-gc-every-frame.png` | PIE with `gc.CollectGarbageEveryFrame 1` in the console (once run) |
| `gc-audit-08-obj-list.png` | `obj list Class=HierarchicalInstancedStaticMeshComponent` output (once run) |
