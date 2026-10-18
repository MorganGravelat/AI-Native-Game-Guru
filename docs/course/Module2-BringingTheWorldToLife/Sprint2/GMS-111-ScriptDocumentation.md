# GMS-111 – Module 1 Camera Follows and Supports the Real Player Character
## Development and Recording Guide

This document covers **one Jira issue only**. Each Jira issue from now on gets its own guide in this style, instead of one large sprint document.

Status of this document: **implemented, verified except restart and in-game map load**. The code changes are made, and the developer has tested the follow, smoothness, camera controls, collision and pause/resume (see "What Actually Happened" and "Final Review" near the end). In-game map loading cannot be tested yet, because the project has no system to load a map during play. Restart was not reported. Sections marked `_you fill in_` are left blank on purpose and must only be filled in with what actually happened.

---

## Jira Issue

> **As a player**, the existing perspective camera follows the real player character so the Module 1 camera foundation becomes the gameplay camera rather than a free-exploration tool.

Preserve useful Module 1 camera architecture instead of replacing it wholesale.

### Done When

- The camera can follow the spawned player character.
- Existing spring-arm/camera behavior is reused where practical.
- Camera movement remains smooth during normal walking/running.
- The camera does not detach or lose the player after pause/resume or map load.
- Future camera extensions can be made without coupling them directly to TileGen-generated filenames/data.

### Course Capture Requirement (from the Jira issue)

- Record short raw clips while building the feature, not only a finished demo.
- Save at least one screenshot for each meaningful step and use numbered filenames.
- Save useful AI prompts, corrections, and at least one verification result.
- Upload raw media to the shared media folder, not the Git repository.
- Documentation must describe what actually happened. Do not claim a test passed unless it was performed.

---

# Starting Point (What Already Exists)

This is from reading the source before GMS-111. It is not a test result.

```text
BP_GrantMoneyGameMode (derives from AGrantMoneyGameMode)
   ├─ spawns AGrantMoneyPlayerCharacter at the PlayerStart
   │       └─ possessed by MapCameraPlayerController
   └─ spawns AMapCameraPawn (NOT possessed) at the PlayerStart
           └─ is the controller's view target; static position
```

| Piece | Current state |
|---|---|
| `AMapCameraPawn` | Scene root, then spring arm, then camera. Yaw is the actor's own rotation, pitch is the arm's relative rotation, zoom is the arm length. |
| Follow hook | `SetFollowTarget(AActor*)`, `FollowTarget` and `FollowOffset` already exist. While a target is set, `Tick` does `SetActorLocation(Target + FollowOffset)`. Nothing calls `SetFollowTarget` yet. |
| Smoothing | The spring arm's camera lag and rotation lag are available as properties, and are off by default. |
| Camera input | Mouse turn, pitch and zoom are bound on `MapCameraPlayerController` and forwarded to the view-target camera pawn (GMS-103). |
| Movement | The character moves relative to the active camera's yaw through `PlayerCameraManager` (GMS-103). It does not reference a camera class. |
| Map bounds | `ApplyMapBounds` clamps only a free-roaming camera. While following, bounds are not applied. |
| Pause | Lives on the controller. The camera pawn has no pause logic of its own. |

Questions the investigation should answer before any edit are in Step 2.

---

# Development and Documentation Workflow

```text
Record baseline
        ↓
Inspect existing architecture (read-only)
        ↓
Plan the smallest change
        ↓
Implement one meaningful step
        ↓
Compile / test
        ↓
Capture screenshot or short video
        ↓
Tell the AI what actually happened
        ↓
Update documentation
        ↓
Review diff
        ↓
Commit / PR
```

Do not wait until the end to recreate the process from memory.

---

# Media Capture Rules

- Short clips, about **10–30 seconds**. Voice-over can be added later.
- Record while building the feature, not only the finished result.
- Use numbered filenames so the editor can match footage to this document.
- Raw video goes in the shared media folder, **not in Git**.
- Failures are useful footage. Do not stage or delete them.

### Recommended Media Folder Structure

```text
Module2/
└── Sprint2/
    └── GMS-111/
        ├── 00-baseline-static-camera.mp4
        ├── 00-baseline-static-camera.png
        ├── 01-existing-camera-architecture.png
        ├── 02-ai-inspection-findings.png
        ├── 03-follow-code.png
        ├── 04-compile-success.png
        ├── 05-camera-follows-player.mp4
        ├── 06-smoothness-comparison.mp4
        ├── 07-pause-resume-follow.mp4
        ├── 08-restart-and-map-load.mp4
        ├── 09-camera-controls-while-following.mp4
        ├── 10-decoupling-check.png
        └── 11-final-verification.png
```

Use these names, or update this list if you rename them.

---

# Step 1 – Create the Development Branch

Start from the latest `main`, after GMS-102, GMS-103 and GMS-104 are merged.

```bash
git checkout main
git pull
git checkout -b GMS-111-camera-follows-player
git status
```

One Jira issue, one branch, one pull request.

---

## Capture

### Clip

```text
00-baseline-static-camera.mp4
```

Record about 15–20 seconds of the **current** behaviour, before any GMS-111 change:

1. press Play;
2. walk the character away from the starting view with WASD;
3. let the character leave the screen;
4. turn the camera with the mouse to show it is still a free camera that does not follow.

### Screenshot

```text
00-baseline-static-camera.png
```

The character visibly out of view or at the edge of the screen, with the camera left behind.

---

## Future Video Editor Use

This is the "before" shot for the whole issue.

Suggested narration:

> "Right now the camera and the player are two separate things. We can walk the character around, but the camera stays where it spawned, and the character walks off the screen. Our goal is to turn the Module 1 camera into the gameplay camera."

---

# Step 2 – Ask AI to Inspect the Existing Camera Architecture

Do not edit anything yet.

---

## AI Prompt

```text
We are starting GMS-111: the Module 1 camera follows and supports the real player character.

Follow CLAUDE.md.

Inspect only the relevant camera, player character, PlayerController and GameMode source.

Do not edit yet.

Determine:

- what currently owns the Module 1 camera and how it is positioned;
- whether a follow mechanism already exists, and what it does and does not do;
- where the follow target should be set so it happens once, reliably, at spawn;
- whether the camera pawn and the character tick in an order that could cause jitter;
- how to keep movement smooth while walking and running, reusing the spring arm where practical;
- what should happen to the camera bounds while following;
- what happens to the follow target after pause/resume, a level restart, and if the character is destroyed;
- how a future camera extension can be added without coupling it to TileGen-generated filenames or data.

Do not change TileGen.
Do not change MapArrays.
Do not replace the Module 1 camera.
Do not add animation or combat.

Return a proposed implementation and the minimum files to change before modifying anything.
```

---

## Capture

```text
01-existing-camera-architecture.png
02-ai-inspection-findings.png
```

- `01`: the `AMapCameraPawn` component hierarchy (spring arm and camera), or the follow-related lines in `MapCameraPawn.h` (`SetFollowTarget`, `FollowTarget`, `FollowOffset`).
- `02`: the AI's reply to the inspection prompt, showing what it found and what it proposed.

---

## Future Video Editor Use

Suggested narration:

> "Before changing anything, we ask the AI what already exists. The Module 1 camera already has a follow hook, so we don't need to rewrite it. We need to use it."

---

# Step 3 – Review the Proposal Before Accepting It

Check the AI's proposal against these questions. Write down your own answers in the log below, including where you disagree with the AI.

| Decision | Options to weigh | Your decision |
|---|---|---|
| Where is the follow target set? | The GameMode, where the camera pawn is already spawned and the view target is already set, or a new component | _you fill in_ |
| How is smoothness achieved? | The existing spring-arm camera lag, or interpolating the pivot position, or both | _you fill in_ |
| Tick order | The camera pawn tracks the character each frame, so it can run before or after the character moves. A camera that runs first lags one frame behind. Check whether this is visible and, if so, make the camera tick after the character. | _you fill in_ |
| Pivot height | `FollowOffset` is applied to the character's location, which is its capsule centre. Check that the camera looks at a sensible point. | _you fill in_ |
| Bounds while following | Currently ignored. Decide whether to keep ignoring them, clamp the camera, or leave this for a later camera ticket. | _you fill in_ |
| Target lost | If the character is destroyed, the follow target becomes invalid. Decide what the camera does (stay put, or re-acquire the new pawn). | _you fill in_ |

Reject changes that:

- replace `AMapCameraPawn` wholesale;
- attach the camera to the character as a fresh camera, instead of reusing the Module 1 arm;
- reference `TileMapRenderer`, `MapArrays` or tile asset names from any new camera or follow code;
- hardcode a position, a level number or a map size;
- move pause or input ownership.

Record at least one place where you changed or rejected an AI suggestion. That is course material.

---

# Step 4 – Implement the Smallest Change

Keep the change small. The existing code already supports most of this.

Implementation notes to preserve in the log:

- Which file sets the follow target, and why that file.
- Any property added for smoothing, and its default.
- Whether tick ordering was changed, and the observed reason.
- What was deliberately left out (bounds, retargeting, new camera types).

Do not claim the change works until Step 6 is done.

---

## Capture

```text
03-follow-code.png
```

The lines that set the follow target, and any new smoothing or tick-order lines. Use the diff view so added lines are highlighted.

---

## Future Video Editor Use

Suggested narration:

> "The Module 1 camera already knows how to follow an actor. The real work is telling it which actor, at the right moment, and making the motion smooth."

---

# Step 5 – Compile

Close the Unreal Editor first. Live Coding does not reliably pick up constructor and new-class changes.

1. Build `GrantMoneyEditor` (Development Editor, Win64) in Visual Studio.
2. Fix any errors and send them to the AI, with the exact message.
3. Reopen `PlayGame1`.

Record what actually happened. Do not write "compiled successfully" unless it did.

---

## Capture

```text
04-compile-success.png
```

The successful build output, or a failure screenshot if one happened first. Keep failures. They are useful.

---

# Step 6 – Verify the Camera Follows the Player

Press Play.

Verify:

```text
Character spawns
→ camera is centred on the character at start
→ WASD moves the character and the camera follows it
→ the character stays in view while walking and running
```

Also try:

- walking in a straight line for a long distance;
- walking in circles;
- changing direction quickly;
- turning the camera with the mouse while walking, and confirming movement is still relative to the view.

---

## Capture

```text
05-camera-follows-player.mp4
```

About 20–30 seconds: spawn, walk, turn the camera, keep walking. The character should stay on screen the whole time. This is the main result clip for GMS-111.

---

## Future Video Editor Use

Suggested narration:

> "The camera is the same Module 1 camera, with the same spring arm, pitch and zoom. The difference is that it now follows the real player character, so it behaves like a gameplay camera."

---

# Step 7 – Verify Smoothness

"Smooth during normal walking and running" is a feel check. Describe what you saw in plain words, and do not claim more than you observed.

Check:

- no visible stutter or shaking of the character on screen while walking;
- no visible one-frame lag where the character jitters against the background;
- changing direction does not snap or pop the camera;
- running in circles keeps the character steady in the frame.

If the character shakes, it is usually tick order. Note it, fix it, and record the "before" and "after" as separate clips.

---

## Capture

```text
06-smoothness-comparison.mp4
```

If you had a problem and fixed it, show the jittery version and the fixed version back to back. If it was smooth from the start, record a short walking and running clip and say so.

---

## Future Video Editor Use

Failed behaviour, such as jitter, is good instructional material. Use it before showing the fix.

---

# Step 8 – Verify Pause and Resume

Pause lives on the controller. The camera pawn has no pause code, so this should be fine, but it must be tested.

Test:

1. hold W so the character is moving;
2. pause. The game and the camera should both stop;
3. wait a few seconds;
4. resume. The camera should still be on the character, with no jump.

Also pause while standing still, turn nothing, and resume.

---

## Capture

```text
07-pause-resume-follow.mp4
```

Walk, pause, resume, keep walking. The camera must stay on the character.

---

# Step 9 – Verify Restart and Map Load

The camera must not lose the player after a restart or a map load.

Test:

1. stop and start Play (PIE) several times;
2. if more than one registered level can be loaded, load each one and check the camera;
3. open a level, then reload the same level.

Note which levels you actually tested. Do not write "map load works" if you only tested one level.

Also note what happens if the character is destroyed (for example, it falls out of the world with ground collision off, then the follow target is invalid). This is an edge case, not a pass or fail item. Record what you see.

---

## Capture

```text
08-restart-and-map-load.mp4
```

Two or three starts, and any level load, each showing the camera on the character.

---

# Step 10 – Verify the Existing Camera Controls Still Work

While following, confirm the Module 1 camera features still work:

- mouse turn (yaw);
- look up and down (pitch limits);
- mouse wheel zoom (min and max arm length);
- WASD still moves the character relative to the camera.

The camera's own WASD free-roaming is not part of gameplay any more, and that is expected.

---

## Capture

```text
09-camera-controls-while-following.mp4
```

Turn, pitch, zoom in and out, and walk, with the character staying on screen.

---

# Step 11 – Verify the Camera Is Not Coupled to TileGen

The camera and follow code must not depend on generated map data or file names.

Check the diff for any new code in the camera pawn, GameMode or character:

- no `#include "MapArrays.h"` or `#include "TileMapRenderer.h"` added;
- no tile asset paths or generated filenames;
- no level numbers;
- no hardcoded map coordinates.

Note that `AMapCameraPawn::ApplyMapBounds` already uses `TileMapRenderer` and `MapArrays` from Module 1. GMS-111 must not add to that, and must not move or rewrite it.

A quick way to check is to search the diff for `MapArrays`, `TileMapRenderer`, `Tile` and `Level`.

---

## Capture

```text
10-decoupling-check.png
```

The diff search result, or the diff view, showing no new TileGen references.

---

## Future Video Editor Use

Suggested narration:

> "Future camera ideas, such as a different follow style or a lock-on camera, should plug into the camera pawn. They should not need to know which tiles the map is made of."

---

# Step 12 – Final GMS-111 Verification

Verify each acceptance criterion individually. Mark only what you actually did.

```text
[ ] The camera follows the spawned player character.
[ ] The existing spring arm, pitch, yaw and zoom are reused.
[ ] Walking stays smooth, with no visible jitter or pop.
[ ] Running stays smooth, with no visible jitter or pop.
[ ] The camera stays on the player after pause and resume.
[ ] The camera stays on the player after restarting Play.
[ ] The camera stays on the player after a map load (note which levels).
[ ] Mouse turn, pitch and zoom still work while following.
[ ] WASD movement is still relative to the camera.
[ ] The pause menu still works.
[ ] TileMapRenderer still generates the map.
[ ] No new TileGen or MapArrays references in the camera code.
[ ] Project compiles.
```

---

## Capture

```text
11-final-verification.png
```

The Output Log with no new errors, and the Editor showing the character and camera. Add the filled-in checklist to the final report.

---

# Step 13 – Ask AI to Review the Final Diff

---

## AI Prompt

```text
GMS-111 verification results:

[PASTE THE ACTUAL PASS/FAIL RESULTS HERE]

Review the diff for GMS-111 only.

Check specifically for:

- unnecessary changes;
- unrelated refactors;
- the Module 1 camera being replaced instead of reused;
- duplicated camera or follow logic;
- Blueprint gameplay logic;
- coupling to TileGen, MapArrays, tile asset names or level numbers;
- changes that belong to other issues;
- any documentation claim that was not actually verified.

Then generate the final GMS-111 milestone report containing:

- implementation summary;
- files changed and why;
- step-by-step reproduction procedure;
- AI prompts used;
- AI mistakes or corrections;
- actual verification results;
- screenshot and recording references;
- known limitations;
- suggested commit message;
- short instructional voice-over outline.

Do not commit or push.
```

---

# Known Risks and Open Questions

These came from reading the source. Confirm each during testing and record what you find.

| Risk | Why it matters |
|---|---|
| Tick order | The camera pawn follows the character's position each frame. If the camera ticks before the character moves, the view is one frame behind, which can show as shaking. |
| Pivot height | The follow offset is added to the character's capsule centre. The look-at point may feel too high or too low. |
| Camera bounds | The map bounds are not applied while following, so the camera can move past the map edge. |
| Target lost | If the character is destroyed, the camera keeps its last position with no target. |
| Spring-arm lag | Camera lag smooths motion, but too much feels floaty, and rotation lag can make the camera feel slow when turning. |
| Camera collision | The spring arm has collision testing off, so the camera can pass through the walls. This is the existing Module 1 behaviour and is not changed by this issue. |
| Multiple levels | Only levels that can actually be loaded can be tested. Say which. |

---

# What Actually Happened (Fill In During Development)

Do not fill this in from memory afterwards. Add to it as each step is done.

## Deviations From the Plan

| Plan | What actually happened | Why |
|---|---|---|
| Step 3 listed several smoothing options (spring-arm lag, interpolating the pivot, or both) | Only the existing spring-arm camera lag was enabled. No new smoothing code. | Reusing the Module 1 arm was the smallest change and it was enough. |
| Step 3 asked whether tick ordering needed changing | The camera pawn now ticks in the post-physics group, so it runs after the character moves | A camera that ticks first can read the character's position a frame late and shake. This was a precaution from reading the code. The shaking was not observed beforehand, so it is not known whether it was needed. |
| Bounds while following | Left alone: bounds are not applied while following | The walls already keep the character in the playable area. |
| Target lost | Left alone | There is no death or respawn system yet. |
| Step 9, map load | Not tested yet | The developer is unsure whether the project uses seamless travel. |

## Files Changed

| File | Change |
|---|---|
| `Source/GrantMoney/Private/GrantMoneyGameMode.cpp` | After the camera pawn is spawned and set as the view target, calls `CameraPawn->SetFollowTarget(PlayerPawn)` and logs `camera ... now follows ...`. |
| `Source/GrantMoney/Private/MapCameraPawn.cpp` | Sets the actor tick group to post-physics, so the camera follows after the character has moved. |
| `Source/GrantMoney/Public/MapCameraPawn.h` | `bEnableCameraLag` now defaults to `true` (lag speed unchanged at 10). Comment updated. |
| `Source/GrantMoney/Public/GrantMoneyGameMode.h` | Class comment updated. |

Not touched: `TileMapRenderer`, `MapArrays`, `MapBoundaryGenerator`, `MapCameraPlayerController`, `GrantMoneyPlayerCharacter`, any ini file.

## Verification Results

Only results the developer reported are marked as confirmed.

| Criterion | Result | Evidence |
|---|---|---|
| Project compiles | Not stated explicitly. The new behaviour was observed, which needs the new build. | Developer ran and tested the change |
| Camera follows the character | Confirmed by developer | "Pretty smooth", camera tracks the player |
| Smooth walking | Confirmed by developer | "No jittery nature or problems" moving in all directions |
| Smooth running | Not tested separately. Moving in all directions was tested. | |
| No start-of-play glide | Confirmed by developer | "No swoop either at the start" |
| Camera controls while following | Confirmed by developer | Zooming in and out, and turning around, worked while moving |
| Character collision still works | Confirmed by developer | "The character still has collision" |
| Pause and resume | Confirmed by developer | "The pause and play was fine" |
| Restart | _not reported_ | Stop and start Play a few times and check the follow log line each time |
| Map load / level change | Not testable in-game | Different maps can load, but the project has no system to load one during play, so this cannot be tested yet. Seamless travel is also unknown. |
| No TileGen coupling | Confirmed by AI inspection of the diff | The diff adds no `MapArrays`, `TileMapRenderer`, tile asset name, level number or coordinate. Re-run the Step 11 search yourself if you want your own evidence. |

## Observations (Not Pass or Fail)

- **The camera clips through walls when turned around.** The spring arm's collision test is off, as it was in Module 1. Known, and planned for a later change.
- **With ground collision off, the camera falls with the player.** The follow target stays valid until the character is destroyed, so the camera tracks it down. This differs from the earlier assumption that the camera would stay put, but that assumption was only about the moment after the character is destroyed, which was not observed.

## AI Workflow Log

| # | Prompt | Result |
|---|---|---|
| 1 | Inspection prompt (Step 2): follow mechanism, where to set the target, tick order, smoothness, bounds, pause/restart/destroyed target, and decoupling from TileGen. | Findings and a minimal proposal. The follow hook already existed. |
| 2 | "I am unsure about seamless travel and level loading... please implement the changes... let me know what I can do along the way to test." | Implemented the three-file change and a step-by-step test list. |
| 3 | Developer reported the test results (smooth, no swoop, collision fine, camera clips through walls, camera falls with the player when collision is off). | Results recorded here. |

## AI Mistakes, Corrections and Surprises

- **The follow code already existed.** The AI found that `SetFollowTarget` was in the camera pawn from Module 1, so the change was a few lines and not a rewrite. That is the "reuse, don't replace" lesson.
- **Tick order was a precaution.** The AI proposed the post-physics tick group from reading the code, not from seeing jitter. No jitter was seen after the change. It is not known whether the change was needed.
- **Camera falling with the player.** The AI predicted the camera would stay put if the character was destroyed. With ground collision off, the camera instead followed the player down, because the player had not been destroyed yet.
- **Honesty about unknowns.** The AI said it did not know whether seamless travel would work, and did not claim it did.

## Known Limitations

- The camera passes through walls when turned, because the arm's collision test is off.
- The map-load and seamless-travel behaviour is not verified. The project has no in-game way to load a map yet, so it cannot be tested until one exists. The design should be fine for a normal level load (a new GameMode would spawn and follow again), but that is reasoning, not a test.
- Restart (stopping and starting Play) was not reported as tested.
- If the character is destroyed (for example, it falls out of the world), the camera stays at its last position and does not find a new pawn.
- Bounds are not applied while following, so the camera can move past the map edge.
- The pivot sits about 138 units above the ground (the capsule centre plus the 50 follow offset), where Module 1's free-roam pivot was 100.

## Suggested Commit Message

```text
GMS-111: Make the Module 1 camera follow the player character

- GrantMoneyGameMode sets the camera pawn's follow target to the spawned
  player character at spawn
- MapCameraPawn ticks after physics so it follows after the character moves
- Enable the spring arm's existing camera lag by default for smooth trailing
- No changes to TileGen, MapArrays, input, or the player character

The camera still passes through walls when turned (arm collision test is off).

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>
```

Branch: `GMS-111-camera-follows-player`. Do not commit or push until the diff has been reviewed.

---

# Final Review (End-of-Task Check)

This is the result of running the End-of-Task AI Prompt Template against the real diff. It reviews only GMS-111, and was done on branch `GMS-111-camera-follows-player` after GMS-102, GMS-103 and GMS-104 were merged. Nothing here has been committed or pushed.

## Diff Summary

```text
Game/Source/GrantMoney/Private/GrantMoneyGameMode.cpp   +5
Game/Source/GrantMoney/Private/MapCameraPawn.cpp        +3
Game/Source/GrantMoney/Public/GrantMoneyGameMode.h      comment only
Game/Source/GrantMoney/Public/MapCameraPawn.h           one default changed, two comments
4 files changed, 14 insertions(+), 5 deletions(-)
```

## Review Checklist

| Check | Result |
|---|---|
| Unrelated changes | None in the source. See the warning below about `PlayGame1.umap`. |
| Unnecessary refactoring | None. No code was moved or renamed. |
| Duplicated systems | None. The existing `SetFollowTarget` is reused, not a second follow system. |
| Hardcoded map-specific logic | None. No coordinates, level numbers or tile names were added. |
| Changes that belong to another Jira issue | None. No input, collision, bounds or animation changes. |
| Documentation claims not actually verified | Checked. The doc marks restart as not reported and map load as not testable, and records the developer's results as the developer's. |

## Warning: `PlayGame1.umap` Is Modified

`git status` shows `Game/Content/Maps/PlayGame1.umap` as modified. It is not part of GMS-111. No source change touches the map, so this is most likely the Editor re-saving it when you opened or played it. Its contents were not read, because it is a binary asset.

Before committing:

1. Do not stage it with the GMS-111 files, unless you meant to change the map.
2. If you did not mean to, close the Editor first (it can re-save the file), then discard the change with `git restore Game/Content/Maps/PlayGame1.umap`.
3. Check `git status` again so only the four source files (and the new `Sprint2` docs folder) remain.

## Implementation Summary

The Module 1 camera already had a follow hook that nothing called. GMS-111 calls it, once, when the player spawns, and makes the camera follow after the character moves. The spring arm's existing lag smooths the motion. Pitch, yaw and zoom still come from the Module 1 camera, and the controller still forwards the mouse input.

## Reproducible Steps (what actually happened)

1. Created the branch `GMS-111-camera-follows-player` from `main`.
2. Asked the AI to inspect the camera, GameMode, controller and character (read-only).
3. Reviewed the proposal and asked for the implementation.
4. Called `SetFollowTarget(PlayerPawn)` in `HandleStartingNewPlayer_Implementation`.
5. Set the camera pawn's tick group to post-physics.
6. Turned on `bEnableCameraLag` by default.
7. Built and tested in the Editor.
8. Reported results; the doc was updated.

## Screenshot and Video References

Whether each file below has been captured is for you to confirm. This table only lists what the plan calls for.

| File | Status |
|---|---|
| `00-baseline-static-camera.mp4` / `.png` | _you confirm_ |
| `01-existing-camera-architecture.png` | _you confirm_ |
| `02-ai-inspection-findings.png` | _you confirm_ |
| `03-follow-code.png` (GameMode lines 49–57), plus optional `03b-tick-group.png` and `03c-camera-lag-default.png` | _you confirm_ |
| `04-compile-success.png` | _you confirm_ |
| `05-camera-follows-player.mp4` | _you confirm_ |
| `06-smoothness-comparison.mp4` | _you confirm_ |
| `07-pause-resume-follow.mp4` | _you confirm_ |
| `08-restart-and-map-load.mp4` | Map load cannot be recorded yet. Record restart only, or skip. |
| `09-camera-controls-while-following.mp4` | _you confirm_ |
| `10-decoupling-check.png` | _you confirm_ |
| `11-final-verification.png` | _you confirm_ |

## Voice-Over Outline

> **The problem (over `00-baseline-static-camera.mp4`).** "We can walk the character, but the camera stays behind and the character walks off the screen."
>
> **Reuse, don't replace (over `01` and `02`).** "We ask the AI to inspect the project first. The Module 1 camera already knows how to follow an actor. Nothing was using it."
>
> **The change (over `03-follow-code.png`).** "One call, when the player spawns: tell the camera to follow the character. The camera keeps its arm, pitch, yaw and zoom."
>
> **Smoothness (over `06-smoothness-comparison.mp4`).** "A camera that follows isn't the same as a camera that feels good. We make it follow after the character has moved, and use the spring arm's existing lag."
>
> **The result (over `05` and `09`).** "The camera follows the player, and the mouse look and zoom still work."
>
> **Robustness (over `07`).** "Pause and resume doesn't lose the player. We can't test loading a different map during play yet, because the project has no way to do that, and we say so."
>
> **Closing.** "The Module 1 camera is now the gameplay camera, and it didn't need to be rewritten."

---

# Master Capture Checklist

Check each one off when the file exists in the shared media folder. Nothing here has been captured yet.

| File | What to show | Notes |
|---|---|---|
| `00-baseline-static-camera.mp4` / `.png` | The camera staying behind while the character walks off screen | Record **before** any GMS-111 change |
| `01-existing-camera-architecture.png` | `AMapCameraPawn` components, or the follow lines in `MapCameraPawn.h` | |
| `02-ai-inspection-findings.png` | The AI's reply to the inspection prompt | |
| `03-follow-code.png` | The lines that set the follow target, plus any smoothing or tick-order lines | Diff view |
| `04-compile-success.png` | Successful build output | Keep failures too |
| `05-camera-follows-player.mp4` | Spawn, walk, turn the camera, keep walking | Main result clip |
| `06-smoothness-comparison.mp4` | Jitter before and after, if there was any | If smooth from the start, a short walking and running clip |
| `07-pause-resume-follow.mp4` | Walk, pause, resume, keep walking | |
| `08-restart-and-map-load.mp4` | Several starts and any level load | Say which levels |
| `09-camera-controls-while-following.mp4` | Turn, pitch, zoom, walk | |
| `10-decoupling-check.png` | The diff showing no new TileGen references | |
| `11-final-verification.png` | Output Log with no errors, plus the filled-in checklist | |

### Suggested Recording Order

1. Record `00-baseline-static-camera` first. It needs the build from before the change.
2. Take `01` and `02` during the investigation.
3. Take `03` and `04` after the implementation and build.
4. Record `05`, `06`, `07`, `08` and `09` from the new build.
5. Take `10` and `11` last.

---

# Suggested Video Structure

## Section 1 – The Problem

Use:

```text
00-baseline-static-camera.mp4
```

Explain that the camera and the player are separate, and the player walks off the screen.

## Section 2 – Reuse, Don't Replace

Use:

```text
01-existing-camera-architecture.png
02-ai-inspection-findings.png
```

Explain that the Module 1 camera already has a follow hook, and the AI inspection found it.

## Section 3 – The Change

Use:

```text
03-follow-code.png
```

Explain how little code was needed, and what the follow target is set to.

## Section 4 – Smoothness

Use:

```text
06-smoothness-comparison.mp4
```

Explain the difference between a camera that follows and a camera that feels good.

## Section 5 – The Result

Use:

```text
05-camera-follows-player.mp4
09-camera-controls-while-following.mp4
```

Show the camera following, with the Module 1 controls (turn, pitch, zoom) still working.

## Section 6 – Robustness

Use:

```text
07-pause-resume-follow.mp4
08-restart-and-map-load.mp4
10-decoupling-check.png
```

Explain pause, restart and map load, and why the camera code does not depend on the map data.

Suggested closing narration:

> "The Module 1 camera is no longer a free-exploration tool. It's the gameplay camera, and it follows the real player character, using the same spring arm and controls we built before."

---

# AI Workflow Material to Preserve

Do not save only the successful prompts. Also keep examples where:

- the AI proposed replacing the camera instead of reusing it;
- the AI coupled the follow code to map data;
- the camera jittered and the cause was not the first one guessed;
- a result looked like a bug but was not;
- you changed the AI's proposed design.

A useful lesson is:

```text
We inspected the project
→ AI proposed a solution
→ we tested it
→ something was wrong
→ we identified why
→ corrected the prompt or implementation
→ verified the final result
```

---

# End-of-Task AI Prompt Template

```text
GMS-111 is now at its final verification stage.

Actual results:

[PASTE THE REAL TEST RESULTS]

Review only the changes associated with GMS-111.

Check for:

- unrelated changes;
- unnecessary refactoring;
- duplicated systems;
- hardcoded map-specific logic;
- changes that belong to another Jira issue;
- documentation claims that were not actually verified.

Then create:

1. implementation summary;
2. files changed and why;
3. reproducible development steps;
4. AI prompts used;
5. AI mistakes/corrections;
6. actual verification results;
7. screenshot and video references;
8. known limitations;
9. suggested Git commit message;
10. instructional voice-over outline.

Do not invent missing steps.
Do not claim tests passed unless I explicitly confirmed them.
Do not commit or push.
```

---

# GMS-111 Definition of Done

```text
[ ] The camera follows the spawned player character.
[ ] The Module 1 spring arm and camera are reused.
[ ] Walking and running are smooth.
[ ] The camera stays on the player after pause/resume.
[ ] The camera stays on the player after restart and map load.
[ ] Camera turn, pitch and zoom still work.
[ ] No new TileGen, MapArrays or level-specific coupling.
[ ] Project compiles.
[ ] Numbered screenshots exist for each step.
[ ] Numbered raw clips exist in the shared media folder (not in Git).
[ ] AI prompts and corrections were preserved.
[ ] At least one verification result is recorded.
[ ] Documentation reflects what actually happened.
```
