# Procedural Tile Generator

A WinForms / .NET 8 application that generates seamlessly-tileable
64 × 64 pixel tiles for the Entitled Logic overhead map.

---

## Requirements

| Tool          | Version    |
|---------------|------------|
| Visual Studio | 2022       |
| .NET SDK      | 8.0        |
| Windows       | 10 / 11    |

---

## Quick Start

1. Open **EntitledLogicTileGen.sln** (or open the .csproj directly in VS 2022).
2. Press **F5** to build and run.
3. Choose two colours, adjust the settings, then click **Generate & Save Tile**.

---

## How Seamless Tiling Works

The generator builds a **32 × 32** noise sample and then "folds" it into
a **64 × 64** tile by mirroring it in both axes:

```
sx = px < 32 ? px : (63 - px)   // horizontal fold
sy = py < 32 ? py : (63 - py)   // vertical fold
```

Because the left edge (`px = 0 → sx = 0`) is identical to the right edge
(`px = 63 → sx = 0`), any two tiles placed side-by-side match perfectly.
The same holds vertically, so the tile tiles seamlessly in all four directions.

---

## Controls

### Colors
* **Panel swatch** — live RGB preview.
* **Pick…** — opens the Windows colour picker.
* **R / G / B spinners** — type or spin a value; the swatch updates live.

### Generation Settings

| Control        | Range    | Default | Effect                                        |
|----------------|----------|---------|-----------------------------------------------|
| Algorithm      | 6 choices| Perlin  | Noise algorithm (see below)                   |
| Saturation     | 1 – 100  | 100     | HSL saturation of the blended output colours  |
| Noise Level    | 0 – 100  | 15      | Contrast of the pattern (0 = flat, 100 = full)|
| Seed           | 0 – 2^31 | random  | Reproducible results when "Random seed" is off|

### Algorithms

| Name                       | Character                                        |
|----------------------------|--------------------------------------------------|
| Perlin Noise               | Smooth gradient; classic terrain look            |
| Simplex Noise              | Like Perlin but with fewer directional artefacts |
| Value Noise                | Soft, rounded blobs; lower frequency feel        |
| Fractal Brownian Motion    | Layered Perlin (6 octaves); rich multi-scale detail|
| Worley / Cellular Noise    | Cell-like organic pattern; cracks / scales        |
| Turbulence                 | Absolute-value Perlin sum; fire / cloud texture  |

### Output
* **Path** — folder where PNGs are saved.  Created automatically if it does not exist.
* **Browse…** — opens a folder browser.
* **Generate & Save Tile** — creates one tile; filename: `tile_YYYYMMDD_HHmmss_fff.png`
* **Batch: 10 Tiles** — generates ten tiles with spread seeds; useful for variety.

### Preview
* Shows the tile scaled up and optionally in a 2 × 2 tiled arrangement.
* A faint grid line marks tile boundaries in tiled view.
* **Preview scale** — 1× to 7× (clamped to fit the preview box).

---

## Project Structure

```
EntitledLogicTileGen/
├── EntitledLogicTileGen.csproj
├── Program.cs
├── NoiseAlgorithm.cs        — enum
├── TileGenerator.cs         — core generation logic
├── MainForm.cs              — WinForms UI (no Designer.cs needed)
└── Noise/
    ├── PerlinNoise.cs
    ├── SimplexNoise.cs
    ├── ValueNoise.cs
    ├── WorleyNoise.cs
    ├── FractalBrownianMotion.cs
    └── TurbulenceNoise.cs
```

---

## Integration with Entitled Logic

Tiles are saved as 64 × 64 RGBA PNGs.  Load them into your tile-map
renderer as you would any spritesheet.  Because every tile is seamlessly
tileable, the same PNG can be reused across the entire 64 × 64 tile map
without visible seams.
