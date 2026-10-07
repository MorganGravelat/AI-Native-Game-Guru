# Map Boundary Walls: What Was Added and How to Use It

This guide is for anyone on the team, including people who have never touched the code. Technical words are explained in plain language the first time they appear.

---

## 1. The Short Version

The game map is a flat floor made of colored squares (called **tiles**). Before this change, the floor just stopped at the edge. There was nothing to show where the playable area ends and nothing to stop a character from walking off.

We added an **automatic wall builder**. When you press Play, it looks at the map, finds every edge of the floor, and builds a tall wall along it. You can turn it on or off and change the wall's height and thickness from the Unreal Editor, with no coding.

For now the walls are plain gray-ish **cubes** stretched into wall shapes. They are placeholders and will be replaced with nicer-looking art later.

---

## 2. How the Project Fits Together (Background)

You don't need this to use the feature, but it helps to see the big picture.

1. **TileGen** is a separate tool that designs maps. It exports each map as a **64 x 64 grid** of numbers. Each number says what kind of tile goes in that grid square (0 means "nothing here"). It also exports a second grid saying which squares are part of the playable map.
2. **MapArrays** (`MapArrays.h` / `MapArrays.cpp`) is the "librarian." You give it a level number (1, 2, ...) and it hands back that level's grids. It is the only place that knows which TileGen files exist.
3. **TileMapRenderer** is an **Actor** (an object you place in an Unreal level) that builds the visible floor. It asks MapArrays for the selected level and draws one flat square for each tile.
4. **MapBoundaryGenerator** (new) is a small helper that the TileMapRenderer calls after the floor is built. It uses the same grids to build the walls.

```
PlayGame level
   |
   v
TileMapRenderer  --asks for level-->  MapArrays  --returns grids-->  (TileMapRenderer)
   |                                                                      |
   |-- builds the floor                                                   |
   '-- hands the same grids to --> MapBoundaryGenerator --> builds walls
```

Because the walls come from the same grids as the floor, every level gets walls that fit its own shape automatically.

---

## 3. Exactly What Was Added or Changed

### New files

| File | What it is, in plain words |
|------|----------------------------|
| `Game/Source/GrantMoney/Public/MapBoundaryGenerator.h` | The "menu" for the wall builder. It describes what the builder can do and what information it needs. (`.h` files are called *headers*.) |
| `Game/Source/GrantMoney/Private/MapBoundaryGenerator.cpp` | The "recipe": the actual step-by-step instructions the wall builder follows. |

### Changed files

| File | What changed |
|------|--------------|
| `Game/Source/GrantMoney/Public/TileMapRenderer.h` | Added four new settings that show up in the Unreal Editor (see section 5), plus an internal slot that holds the cube shape used for walls. |
| `Game/Source/GrantMoney/Private/TileMapRenderer.cpp` | Loads Unreal's built-in cube shape when the Actor is created, and, at the very end of building the map, calls the wall builder if walls are turned on. |

### Not changed

- TileGen and its exported map files.
- `MapArrays.h` / `MapArrays.cpp`.
- The floor drawing, the `M_Tile` material, the tile textures and the camera.
- The `PlayGame` level file.

If walls are switched off, the game behaves exactly as it did before.

### How the wall builder works

1. **Decide which squares are "playable."** A square counts only if the validity grid says it is part of the map **and** its tile number is not 0. Anything beyond the edge of the 64 x 64 grid counts as *not* playable.
2. **Look at the four neighbors** of each playable square (left, right, up, down; not diagonals).
3. **Add one wall piece** on each side where the neighbor is not playable. No walls are placed between two playable squares, so you never get walls cutting through the floor. Holes in the middle of the map get walls around them too.
4. **Shape each piece.** The built-in cube is 100 x 100 x 100 units (Unreal's measuring unit is the *centimeter*, so that is about one meter). Each cube is stretched to be as long as one tile, as thick as `Wall Thickness`, and as tall as `Wall Height`. Its bottom rests on the floor.
5. **Rotate pieces on the top and bottom sides** by 90 degrees so they run in the correct direction.
6. **Make the walls solid.** They use Unreal's `BlockAll` collision setting, which means characters and objects cannot pass through them.
7. **Report the result** in the log, for example `MapBoundaryGenerator: Created 312 wall segments.`

Two details worth knowing:

- **One component, many walls.** All wall pieces live in a single *HISM component* (Hierarchical Instanced Static Mesh). In plain terms, it is a trick where Unreal stores one copy of the cube and draws it hundreds of times, which is far faster than creating hundreds of separate objects. The floor uses the same trick.
- **Slightly longer pieces.** Each piece is as long as a tile *plus* the wall thickness. This makes neighboring pieces overlap a little at corners so there are no little notches. You will not see the overlap.

---

## 4. Before You Start

You need:

- The project downloaded from GitHub (with the large asset files; the project uses Git LFS for these).
- Unreal Engine 5.8 installed.
- Visual Studio 2022 with the "Game development with C++" workload (needed so Unreal can compile the code).

---

## 5. Step-by-Step: Using the Walls

### Step 1: Compile the code

Unreal projects that contain C++ code must be **compiled** (turned from human-readable code into a program the computer can run) whenever the code changes.

1. **Close Unreal Editor** if it is open. This matters: new settings can't be added while the editor is running.
2. Open `Game/GrantMoney.uproject` by double-clicking it. Unreal will say the project needs to be rebuilt. Click **Yes**.
3. Wait while it compiles. This can take a few minutes the first time.
4. The editor opens when the build is done. If you see an error window instead, copy its text and share it with the team.

> Alternative: right-click `GrantMoney.uproject`, choose **Generate Visual Studio project files**, open the `.sln` file, set the configuration to **Development Editor** and **Win64**, then use **Build > Build Solution**.

### Step 2: Open the game level

1. In the **Content Browser** (the file window at the bottom), go to `Maps`.
2. Double-click `PlayGame` to open it.

### Step 3: Find the TileMapRenderer

1. In the **Outliner** (the list of everything in the level, usually top right), click **TileMapRenderer**.
2. The **Details** panel (usually right side) shows its settings.

### Step 4: Check the settings

Under the **Map** heading:

| Setting | Meaning |
|---------|---------|
| **Level To Render** | Which map to draw. `1` is the first map, `2` is the second. |

Under the new **Boundary** heading:

| Setting | Default | Meaning |
|---------|---------|---------|
| **Generate Boundaries** | On (checked) | Turn walls on or off. |
| **Wall Height** | 300 | How tall the walls are, in Unreal units (300 is about 3 meters). Minimum 1. |
| **Wall Thickness** | 20 | How thick the walls are (20 is about 20 cm). Minimum 1. |
| **Boundary Material** | None | The "skin" that covers the walls. Leave empty to use the cube's plain default look. |

The height, thickness and material fields are grayed out if Generate Boundaries is unchecked.

### Step 5: Press Play

1. Click the **Play** button at the top of the editor.
2. The map builds and the walls appear around the edge.

The game camera currently looks straight down, so from above the walls look like thin outlines around the floor. Their height is hard to judge from that angle. To look at them from the side:

1. Click **Play** drop-down arrow (three dots next to Play) and choose **Simulate**, or press **Alt+S**.
2. Hold the **right mouse button** and use **W A S D** to fly around.
3. Press **Esc** to stop.

### Step 6: Check the log (optional but useful)

1. Open **Window > Output Log**.
2. In the search box, type `MapBoundaryGenerator` or `TileMapRenderer`.
3. You should see lines like:
   - `TileMapRenderer finished. Created 2819 tiles.` (Level 1 floor; this number is the same as before the walls existed)
   - `MapBoundaryGenerator: Created N wall segments.`

### Step 7: See the collision (optional)

While playing, press the tilde key (**`**) to open the console and type `show collision`, then press Enter. The solid areas get outlined. Type it again to turn it off.

---

## 6. Common Tasks

### Make taller or thicker walls

Select **TileMapRenderer**, change **Wall Height** or **Wall Thickness** in Details, and press Play. No compiling needed.

### Turn walls off

Uncheck **Generate Boundaries**. The map returns to its original wall-free look.

### Show a different level

Change **Level To Render** to `2` and press Play. The walls follow that level's shape. If you pick a level that doesn't exist (for example `999`), the log says `Failed to load level 999...`, nothing is built and the game does not crash. Set it back to `1` afterward.

### Give the walls a custom look

1. In the Content Browser, create or find a **Material** (a material is the "paint and texture" applied to a 3D object).
2. Open it and, in the **Details** panel of the Material Editor, tick **Used with Instanced Static Meshes** (under *Usage*). **This is required.** Without it, Unreal shows a gray checkerboard instead of your material, because instanced shapes need that permission.
3. Save the material.
4. Select **TileMapRenderer** and drag the material into **Boundary Material** (or click the dropdown and pick it).
5. Press Play.

Note: the floor material `M_Tile` is deliberately **not** used for walls. It is built for the floor's picture tiles.

### Add a new map

Adding a map is documented in the MapArrays guide. Once the new map is registered there, set **Level To Render** to its number. The walls need no extra work.

---

## 7. If Something Looks Wrong

| What you see | Likely reason | What to do |
|--------------|---------------|-----------|
| No **Boundary** section in Details | The code wasn't compiled with the new files, or the editor was open during the build. | Close the editor and repeat Step 1. |
| No walls in Play | **Generate Boundaries** is unchecked, or the level failed to load. | Check the box; check the Output Log for errors. |
| Walls are gray with a checkerboard pattern | A custom Boundary Material is missing **Used with Instanced Static Meshes**. | Enable it in the Material Editor (see section 6). |
| `Failed to load level N` in the log | That level number isn't registered. | Use a registered level (1 or 2), or register the new map. |
| `MapBoundaryGenerator: Invalid sizes` in the log | Height or thickness is zero or negative. | Use values of at least 1. |
| `Created 0 wall segments` | The map has no playable squares. | Check the map data. |
| Compile error about a missing file | Files weren't downloaded or got moved. | Make sure `MapBoundaryGenerator.h` is in `Public` and `MapBoundaryGenerator.cpp` is in `Private`. |

---

## 8. Current Limits

- Walls are plain cubes. Nicer art will replace them later.
- The map and walls are built **once**, when Play starts. Changing settings while the game is running does nothing until you stop and press Play again.
- There is no player character yet, so wall collision has not been tested with a real character. It is set up so that a character will be blocked when one exists.
- Wall corners are simple overlaps, not special corner pieces.
- The `Deco` and `Collision` grids that TileGen exports are not used by the walls.

---

## 9. Build and Test Status

- **Compiled:** Yes. The project built successfully with `Development Editor | Win64` after these changes.
- **Tested in the Unreal Editor:** Not yet. Following sections 5 and 6 above is the test. The things to confirm are: walls appear around Level 1 and Level 2, turning the setting off removes them, an invalid level doesn't crash, and the floor still shows 2,819 tiles with the right textures.
- Nothing has been committed or pushed. Review the changes in Git before committing.
