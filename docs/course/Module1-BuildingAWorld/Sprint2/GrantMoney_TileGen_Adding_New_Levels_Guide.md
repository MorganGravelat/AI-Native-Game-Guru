# GrantMoney — Creating and Adding New TileGen Levels

**Beginner's step-by-step guide | Unreal Engine 5.8 | AI-Native Game Guru**

## What you will accomplish

By the end of this guide, you will have made a new 64 × 64 map in TileGen, exported its C++ map data, connected it to the GrantMoney Unreal project, and displayed it in the existing `PlayGame` level. You will be able to choose between Level 1, Level 2 and later registered maps by changing **Map → Level To Render** on the `TileMapRenderer` Actor. You will **not** need to create a new Unreal level (`.umap`) or edit the rendering loop for every TileGen map.

We will also preserve the map's **valid-cell array**, which the upcoming boundary-wall generator will use to decide where walls belong.

> **Two different meanings of “level”:** `PlayGame.umap` is the reusable Unreal world. A *TileGen level* is one set of generated tile data displayed inside that world. You can keep one `PlayGame.umap` and select many TileGen maps.

---

## 1. What you need before starting

On a Windows development computer, have the following available:

- Unreal Engine version used by the team (the original project was built in the UE 5.8 series).
- Visual Studio 2022, with **Game development with C++** and the required Windows SDK/compiler installed.
- The GrantMoney Git repository, cloned locally (for example `C:\Dev\AI-Native-Game-Guru`).
- Git and Git LFS installed, with `git lfs install` run at least once.
- The project's supplied **TileGen** program and tile library/project files.
- Enough disk space for Unreal to regenerate its build files.

The important working files in the repository should resemble:

```text
AI-Native-Game-Guru/
├── Game/
│   ├── GrantMoney.uproject
│   ├── Config/
│   ├── Content/
│   │   ├── Maps/PlayGame.umap
│   │   ├── Materials/M_Tile.uasset
│   │   └── Tiles/                 (imported Unreal texture assets)
│   └── Source/GrantMoney/
│       ├── Public/
│       │   ├── MapArrays.h
│       │   └── TileMapRenderer.h
│       └── Private/
│           ├── MapArrays.cpp
│           ├── TileMapRenderer.cpp
│           └── Maps/             (TileGen's generated .h/.cpp data)
└── docs/
```

**Do not put your active Unreal project in a OneDrive-synced folder.** A normal local directory such as `C:\Dev` is preferable.

### Protect existing work first

1. Close Unreal Editor and Visual Studio.
2. Open the repository folder in File Explorer. Right-click an empty space and select **Open in Terminal** or open Git Bash there.
3. Run `git status`. Read the results: it tells you if you have unsaved-to-Git changes.
4. Preserve any important work on your current branch before changing source files. If starting a new Jira ticket from a clean `main`, use the following (replace `XX` with the real ticket number):

```bash
git switch main
git pull origin main
git lfs pull
git switch -c GMS-XX-add-tilegen-level
```

Do **not** use `git switch main` if you have unfinished changes that you have not safely committed or preserved.

---

## 2. Understand what TileGen produces

TileGen is the external program used to design a tile-based map. Each map is a grid of **64 columns by 64 rows**. Instead of hand-placing thousands of objects in Unreal, TileGen exports data that tells Unreal what is at each grid position.

The essential exported information is:

| Exported item | Plain-English meaning | Why we need it |
|---|---|---|
| Ground array | Which tile ID occupies each grid cell | Draws the ground |
| Valid array | Whether each grid cell belongs to the playable map (1) or not (0) | Draws only usable cells; later locates border walls |
| Tile asset lookup | Which Unreal texture belongs to each numeric tile ID | Makes the ground display the correct textures |
| PNG tile images / import script | Original visual images and a helper script | Allows new texture assets to be imported into Unreal |

A `0` ground ID means **no ground tile**. A `0` validity value means **outside the playable map**. These are related but different concepts. Do not merge or reverse them.

**Important:** TileGen's arrays are indexed `[X][Y]`: X means column and Y means row. Tile IDs are 16-bit unsigned values. Do not convert them to 8-bit values or swap the coordinates.

---

## 3. Open TileGen and create a new map

This section assumes you are new to TileGen. The exact button wording can vary across versions of the supplied program; locate the corresponding **project/map selection**, **map generation** and **export** controls rather than relying on a guessed button label.

1. Find the supplied TileGen directory on your computer. If the distribution includes an extracted `TileGenBinaries` folder, open that folder and launch `TileGen.exe`. If the program was supplied only as source code, ask the team lead for the built executable and any required .NET runtime.
2. Wait for the TileGen window to open. Familiarize yourself with three separate areas: the available **tile library**, the **map/map-set selection**, and the **map preview or generation controls**.
3. Load the supplied TileGen project or tile collection, if it does not load automatically. We recommend using the **same tile library** as existing levels for your first additional map. This avoids changing the meaning of existing tile IDs.
4. Identify the existing map(s). Do **not** overwrite a map that is already in the Unreal project. Either create a new map entry in the map set or make a clearly named copy that can be edited independently.
5. Keep the map dimensions at **64 × 64**. The current C++ interface is designed around that fixed size.
6. Generate or edit the new ground layout. For a test, give it an obvious difference from Level 1—for example, a central opening, a different path pattern or a differently shaped playable area.
7. Inspect the **valid/playable mask** in TileGen if the tool exposes it. Deliberately leave some cells outside the playable area so you can verify that Unreal skips them. The future border generator will use these cells too.
8. Preview the result before exporting. Make sure the new map is distinguishable and contains visible ground within playable cells.
9. Save the TileGen project/map-set data so you can reproduce the same export later.

**Tip for beginners:** A TileGen map is data, not an Unreal `.umap` file. Generating a map in TileGen will not automatically make a new level appear in Unreal's Content Browser. The following export and C++ registration steps connect the two programs.

---

## 4. Export the new map from TileGen

1. Create a separate export directory on your local computer, such as `C:\Dev\TileGenExports\Level2`. This keeps exports organized and prevents new files from overwriting old ones.
2. In TileGen, locate its map/C++ export workflow and export the newly created map (or map set). Use the tool's Unreal-compatible tile asset destination so the resulting paths point under `/Game/Tiles/`.
3. If TileGen offers an associated PNG export and Unreal Python importer, export those too **only if the new map requires newly generated image assets**. A new layout using an unchanged, already imported library normally doesn't require reimporting all textures.
4. Open the exported directory in File Explorer. Look for the C++ data and lookup files. In the individual-map format used by this project, a new map might generate:

```text
Map1_2.cpp
Map1_2.h
Map1_2_TileAssets.h
```

These are **examples, not guaranteed names**. TileGen may give your new map a different name or produce a consolidated map-set export. Use the actual generated names everywhere in this guide.

5. Inspect the new `.h` file in a text editor. Identify the ground-array name and the validity-array name. Typical declarations resemble:

```cpp
extern const unsigned short Map1_2[64][64];
extern const unsigned char Map1_2_Valid[64][64];
```

6. Inspect the `_TileAssets.h` file. Find the tile asset table and its count, typically named like `Map1_2_TileAssets` and `Map1_2_TileAssetCount`.
7. Check at least one texture path. It must identify a real imported Unreal asset, normally beginning `/Game/Tiles/`. If paths use a Windows directory or `/Content/Tiles`, correct TileGen's export destination and regenerate the output.
8. Confirm that the second export uses **distinct C++ symbols** from existing maps. **Renaming only the exported files is not enough** if they still define `Map1_1` internally.

> If your TileGen version exports a **single consolidated map set** containing several maps, do not copy identical `.cpp` definitions multiple times. Register the appropriate entry or entries from its generated directory table in `MapArrays.cpp`. The next section illustrates the individual-map format that the current GrantMoney implementation uses.

---

## 5. Copy the generated C++ data into GrantMoney

1. Close Unreal Editor and Visual Studio before changing generated C++ files.
2. Open your repository's `Game/Source/GrantMoney/Private/Maps/` folder.
3. Copy the new map's generated `.cpp`, `.h` and tile-assets `.h` files into this directory. **Keep the original map files untouched.**

For a second map with the example filenames, the folder now looks like:

```text
Game/Source/GrantMoney/Private/Maps/
├── Map1_1.cpp
├── Map1_1.h
├── Map1_1_TileAssets.h
├── Map1_2.cpp             NEW
├── Map1_2.h               NEW
└── Map1_2_TileAssets.h    NEW
```

Don't copy the exported arrays into `Content/Maps/`: that is primarily for Unreal level assets. These generated files are C++ source, so they belong inside the C++ module.

---

## 6. Check whether the new map needs texture imports

Open `Map1_2_TileAssets.h` (or its real equivalent) and compare the referenced Unreal asset names with the assets in `Game/Content/Tiles/`.

### If all textures already exist

There is **nothing to reimport**. The new map only changes where tile IDs are placed, and can reuse the existing texture library.

### If the export includes genuinely new PNG textures

1. Find the exported PNG directory and the matching generated `import_tiles_to_unreal.py` file.
2. Open the Python script in a text editor. Check that its source directory variable points to the new PNG directory. If necessary, update its local Windows path and preserve proper Python string escaping (for example, a raw path string).
3. Confirm the destination is the Unreal asset location `/Game/Tiles/`. This is an **Unreal asset path**; it is not a Windows folder path.
4. Open `GrantMoney.uproject`. In Unreal, go to **Edit → Plugins**, search for and enable **Python Editor Script Plugin** if required, and restart the editor when prompted.
5. Use **Tools → Execute Python Script** to select the generated importer. Wait for it to finish.
6. Open the Content Drawer (`Ctrl + Space`), select `Content/Tiles`, and confirm the textures now exist.
7. Open a sample texture and inspect its imported properties: 64 × 64 source resolution, appropriate filtering (the original workflow uses Nearest), and the expected mipmap/sRGB settings for the project.
8. Use **File → Save All**.
9. Close Unreal again before the next C++ build.

**Avoid overwriting the existing library:** If a new export gives an *unrelated* picture the *same filename* as a texture already in `/Game/Tiles`, importing it may replace the asset used by older maps. Keep the same tile ID/filename contract when sharing a library. If developing genuinely separate libraries, coordinate the naming and asset paths with the team before import.

---

## 7. Register the new level in `MapArrays.cpp`

This is the central change. `MapArrays.cpp` is the project's address book: it connects simple in-game numbers, such as **Level 2**, to the exported arrays. The renderer does not need to know the generated filenames.

1. Open `Game/Source/GrantMoney/Private/MapArrays.cpp` in Visual Studio or a code editor.
2. Near the top, locate the generated header includes. They currently include the first map, conceptually:

```cpp
#include "MapArrays.h"
#include "Maps/Map1_1.h"
#include "Maps/Map1_1_TileAssets.h"
```

3. Under those lines, include the generated headers for your new map:

```cpp
#include "Maps/Map1_2.h"
#include "Maps/Map1_2_TileAssets.h"
```

4. Find the `FindMap(int32 Level)` function and its `RegisteredMaps[]` array. A row consists of **five items**, in this order: game level number, ground array, validity array, tile-assets table, tile-assets count.
5. Retain Level 1 and add Level 2. Using the example symbols, the registration should resemble:

```cpp
static const FRegisteredMap RegisteredMaps[] =
{
    { 1, Map1_1, Map1_1_Valid, Map1_1_TileAssets, Map1_1_TileAssetCount },
    { 2, Map1_2, Map1_2_Valid, Map1_2_TileAssets, Map1_2_TileAssetCount },
};
```

6. Save `MapArrays.cpp`. Match the **actual names** you identified in your export—not assumed names from an example.

### Adding Level 3, Level 4 and later

Repeat the same pattern: copy that map's actual generated files, add two corresponding `#include` statements, and append another registration row with a **unique game-facing level number**. There is no need to alter `TileMapRenderer.cpp` each time.

**Do not edit the arrays or the renderer to change the map:** The renderer gets its data through:

```cpp
FMapArrays::GetMap(LevelToRender, MapData);
FMapArrays::GetTileAsset(LevelToRender, TileId);
```

---

## 8. Regenerate Visual Studio project files and compile

The new map includes a new `.cpp` source file. Unreal needs to discover and compile it.

1. Save every changed source file and **close Unreal Editor completely**. In particular, don't rely on Live Coding for this source-file integration.
2. Open File Explorer at `Game/` and right-click `GrantMoney.uproject`.
3. Select **Generate Visual Studio project files** (or the equivalent entry provided by your installed Unreal version). Wait for generation to finish.
4. Open the generated GrantMoney Visual Studio solution.
5. At the top, choose **Development Editor** and **Win64**.
6. Use **Build → Build Solution** or press `Ctrl + Shift + B`.
7. Wait for a successful compilation. Don't open Unreal until the build has finished.

**Common errors:**

| Error | What to check |
|---|---|
| `Cannot open include file: Maps/Map1_2.h` | Is the real header in `Source/GrantMoney/Private/Maps/`? Does your include exactly match its filename? |
| `Map1_2 was not declared` | Open the export and use its actual array symbol; changing a filename does not rename the symbol inside it. |
| Duplicate-symbol linker error | Did you import the same generated definitions twice or copy a consolidated map-set `.cpp` more than once? |
| Unresolved external symbol | Is the new generated `.cpp` included in the project/build? Did project-file generation finish successfully? |
| Build blocked by Live Coding/Editor | Close Unreal completely and run the Visual Studio build again. |

---

## 9. Load the new map in Unreal

1. Open `Game/GrantMoney.uproject` from your **local Git clone**.
2. Open the existing level **`Content/Maps/PlayGame`**. Do not create a new `.umap` for Level 2.
3. In the **Outliner** panel, find and select the existing `TileMapRenderer` Actor. If the actor is missing, the level is not configured to construct your map; tell the team lead rather than creating duplicate renderer Actors by accident.
4. In its **Details** panel, find the **Map** section. This section comes from a C++ `UPROPERTY`, not a Blueprint graph.
5. Change **Level To Render** from `1` to `2` (or whichever level number you registered).
6. Press **Play**. The renderer reads the selected registration and creates the visible tiles.
7. Open **Tools → Debug → Output Log**, then search for `TileMapRenderer`. You should see a completion line such as:

```text
TileMapRenderer finished. Created #### tiles.
```

`####` depends on how many cells in your new map are both valid and nonempty. Level 1's known reference result was **2,819 tiles**; a deliberately different new map need not have the same count.

8. Compare the visible shape against TileGen's preview. Confirm that unused cells are skipped and the textures are correct.
9. Stop Play and change **Level To Render** back to `1`. Press Play again. Confirm the original layout appears and that Level 1 still has the expected behavior.
10. As an optional safety test, try an unregistered value such as `999`. The renderer should log that the map is not registered and avoid building geometry; reset the property afterward.
11. Decide which map should be the default whenever the level opens. Set **Level To Render** to that value and save `PlayGame` (`Ctrl + S`). Saving the level changes `PlayGame.umap`, which you should include in Git only if that new default was intentional.

### Troubleshooting a map that loads but looks wrong

- **Nothing shows:** Check Output Log for an unregistered level, invalid map pointers, or a mesh/material failure. Confirm you selected the correct renderer Actor.
- **Wrong shape or rotation:** Confirm the export is the intended map and uses `[X][Y]`, not `[Y][X]`.
- **Grey checkerboards:** Open `M_Tile`, select the base material settings, enable **Used with Instanced Static Meshes**, apply and save. Verify each tile texture loads.
- **You cannot see the whole map:** Confirm the temporary overhead Camera Actor is aimed down with **Pitch (Rotation Y) = -90°**. A working test setup used **Ortho Width = 10,000**; adjust framing to your viewport and map.
- **Map works in Editor but textures vanish in package:** Check cooking settings include the `/Game/Tiles` asset directory. Asset-path tables alone may not be discovered as dependencies by the cooker.

---

## 10. Save reproducible source and commit your work

The generated files and imported textures belong in the Git repository when they are required by the game. Keep the original TileGen project/export somewhere documented as well, so another developer can recreate the result.

1. Close Unreal/Visual Studio and open Git Bash in the repository root.
2. Run:

```bash
git status
git diff -- Game/Source/GrantMoney/Private/MapArrays.cpp
```

3. Stage the **actual** new generated files and registration. For the illustrative Level 2 filenames:

```bash
git add Game/Source/GrantMoney/Private/MapArrays.cpp
git add Game/Source/GrantMoney/Private/Maps/Map1_2.cpp
git add Game/Source/GrantMoney/Private/Maps/Map1_2.h
git add Game/Source/GrantMoney/Private/Maps/Map1_2_TileAssets.h
```

4. If you added actual new Unreal texture assets, review and stage the necessary `Game/Content/Tiles/` files (they are generally tracked by Git LFS). Stage `Game/Content/Maps/PlayGame.umap` **only if you intentionally saved a changed default selection**.
5. Include any new documentation and the matching original TileGen source/export if that is part of your team's reproducibility process. Avoid staging `.zip` backups, recordings, `Binaries/`, `Intermediate/`, `Saved/` or `DerivedDataCache/`.
6. Inspect precisely what will be committed:

```bash
git diff --cached --stat
git status
```

7. Commit and push to your **feature branch**:

```bash
git commit -m "GMS-XX: add and verify new TileGen map"
git push -u origin HEAD
```

8. Open a Pull Request targeting `main`, describe the new map, and ask a teammate to verify it from their own clone.

---

## 11. How this feeds the upcoming border-wall generator

The boundary-wall generator will **not require you to redraw boundaries inside Unreal by hand**. It will use the very same registered map data that `TileMapRenderer` now reads.

The existing interface returns two important arrays:

```cpp
MapData.Ground[X][Y] // Which ground tile occupies this cell
MapData.Valid[X][Y]  // Whether this cell is inside the playable map
```

A wall segment will be needed wherever a **valid cell** touches an **invalid cell** or the outer edge of the 64 × 64 grid. The planned generator will check each valid cell's four neighboring sides and place a modular wall mesh on exposed sides. It can use the same selected `LevelToRender`/`FMapArrays::GetMap` approach, keeping wall placement aligned with whichever TileGen level is loaded.

**This wall generator is the next feature, not something this guide claims is already implemented.** Preserve the validity array during export/registration, and test that the new map's playable shape matches TileGen's preview; that is the critical preparation for boundary generation.

---

## Final acceptance checklist

- [ ] New TileGen map created and saved independently of existing maps
- [ ] Export is 64 × 64 and has distinct C++ symbols
- [ ] Ground, valid and asset-path data inspected
- [ ] Texture references resolve under `/Game/Tiles/`
- [ ] New C++ map files copied into `Private/Maps/` without replacing older files
- [ ] New level registered in `MapArrays.cpp`
- [ ] `TileMapRenderer.cpp` does **not** require modification
- [ ] Visual Studio **Development Editor | Win64** build succeeds
- [ ] New level loads through **Map → Level To Render** inside `PlayGame`
- [ ] Visible geometry matches TileGen's preview
- [ ] Previously registered levels still work
- [ ] Any new texture assets and deliberate level changes are saved
- [ ] Git changes are reviewed, committed and proposed in a Pull Request
- [ ] Validity data remains intact for the future border-wall generator

**Core principle:** TileGen creates map data. `MapArrays.cpp` registers it. `TileMapRenderer` draws whichever registered level is selected. Future environment systems—including boundary walls—should consume the same registered map instead of hardcoding an individual generated export.
