using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;
using System.Text.Json;

namespace EntitledLogicTileGen;

/// <summary>
/// Packs the loaded tile palette into a single atlas (spritesheet) PNG plus
/// a JSON manifest mapping each tile index to its pixel rectangle in the
/// sheet — the form most engines prefer over loose per-tile PNGs.
/// </summary>
public sealed partial class MainForm
{
    private const int AtlasTileSize = 64;

    private sealed class AtlasEntry
    {
        public int Index { get; set; }
        public string Filename { get; set; } = "";
        public int X { get; set; }
        public int Y { get; set; }
        public int W { get; set; }
        public int H { get; set; }
    }

    private sealed class AtlasManifest
    {
        public int TileSize { get; set; }
        public int Columns { get; set; }
        public int Rows { get; set; }
        public int Count { get; set; }
        public int SheetWidth { get; set; }
        public int SheetHeight { get; set; }
        public List<AtlasEntry> Tiles { get; set; } = new();
    }

    private void ExportAtlas_Click(object? sender, System.EventArgs e)
    {
        if (_palette.Count == 0)
        {
            ShowMapStatus("✘  Load tiles into the palette first (Browse… on the palette).", success: false);
            return;
        }

        string folder = _tilesFolder.Text.Trim();
        if (string.IsNullOrEmpty(folder) || !Directory.Exists(folder))
        {
            folder = _mapOutputPath.Text.Trim();
        }
        if (string.IsNullOrEmpty(folder))
        {
            ShowMapStatus("✘  No output folder available for the atlas.", success: false);
            return;
        }

        try
        {
            Directory.CreateDirectory(folder);

            int count = _palette.Count;
            int cols = (int)System.Math.Ceiling(System.Math.Sqrt(count));
            int rows = (int)System.Math.Ceiling(count / (double)cols);

            int sheetW = cols * AtlasTileSize;
            int sheetH = rows * AtlasTileSize;

            AtlasManifest manifest = new()
            {
                TileSize = AtlasTileSize,
                Columns = cols,
                Rows = rows,
                Count = count,
                SheetWidth = sheetW,
                SheetHeight = sheetH
            };

            using (Bitmap sheet = new(sheetW, sheetH, PixelFormat.Format32bppArgb))
            {
                using (Graphics g = Graphics.FromImage(sheet))
                {
                    g.InterpolationMode = InterpolationMode.NearestNeighbor;
                    g.PixelOffsetMode = PixelOffsetMode.Half;
                    g.Clear(Color.Transparent);

                    for (int i = 0; i < count; i++)
                    {
                        TileInfo tile = _palette[i];
                        int col = i % cols;
                        int row = i / cols;
                        int px = col * AtlasTileSize;
                        int py = row * AtlasTileSize;

                        if (tile.Original != null)
                        {
                            g.DrawImage(tile.Original, px, py, AtlasTileSize, AtlasTileSize);
                        }

                        manifest.Tiles.Add(new AtlasEntry
                        {
                            Index = tile.Index,
                            Filename = tile.Filename,
                            X = px,
                            Y = py,
                            W = AtlasTileSize,
                            H = AtlasTileSize
                        });
                    }
                }

                string stamp = System.DateTime.Now.ToString("yyyyMMdd_HHmmss");
                string pngName = $"tile_atlas_{stamp}.png";
                string jsonName = $"tile_atlas_{stamp}.json";

                sheet.Save(Path.Combine(folder, pngName), ImageFormat.Png);

                JsonSerializerOptions opts = new() { WriteIndented = true };
                File.WriteAllText(Path.Combine(folder, jsonName),
                    JsonSerializer.Serialize(manifest, opts));

                ShowMapStatus(
                    $"✔  Atlas: {pngName} ({cols}×{rows}, {count} tiles) + manifest  →  {folder}",
                    success: true);
            }
        }
        catch (System.Exception ex)
        {
            ShowMapStatus($"✘  Atlas export error: {ex.Message}", success: false);
        }
    }
}
