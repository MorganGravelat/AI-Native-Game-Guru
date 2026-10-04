# Copilot Agent Task — Refactor GrantMoney TileMapRenderer for Dynamic Multi-Map Support

## 1. Project Context

You are working as a C++ gameplay engineer on **GrantMoney**, an Unreal Engine 5.8 project developed for the AI-Native Game Guru capstone.

The current project has an operational TileGen-to-Unreal rendering pipeline.

TileGen is an external map-generation tool that exports 64×64 tile maps as C++ arrays, along with header files containing tile asset lookup information.

We have successfully implemented a C++ Actor named `ATileMapRenderer` that reads the generated `Map1_1` data, constructs tile geometry using `UHierarchicalInstancedStaticMeshComponent` (HISM), loads Unreal textures, and applies them through dynamic material instances.

### Current known working state

- The Unreal project compiles successfully.
- The `PlayGame` level exists.
- `TileMapRenderer` is placed inside `PlayGame`.
- The renderer currently creates 2,819 tile instances from the existing Map 1 export.
- The map displays the correct TileGen textures.
- Tile rendering uses Unreal's built-in Plane mesh.
- The base material is `/Game/Materials/M_Tile`.
- The material contains a texture parameter named `TileTexture`.
- The material has **Used with Instanced Static Meshes** enabled.
- The map uses an overhead orthographic camera.
- The temporary camera uses Pitch -90° and Ortho Width 10,000.
- The project is stored in GitHub and uses Git LFS for Unreal binary assets.

**This is a working implementation. Preserve that functionality throughout this task.**

Do not rebuild the renderer from scratch when a targeted refactor can accomplish the objective.

---

## 2. The Problem We Need to Solve

The current `TileMapRenderer.cpp` is hardcoded to one particular TileGen export.

It directly includes files similar to:

```cpp
#include "Maps/Map1_1.h"
#include "Maps/Map1_1_TileAssets.h"
```

It also directly accesses variables such as:

```cpp
Map1_1[X][Y]
Map1_1_Valid[X][Y]
Map1_1_TileAssets[TileId]
Map1_1_TileAssetCount
```

This creates an architectural problem.

If we generate `Map1_2`, `Map1_3`, or additional map sets, we should not need to rewrite the renderer or change its generated-file includes.

The renderer should know **how to render a map**, not **where a specific generated map came from**.

We want to separate the two responsibilities.

### Intended architecture

```text
PlayGame
   |
   v
ATileMapRenderer
   |
   | Requests LevelToRender
   v
FMapArrays
   |
   | Resolves selected level
   v
Generated TileGen Map Data
   |
   | Returns map arrays and tile asset paths
   v
ATileMapRenderer
   |
   | Builds HISM instances
   v
Visible Unreal World
```

This architecture follows the sponsor's original Module 1 Programming Sprint 1 design, which introduces a small `MapArrays` boundary between generated TileGen data and game systems.

---

# 3. Your Primary Objective

Refactor the existing implementation to support a **data-driven, configurable map-selection system**.

The finished implementation should allow a developer to select a registered map through a property on the `TileMapRenderer` Actor in Unreal Editor.

For example:

```text
TileMapRenderer
    |
    +-- Map
         |
         +-- Level To Render: 1
```

If Level 2 has been registered, changing this property to `2` should make the renderer construct Level 2 when gameplay starts.

The renderer itself must no longer directly depend on the filenames or global array symbols of a particular TileGen export.

## Primary requirements

1. Introduce `MapArrays.h` and `MapArrays.cpp`, or adapt existing versions if they are already present.
2. Define a common structure for accessing TileGen map data.
3. Implement a function for resolving game-facing level numbers to generated map data.
4. Implement a function for resolving tile IDs to their Unreal asset paths.
5. Refactor `ATileMapRenderer` to consume this interface.
6. Expose the requested level through a C++ `UPROPERTY`.
7. Preserve the existing HISM rendering implementation.
8. Preserve current material and texture-loading behavior.
9. Validate map dimensions, pointers and tile IDs before accessing them.
10. Maintain the current successful rendering result for Level 1.

Do not introduce unnecessary gameplay systems, Blueprint gameplay logic, menus, save systems or camera changes.

---

# 4. First Action: Inspect the Existing Implementation

Before modifying any code, inspect the actual project.

Locate and read:

```text
Game/Source/GrantMoney/Public/TileMapRenderer.h

Game/Source/GrantMoney/Private/TileMapRenderer.cpp
```

Also inspect the generated map files, expected to be somewhere similar to:

```text
Game/Source/GrantMoney/Private/Maps/
```

Locate the real declarations for:

- Ground tile arrays.
- Playable-cell/validity arrays.
- Map dimensions.
- Tile asset lookup tables.
- Tile asset counts.
- Any existing map directory or map-set structures.

Search for any existing:

```text
MapArrays.h
MapArrays.cpp
```

If they already exist, inspect and adapt them instead of creating duplicate systems.

### Important inspection rules

Do not assume the generated symbols match the examples in this prompt.

The repository is the source of truth.

Check whether the current TileGen export uses individual map arrays such as:

```cpp
Map1_1
Map1_1_Valid
```

or a consolidated map-set export containing multiple maps and a directory table.

Support the format actually present in the repository.

Do not invent generated filenames, variable names or lookup structures.

Do not modify the contents of TileGen-generated arrays.

Do not manually copy thousands of tile IDs into new arrays.

Preserve the generated map data exactly as exported.

### Important type and indexing requirements

TileGen ground tile IDs are 16-bit unsigned values.

Validity data is 8-bit.

The arrays use:

```cpp
Ground[X][Y]
```

where X is the column and Y is the row.

Do not accidentally transpose the arrays.

Do not convert ground tile IDs into `uint8`, as this could truncate IDs above 255.

Before implementation, briefly identify the existing export format and the files you intend to modify.

---

# 5. Implement the MapArrays Interface

Create or adapt the following two files:

```text
Game/Source/GrantMoney/Public/MapArrays.h

Game/Source/GrantMoney/Private/MapArrays.cpp
```

Follow the project's existing Unreal module organization and include conventions.

The interface should return the actual map data needed by the renderer without copying the entire generated map.

A suggested conceptual structure is:

```cpp
struct FGrantMoneyMapData
{
    static constexpr int32 MapSize = 64;

    const unsigned short (*Ground)[64] = nullptr;
    const unsigned char (*Valid)[64] = nullptr;

    int32 Width = 64;
    int32 Height = 64;
};
```

You may use Unreal's fixed-width aliases (`uint16`, `uint8`) where appropriate, provided they exactly match the generated declarations.

Adjust this structure if necessary to match the actual generated map-set data safely.

The structure should contain only the information required by the renderer and associated map systems.

Do not make unnecessary copies of all 4,096 entries.

## Required interface functions

Provide functionality equivalent to:

```cpp
class FMapArrays
{
public:

    static bool GetMap(
        int32 Level,
        FGrantMoneyMapData& OutMap
    );

    static const TCHAR* GetTileAsset(
        int32 Level,
        int32 TileId
    );
};
```

You may choose slightly different signatures if the generated asset-path representation requires it.

### GetMap requirements

`GetMap()` must:

- Accept a game-facing level identifier.
- Return the correct ground array.
- Return the correct validity array.
- Provide the supported map dimensions.
- Reject unsupported level identifiers.
- Avoid returning dangling pointers.
- Reset the output structure to a safe invalid state when resolution fails.
- Avoid copying the generated tile arrays.

### GetTileAsset requirements

`GetTileAsset()` must:

- Resolve tile IDs using the correct generated tile asset table.
- Use the tile asset table corresponding to the selected map or map set.
- Handle tile ID 0 as an empty tile.
- Reject out-of-range tile IDs safely.
- Reject invalid or missing asset paths.
- Return a valid asset path for supported tile IDs.

Do not create filenames by assuming that Tile 1 is grass, Tile 2 is dirt, etc.

Use the generated lookup table.

Unreal tile asset paths should resolve beneath:

```text
/Game/Tiles/
```

Do not use Windows filesystem paths or `/Content/Tiles/` as runtime Unreal asset references.

---

# 6. Implement Map Registration

Initially register the existing known-working Level 1 map.

The interface must be structured so additional TileGen exports can be registered without modifying `TileMapRenderer.cpp`.

For an individual-map export format, a simple switch or explicit registration table is acceptable.

For example, conceptually:

```cpp
switch (Level)
{
    case 1:
        // Return the existing Map1_1 data.
        break;

    case 2:
        // Return Map 2 once a real export exists.
        break;

    default:
        return false;
}
```

**Do not reference Map 2 symbols unless the actual Map 2 files exist.**

If the repository already contains a consolidated TileGen map-set table, prefer using that directory rather than duplicating its information.

Keep all direct references to generated map names inside `MapArrays.cpp`.

The system should be easy to extend when new maps are generated.

---

# 7. Refactor TileMapRenderer.h

Modify the existing renderer header without removing functionality that the current implementation needs.

Add a C++ property equivalent to:

```cpp
UPROPERTY(
    EditAnywhere,
    Category = "Map",
    meta = (ClampMin = "1")
)
int32 LevelToRender = 1;
```

This should expose the selected level in Unreal Editor's Details panel.

The default must remain Level 1 to preserve the existing project behavior.

A developer should be able to select the existing `TileMapRenderer` Actor and change the level number without modifying C++ source code.

No Blueprint scripting should be introduced.

Continue to use a C++ Actor.

---

# 8. Refactor TileMapRenderer.cpp

Remove its direct dependency on the generated Level 1 headers.

Replace includes such as:

```cpp
#include "Maps/Map1_1.h"
#include "Maps/Map1_1_TileAssets.h"
```

with the public interface:

```cpp
#include "MapArrays.h"
```

The renderer should never directly reference:

```cpp
Map1_1
Map1_1_Valid
Map1_1_TileAssets
Map1_1_TileAssetCount
```

Instead, at the beginning of `BuildMap()`, request the selected map.

Conceptually:

```cpp
FGrantMoneyMapData MapData;

if (!FMapArrays::GetMap(LevelToRender, MapData))
{
    UE_LOG(
        LogTemp,
        Error,
        TEXT("Failed to load level %d."),
        LevelToRender
    );

    return;
}
```

Validate the returned data before rendering.

For example:

```cpp
if (!MapData.Ground || !MapData.Valid)
{
    UE_LOG(
        LogTemp,
        Error,
        TEXT("MapArrays returned invalid map pointers.")
    );

    return;
}
```

Validate the dimensions as well.

### Update the rendering loop

Replace hardcoded dimensions and arrays with the data returned by the interface.

Conceptually:

```cpp
for (int32 X = 0; X < MapData.Width; ++X)
{
    for (int32 Y = 0; Y < MapData.Height; ++Y)
    {
        if (!MapData.Valid[X][Y])
        {
            continue;
        }

        const int32 TileId = MapData.Ground[X][Y];

        if (TileId == 0)
        {
            continue;
        }

        // Continue using the existing tile-rendering logic.
    }
}
```

Use `FMapArrays::GetTileAsset()` for texture-path resolution.

Do not directly access generated asset tables from the renderer.

---

# 9. Preserve the Existing Rendering Implementation

The current renderer is known to work.

Do not replace it with a completely different rendering strategy.

Specifically, preserve:

- `UHierarchicalInstancedStaticMeshComponent`.
- Grouping tile instances by tile ID.
- Unreal's built-in Plane mesh.
- The existing tile spacing of 100 Unreal units.
- Dynamic material instances.
- The `TileTexture` texture parameter.
- The `M_Tile` base material.
- The existing world-position calculation.
- Disabled tile collision for this initial prototype.
- The current logging of generated tile counts.

Do not create thousands of independent Actor instances.

Continue to group identical tile types into HISM components.

The purpose of this task is to make the existing renderer reusable, not redesign its visual implementation.

### Preserve material compatibility

The base material is:

```text
/Game/Materials/M_Tile.M_Tile
```

Its texture parameter is:

```text
TileTexture
```

The material must support Instanced Static Meshes.

We previously encountered this error:

```text
Material missing usage flag InstancedStaticMeshes!
Default Material will be used in game.
```

Do not introduce changes that cause Unreal to display its grey default checkerboard material again.

---

# 10. Handle Invalid Data Gracefully

Improve error handling where necessary.

The renderer should not crash when:

- An unsupported level is requested.
- A map pointer is null.
- Map dimensions are invalid.
- A tile ID is zero.
- A tile ID is outside the asset table.
- A texture path cannot be resolved.
- A texture fails to load.
- The base material cannot be found.

Log useful information including the level number, tile ID and coordinates where appropriate.

Avoid producing thousands of identical warnings for the same missing texture.

Where practical, log texture-loading failures once per tile type rather than once per map cell.

An invalid selected level should result in a clear error and no attempted out-of-bounds rendering.

---

# 11. Avoid Unnecessary Runtime Complexity

For this first implementation, changing `LevelToRender` in the Unreal Details panel and pressing Play is sufficient.

The map can continue to be built once during `BeginPlay()`.

Do not introduce menus, level-transition screens, save systems, or asynchronous loading.

However, consider the future interface:

```cpp
void LoadLevel(int32 NewLevel);
```

The eventual implementation should be able to:

1. Validate the requested map.
2. Remove the old generated tile components.
3. Retrieve the new map.
4. Build the replacement map.
5. Update the current level.

Do not implement runtime map switching unless it can be added cleanly without destabilizing the existing renderer.

If runtime switching is implemented, ensure old HISM components are properly unregistered/destroyed and references cleared so maps do not stack on top of each other.

Otherwise, document runtime switching as future work.

---

# 12. Testing and Acceptance Criteria

Once the refactor is complete, test it against the existing Level 1 first.

## Test A — Existing map regression

Open `PlayGame`.

Ensure the existing `TileMapRenderer` Actor has:

```text
Level To Render = 1
```

Press Play.

Expected:

- Project compiles successfully.
- MapArrays resolves Level 1.
- TileMapRenderer builds the level.
- Approximately 2,819 tile instances are produced, matching the previous known-working map.
- Tile textures display correctly.
- No grey checkerboard material appears.
- Existing overhead camera still works.
- Map orientation is unchanged.
- No out-of-bounds array access occurs.

## Test B — Unsupported level

Set:

```text
Level To Render = 999
```

Press Play.

Expected:

- Unreal does not crash.
- A useful error appears in the Output Log.
- Renderer does not attempt to access missing generated arrays.
- No invalid map geometry is constructed.

Restore:

```text
Level To Render = 1
```

after testing.

## Test C — Multiple maps

If the repository contains a second valid TileGen export, register it through `MapArrays.cpp`.

Set:

```text
Level To Render = 2
```

Press Play.

Expected:

- Level 2 renders successfully.
- The correct Level 2 data is used.
- The corresponding tile lookup resolves properly.
- Level 2 has its own expected geometry/layout.
- Switching back to Level 1 requires no change to `TileMapRenderer.cpp`.

If Map 2 is not present, do not fabricate its contents or mark this test as passed.

Report that the architecture supports additional maps but practical multi-map verification is pending a second export.

---

# 13. Build and Verification Instructions

After editing the files, attempt to compile the project using the available Unreal build environment.

The intended build configuration is:

```text
Development Editor | Win64
```

Do not modify the engine installation.

Do not perform a full clean/rebuild unless necessary.

If Unreal Editor is running and the changes involve reflected headers or `UPROPERTY`, explain that a full build with the Editor closed is recommended.

If automated compilation is unavailable in the agent environment, do not claim the project compiled successfully.

Instead, identify the limitation and provide the exact manual build steps and anticipated verification procedure.

Check the implementation for Unreal-specific requirements, including necessary header dependencies, API export macros, component lifetime management and C++ syntax.

Do not change unrelated project settings.

---

# 14. Files You Are Allowed to Modify

Expected modifications:

```text
Game/Source/GrantMoney/Public/MapArrays.h

Game/Source/GrantMoney/Private/MapArrays.cpp

Game/Source/GrantMoney/Public/TileMapRenderer.h

Game/Source/GrantMoney/Private/TileMapRenderer.cpp
```

You may make minimal additional changes when necessary for compilation or genuine integration.

Do not modify:

- TileGen's original generated numerical map data.
- Existing imported tile textures.
- The `PlayGame.umap` asset unless absolutely required.
- Existing camera configuration.
- The material graph unless a verified compatibility problem exists.
- Unrelated gameplay code.
- Repository-wide `.gitignore` or `.gitattributes`.
- Existing documentation outside this ticket.

Do not delete generated exports or replace them with manually recreated values.

---

# 15. Documentation Requirement

This is an AI-native capstone project, so the implementation must be explainable and reproducible.

After completing the changes, create a technical task document at:

```text
docs/course/Module1-BuildingAWorld/Sprint2/MapArraysIntegration.md
```

If a relevant document already exists, update it instead of creating a duplicate.

Include:

### What Was Built
Explain the new dynamic map-selection architecture.

### Why We Built It
Explain why the old renderer was too closely coupled to `Map1_1`.

### Files Changed
Explain the responsibility of each modified C++ file.

### Implementation Steps
Describe the changes clearly enough that another developer could reproduce them.

### AI Use
Record the role of Copilot, important generated suggestions, human decisions and verification results. Distinguish what was actually tested from what still needs testing.

### Verification
Explain how to test Level 1, an unsupported level, and Level 2 if available.

### Known Limitations
Identify any outstanding constraints, such as runtime map switching not yet being implemented.

---

# 16. Git and Repository Safety

You are working in a shared Git repository.

Inspect the current branch and working tree before editing.

Do not discard existing changes.

Do not reset or overwrite unrelated files.

Do not automatically commit, push, merge, rebase or force-push.

Make the source changes and present them for review.

Do not stage generated Unreal directories such as:

```text
Binaries/
Intermediate/
Saved/
DerivedDataCache/
```

The project uses Git LFS for legitimate Unreal binary assets.

Preserve that configuration.

---

# 17. Required Final Agent Report

When finished, give me a concise engineering report with these sections:

1. **Existing implementation discovered:** Explain the actual TileGen export format and renderer structure you found.
2. **Files modified:** List every source file created or changed.
3. **Architecture:** Explain how the renderer now selects maps without referencing generated map names.
4. **Testing performed:** Identify which tests were actually run and their results.
5. **Remaining limitations:** Identify anything not yet implemented or verified.
6. **Manual verification steps:** Explain exactly what I should do when reopening Unreal Editor.

Do not report tests as passed unless they were actually executed.

---

# 18. Final Definition of Done

This task is complete when:

- [ ] A common `MapArrays` interface exists.
- [ ] Level 1 resolves successfully.
- [ ] The renderer no longer directly includes generated map headers.
- [ ] The renderer no longer directly references `Map1_1` global variables.
- [ ] Level selection is exposed through `UPROPERTY`.
- [ ] Ground and validity arrays retain their correct data types and indexing.
- [ ] Tile asset resolution uses the generated lookup tables through `MapArrays`.
- [ ] Existing HISM rendering behavior is preserved.
- [ ] Missing maps and invalid tile IDs are handled safely.
- [ ] The existing Level 1 map still generates the expected 2,819 tiles.
- [ ] Textures and materials remain functional.
- [ ] The existing camera and level remain unchanged.
- [ ] A second map can be registered without modifying TileMapRenderer.
- [ ] The implementation is documented.
- [ ] Build/test results are accurately reported.

**Priority: preserve the working renderer, isolate generated TileGen data behind MapArrays, and establish a clean foundation for rendering multiple levels.**
