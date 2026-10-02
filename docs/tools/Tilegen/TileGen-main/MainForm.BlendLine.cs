using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Windows.Forms;

namespace EntitledLogicTileGen;

/// <summary>
/// "Blend Line" map tool.  It first draws the selected tile along a Bresenham
/// line exactly like the Line tool, then — on mouse-up — softens each line
/// cell into the terrain on either side of the line by generating a blended
/// transition tile (via <see cref="TileBlender.BlendPatch"/>) and dropping it
/// into the cell.
///
/// Approach #1 — blended bitmaps become first-class palette tiles:
///   • Each gets the next index (Index == _palette.Count + 1, preserving the
///     FindTileByIndex invariant), full-size Original + Thumbnail + MiniThumb,
///     and a generated PNG filename so the C++ / atlas export keeps working.
///     When the tiles folder is known and writable, the PNG is also written to
///     disk so the Unreal import script picks it up.
///   • Identical / near-identical neighbours are skipped: a neighbour is only
///     blended toward when its average color differs from the line cell's by
///     more than <see cref="BlendSameColorThreshold"/>.  A line drawn inside a
///     uniform region therefore creates no transition tiles at all.
///   • The blend reach (how many pixels the seam eases across — the "4–5 px"
///     band) is the Blend-px slider value, fed straight to BlendPatch.
///   • Identical local arrangements are de-duplicated, so repeated patterns
///     along a line reuse one generated tile instead of piling up duplicates.
///
/// Notes:
///   • Only meaningful on the Ground / Decoration layers (their cells are tile
///     indices).  On the Collision layer it falls back to a plain line.
///   • Generated tiles persist in the palette after an Undo — Undo reverts the
///     map cells, not your tile library (like importing tiles).  De-dup keeps
///     the palette from growing on repeated runs.
/// </summary>
public sealed partial class MainForm
{
    // Assigned in BuildMapToolsColumn (see MainForm.MapLayers.cs).
    private NumericUpDown _blendLineWidth = null!;

    // Two tiles count as "the same" (skip blending) when the Euclidean distance
    // between their average colors is below this.  0 = identical; raise it to
    // also skip near-identical variants.  Tweak to taste.
    private const double BlendSameColorThreshold = 24.0;

    // Average color per palette index, computed once and reused.
    private readonly Dictionary<int, Color> _avgColorCache = new();

    // Maps a local-arrangement key -> the palette index of the blended tile
    // already generated for it.  Cleared in LoadTiles when the palette reloads.
    private readonly Dictionary<string, int> _blendTileCache = new();

    // Blend tiles minted this session and the subset already written to disk.
    // Both are cleared in LoadTiles (a reload turns saved blend PNGs back into
    // ordinary palette tiles).  Used to batch-save per stroke and to flush any
    // stragglers at map-save time so _TileAssets.h never references a missing PNG.
    private readonly List<TileInfo> _sessionBlendTiles = new();
    private readonly HashSet<string> _savedBlendFiles =
        new(StringComparer.OrdinalIgnoreCase);

    /// <summary>
    /// Draws the selected tile along the line (identical to the Line tool),
    /// then blends each painted cell into its differing neighbours.
    /// </summary>
    private void ApplyBlendLine(int x1, int y1, int x2, int y2, bool erase)
    {
        // 1) Same initial line draw as the Line tool.
        ApplyLine(x1, y1, x2, y2, erase);

        // Blending only makes sense for tile-bearing layers and non-erase draws.
        if (erase || _activeLayer == MapLayer.Collision)
        {
            RedrawMap();
            return;
        }

        int reach = (int)_blendLineWidth.Value;

        // 2) Walk the same Bresenham cells and blend each into its neighbours.
        bool added = false;
        Cursor = Cursors.WaitCursor;
        try
        {
            foreach ((int cx, int cy) in LineCells(x1, y1, x2, y2))
            {
                if (_palette.Count >= MaxTiles) break;   // ushort index cap
                if (BlendOneCell(cx, cy, reach)) added = true;
            }
        }
        finally
        {
            Cursor = Cursors.Default;
        }

        if (added)
        {
            // Reflect the new tiles in the palette browser, keeping the user's
            // current paint selection highlighted.
            BuildPaletteEntries();
            ReselectPaletteEntry(_selectedTileIndex);

            // Persist the freshly-minted tiles and report the result plainly.
            SaveNewBlendTiles();
        }

        RedrawMap();
    }

    /// <summary>
    /// Enumerates the cells a Bresenham line visits (the same traversal
    /// ApplyLine uses), clipped to valid in-map cells.
    /// </summary>
    private IEnumerable<(int x, int y)> LineCells(int x1, int y1, int x2, int y2)
    {
        int dx = Math.Abs(x2 - x1), dy = Math.Abs(y2 - y1);
        int sx = x1 < x2 ? 1 : -1, sy = y1 < y2 ? 1 : -1;
        int err = dx - dy;
        int x = x1, y = y1;

        while (true)
        {
            if (x >= 0 && x < MapCells && y >= 0 && y < MapCells && _inMap[x, y])
            {
                yield return (x, y);
            }
            if (x == x2 && y == y2) break;
            int e2 = 2 * err;
            if (e2 > -dy) { err -= dy; x += sx; }
            if (e2 < dx) { err += dx; y += sy; }
        }
    }

    /// <summary>
    /// Blends one already-painted line cell into its (differing) neighbours,
    /// replacing it with a freshly-minted blended tile.  Returns true if a new
    /// palette tile was added (false if nothing to blend, or a cached tile was
    /// reused).
    /// </summary>
    private bool BlendOneCell(int cellX, int cellY, int reach)
    {
        ushort[,] arr = ActiveArray();
        int centerIdx = arr[cellX, cellY];
        if (centerIdx == 0) return false;

        TileInfo? center = FindTileByIndex(centerIdx);
        if (center?.Original == null) return false;

        // Build a 3×3 source patch: centre + each neighbour that is present,
        // valid, and visually DIFFERENT from the centre.
        Bitmap?[,] sources = new Bitmap?[3, 3];
        sources[1, 1] = center.Original;

        int[,] keyIdx = new int[3, 3];   // arrangement, for the de-dup key
        keyIdx[1, 1] = centerIdx;
        int neighbours = 0;

        for (int oy = -1; oy <= 1; oy++)
        {
            for (int ox = -1; ox <= 1; ox++)
            {
                if (ox == 0 && oy == 0) continue;
                int nx = cellX + ox, ny = cellY + oy;
                if (nx < 0 || nx >= MapCells || ny < 0 || ny >= MapCells) continue;
                if (!_inMap[nx, ny]) continue;

                int nIdx = arr[nx, ny];
                if (nIdx == 0) continue;

                TileInfo? nTile = FindTileByIndex(nIdx);
                if (nTile?.Original == null) continue;

                if (ColorDistance(AvgColor(centerIdx, center), AvgColor(nIdx, nTile))
                        < BlendSameColorThreshold)
                {
                    continue;   // same / near-same tile — don't blend toward it
                }

                sources[ox + 1, oy + 1] = nTile.Original;
                keyIdx[ox + 1, oy + 1] = nIdx;
                neighbours++;
            }
        }

        if (neighbours == 0) return false;   // nothing different to blend into

        // De-dup: identical arrangement + reach -> reuse the existing tile.
        string key = BuildBlendKey(keyIdx, reach);
        if (_blendTileCache.TryGetValue(key, out int cachedIdx)
                && FindTileByIndex(cachedIdx) != null)
        {
            arr[cellX, cellY] = (ushort)cachedIdx;
            return false;
        }

        if (_palette.Count >= MaxTiles) return false;

        // Generate the blend; we keep only the centre cell.
        Bitmap?[,] blended = TileBlender.BlendPatch(sources, reach);
        Bitmap? result = blended[1, 1];
        if (result == null)
        {
            DisposeGrid(blended);
            return false;
        }

        int newIdx = _palette.Count + 1;
        string filename = $"blend_{newIdx:D5}_{BlendKeyHash(key)}.png";

        TileInfo info = new TileInfo
        {
            Index = newIdx,
            Filename = filename,
            Original = result,                              // take ownership
            Thumbnail = MakeThumbnail(result, ThumbSize),
            MiniThumb = MakeThumbnail(result, MapCellSize)
        };
        _palette.Add(info);
        _blendTileCache[key] = newIdx;
        _sessionBlendTiles.Add(info);   // written to disk by SaveNewBlendTiles

        // result is now owned by the TileInfo; dispose only the other 8 cells.
        blended[1, 1] = null;
        DisposeGrid(blended);

        arr[cellX, cellY] = (ushort)newIdx;
        return true;
    }

    /// <summary>
    /// Writes any session blend tiles not yet on disk, reporting the outcome in
    /// the map status line.  Targets the tiles folder (where the Unreal import
    /// script reads), falling back to the map output folder so there is always a
    /// destination.  Reuses the same write-block detection as Map Save.
    /// </summary>
    private void SaveNewBlendTiles()
    {
        List<TileInfo> pending = new();
        foreach (TileInfo t in _sessionBlendTiles)
        {
            if (!_savedBlendFiles.Contains(t.Filename)) pending.Add(t);
        }
        if (pending.Count == 0) return;

        if (!TryResolveBlendFolder(out string folder, out string error))
        {
            ShowMapStatus($"✘  {pending.Count} blend tile(s) not saved — {error}", success: false);
            return;
        }

        (int saved, int failed, string? lastError) = WriteBlendTiles(pending, folder);

        if (failed == 0)
        {
            ShowMapStatus(
                $"✔  Saved {saved} blend tile{(saved == 1 ? "" : "s")}  →  {folder}",
                success: true);
        }
        else
        {
            ShowMapStatus(
                $"✘  Saved {saved}, but {failed} blend tile(s) failed: {lastError}",
                success: false);
        }
    }

    /// <summary>
    /// Map-save hook: flushes any blend tiles still missing from disk so the
    /// emitted _TileAssets.h never references a PNG that isn't there.  Silent on
    /// success; returns (ok, stillUnsaved) for the caller to fold into its
    /// status message.
    /// </summary>
    private (bool ok, int unsaved) EnsureBlendTilesSaved()
    {
        List<TileInfo> pending = new();
        foreach (TileInfo t in _sessionBlendTiles)
        {
            if (!_savedBlendFiles.Contains(t.Filename)) pending.Add(t);
        }
        if (pending.Count == 0) return (true, 0);

        if (!TryResolveBlendFolder(out string folder, out _))
        {
            return (false, pending.Count);
        }

        (_, int failed, _) = WriteBlendTiles(pending, folder);
        return (failed == 0, failed);
    }

    /// <summary>
    /// Writes the given tiles' PNGs into <paramref name="folder"/>, marking each
    /// success in <see cref="_savedBlendFiles"/>.  Returns counts plus the last
    /// error seen (for reporting).
    /// </summary>
    private (int saved, int failed, string? lastError) WriteBlendTiles(List<TileInfo> tiles, string folder)
    {
        int saved = 0, failed = 0;
        string? lastError = null;

        foreach (TileInfo t in tiles)
        {
            if (t.Original == null) { failed++; continue; }
            try
            {
                t.Original.Save(Path.Combine(folder, t.Filename), ImageFormat.Png);
                _savedBlendFiles.Add(t.Filename);
                saved++;
            }
            catch (Exception ex)
            {
                failed++;
                lastError = ex.Message;
            }
        }

        return (saved, failed, lastError);
    }

    /// <summary>
    /// Resolves where blend tiles should be written: the tiles folder first
    /// (the Unreal import source), else the map output folder.  Runs the shared
    /// write-test so OneDrive / Controlled-Folder-Access blocks are reported in
    /// plain English instead of failing mid-write.
    /// </summary>
    private bool TryResolveBlendFolder(out string folder, out string error)
    {
        string raw = _tilesFolder.Text.Trim();
        if (string.IsNullOrEmpty(raw)) raw = _mapOutputPath.Text.Trim();

        if (string.IsNullOrEmpty(raw))
        {
            folder = string.Empty;
            error = "set a Tiles folder (or a Map output path) first.";
            return false;
        }

        return TryPrepareOutputFolder(raw, out folder, out error);
    }

    // -------------------------------------------------------
    //  Color-similarity helpers
    // -------------------------------------------------------

    private Color AvgColor(int index, TileInfo tile)
    {
        if (_avgColorCache.TryGetValue(index, out Color cached)) return cached;
        Color avg = ComputeAverageColor(tile.Original!);
        _avgColorCache[index] = avg;
        return avg;
    }

    /// <summary>Average color over a strided sample of the bitmap (fast and ample).</summary>
    private static Color ComputeAverageColor(Bitmap bmp)
    {
        long r = 0, g = 0, b = 0;
        int count = 0;
        int stepX = Math.Max(1, bmp.Width / 16);
        int stepY = Math.Max(1, bmp.Height / 16);

        for (int y = 0; y < bmp.Height; y += stepY)
        {
            for (int x = 0; x < bmp.Width; x += stepX)
            {
                Color c = bmp.GetPixel(x, y);
                r += c.R;
                g += c.G;
                b += c.B;
                count++;
            }
        }

        if (count == 0) return Color.Black;
        return Color.FromArgb((int)(r / count), (int)(g / count), (int)(b / count));
    }

    private static double ColorDistance(Color a, Color b)
    {
        int dr = a.R - b.R, dg = a.G - b.G, db = a.B - b.B;
        return Math.Sqrt(dr * dr + dg * dg + db * db);
    }

    // -------------------------------------------------------
    //  De-dup key helpers
    // -------------------------------------------------------

    private static string BuildBlendKey(int[,] idx, int reach)
    {
        // The 9 indices (fixed order) plus the reach uniquely identify a
        // BlendPatch result — neighbour positions matter, so order is preserved.
        System.Text.StringBuilder sb = new();
        for (int y = 0; y < 3; y++)
        {
            for (int x = 0; x < 3; x++)
            {
                sb.Append(idx[x, y]).Append(',');
            }
        }
        sb.Append('w').Append(reach);
        return sb.ToString();
    }

    private static string BlendKeyHash(string key)
    {
        // Short, filename-safe, stable hash (FNV-1a) for the generated PNG name.
        uint h = 2166136261u;
        foreach (char c in key)
        {
            h ^= c;
            h *= 16777619u;
        }
        return h.ToString("x8");
    }

    // -------------------------------------------------------
    //  Palette selection helper
    // -------------------------------------------------------

    /// <summary>
    /// Re-applies the selection highlight to the palette entry whose tile has
    /// the given index, after the palette browser has been rebuilt.
    /// </summary>
    private void ReselectPaletteEntry(int index)
    {
        _selectedEntry = null;
        if (index <= 0) return;
        foreach (Control c in _tilesScroll.Controls)
        {
            if (c is TilePaletteEntry entry && entry.Tile.Index == index)
            {
                entry.IsSelected = true;
                entry.Invalidate();
                _selectedEntry = entry;
                return;
            }
        }
    }
}
