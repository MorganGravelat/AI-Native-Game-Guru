# GMS-106 – Safe Player Start Flow for Supported Maps
## Development and Recording Guide

This document covers **one Jira issue only**, in the same style as the GMS-111 guide.

Status of this document: **Complete, needs review**

---

## Jira Issue

> Create a simple, reusable player-start approach so the character begins inside valid playable space instead of relying on a one-off editor placement.

### Done When

- The player begins in a known valid playable location.
- The spawn approach works with the existing level/map selection flow.
- The character does not spawn inside a wall or obvious blocking prop.
- The design can be extended to additional generated maps without rewriting the player class.
- Failure to resolve a valid start produces a clear log/error rather than a crash.

### Course Capture Requirement (from the Jira issue)

- Record short raw clips while building the feature, not only a finished demo.
- Save at least one screenshot for each meaningful step and use numbered filenames.
- Save useful AI prompts, corrections, and at least one verification result.
- Upload raw media to the shared media folder, not the Git repository.
- Documentation must describe what actually happened. Do not claim a test passed unless it was performed.

---

# Starting Point (What Already Exists)

This is from reading the source before GMS-106. It is not a test result.

```text
PlayGame1.umap / PlayGame2.umap  (one level per map)
   ├─ ATileMapRenderer   LevelToRender = 1 or 2 → FMapArrays::GetMap(Level)
   │       └─ BeginPlay builds tiles, ground collision (GMS-104) and boundary walls
   └─ PlayerStart        placed by hand in the Editor

BP_GrantMoneyGameMode (derives from AGrantMoneyGameMode)
   ├─ spawns AGrantMoneyPlayerCharacter at the PlayerStart (engine default)
   └─ spawns AMapCameraPawn at the PlayerStart, then follows the character (GMS-111)
```

| Piece | State before GMS-106 |
|---|---|
| Spawn location | Engine default: `FindPlayerStart` returns the PlayerStart actor. If the level has none, it returns World Settings (the world origin). |
| Map selection | One umap per map. The placed `ATileMapRenderer`'s `LevelToRender` chooses the registered map in `MapArrays.cpp`. |
| Playable cell | `Valid[X][Y] != 0 && Ground[X][Y] != 0` (`FMapBoundaryGenerator`). |
| Cell position | Cell `(X, Y)` is centred at `(X * 100, Y * 100, 0)` in renderer space. Walls sit on the edges between playable and non-playable cells. |
| Timing | The local player's pawn spawns **before** actor `BeginPlay`. So the ground slabs and walls do not exist yet when the spawn location is chosen. |

The last row decided the design: the safety check cannot only trace against generated collision, because that collision does not exist yet. It must come from the map data.

---

# Development and Documentation Workflow

```text
Branch → Investigate → Design → Implement (resolver, renderer helper, GameMode)
       → Compile → Predict expected result offline → Editor verification → Review diff
```

---

# Step 1 – Create the Development Branch

```bash
git checkout main
git status            # clean
git checkout -b GMS-106-safe-player-start
```

### Capture

| File | Content |
|---|---|
| `01-branch-created.png` | Terminal showing the new branch. |

---

# Step 2 – Ask AI to Inspect the Spawn Path

## AI Prompt

> Start implementation of GMS-106, going step by step, on the GMS-106 branch off of main.

The AI first listed the files it intended to read: the GameMode, `MapArrays`, the `TileMapRenderer` header, and the player character header. It then read only those, plus the cell placement code in `TileMapRenderer.cpp` and `MapBoundaryGenerator.h`. The findings are the "Starting Point" table above.

### Capture

| File | Content |
|---|---|
| `02-spawn-path-investigation.png` | The AI's findings list, or `GrantMoneyGameMode.cpp` before the change. |

---

# Step 3 – Design Decision

| Option | Decision |
|---|---|
| Keep moving the PlayerStart by hand on each map | Rejected. This is the one-off placement the ticket asks us to remove. |
| Hard-code a start coordinate per map | Rejected. It is map-specific and breaks for every new TileGen export. |
| Trace against generated collision at spawn | Not possible alone. The ground and walls are built after the pawn spawns (see Timing). |
| **Pick the start from the map data, then confirm with a capsule overlap test** | **Chosen.** It works for any registered map and needs no player class change. The overlap test catches props placed in the level. |

Rules of the chosen design:

1. A **safe cell** is a playable cell whose eight neighbours are all playable. The capsule (radius 34) is then at least one full tile from any boundary wall.
2. Safe cells are ordered by distance to the **centre of the playable area**, with ties in grid order. The start is deterministic ("known") for each map.
3. The GameMode tries the cells in that order. It uses the first one where the character's capsule does not overlap anything blocking.
4. If nothing works, it logs an error and falls back to the PlayerStart. If there is no start spot at all, it logs an error and spawns nothing. In both cases there is no crash.
5. The PlayerStart's **yaw** is still used, so the initial camera direction is designed in the Editor as before.

---

# Step 4 – Implement

| File | Change |
|---|---|
| `Game/Source/GrantMoney/Public/PlayerSpawnResolver.h` (new) | `FPlayerSpawnResolver::FindSafeStartCells(const FGrantMoneyMapData&)`. Plain C++, like `FMapBoundaryGenerator`. |
| `Game/Source/GrantMoney/Private/PlayerSpawnResolver.cpp` (new) | Collects safe cells and the playable centre, then stable-sorts by distance to the centre. |
| `Game/Source/GrantMoney/Public/TileMapRenderer.h` / `Private/TileMapRenderer.cpp` | `GetCellWorldLocation(FIntPoint)`: the one place that turns a grid cell into a world position, using the renderer's transform. |
| `Game/Source/GrantMoney/Public/GrantMoneyGameMode.h` / `Private/GrantMoneyGameMode.cpp` | `SpawnDefaultPawnFor` override plus `FindSafeSpawnLocation`. The camera pawn now starts on the character instead of the PlayerStart. |

Not changed: `AGrantMoneyPlayerCharacter`, `MapArrays`, the generated `Maps/*` files, `MapBoundaryGenerator`, and every `.umap` / `.uasset`.

### Capture

| File | Content |
|---|---|
| `03-resolver-code.png` | `PlayerSpawnResolver.cpp`, the neighbour check. |
| `04-gamemode-spawn-override.png` | `SpawnDefaultPawnFor_Implementation` and its fallback branches. |

---

# Step 5 – Camera Start Follows the New Spawn

Before GMS-106, the camera pawn was spawned at the PlayerStart. With the new spawn, the character can be far from the PlayerStart. The camera would then start at the PlayerStart and jump to the character on the first follow tick. It now spawns at the character's transform, which already carries the PlayerStart's yaw.

---

# Step 6 – Compile

```bash
"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" GrantMoneyEditor Win64 Development -Project="<repo>/Game/GrantMoney.uproject" -WaitMutex
```

**Result (actually run):** `Result: Succeeded`. The only warnings were deprecation warnings from engine headers (`Character.h` `GetMovementBase`, `HierarchicalInstancedStaticMeshComponent.h` `BuildComponentInstanceData`). None came from GMS-106 code.

### Capture

| File | Content |
|---|---|
| `05-compile-succeeded.png` | The `Result: Succeeded` line. |

---

# Step 7 – Predict the Expected Result Before Testing

A short Python script (kept outside the repo) applied the same rules to the generated arrays in `Maps/Map1_1.cpp` and `Maps/Map1_2.cpp`. It is a prediction, not an in-engine test.

| Map | Playable cells | Safe cells | Playable centre | Expected start cell |
|---|---|---|---|---|
| Level 1 (`PlayGame1`) | 2819 | 2531 | (32.12, 30.64) | **(32, 31)** |
| Level 2 (`PlayGame2`) | 2896 | 2604 | (31.50, 31.50) | **(31, 31)** |

If the renderer sits at the world origin, Level 1 spawns at about `X=3200 Y=3100 Z=93` (half height 88 + 5 clearance).

---

# Step 8 – Verify in the Editor

Run each level in PIE and open the Output Log (filter `GrantMoneyGameMode`).

### Log lines to look for

```text
GrantMoneyGameMode: safe start on level 1 at cell (32, 31), X=... Y=... Z=... (0 blocked candidates skipped).
GrantMoneyGameMode: camera ... now follows ...
```

### Checklist

| Check | How | Result |
|---|---|---|
| PlayGame1 starts at the expected cell | Log line shows level 1, cell (32, 31) | LogTemp: GrantMoneyGameMode: safe start on level 1 at cell (32, 31), X=3200.000 Y=3100.000 Z=93.000 (0 blocked candidates skipped). |
| PlayGame2 starts at the expected cell | Log line shows level 2, cell (31, 31) | LogTemp: GrantMoneyGameMode: safe start on level 2 at cell (31, 31), X=3100.000 Y=3100.000 Z=93.000 (0 blocked candidates skipped). |
| Character lands on the ground and can move | Play, walk around | Working |
| Camera starts on the character without a jump | Watch the first second of PIE | Working |
| PlayerStart position no longer matters | Move the PlayerStart off the map, then PIE. The character still spawns at the safe cell. Undo the move afterwards. | Working |
| Blocking prop is avoided | Place a cube with BlockAll collision on the expected cell, then PIE. The log should say `1 blocked candidates skipped` (or more). Remove the cube afterwards. | Working |
| Failure is logged, not a crash | Temporarily set the renderer's `LevelToRender` to an unregistered number (e.g. 99), then PIE. Expect the `is not a valid registered map` error and the `falling back to PlayerStart` error. Restore the value. | Working |

### Capture

| File | Content |
|---|---|
| `06-pie-safe-start-log.png` | The Output Log line for PlayGame1. |
| `07-pie-player-at-start.png` | The character standing at the start in PlayGame1. |
| `08-prop-avoided.png` | The blocking cube and the log showing a skipped candidate. |
| `09-failure-log.png` | The error/fallback log with an unregistered level. |
| Clip `gms106-spawn-demo.mp4` | PIE start on both maps. Raw media goes to the shared media folder, not Git. |

---

# What Actually Happened

Worked with no errors. Player spawns in a valid position on the map, no matter the location of PlayerStart. It correctly avoids blocking assets and materials.

---

# AI Workflow Log

| # | Prompt / action | Result |
|---|---|---|
| 1 | "Read through the CLAUDE documentation and rules, as well as familiarize yourself with the context of the docs folder for output. Create a new branch for GMS-106 and check in to it. Do not implement yet." | The AI read the project rules, the delivery process doc and the GMS-111 guide. It fetched GMS-106 from Jira and created `GMS-106-safe-player-start`. |
| 2 | "GMS-106 goes in the Sprint 1 folder, as indicated by the module 2 sprint 1 backlog on Jira. Start implementation of GMS-106, going step by step, on the GMS-106 branch off of main." | Investigation (Step 2), design (Step 3), implementation (Steps 4–5), compile (Step 6), offline prediction (Step 7). |
| 3 | "Compile this session into a text prompt document separated by my prompts and your response in the docs/course/Module2/Prompt/Sprint1 folder." | Session log saved as `Prompt/Sprint1/GMS-106-SafePlayerStart`. |
| 4 | "What screenshots/video clips do I need to take to satisfy showing the development of this ticket?" | Capture list 01–09 plus two clips, mapped to the Jira "Done when" items. |
| 5 | "Make a pull request for this branch on Main. Do not submit it to main, it needs to be reviewed first." | The developer had run the PIE checklist. The AI committed the GMS-106 files only, leaving out a `PlayGame1.umap` change left over from testing, pushed the branch and prepared the PR. |

# AI Mistakes, Corrections and Surprises

- **Surprise: spawn happens before `BeginPlay`.** The investigation showed the ground and walls are built in the renderer's `BeginPlay`, which runs after the player's pawn is spawned. A trace or overlap against generated collision at spawn time would find nothing. So the wall safety comes from the map data, and the overlap test only covers placed props.
- **Doc location.** The AI asked whether to use the Sprint 1 or the Sprint 2 folder. The developer said Sprint 1, matching the Jira backlog.

---

# Known Limitations

- The start is the safe cell nearest the map's centre. There is no per-map "preferred start cell" yet. That could be added later as a property on `ATileMapRenderer`, without touching the player class.
- If a level has more than one `ATileMapRenderer`, the first one found is used, with a warning.
- The overlap test only sees collision that exists at spawn time: placed props, not generated walls. Generated walls are avoided by the neighbour rule instead.
- The fallback to the PlayerStart is not itself checked for safety. It is the pre-GMS-106 behaviour.
- Multiplayer is not considered. Every player would get the same cell, but the second capsule would make that cell fail the overlap test, so the next candidate is used. This is not tested.

---

# Suggested Commit Message

```text
GMS-106: spawn the player on a safe cell of the generated map

- Add FPlayerSpawnResolver: safe start cells (playable with all eight
  neighbours playable), ordered by distance to the playable centre
- Add ATileMapRenderer::GetCellWorldLocation for grid-to-world conversion
- GrantMoneyGameMode overrides SpawnDefaultPawnFor: first safe cell whose
  capsule overlap test is clear; logs and falls back to the PlayerStart
- Camera pawn now starts at the character instead of the PlayerStart
```
