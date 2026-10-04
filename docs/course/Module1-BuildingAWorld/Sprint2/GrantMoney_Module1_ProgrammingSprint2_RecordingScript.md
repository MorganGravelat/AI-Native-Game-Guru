# Grant Money — Module 1: Building a World
## Programming Sprint 2 — Render the Generated World and Register a Second TileGen Map

**Document type:** Recording-ready instructor script + learner field manual  
**Audience:** Engineer familiar with basic C++, new to Unreal/TileGen  
**Starting point:** The exact END of the sponsor's three Programming Sprint 1 videos  
**Target:** A textured, data-driven world generated from registered TileGen C++ arrays, with two independently selectable maps in one `PlayGame` level  
**AI approach:** ChatGPT/Copilot proposes and explains implementation; human verifies every output in Visual Studio and Unreal  
**Reference implementation supplied:** `MapArrays.h`, `MapArrays.cpp`, `TileMapRenderer.h`, `TileMapRenderer.cpp`, and `MapArraysIntegration.md` (uploaded with this task)

> **Production rule:** The following is a script for recording a demonstrable process, not a claim that every exercise has already been executed. The uploaded `MapArrays.cpp` currently registers **only Map 1**. The integration note explicitly says Copilot did not finish build/in-editor verification. Complete and capture the testing checkpoints before recording final narration. Where the lesson describes adding Map 2, use the exact filenames and generated symbols produced on the recording machine, not assumptions.

---

# 0. Editorial handoff: where Sprint 1 ended

The sponsor's Sprint 1 has **three videos**:

1. **Part 1 — Known starting point:** blank Unreal C++ project, `PlayGame`, project layout, editor/game startup map, packaging the intentionally empty world.
2. **Part 2 — Repeatable tile import:** generate/export TileGen PNGs; review and execute `import_tiles_to_unreal.py` inside Unreal; check 64×64 textures, nearest filtering, no mipmaps, sRGB and `/Game/Tiles` cooking.
3. **Part 3 — Map data integration without graphics:** inspect generated tile-ID and validity arrays and asset table, compile the generated C++ as part of GrantMoney, create a small `MapArrays.h/.cpp` boundary, verify a registered level and a tile path **without drawing a tile**.

The sponsor closes Part 3 immediately after stressing that renderer code must not include timestamped/generated names directly: that boundary belongs in `MapArrays.cpp`. This is Sprint 2's precise jumping-off point. **Do not re-teach project creation or the full PNG import from scratch**; provide a brief recap and refer viewers to the prior videos.

**Important reconciliation with supplied working code:** The attached `MapArraysIntegration.md` says the developer's intermediate single-map prototype did not yet contain `MapArrays`; its first renderer directly referenced `Map1_1`. The supplied CURRENT files already repair that problem: `TileMapRenderer.cpp` includes only `MapArrays.h`, while `MapArrays.cpp` owns generated headers. For a recording starting at the sponsor's ideal Sprint 1 checkpoint, adapt the already-created boundary to the concrete export; for reconstructing this particular prototype, include the brief refactor segment below. Do not imply the original prototype exactly matched the sponsor's endpoint.

**End of Sprint 1 on screen:** `PlayGame` may look black/empty, tiles exist under `Content/Tiles`, generated C++ builds, and a `GetMap(1)`/tile lookup test succeeds. There is not yet a finished visible tile world.

## Sprint 2 acceptance criteria

- A C++ `ATileMapRenderer` builds the selected TileGen ground map as **HISM instances**, grouped by tile ID, using Unreal's Engine plane mesh.
- The renderer requests data through `FMapArrays`, not `#include "Maps/Map1_1.h"` or direct references to generated map globals.
- `M_Tile` uses an Unlit `TileTexture` parameter connected to Emissive Color and has **Used with Instanced Static Meshes** enabled.
- Level 1 reproduces the reference implementation's **2,819 rendered tile instances** with its original export; other exported maps will have their own counts.
- An invalid level exits safely with a clear log.
- A genuine second export is registered as game-facing Level 2. Switching **Map → Level To Render** between 1 and 2 changes the built layout without editing `TileMapRenderer.cpp` or making a second `.umap`.
- At least one full editor build and a packaged Windows build are verified; `/Game/Tiles` is cooked.
- Screenshots, exact AI prompts, failures/corrections, changed files and a Git checkpoint are documented.

**Out of scope for this video:** finished C++ camera panning/zoom/bounds (Sprint 3), player, collision/decorations, UI, save/load, automatic real-time runtime map switching. A temporary Camera Actor is only a viewing tool.

---

# 1. Suggested recording segmentation and pacing

| Segment | Suggested screen/demo | Key result |
|---|---|---|
| Opening (3–5 min) | Sprint 1 checkpoint + simple data-flow diagram | Viewer understands handoff |
| A. Source checkpoint (4–6 min) | Git branch, inspect actual exports | Safe starting point |
| B. AI-driven renderer design (8–12 min) | Prompt, inspect proposed C++ class | Actor + HISM strategy |
| C. Build initial renderer (15–25 min) | Create class, compile, attach to `PlayGame` | Valid cells generate mesh instances |
| D. Materials and diagnosis (10–15 min) | `M_Tile`, `TileTexture`, instancing usage | Actual TileGen textures display |
| E. Data boundary/refactor (12–18 min) | `MapArrays` structure, registration, renderer changes | No generated-name coupling |
| F. Temporary viewing camera (4–6 min) | Actor placement + orthographic camera | Legible overhead map |
| G. Make/register Map 2 (15–25 min) | TileGen map save, C++ export, registry edit | Same renderer handles a new layout |
| H. Tests, package, close (8–12 min + package time) | Level 1/2/999, package, PR/doc | Reproducible milestone |

The order can be adapted to existing source: if beginning with the supplied *already-refactored* files, demonstrate the data boundary before inspecting the renderer, and show the single-map version only as a short before/after diff. The learner should never be asked to recreate a known broken version just for dramatic effect.

**Presentation convention below:** each stage has *Say* (spoken script), *Show/do* (click-by-click), *AI prompt* (copy into Agent Mode), and *Verify* (human acceptance gate). Read the spoken script naturally rather than reciting code verbatim.

---

# Opening — From arrays to a world

### SAY

> Welcome back to Grant Money. In the last programming sprint, we established a known-good Unreal C++ project, imported TileGen's texture library, and made its generated map arrays accessible through a small game-facing interface. But the map is still only **data**. An integer in `Ground[X][Y]` describes which texture should occupy a cell; it does not create anything in the level. Today we're going to write one renderer that converts this data into visible tiles. Then we'll generate a second map and prove the renderer can display either layout without being rewritten. We're using AI to accelerate the repetitive code and identify Unreal APIs, but a compiler and actual in-engine tests—not the AI response—determine whether the work is correct.

### SHOW/DO

1. Open `Game/GrantMoney.uproject` (or the learner's matching project).
2. Open `Content/Maps/PlayGame`; show the blank/previous Sprint 1 stage.
3. Show `Content/Tiles` and a sample imported 64×64 texture.
4. Open the project's generated map header/asset header and `MapArrays` boundary.
5. Draw/show this diagram:

```text
PlayGame (.umap — container for Actors)
       |
       v
ATileMapRenderer (C++ — HOW to draw)
       |
       v
FMapArrays (C++ — WHICH data/texture paths)
       |
       v
TileGen generated arrays and tile-asset tables
       |
       v
HISM plane instances using M_Tile + TileTexture
```

### VERIFY

- No one is manually dragging thousands of planes into `PlayGame`.
- `PlayGame.umap` and a TileGen-generated C++ map are distinct things.

---

# A — Protect the Sprint 1 checkpoint and inspect real files

### SAY

> First, we preserve the previous milestone. An AI agent needs to inspect the source that actually exists. A tutorial's example filenames are not a license to invent C++ symbols. In this version our generated export uses `Map1_1` and its associated validity and asset-path tables; your generated filenames may differ.

### SHOW/DO

1. Close Unreal before switching branches or changing reflected C++ headers.
2. In Git Bash, from the **repository root**, run (only if the current working tree is clean):

```bash
git status
git switch main
git pull origin main
git lfs pull
git switch -c GMS-XX-sprint2-world-renderer
```

3. Open `Game/GrantMoney.uproject` and the C++ solution in Visual Studio. Ensure **Development Editor / Win64** is available.
4. Inspect `Game/Source/GrantMoney/Private/Maps/` (or the actual verified generated-data location). Read the symbols declared by `Map1_1.h` and `Map1_1_TileAssets.h`.
5. Look for `MapArrays.h/.cpp` from Sprint 1. The **uploaded target implementation** contains:

```text
Public/MapArrays.h
Private/MapArrays.cpp
Public/TileMapRenderer.h
Private/TileMapRenderer.cpp
Private/Maps/Map1_1.cpp
Private/Maps/Map1_1.h
Private/Maps/Map1_1_TileAssets.h
```

6. If you notice duplicate `.cpp/.h` exports parked in `Content/Maps`, do not use those as authoritative compiled sources: Unreal assets live under `Content`, C++ build sources under `Source`. Preserve original exports in the tooling documentation if desired, and verify only the intended compiled copy is active.

### AI PROMPT A — Repository audit (ChatGPT/Copilot Agent Mode)

```text
Inspect my GrantMoney Unreal Engine C++ repository before writing code.
Start at the end of Module 1 Programming Sprint 1: PlayGame, imported
/Game/Tiles textures, and generated TileGen map data exist. Find the actual
map arrays, validity arrays, tile asset lookup tables and any MapArrays
interface. Report the exact filenames, symbols, array dimensions, types,
include paths, and current build organization. Ground arrays must remain
16-bit and indexed [X][Y], validity arrays 8-bit. Identify whether this
export is individual-map or consolidated map-set format. Do not modify
anything yet; list the smallest changes needed to render Level 1 and later
register a distinct Level 2. Do not invent filenames or C++ symbols.
```

### VERIFY

- The agent's description matches the real files.
- Preserve a working backup/commit before any large refactor.
- If `MapArrays` was supplied by Sprint 1 already, **reuse** it; do not build an unrelated second interface.

---

# B — Ask AI to design a minimal renderer

### SAY

> We could spawn one Actor for every tile, but that turns a small data grid into thousands of independently managed game objects. Instead, we'll use Unreal's Hierarchical Instanced Static Mesh components. Each distinct tile type gets one component; its many squares become instances. That keeps the implementation simple and avoids creating thousands of Actors.

### SHOW/DO

1. Show `Map1_1`'s 64×64 arrays. Remind the audience that a map has **up to 4,096 cells**, not necessarily 4,096 rendered tiles; invalid and ID-zero cells are skipped.
2. Show/illustrate coordinate conversion: `X * 100`, `Y * 100`, `Z = 0` Unreal units. The built-in plane is approximately 100×100 units.
3. Explain the role of an Unreal **Actor**: a placeable C++ class whose `BeginPlay` can build the world. The map does not need per-frame `Tick`.

### AI PROMPT B — Renderer plan

```text
Plan a minimal Unreal C++ Actor named ATileMapRenderer that converts the
existing 64x64 TileGen ground/validity data into a visible flat world.
Use UHierarchicalInstancedStaticMeshComponent grouped by tile ID, Unreal's
/Engine/BasicShapes/Plane.Plane, TileSize 100 Unreal units, and disable tile
collision for this first visual milestone. Build once from BeginPlay; disable
Tick. Skip invalid cells and tile ID 0. Resolve tile paths from the generated
lookup behind FMapArrays, not by constructing filenames. Explain component
ownership, material assignment, and how you will log rendered tile counts.
Do not implement a player, menus, Blueprint gameplay, camera movement,
decoration, collision layers or runtime map switching. Give the design and
file list first; wait for review before editing.
```

### VERIFY

- The proposed renderer uses HISM rather than 2,819 independent Actor objects.
- The renderer consumes map data through `MapArrays` (or the team agrees to a short temporary prototype followed by the boundary refactor in Section E).

---

# C — Create `TileMapRenderer` and prove geometry before textures

### SAY

> Unreal requires an Actor class for an object we place into a level. We'll create `TileMapRenderer` inside Unreal Editor, but its gameplay logic lives in Visual Studio as ordinary C++. Our first test will draw valid map cells with a diagnostic material. We are deliberately separating geometry errors from texture errors.

### SHOW/DO — Detailed steps

1. In Unreal Editor, choose **Tools → New C++ Class** (or the engine-version equivalent).
2. Choose parent class **Actor**, not Pawn, Character, Blueprint or GameMode.
3. Name the class `TileMapRenderer`; Unreal generates `ATileMapRenderer` in C++.
4. Select a normal module location (`Public` header, `Private` source is appropriate if offered). Create the class.
5. Open the generated `.h` and `.cpp` in Visual Studio. Preserve Unreal-generated module export macro (e.g. `GRANTMONEY_API`) and `TileMapRenderer.generated.h` placement.
6. Ask Copilot to implement the reviewed plan, targeting the actual `MapArrays` API. The uploaded target `.h` exposes `LevelToRender`; the uploaded `.cpp` already includes only `MapArrays.h`.
7. Ensure construction creates `SceneRoot`, obtains the built-in Plane mesh and attempts to load `/Game/Materials/M_Tile.M_Tile`. Disable Tick.
8. In `BeginPlay`, log `TileMapRenderer starting...` then call `BuildMap()`.
9. In `BuildMap`, check plane/material availability, obtain `FGrantMoneyMapData`, call `IsValid`, loop over `MapData.Width` and `.Height`, skip `!Valid[X][Y]` and ID-zero cells, obtain/create a tile-type HISM group, and call `AddInstance` using `FVector(X*100, Y*100, 0)`.
10. For the geometry-only diagnostic, use one known material on the HISM groups before adding dynamic texture parameters. This may be temporary; the finished submitted source uses dynamic materials.
11. **Save code; close Unreal for reflected header/class changes.** Build `GrantMoneyEditor` using **Development Editor / Win64** in Visual Studio (Build → Build Solution, Ctrl+Shift+B), reopen the project.
12. In the Content Browser, show C++ Classes if hidden. Drag `TileMapRenderer` into `PlayGame` and set its Actor transform to Location `(0,0,0)`, Rotation `(0,0,0)`. Save the level.
13. Press Play and examine **Output Log**; F8 can eject from the current camera to inspect the generated mesh instances in the world.

### AI PROMPT C — Implement with a test gate

```text
Implement the approved ATileMapRenderer in the existing GrantMoney module.
Use the ACTUAL public MapArrays interface to request selected Level 1; keep
generated filenames out of TileMapRenderer.cpp. Add a public EditAnywhere
UPROPERTY in category Map named LevelToRender, default 1, minimum 1. Create
one HISM component per tile ID, attach to a SceneRoot, set the built-in
plane mesh, disable collision and Tick, and create instances using generated
[X][Y] coordinates and 100-unit spacing. Add clear BeginPlay and final tile
count logs. Before applying actual textures, make it possible to test with a
single diagnostic material. Validate pointers and dimensions and fail
cleanly for unsupported levels. Preserve any already-working renderer logic.
Change only necessary C++ files; do not alter PlayGame, the generated arrays,
or unrelated assets. Report modified files and compile-test status honestly.
```

### VERIFY

- `ATileMapRenderer` exists in the level Outliner.
- The Output Log says it started and gives a nonzero final tile count.
- If reproducing the same original Map 1 and current exclusion logic, the uploaded reference expects **2,819 instances**. If different, inspect missing/extra cells before claiming equivalence.
- Runtime generated meshes need not be visible in the Edit viewport before Play.

**Common mistake from development:** `fatal error C1083: Cannot open include file: 'Maps/Map1_1.h'` means include paths and generated source/header placement do not match. Our working prototype stored the generated files in `Source/GrantMoney/Private/Maps/`. Do not “fix” the error by pasting arrays into `TileMapRenderer.cpp`.

---

# D — Create the tile material and display actual TileGen textures

### SAY

> A tile ID has to become an Unreal texture. We will make one reusable material with a texture parameter. Each HISM tile-type group receives a dynamic material instance using its own texture. This is more maintainable than manually building a material asset for every generated PNG.

### SHOW/DO — Material

1. In `Content/Materials`, create a Material named **exactly** `M_Tile`.
2. Open it. Set **Shading Model = Unlit** for this first overhead rendering test.
3. Add a **Texture Sample Parameter 2D** and name the parameter **exactly** `TileTexture`.
4. Connect **RGB → Emissive Color**. Apply/Save.
5. Click the material graph background, locate **Usage**, and enable **Used with Instanced Static Meshes**. Apply/Save and allow shader compilation.
6. Confirm the renderer's material reference matches its real asset path: `/Game/Materials/M_Tile.M_Tile`. Right-click the asset in the Content Browser and **Copy Reference** if unsure.
7. For each tile group, use `FMapArrays::GetTileAsset(LevelToRender, TileId)`, load `UTexture2D`, create `UMaterialInstanceDynamic`, set `TileTexture`, then `TileGroup->SetMaterial(0, DynamicMaterial)`.
8. Ensure assets actually exist in `Content/Tiles` and their lookup paths use `/Game/Tiles/...`, not a Windows path.
9. Save, test in PIE and inspect Output Log for missing tile textures.

### AI PROMPT D — Material handling

```text
Extend the working HISM TileMapRenderer to show actual TileGen textures.
The base material is /Game/Materials/M_Tile.M_Tile and contains a
TextureSampleParameter2D named TileTexture (RGB connected to Emissive; Unlit;
Used with Instanced Static Meshes enabled in the editor). For each new tile
ID, call FMapArrays::GetTileAsset(LevelToRender, TileId), reject null paths,
load its UTexture2D, make a dynamic material instance and apply that texture
to material slot 0 of that tile-type HISM component. Cache one group per tile
ID, not one material per cell. Log unusable IDs once per ID instead of once
per cell. Preserve map layout and existing tile count. Include precise manual
Unreal material-editor settings because C++ cannot substitute for enabling
the material usage flag in the asset.
```

### Troubleshooting demonstration — use real lesson learned

If the renderer log shows created tiles but the world displays a grey default checker:

```text
LogMaterial: Warning: Material /Game/Materials/M_Tile.M_Tile
missing usage flag InstancedStaticMeshes! Default Material will be used in game.
```

**Fix:** Open `M_Tile` → enable **Used with Instanced Static Meshes** → Apply/Save. To isolate asset-loading errors, temporarily connect a vivid solid color to Emissive. If the tiles then show that color, mesh/camera/material usage are working; investigate the texture paths/parameter next. Restore `TileTexture` afterward.

If `TileMapRenderer: Could not load M_Tile` appears, confirm material location, Copy Reference, save and restart the Editor; the sample constructor uses `ConstructorHelpers::FObjectFinder`.

If `Could not load texture for Tile ID ...` appears, inspect the exact string in the generated `*_TileAssets.h`, the Unreal import destination and the corresponding asset in `/Game/Tiles/`.

### VERIFY

- Textured map visible, not default checker or solid debug color.
- `M_Tile` has the instancing flag saved; warning does not recur on every editor launch.
- The reference Level 1's 2,819 tiles render with appropriate textures.

---

# E — Protect the renderer from hardcoded generated filenames (`MapArrays`)

### SAY

> Our first prototype made the ground map appear by directly including `Map1_1`. That proved the concept, but it created a maintenance problem: each newly generated map would require changes to the renderer. In the prior sprint the sponsor established a map-data boundary for this reason. Now we finish connecting the visible-world code to that boundary. **The renderer renders; MapArrays selects the source.**

### SHOW/DO — Explain the uploaded ACTUAL implementation

The uploaded target files contain:

**`Public/MapArrays.h`:**

```cpp
struct GRANTMONEY_API FGrantMoneyMapData
{
    static constexpr int32 MapSize = 64;
    const uint16 (*Ground)[MapSize] = nullptr;
    const uint8 (*Valid)[MapSize] = nullptr;
    int32 Width = 0;
    int32 Height = 0;

    bool IsValid() const
    {
        return Ground && Valid && Width > 0 && Width <= MapSize
            && Height > 0 && Height <= MapSize;
    }

    void Reset() { *this = FGrantMoneyMapData(); }
};

class GRANTMONEY_API FMapArrays
{
public:
    static bool GetMap(int32 Level, FGrantMoneyMapData& OutMap);
    static const TCHAR* GetTileAsset(int32 Level, int32 TileId);
};
```

The pointers **borrow** the compiled generated arrays; they do not duplicate the 64×64 data. IDs stay `uint16`, validity `uint8`, and the exported indexing remains `[X][Y]`.

**`Private/MapArrays.cpp`:** it includes the real generated map headers and stores a small registry. The uploaded version currently contains:

```cpp
static const FRegisteredMap RegisteredMaps[] =
{
    { 1, Map1_1, Map1_1_Valid, Map1_1_TileAssets, Map1_1_TileAssetCount },
};
```

Its `GetMap()` resets the outgoing structure, finds the entry and sets the pointers plus 64×64 dimensions. Its `GetTileAsset()` returns `nullptr` for empty ID 0, unsupported levels, out-of-range IDs and missing/empty paths.

**`Public/TileMapRenderer.h`:** `LevelToRender` is exposed to Unreal's Details panel:

```cpp
UPROPERTY(EditAnywhere, Category = "Map", meta = (ClampMin = "1"))
int32 LevelToRender = 1;
```

**`Private/TileMapRenderer.cpp`:** the sole project-level map include is:

```cpp
#include "MapArrays.h"
```

Its `BuildMap` retrieves and checks the selected map:

```cpp
FGrantMoneyMapData MapData;
if (!FMapArrays::GetMap(LevelToRender, MapData))
{
    UE_LOG(LogTemp, Error,
        TEXT("TileMapRenderer: Failed to load level %d. It is not registered in MapArrays."),
        LevelToRender);
    return;
}
if (!MapData.IsValid())
{
    UE_LOG(LogTemp, Error, TEXT("TileMapRenderer: Invalid map data."));
    return;
}
```

It then loops over `MapData.Width`/`.Height`, reads `MapData.Valid[X][Y]` and `MapData.Ground[X][Y]`, and requests textures through `FMapArrays::GetTileAsset(LevelToRender, TileId)`. It groups by tile ID using HISM and logs its total.

### AI PROMPT E — Refactor (use only if not already done)

```text
Refactor my existing working ATileMapRenderer to honor the Sprint 1
MapArrays abstraction. Inspect the actual generated declarations. Create
or ADAPT MapArrays.h/.cpp, do not duplicate an existing interface. Expose
GetMap(Level, OutMap) and GetTileAsset(Level, TileId), using const pointers
to compiled 16-bit ground and 8-bit validity arrays indexed [X][Y]. Keep
only generated-file includes and symbols in MapArrays.cpp; put registered
level-to-export entries in one compact table. Add LevelToRender as a public
EditAnywhere UPROPERTY with default 1. Replace all direct Map1_1 references
in TileMapRenderer.cpp with MapData fields and GetTileAsset. Preserve HISM,
100-unit placement, M_Tile/TileTexture and logging. Reject unknown levels
without a crash. Do not invent a second map, modify generated numbers or
implement runtime map switching. Present a diff and test checklist first.
```

### Human test of refactor

1. Save all code. Because `UPROPERTY`/header changes affect Unreal reflection, **close the Editor**, then build **Development Editor / Win64**; do not rely on Live Coding for this change.
2. Open `PlayGame` and select the `TileMapRenderer` Actor.
3. In Details → **Map → Level To Render**, select `1` and press Play.
4. For the same original export, expect `Created 2819 tiles` and working textures.
5. Stop Play; set Level To Render to `999`, Play and check for a helpful not-registered error with no crash or tiles. Stop and restore `1`.
6. Capture screenshots and save `PlayGame` with Level 1 selected.

**Record accurately:** The supplied integration note says the Copilot agent attempted a build but was blocked because the Editor/Live Coding was running; its note lists tests A/B/C as pending. Do not state that agent automation verified those tests. Show the human build and PIE verification on the recorded baseline.

---

# F — Put the world on camera (temporary validation view only)

### SAY

> A generated map can exist even when our gameplay viewport looks black. The Actor might be correctly placing geometry but the active camera can be somewhere else—or pointed horizontally. We'll use a temporary overhead Camera Actor to inspect the result. The actual C++ navigation, zoom controls and world bounds belong to Programming Sprint 3.

### SHOW/DO

1. Select the `TileMapRenderer` Actor. For this sample set its location and rotation to `(0,0,0)`.
2. Place a **Camera Actor** into `PlayGame`.
3. Enter initial Location `(3150,3150,7000)` (the center of a 64×64 grid spaced by 100 units is near `3150,3150`).
4. Enter **Rotation X/Roll = 0, Y/Pitch = -90, Z/Yaw = 0**. In the team's first test the mistaken X=-90 rotated the camera rather than aiming down.
5. Set Projection Mode **Orthographic**, Ortho Width **10,000** (the working value in this development session; adjust for viewport aspect ratio and desired margins).
6. Set **Auto Activate for Player → Player 0**; save level.
7. Press Play; use F8 to eject/inspect if the game viewport is black. Verify the actual active view target before assuming the renderer failed.

### AI PROMPT F — Camera diagnosis, not final implementation

```text
We have a 64x64 flat tile grid, 100 Unreal units per tile, built in BeginPlay
by a C++ Actor at world origin. Give me a minimal temporary Camera Actor
validation setup for Unreal: expected center/location, orthographic viewing,
correct Transform rotation axis for looking straight down, and how to check
whether the camera is the active Player 0 view target. Do not add Blueprint
gameplay or a final movement/zoom system; those are next sprint.
```

### VERIFY

- The editor camera/game viewport sees the correct map; map assets themselves need not appear in the browser as a new `.umap`.
- If this capture belongs to Sprint 2, label the camera **temporary visualization**, not the finished camera system.

---

# G — Produce a real second map using TileGen and register Level 2

**This is the main new demonstration.** A configurable `LevelToRender` property alone does not prove multi-map support; we must create and register a genuinely different export.

## G1. Preserve/export the original tile library

### SAY

> We're not going to regenerate every texture or randomly rename the files. Tile IDs refer to the exported tile library. If that relationship changes, the same map array can display a different image without any change to its numbers. For this test we'll keep the existing PNG filenames stable and generate a different arrangement from the same library.

### SHOW/DO

1. Close the running game. Open the version of **TileGen used for the existing export** (or build/run its provided binaries if not installed). Do not modify the existing working export in place without a copy.
2. Create a clearly separated backup folder and export folder outside OneDrive, such as:

```text
C:\Dev\TileGenWorkspace\Tiles\         # unchanged source PNG library
C:\Dev\TileGenWorkspace\MapExports\   # working output
C:\Dev\TileGenWorkspace\BackupMap1\   # original generated package
```

3. Copy the exact original tile PNG library into `Tiles`. Check unique names and 64×64 size. Preserve original `Map1_1.cpp/.h/_TileAssets.h` and import script in the backup/repository tooling folder.
4. In TileGen **Map Generator**, browse to `C:\Dev\TileGenWorkspace\Tiles`. Check that the palette contains the expected original tiles.
5. Set the map-output Path to `...\MapExports`. Use the generator's level/map setting deliberately; record the generated export filenames rather than assuming the UI's level field automatically equals game-facing Level 2.

## G2. Draw a visibly different layout and export it

### SAY

> We'll make the difference obvious enough that a viewer can recognize it instantly when we change `Level To Render`. The second layout should not merely change the camera. Its TileGen ground and playable-cell arrays should actually differ.

### SHOW/DO

1. Start from the familiar **Open arena** setting; optionally use **Border wall**. Click **Generate Map**.
2. In the **Ground** layer, choose an existing ground tile and use **Fill** on the valid connected area.
3. Use another existing tile with Paint/Rectangle/Line to make a large distinctive shape—for example, a wide cross, a central dirt square plus paths, or two contrasting regions. Keep all edits inside the valid arena.
4. Save the map with **Save Map**. TileGen may increment the export/version naming (`Map1_2` in the format used by our current source), but **inspect the generated three filenames and the symbols inside them**. A UI label such as “Level 2” is not proof that C++ symbols are `Map1_2`.
5. Confirm the new generated ground array, validity array and tile-asset lookup header correspond to the same export. Open the headers and record their exact names, e.g. *if* the export really is `Map1_2`:

```text
Map1_2.cpp
Map1_2.h
Map1_2_TileAssets.h
```

6. Compare sample positions or a rendered/export screenshot with Map 1 and confirm a genuine change. Retain the export/source PNGs/screenshots for reproducibility.

### AI PROMPT G1 — Inspect TileGen's second export

```text
I made a second visually distinct map in TileGen using the SAME source PNG
library as Level 1. Inspect the new exported .cpp, .h and _TileAssets.h. Report
their actual generated symbols, dimensions, [X][Y] convention, tile ID type,
validity array and asset path table/count. Compare the important declarations
with the existing Map 1 export. Identify duplicate global symbols, missing
files or changed filename-to-ID assignments BEFORE we add it to Unreal. Do
not rewrite thousands of numeric values or assume the export is named
Map1_2 just because this is our game-facing Level 2.
```

## G3. Import missing textures only if required

### SHOW/DO

1. If Map 2 reuses the exact same original tile PNG library and all paths already resolve in `Content/Tiles`, **do not import duplicate textures**. Compare the second `_TileAssets.h` values with the actual `/Game/Tiles` assets.
2. If Map 2 introduces additional PNGs, create/review TileGen's `import_tiles_to_unreal.py`. Correct its **source directory** for this machine, and retain the Unreal destination `/Game/Tiles`.
3. In Unreal, enable/use **Python Editor Script Plugin** and **Editor Scripting Utilities** as taught in Sprint 1. Execute the import via **Tools → Execute Python Script**, then Save All.
4. Verify texture dimensions/filtering and confirm newly referenced asset paths exist. In **Project Settings → Packaging**, confirm `/Game/Tiles` is in **Additional Asset Directories to Cook**.
5. If the new export references a filename that conflicts with an existing Unreal asset but has different intended art, resolve and re-export the library deliberately; do not silently replace Level 1 art.

## G4. Integrate Map 2 into C++

### SAY

> Here is the payoff from our new architecture. To add another game-facing map, we add the generated export to our source tree and register it in ONE boundary file. We should not change a line of `TileMapRenderer.cpp`.

### SHOW/DO — Exact example for a `Map1_2` export

1. Stop PIE and close Unreal for C++ integration.
2. Copy the actual second export's three C++ files into the same compiled generated-source location as Map 1:

```text
Game/Source/GrantMoney/Private/Maps/
    Map1_1.cpp
    Map1_1.h
    Map1_1_TileAssets.h
    Map1_2.cpp                 # example only: use generated name
    Map1_2.h
    Map1_2_TileAssets.h
```

3. Confirm the new `.cpp` includes the matching header and that the compiler will compile it through the Unreal module. Regenerate Visual Studio project files if needed.
4. In **`Game/Source/GrantMoney/Private/MapArrays.cpp` only**, add the second export includes, assuming the inspected symbols match:

```cpp
#include "Maps/Map1_1.h"
#include "Maps/Map1_1_TileAssets.h"
#include "Maps/Map1_2.h"
#include "Maps/Map1_2_TileAssets.h"
```

5. Expand the real existing registry inside `FindMap()`:

```cpp
static const FRegisteredMap RegisteredMaps[] =
{
    { 1, Map1_1, Map1_1_Valid, Map1_1_TileAssets, Map1_1_TileAssetCount },
    { 2, Map1_2, Map1_2_Valid, Map1_2_TileAssets, Map1_2_TileAssetCount },
};
```

6. **Do not paste these exact names if the export uses different symbols.** Read the new header and substitute its identifiers. If the export is a consolidated map set, adapt the registry to the directory table rather than inventing individual arrays.
7. Review `MapArrays.h`: its current `MapSize=64` and fixed pointer-to-row types mean both compiled exports must actually be 64×64. If the output format differs, stop and reconcile the data contract rather than coercing types unsafely.
8. **Do not edit `TileMapRenderer.cpp` for Level 2.** That is the acceptance test of the architecture.
9. Save, build **Development Editor / Win64**, inspect first compiler error if any, then reopen Unreal.

### AI PROMPT G2 — Add Level 2 with minimal change

```text
Register this real second TileGen export as game-facing Level 2. Inspect the
actual new header/symbols and the current MapArrays.cpp registration table.
Copy/include the generated export correctly under the existing Source/...
Private/Maps layout; do NOT change the generated array values. Update ONLY
the necessary generated source placement and MapArrays.cpp includes/registry,
using the real 16-bit Ground, 8-bit Valid, TileAssets and TileAssetCount
symbols. Ensure both exports compile with no duplicate globals or bad paths.
Preserve Level 1 registration. IMPORTANT: do not modify TileMapRenderer.cpp,
its material, or PlayGame for the Level 2 data addition. Explain every diff
and list manual checks. Do not claim the second map has rendered unless we
run Unreal and demonstrate it.
```

### VERIFY — Actual on-camera map-switch demonstration

1. In `PlayGame`, select the ONE `TileMapRenderer` Actor in the Outliner.
2. Under **Details → Map → Level To Render**, leave `1`, press Play and show its original shape and the Output Log tile count. Stop Play.
3. Set **Level To Render = 2** (the new public `UPROPERTY`), press Play and show the DISTINCT TileGen map shape. Capture the new tile count—it is not required to be 2,819. Stop Play.
4. Re-select `1`, Play again, and verify the original shape returns. Save the level in whichever default state the team wants, generally Level 1.
5. Set `999`, Play: expected `Failed to load level 999... It is not registered in MapArrays.` and no crash. Restore `1` and save.
6. Show the source diff proving the additional map required **no change to the renderer**.

**Clarify for viewers:** this is *configurable map selection before Play*, not live map switching while a session is running. `BuildMap()` runs once in `BeginPlay`. A future `LoadLevel()` would need to destroy/release prior `TileGroup_*` components before rebuilding; do not advertise that feature yet.

---

# H — Final integration, packaging and AI-workflow review

### SAY

> We have now separated three jobs: TileGen produces map data and texture identities, `MapArrays` registers the maps under game-facing level numbers, and `TileMapRenderer` converts whichever map is selected into Unreal geometry. We used AI for the initial design, repetitive C++ implementation and refactor, but the decisive tests were ours: the source compiled, the world was visible, the material was genuinely compatible with HISM, invalid levels failed safely, and a second export rendered without changing the renderer.

### SHOW/DO — Final checks

1. **Build:** close Unreal, Visual Studio Build → Build Solution, **Development Editor / Win64**. Capture successful build output.
2. **Level 1:** after opening `PlayGame`, choose Level 1. The original reference export should report `TileMapRenderer finished. Created 2819 tiles.`; verify textured image/no default material.
3. **Level 2:** choose Level 2; capture different layout, its actual instance count and no repeated missing-asset warnings.
4. **Invalid:** choose 999; capture safe error, restore the chosen valid default.
5. **Cooking:** Edit → Project Settings → Packaging: required `PlayGame` level included and `/Game/Tiles` listed among additional cooked directories. Verify shared `Config` changes are saved.
6. **Package Windows Development build** into a local output folder OUTSIDE `Game/`/Git history. Launch its executable without Unreal Editor. Confirm the selected level and tile textures are present. If the build fails, record the actual output and do not call packaging passed.
7. **Clean clone (recommended):** on another team machine, clone repo + `git lfs pull`, regenerate/build C++ binaries, open `PlayGame` and repeat Level 1/2 tests. TileGen itself does not need to run to play an already imported/compiled map, but its source/export package should be available to reproduce authoring.
8. Capture a final screenshot, log excerpt, source tree and meaningful prompt/decision record. Preserve exact PR/commit link and mention any deferred defects.

### AI PROMPT H — Verification audit and draft documentation

```text
Review the actual code diff for Module 1 Programming Sprint 2. Audit these
requirements: renderer accesses maps only via FMapArrays; ground IDs uint16,
validity uint8, [X][Y]; TileGen asset table used rather than guessed paths;
HISM one group per ID; TileSize 100; M_Tile TileTexture parameter; safe
unsupported level behavior; genuine Level 2 registration; no unnecessary
Blueprint gameplay or runtime switching. Give a manual Unreal testing matrix
for Level 1, Level 2, invalid 999 and a packaged Windows build. Write a
clear field-manual draft from steps ACTUALLY completed, and a list of common
mistakes/AI corrections. Do not call unperformed tests 'passed'.
```

## Testing matrix to complete before publishing the video

| Test | Observable pass criterion | Actual result / evidence |
|---|---|---|
| Clean editor build | Zero blocking compiler/linker errors | **Record during capture** |
| Level 1 | Existing textured shape, reference count 2,819 | **Record during capture** |
| Level 2 | Visibly different real TileGen export, no renderer edit | **Not proven by the initial supplied files; perform on camera** |
| Invalid Level 999 | Not-registered log; safe empty result | **Record during capture** |
| M_Tile instancing | No `InstancedStaticMeshes` missing-usage warning | **Record during capture** |
| Asset resolution | No missing tile textures | **Record during capture** |
| Windows package | Starts standalone with cooked textures and selected level | **Record during capture** |
| Independent reproduction | Different machine can build and run | **Recommended; record when done** |

---

# Troubleshooting insert (short on-camera appendix)

| Symptom | Show/verify | Corrective action |
|---|---|---|
| C1083 missing `Maps/...h` | Actual Source folders + includes | Use matching include path; keep generated code in module source tree, don't copy numeric arrays |
| 2,819 tiles logged but all grey default checker | Output Log material warning | Enable **Used with Instanced Static Meshes** on `M_Tile`; Apply + Save |
| Black game view but F8 shows tiles | Camera transform and active view | Correct Pitch Y = -90, camera active Player 0, temporary Ortho Width ~10,000 |
| `Could not load M_Tile` | Copy Reference | Confirm `/Game/Materials/M_Tile.M_Tile`, save/restart |
| `Could not load texture for Tile ID...` | Asset-table string and Content Browser | Import missing PNGs to `/Game/Tiles`; fix export references, no arbitrary filenames |
| Selected Level 2 draws no tiles | `GetMap` and registry | Check generated names and registration row; full C++ rebuild |
| Level 2 compiles but looks identical | Ground/Valid arrays compared | Make an actually distinct second map, verify you're selecting 2 and watching runtime view |
| Second export duplicate-symbol compiler error | Both export headers | Resolve generator export naming/collision at source or via a controlled adaptation, keep generated data traceable |
| New property absent in Details | Header build status | Close Editor, full Development Editor build, reopen; select C++ Actor instance |
| Package missing tile textures | Packaging config | Cook `/Game/Tiles`; test standalone, not only PIE |

---

# Engineering/AI discussion to use in closing narration

> The first useful AI answer is rarely the final architecture. Our original AI-assisted prototype rendered one generated array directly, which allowed us to test HISM, materials and camera rapidly. We then adapted that proof to the sponsor's `MapArrays` boundary. This is a deliberate AI-native workflow: ask for a bounded implementation, inspect its assumptions, test the smallest visible result, then refactor to the established system contract. The second map is the evidence that the design works: when the data changes, the renderer does not have to.

### Example AI workflow log entry (edit to actual observed details)

```text
Tool: GitHub Copilot Agent / ChatGPT
Goal: Turn TileGen arrays into a visible, reusable world
Initial result: Single-map renderer directly used Map1_1 globals
Decision: Modified
Reason: Coupled renderer to generated map filename; contradicted Sprint 1 boundary
Human verification: Built Level 1, diagnosed missing material usage flag,
  confirmed 2,819 tile instances, switched to FMapArrays, then registered
  and tested a distinct second map (complete this line only after testing)
Evidence: PR link, screenshots, Output Log, package result
```

### Recommended repository outputs

```text
docs/course/Module1-BuildingAWorld/Sprint2/
    README.md                    # this lesson, edited after verification
    images/
        01-sprint1-handoff.png
        02-tilerenderer-code.png
        03-geometry-diagnostic.png
        04-material-settings.png
        05-level1-rendered.png
        06-maparrays-registry.png
        07-tilegen-map2.png
        08-level2-rendered.png
        09-invalid-level-log.png
        10-packaged-build.png
```

**Suggested PR title:** `GMS-XX: render registered TileGen maps and verify Level 2`  
**Suggested commit:** `GMS-XX: add reusable map renderer and second TileGen export`  
**Sprint 3 handoff:** Replace temporary camera with C++ overhead navigation, controlled zoom, boundaries, lighting and multi-map exploration.
