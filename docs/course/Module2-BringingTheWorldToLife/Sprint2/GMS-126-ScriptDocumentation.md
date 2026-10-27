# GMS-126 – Default Skybox and Global Lighting for All Levels
## Development and Recording Guide

This document covers **one Jira issue only** and only the implementation session (the first prompt and the work it caused). Branching, committing and publishing are not documented here.

Status of this document: **Draft, needs review.** The code compiles. The in-Editor behavior was manually tested by Julio Maldonado.

---

## Goal

> I want to add a skybox and global lighting to all levels in the game.

The Jira acceptance criteria were not available in the session, so the goal above is the only requirement used.

---

# Starting Point (From Reading the Source, Not a Test Result)

| Fact | Source |
|---|---|
| All gameplay goes through `AGrantMoneyGameMode` (`BP_GrantMoneyGameMode` derives from it, per GMS-106). | `GrantMoneyGameMode.h` |
| No source file referenced `SkyAtmosphere`, `DirectionalLight`, `SkyLight` or fog. Any lighting lives in the level `.umap` files, which cannot be read as text. | Search of `Game/Source` |
| The module already depends on `Engine`, so no `Build.cs` change is needed. | `GrantMoney.Build.cs` |
| Engine version is 5.8. | `GrantMoney.uproject` |

Decision: editing every level by hand would touch binary `.umap` files, which cannot be diffed or merged. The lighting is therefore created in C++ at runtime by the GameMode, which every level already uses.

---

# Implementation Steps

## Step 1 – Inspect the GameMode

Read `GrantMoneyGameMode.h/.cpp` to find where level-wide setup could live. It had no `BeginPlay`, so one was added.

**Capture:** `01-gamemode-header.png`, the header before the change.

## Step 2 – Create `ASkyLightingActor`

New files `Public/SkyLightingActor.h` and `Private/SkyLightingActor.cpp`. One actor with:

| Component | Setting | Why |
|---|---|---|
| `USkyAtmosphereComponent` | defaults | The procedural skybox. |
| `UDirectionalLightComponent` (`Sun`) | Movable, rotation pitch -50 / yaw 40, intensity 5.0 (see Result), atmosphere sun light index 0 | The sun drives the atmosphere. Its rotation is the time of day. |
| `USkyLightComponent` | Movable, real-time capture on, intensity 1 | Ambient fill that follows the sky, with no baked lighting. |

The sun intensity started at 10.0 and was lowered to 5.0 after the Editor test (see Result).

**Capture:** `02-skylightingactor-cpp.png`, the constructor.

## Step 3 – Spawn it from the GameMode

In `AGrantMoneyGameMode`:

- New property `SkyLightingClass` (`TSubclassOf<ASkyLightingActor>`, default `ASkyLightingActor`; null disables it).
- New `BeginPlay` override that:
  1. returns if `SkyLightingClass` is null;
  2. returns if the level already contains an `ASkyLightingActor`, so a designer's own placement wins;
  3. logs a warning if the level already has an `ASkyAtmosphere`, `ADirectionalLight` or `ASkyLight`, because the default sky would stack with them;
  4. otherwise spawns the actor and logs success or an error.

**Capture:** `03-gamemode-beginplay.png`, the new `BeginPlay`.

## Step 4 – Compile

Built the editor target with Unreal Build Tool: `GrantMoneyEditor Win64 Development`. Result: **Succeeded**, nothing else was run.

**Capture:** `04-build-succeeded.png`, the terminal showing `Result: Succeeded`.

---

# Files Changed

| File | Change | Why |
|---|---|---|
| `Public/SkyLightingActor.h` | New | Declares the sky/sun/sky light actor. |
| `Private/SkyLightingActor.cpp` | New | Creates and configures the components. |
| `Public/GrantMoneyGameMode.h` | Edited | `BeginPlay` override and `SkyLightingClass` property. |
| `Private/GrantMoneyGameMode.cpp` | Edited | Spawns the actor once per level; includes for the duplicate-light check. |

All paths are under `Game/Source/GrantMoney/`.

---

# AI Workflow Log

**Prompt that started the task** (verbatim):

```text
I want to add a skybox and global lighting to all levels in the game
```

**What the AI did:**
1. Searched `Game/Source` for existing sky or light code and read the GameMode.
2. Chose a runtime-spawned actor over editing levels, for the binary-asset reason above.
3. Wrote the actor and GameMode changes, then compiled.

**Suggestions that failed or needed correction:**

| Problem | What happened | Fix |
|---|---|---|
| Wrong include | First draft of `GrantMoneyGameMode.cpp` included `Atmosphere/AtmosphericFog.h`, a legacy fog header unrelated to this feature. | Checked the 5.8 engine headers. `ASkyAtmosphere` is declared in `Components/SkyAtmosphereComponent.h`, so the include was replaced with that. |
| Build command failed | Running `Build.bat` from the Bash tool broke on the space in `C:\Program Files`. | Re-ran the same command from PowerShell with the path quoted. |

---

# Common Mistakes

- Expecting to see the sky in the Editor viewport. It is spawned at runtime, so it only appears in Play.
- Adding a second sun or sky atmosphere to a level and forgetting the default one still spawns. Watch for the warning in the Output Log, or place an `ASkyLightingActor` in the level to replace the default.
- Playing a level that does not use `AGrantMoneyGameMode` (or a Blueprint derived from it). It gets no sky.

---

# Verification Procedure

**Compile:** the editor target compiled successfully.

**Manual Editor test:** performed manually in the Unreal Editor by Julio Maldonado, using these steps:

1. Open each level and press Play.
2. Confirm a sky is visible and the scene is lit.
3. Open the Output Log and filter for `GrantMoneyGameMode`. Expect `spawned default sky lighting`.
4. Place an `ASkyLightingActor` in one level and play it. Expect no spawn message from the GameMode in that level.
5. Adjust sun and sky light intensity if the result is too dark or too bright.

**Result:**

- Sky visible in all levels.
- The sun intensity of 10.0 was too bright, so it was set to 5.0. The final sun intensity is 5.0.
- The sky light intensity stayed at 1.0.

---

# Known Limitations

- Intensity (sun 5.0, sky light 1.0) and sun angle are fixed defaults in code. They can be tuned in a placed `ASkyLightingActor`.
- Applies only to levels that use `AGrantMoneyGameMode`.
- A level that already has its own sky or lights gets the default sky added on top.
- This is a procedural sky. A fixed image skybox (cubemap or sky sphere) would need an asset that is not in the project.

---

# Video Voice-Over Script

> Every level in Grant Money needs a sky and consistent lighting. We could open each level and add one by hand, but those level files are binary, so we can't review or merge those edits. Instead we do it in C++.
>
> We create one actor, `SkyLightingActor`. It holds a sky atmosphere for the skybox, a directional light as the sun, and a sky light that captures the sky for ambient light. The sun's rotation is the time of day.
>
> Then in the GameMode, which every level already uses, we add a `BeginPlay`. It checks whether the level already has its own sky lighting actor. If it does, we leave it alone. If not, we spawn the default one.
>
> We compile, and the build succeeds. Then we press Play in each level and check the Output Log for the spawn message.
