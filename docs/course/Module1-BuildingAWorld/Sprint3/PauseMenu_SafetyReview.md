# Pause Menu — Phase 4 Safety Review
## C++-Driven Pause Menu (Escape toggle, Resume, Quit)

> **Status:** Code is written and **both** the Game target (`GrantMoney Win64 Development`) and the **Editor** target (`GrantMoneyEditor Win64 Development`) compiled successfully. Asset settings were checked headlessly with `UnrealEditor-Cmd` + Python (see *Verification Status*). **Nothing has been run in PIE or in a packaged build.** Everything under *Manual Test Checklist* is still untested.

## Summary

Escape toggles a pause menu. The logic is in C++; `WBP_PauseMenu` is layout only.

- Opening the menu: creates the widget once, adds it to the viewport, pauses, shows the cursor, switches to `FInputModeGameAndUI` and focuses the widget.
- Closing the menu: unpauses, removes the widget, hides the cursor, restores `FInputModeGameOnly`.
- `ResumeButton` runs the same close/unpause steps from inside the widget. `QuitButton` calls `UKismetSystemLibrary::QuitGame`.

## Phase 1 Inspection Findings

| Question | Finding |
|----------|---------|
| Input architecture | `AMapCameraPlayerController` adds `IMC_MapCamera` in `SetupInputComponent()`. `AMapCameraPawn` binds Move/Turn/LookUp/Zoom in `SetupPlayerInputComponent()`. `DefaultInput.ini` already uses `EnhancedPlayerInput` / `EnhancedInputComponent`. |
| GameMode | `BP_GrantMoneyGameMode` selects `MapCameraPawn` and `MapCameraPlayerController`. `AGrantMoneyGameMode` is an empty stub. No conflict. |
| Existing pause code | None in the controller, Pawn, GameMode or `.ini` files. `PauseMenuWidget`, `IA_Pause` and `WBP_PauseMenu` had already been created by hand. |
| Escape mapping | `IMC_MapCamera` contains an `Escape` → `IA_Pause` mapping (verified headlessly). |
| `WBP_PauseMenu` | Parent class `PauseMenuWidget` (verified headlessly); contains `ResumeButton` and `QuitButton` (names found in the saved asset). |

## Files Changed

| File | Change | Why |
|------|--------|-----|
| `Game/Source/GrantMoney/GrantMoney.Build.cs` | Added `UMG`, `Slate`, `SlateCore` to `PublicDependencyModuleNames`. | `UUserWidget` and `UButton` live in UMG. The `.uproject` entry only enables the module; it does not link it. |
| `Game/Source/GrantMoney/Public/PauseMenuWidget.h` | `ResumeButton` / `QuitButton` changed from raw `UButton*` to `TObjectPtr<UButton>`; added a comment that the names must match. | Matches project style; raw pointers are not GC-safe when uninitialised. |
| `Game/Source/GrantMoney/Private/PauseMenuWidget.cpp` | Removed a stray `f` on line 9 (compile error). `AddDynamic` changed to `AddUniqueDynamic`. | The stray character would have broken the build. Unique binding prevents double click handlers if the widget is constructed again. |
| `Game/Source/GrantMoney/Public/MapCameraPlayerController.h` | Added `PauseAction`, `PauseMenuWidgetClass`, `TogglePauseMenu()`, private `OpenPauseMenu()` / `ClosePauseMenu()`, and a `UPROPERTY(Transient)` `PauseMenuWidget`. Updated the class comment. | Pause state lives on the controller. |
| `Game/Source/GrantMoney/Private/MapCameraPlayerController.cpp` | Constructor loads `/Game/Input/IA_Pause.IA_Pause` and `/Game/UI/WBP_PauseMenu`. `SetupInputComponent()` binds `IA_Pause` (`ETriggerEvent::Started`) on the existing Enhanced Input component, after the existing mapping-context code. Added the toggle / open / close functions. | Implements the behavior. |
| `docs/course/Module1-BuildingAWorld/Sprint3/PauseMenu_SafetyReview.md` | This document. | Requested. |

## Design Decisions

- **Pause binding is on the controller, not the Pawn.** The Pawn only handles camera actions. A paused game must still accept Escape, and the controller does not depend on which Pawn is possessed. This is the only deliberate change to the "Pawn owns the action bindings" pattern.
- **"Is the menu open" = widget valid and `IsInViewport()`.** There is no separate boolean that could fall out of sync, so pressing Resume (which removes the widget from inside the widget class) is always seen correctly by the next Escape press. This is what prevents a duplicate widget.
- **The widget is created once and reused.** The reference is kept (not cleared) after closing.
- **`ETriggerEvent::Started`** fires once per press; held keys do not re-toggle.
- **No new Enhanced Input component, no second `SetupInputComponent()`, no new controller.**
- **Preserved:** `IMC_MapCamera` loading/priority, `BeginPlay()` (cursor hidden, game-only input), all Pawn code, camera tuning, bounds, follow target, `TileMapRenderer`, `MapArrays`, TileGen data, world generation.

## Asset Paths To Verify Manually In Unreal

| Path used in code | Where |
|-------------------|-------|
| `/Game/Input/IA_Pause.IA_Pause` | `AMapCameraPlayerController` constructor |
| `/Game/UI/WBP_PauseMenu` | `AMapCameraPlayerController` constructor |
| `/Game/Input/IMC_MapCamera.IMC_MapCamera` | Existing, unchanged |

Both new paths match the files that exist on disk (`Game/Content/Input/IA_Pause.uasset`, `Game/Content/UI/WBP_PauseMenu.uasset`). If you move or rename either asset, the Output Log will show `IA_Pause is not set` or `WBP_PauseMenu class is not set`, and you can set the properties on the controller Blueprint defaults instead.

## Editor Setup Still Required

1. ~~`IA_Pause` → Trigger When Paused~~ **Verified headlessly: `trigger_when_paused = True`.** No action needed.
2. ~~`WBP_PauseMenu` parent class~~ **Verified headlessly: parent is `/Script/GrantMoney.PauseMenuWidget`.** No action needed.
3. **Widget names must match exactly (case-sensitive):** `ResumeButton` and `QuitButton`, both of type **Button**. A mismatch is a Blueprint compile error because of `BindWidget`. Both names were found in the saved asset; open the Designer once and confirm there is no compile error.
4. **Required for keyboard focus:** in `WBP_PauseMenu` → Class Defaults, tick **Is Focusable**. **Verified currently `False`.** Focusability is fixed at construction in UE 5.8 and cannot be set from C++ here. Without it, the "focus the widget" step has nothing to focus and keyboard navigation of the menu will not work (mouse clicks still do).
5. ~~`IMC_MapCamera` Escape → IA_Pause~~ **Verified headlessly: the mapping exists.** No action needed.
6. The editor must be **closed (or Live Coding not active)** when building from the command line.

## Manual Build / Test Steps

1. Build the **GrantMoneyEditor | Win64 | Development** target (already done once successfully; rebuild after any further code change).
2. Open the project; open `WBP_PauseMenu`, tick **Is Focusable**, compile and save.
3. Press Play in `PlayGame1` and run the checklist below.
4. For the Quit test, package (or run `GrantMoney.exe` from a cooked build). In PIE, Quit only ends the play session.

## Manual Test Checklist (all NOT YET RUN)

- [ ] Project compiles (Editor target)
- [ ] ESC opens the menu
- [ ] Gameplay is paused
- [ ] Cursor appears
- [ ] Resume button unpauses
- [ ] ESC closes the menu
- [ ] ESC still closes the menu while paused
- [ ] Camera input resumes after closing
- [ ] Quit button exits the packaged build
- [ ] Existing camera movement, mouse turn/pitch and zoom still work after Resume
- [ ] Pressing ESC repeatedly never creates a second pause widget
- [ ] Resume button then ESC reopens the menu correctly
- [ ] No unrelated files changed (`git status`)

## Verification Status

| Item | Result |
|------|--------|
| Game target (`GrantMoney Win64 Development`) build, including UHT | **Succeeded** (run during this task) |
| Editor target (`GrantMoneyEditor Win64 Development`) build | **Succeeded** (run during this task, editor closed) |
| `IA_Pause` Trigger When Paused | **Verified `True`** via `UnrealEditor-Cmd` + Python |
| `IMC_MapCamera` contains Escape → `IA_Pause` | **Verified** via the same method |
| `WBP_PauseMenu` parent class | **Verified** `/Script/GrantMoney.PauseMenuWidget` |
| `WBP_PauseMenu` Is Focusable | **Verified `False`** (needs enabling, see above) |
| `ResumeButton` / `QuitButton` names | Found in the saved asset's strings; not verifiable via Python (widget tree not exposed). Confirm in Designer |
| PIE / runtime behavior | **Not run** |
| Packaged Quit behavior | **Not run** |

## Notes

- Building the Game target created build output under `Game/Binaries/` and `Game/Intermediate/`; these are not tracked by Git.
- `Game/Content/Input/IMC_MapCamera.uasset` and `Game/GrantMoney.uproject` (adds `UMG`) show as modified in Git; both were changed by hand before this task and were not touched here.
- Nothing was staged, committed or pushed.
