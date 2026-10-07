# GrantMoney — Implement a Procedural MapBoundaryGenerator

## Project Context

We are developing GrantMoney, an Unreal Engine 5.8 C++ Survivor game.

We currently use an external program called TileGen to generate 64x64 map arrays. These maps are integrated into Unreal through our existing FMapArrays interface.

The current system contains:

- MapArrays.h
- MapArrays.cpp
- TileMapRenderer.h
- TileMapRenderer.cpp
- Generated TileGen map arrays under Private/Maps/

TileMapRenderer currently builds the selected level using Hierarchical Instanced Static Mesh components (HISM), with one group per ground tile ID.

The ground renderer uses Unreal's built-in Plane mesh and a TileSize of 100 Unreal units.

Our original Level 1 successfully generated 2,819 visible tile instances.

**We want to extend this system without editing TileGen, modifying its generated arrays, or breaking our working map renderer.**

## Objective

Implement a modular C++ system named `FMapBoundaryGenerator` that automatically generates a three-dimensional wall along the exposed edges of a selected TileGen map.

This is an optional visual-environment enhancement for our eventual third-person Survivor game.

For Version 1, use Unreal's built-in Cube mesh as a temporary wall segment.

Later we will replace the cube appearance with more polished modular environment assets.

## Step 1 — Inspect Existing Source

Before editing, inspect:

Game/Source/GrantMoney/Public/MapArrays.h

Game/Source/GrantMoney/Private/MapArrays.cpp

Game/Source/GrantMoney/Public/TileMapRenderer.h

Game/Source/GrantMoney/Private/TileMapRenderer.cpp

Understand how the existing renderer retrieves FGrantMoneyMapData.

Do not create another map registry or duplicate the level-selection system.

Preserve MapArrays' existing API and indexing convention.

Our arrays use [X][Y], not [Y][X].

## Step 2 — Create New Files

Create:

Game/Source/GrantMoney/Public/MapBoundaryGenerator.h

Game/Source/GrantMoney/Private/MapBoundaryGenerator.cpp

Use a small C++ helper class rather than introducing another independently placed level Actor.

The generator should expose functionality equivalent to:

GenerateBoundary(
    Owner,
    ParentComponent,
    MapData,
    BoundaryMesh,
    BoundaryMaterial,
    TileSize,
    WallHeight,
    WallThickness
)

The implementation may refine the function signature if needed for correct Unreal Engine API usage.

The function should return the number of generated wall instances for logging and testing.

## Step 3 — Determine Playable Cells

Using FGrantMoneyMapData, implement a helper/lambda that determines whether a coordinate contains playable visible ground.

A coordinate is playable only when:

1. X and Y are within the map dimensions.
2. Valid[X][Y] is nonzero.
3. Ground[X][Y] is nonzero.

Coordinates outside the map must always be considered non-playable.

Use this same definition consistently for boundary detection.

Do not modify the underlying TileGen arrays.

## Step 4 — Generate Exposed Edges

Iterate over every playable cell.

Check its four cardinal neighbors:

- X - 1
- X + 1
- Y - 1
- Y + 1

If a neighbor is not playable, generate exactly one wall segment along the corresponding edge.

Do not generate walls between two adjacent playable cells.

Do not generate diagonal walls.

The resulting boundary must follow irregular map shapes and enclose internal empty regions as well as the outer perimeter.

## Step 5 — Generate Wall Geometry

Use Unreal's built-in mesh:

/Engine/BasicShapes/Cube.Cube

The standard cube is approximately 100x100x100 Unreal units.

Use a single UHierarchicalInstancedStaticMeshComponent to hold the generated walls.

Each instance should have an individual transform.

Assuming TileSize = 100, WallHeight = 300 and WallThickness = 20:

- The wall's bottom should be at ground level Z = 0.
- Its center should be at Z = WallHeight / 2.
- Its long axis should extend along the exposed tile edge.
- Its thickness should extend perpendicular to that edge.

For X-facing edges, align the wall's long axis with local/world Y.

For Y-facing edges, rotate the wall 90 degrees so its long axis extends along world X.

Calculate scale based on the engine cube's approximately 100-unit dimensions.

For example, when the wall's long axis is its local Y:

Scale X = WallThickness / 100

Scale Y = TileSize / 100

Scale Z = WallHeight / 100

Use appropriate yaw rotation for the perpendicular orientation.

Place walls at the cell center plus or minus half the tile size along the exposed cardinal direction.

IMPORTANT: The existing TileMapRenderer builds ground instances using transforms relative to its SceneRoot. Boundary instances should use the same local coordinate system so the entire generated world remains aligned if the parent Actor moves.

Avoid corner placement offsets that create visible gaps. Minor overlap between perpendicular wall pieces is acceptable for Version 1.

## Step 6 — Component Ownership and Collision

Create the generated HISM component with TileMapRenderer as its owning Actor.

Register and attach it correctly to the existing renderer's SceneRoot.

Configure the wall instances to use blocking collision.

Use a BlockAll collision profile and appropriate collision settings so the boundary can eventually prevent a third-person character from leaving the arena.

Do not add thousands of separate AActor objects.

Do not introduce Blueprint gameplay.

Do not modify ground-tile collision behavior as part of this task.

## Step 7 — Integrate with TileMapRenderer

Modify TileMapRenderer.h and TileMapRenderer.cpp minimally.

After TileMapRenderer successfully retrieves and validates the selected FGrantMoneyMapData, it should be able to invoke FMapBoundaryGenerator.

The boundary must use the existing LevelToRender selection indirectly through the same MapData object.

Expose these settings through C++ UPROPERTY fields:

- bGenerateBoundaries = true
- WallHeight = 300.0f
- WallThickness = 20.0f
- BoundaryMaterial (optional assignable material)

Use descriptive Unreal Details-panel categories such as "Boundary".

Load or otherwise safely assign the built-in Cube mesh.

Do not change the existing M_Tile material, TileTexture parameter, ground geometry, texture loading, or camera configuration.

Boundary generation should occur once during the existing initial BuildMap process.

## Step 8 — Material Support

Allow a separate wall material to be assigned.

If the material is null, use the mesh's existing default material temporarily.

Document that a custom wall material used with HISM must have "Used with Instanced Static Meshes" enabled in Unreal's Material Editor.

Do not apply the existing PNG-based M_Tile ground material to the walls automatically.

## Step 9 — Debug Logging

Add a clear final message such as:

MapBoundaryGenerator: Created N wall segments.

The current TileMapRenderer tile-count log must remain intact.

Avoid excessive per-cell logging.

If MapArrays cannot resolve the requested level or the returned map data is invalid, do not attempt boundary generation.

Handle missing meshes, invalid dimensions or invalid configuration values safely.

## Step 10 — Preserve Scope

Do not implement the following in this task:

- Changes to TileGen's application or export format.
- A replacement for MapArrays.
- A second independent LevelToRender property.
- Procedural Content Generation (PCG).
- Foliage, rocks, trees or decorative assets.
- A player character or movement system.
- Runtime switching between maps.
- Landscape generation.
- Advanced corner meshes.
- Complex collision/navigation systems.
- Changes to the existing camera.
- Thousands of individual wall Actors.

This first version should be small, deterministic and easy to verify.

## Acceptance Criteria

1. Existing Level 1 renders exactly as before.
2. Ground tile count remains unchanged.
3. A visible 3D wall is generated around the playable area.
4. No unnecessary walls are created between adjacent playable ground cells.
5. Boundary walls have blocking collision.
6. The wall height and thickness can be adjusted from the Unreal Editor.
7. Disabling bGenerateBoundaries restores the existing wall-free map.
8. Selecting another registered map produces boundaries matching that map.
9. Invalid level selection does not crash.
10. No TileGen files or generated array values are modified.

## Deliverables

Create the two new MapBoundaryGenerator files.

Make only necessary integration changes to the existing TileMapRenderer header and implementation.

Provide a brief description of every changed file.

Do not automatically commit, push or merge the changes.

Document the actual build/test results. Do not claim Unreal verification succeeded unless it was performed.

Stop after implementation and give me step-by-step instructions for compiling and testing the system in Unreal Editor.
