using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;
using System.Windows.Forms;

namespace EntitledLogicTileGen;

/// <summary>
/// Map Colorizer tab.  Runs the EXACT same generation as the Map Generator
/// tab (it calls <see cref="GenerateMap"/>, so it honours the Map tab's level
/// and effect settings).  Every room gets a bright random hue and every
/// corridor its own muted random hue, and — like the rest of the program —
/// every region gets real procedural NOISE: each region picks one random
/// noise algorithm + level + seed and is rendered as a single seamless
/// textured tile in its hue ("a single, consistent noise type and level" per
/// room).  Because the texture is seamless it tiles across the region's cells
/// with no internal seams; where two regions meet, the textured tiles are
/// blended with the seam-blend engine (<see cref="TileBlender"/>).
///
/// Two actions:
///   • Generate — generate, texture and colour the map, then preview it.
///   • Save     — write the unique tiles, add them to the palette, remap the
///     map to the loaded palette indices, and save the map.
/// </summary>
public sealed partial class MainForm
{
    private TabPage _tabColorMap = null!;

    private Panel _cmScroll = null!;
    private PictureBox _cmPreview = null!;
    private NumericUpDown _cmBlendWidth = null!;
    private TrackBar _cmZoomBar = null!;
    private Label _cmZoomLabel = null!;
    private Label _cmStatus = null!;

    // The Colorizer's own Map Settings + Map Effects controls (mirrors the
    // Map Generator tab so the Colorizer is self-contained).
    private NumericUpDown _cmLevel = null!;
    private TextBox _cmOutputPath = null!;
    private CheckBox _cmSymX = null!, _cmSymY = null!, _cmBorderWall = null!, _cmOpenArena = null!;
    private CheckBox _cmScatteredPillars = null!, _cmChokePoints = null!, _cmDeadEndPockets = null!,
                     _cmMazeLike = null!;

    // Generation results (held between Generate and Save)
    private bool[,]? _cmValid;
    private ushort[,]? _cmIndex;            // 1-based into _cmTiles; 0 = empty cell
    private List<Bitmap> _cmTiles = new();
    private Bitmap? _cmPreviewBmp;

    // Room rectangles recorded by the real generator (see GenerateMap).
    private readonly List<Rectangle> _lastRoomRects = new();

    private int _cmZoom = 10;             // preview px/cell. Default fits the whole 64-cell map in the panel (no scroll); zoom in for detail.

    // Fixed hue palettes (built from HSV): rooms bright/saturated, hallways
    // muted browns and greys.  Each region's texture varies brightness around
    // its hue.
    private static readonly Color[] BrightRoomColors =
    {
        Hsv(  0, 0.80, 0.92), Hsv( 28, 0.82, 0.95), Hsv( 50, 0.80, 0.92), Hsv(130, 0.72, 0.80),
        Hsv(175, 0.72, 0.82), Hsv(215, 0.75, 0.90), Hsv(270, 0.65, 0.88), Hsv(320, 0.72, 0.88),
    };

    private static readonly Color[] MutedHallColors =
    {
        Hsv( 25, 0.45, 0.40), Hsv( 32, 0.38, 0.50), Hsv( 38, 0.28, 0.58),
        Hsv( 30, 0.10, 0.48), Hsv(210, 0.08, 0.50), Hsv(220, 0.15, 0.40),
    };

    // ═══════════════════════════════════════════════════════
    //  UI
    // ═══════════════════════════════════════════════════════

    private void BuildColorMapTab()
    {
        _cmScroll = new Panel
        {
            Location = new Point(10, 10),
            Size = new Size(764, 700),   // height kept within the tab page so the horizontal bar is visible
            AutoScroll = true,
            BorderStyle = BorderStyle.FixedSingle,
            BackColor = Color.FromArgb(235, 235, 235)
        };

        _cmPreview = new PictureBox
        {
            Location = new Point(0, 0),
            Size = new Size(MapCells * _cmZoom, MapCells * _cmZoom),
            SizeMode = PictureBoxSizeMode.Normal,
            BackColor = Color.FromArgb(235, 235, 235)
        };
        _cmScroll.Controls.Add(_cmPreview);
        _cmScroll.AutoScrollMinSize = new Size(MapCells * _cmZoom, MapCells * _cmZoom);

        int rx = 790;

        Button btnGenerate = new Button
        {
            Text = "⚡  Generate",
            Location = new Point(rx, 14),
            Size = new Size(195, 42),
            Font = new Font("Segoe UI", 11f, FontStyle.Bold),
            BackColor = Color.FromArgb(70, 110, 160),
            ForeColor = Color.White,
            FlatStyle = FlatStyle.Flat,
            Cursor = Cursors.Hand
        };
        btnGenerate.FlatAppearance.BorderSize = 0;
        btnGenerate.Click += ColorMapGenerate_Click;

        Button btnSave = new Button
        {
            Text = "💾  Save",
            Location = new Point(rx + 200, 14),
            Size = new Size(195, 42),
            Font = new Font("Segoe UI", 11f, FontStyle.Bold),
            BackColor = Color.FromArgb(70, 130, 70),
            ForeColor = Color.White,
            FlatStyle = FlatStyle.Flat,
            Cursor = Cursors.Hand
        };
        btnSave.FlatAppearance.BorderSize = 0;
        btnSave.Click += ColorMapSave_Click;

        Label lblW = new Label
        {
            Text = "Blend width (px):",
            Location = new Point(rx, 66),
            AutoSize = true,
            Font = new Font("Segoe UI", 9f)
        };
        _cmBlendWidth = new NumericUpDown
        {
            Location = new Point(rx + 115, 63),
            Size = new Size(60, 23),
            Minimum = 1,
            Maximum = 32,
            Value = 20,
            Font = new Font("Segoe UI", 9f)
        };

        _cmZoomLabel = new Label
        {
            Text = $"Zoom: {_cmZoom}px / cell",
            Location = new Point(rx, 92),
            AutoSize = true,
            Font = new Font("Segoe UI", 9f)
        };
        _cmZoomBar = new TrackBar
        {
            Location = new Point(rx - 4, 110),
            Size = new Size(400, 38),
            Minimum = 8,
            Maximum = 48,
            Value = _cmZoom,
            TickFrequency = 8,
            SmallChange = 2,
            LargeChange = 8
        };
        _cmZoomBar.ValueChanged += ColorMapZoom_Changed;

        // ── Map Settings ───────────────────────────────────────
        GroupBox grpSettings = MakeGroup("Map Settings", rx, 154, 405, 96);
        Label lLevel = MakeLabel("Level:", 12, 30);
        _cmLevel = new NumericUpDown
        {
            Location = new Point(62, 27),
            Size = new Size(60, 23),
            Minimum = 1,
            Maximum = 25,
            Value = 1,
            Font = new Font("Segoe UI", 9f)
        };
        Label lPath = MakeLabel("Path:", 12, 64);
        _cmOutputPath = new TextBox { Location = new Point(62, 61), Size = new Size(228, 23) };
        Button btnBrowse = MakeButton("Browse…", 296, 60, 98, 25);
        btnBrowse.Click += ColorMapBrowseOutput_Click;
        grpSettings.Controls.AddRange(new Control[] { lLevel, _cmLevel, lPath, _cmOutputPath, btnBrowse });

        // ── Map Effects ────────────────────────────────────────
        GroupBox grpEffects = MakeGroup("Map Effects", rx, 258, 405, 156);
        _cmSymX = MakeEffectCheck("Symmetry — X axis", 12, 26);
        _cmSymY = MakeEffectCheck("Symmetry — Y axis", 210, 26);
        _cmBorderWall = MakeEffectCheck("Border wall", 12, 50);
        _cmOpenArena = MakeEffectCheck("Open arena", 210, 50);
        _cmScatteredPillars = MakeEffectCheck("Scattered pillars", 12, 74);
        _cmChokePoints = MakeEffectCheck("Choke points", 210, 74);
        _cmDeadEndPockets = MakeEffectCheck("Dead-end pockets", 12, 98);
        _cmMazeLike = MakeEffectCheck("Maze-like", 210, 98);
        grpEffects.Controls.AddRange(new Control[]
        {
            _cmSymX, _cmSymY, _cmBorderWall, _cmOpenArena,
            _cmScatteredPillars, _cmChokePoints, _cmDeadEndPockets, _cmMazeLike
        });

        // ── Data ───────────────────────────────────────────────
        GroupBox grpData = MakeGroup("Data", rx, 422, 405, 126);
        Button btnJson = MakeButton("Export JSON", 12, 24, 185, 28);
        Button btnCsv = MakeButton("Export CSV", 206, 24, 185, 28);
        Button btnAtlas = MakeButton("Export Atlas", 12, 56, 185, 28);
        Button btnImport = MakeButton("Import Map…", 206, 56, 185, 28);
        Button btnUnreal = MakeButton("Unreal import script…", 12, 88, 379, 28);
        btnJson.Click += ExportJson_Click;
        btnCsv.Click += ExportCsv_Click;
        btnAtlas.Click += ExportAtlas_Click;
        btnImport.Click += ImportMap_Click;
        btnUnreal.Click += ColorMapUnrealScript_Click;
        grpData.Controls.AddRange(new Control[] { btnJson, btnCsv, btnAtlas, btnImport, btnUnreal });

        _cmStatus = new Label
        {
            Location = new Point(rx, 556),
            Size = new Size(405, 80),
            Font = new Font("Segoe UI", 9f),
            ForeColor = Color.FromArgb(90, 90, 90),
            Text = "Generate uses these Map Settings/Effects, then gives each "
                 + "room/hallway its own noise texture and color. Saving also writes "
                 + "a <Map>_TileAssets.h index→asset lookup for Unreal."
        };

        _tabColorMap.Controls.Add(_cmScroll);
        _tabColorMap.Controls.Add(btnGenerate);
        _tabColorMap.Controls.Add(btnSave);
        _tabColorMap.Controls.Add(lblW);
        _tabColorMap.Controls.Add(_cmBlendWidth);
        _tabColorMap.Controls.Add(_cmZoomLabel);
        _tabColorMap.Controls.Add(_cmZoomBar);
        _tabColorMap.Controls.Add(grpSettings);
        _tabColorMap.Controls.Add(grpEffects);
        _tabColorMap.Controls.Add(grpData);
        _tabColorMap.Controls.Add(_cmStatus);
    }

    private void ColorMapBrowseOutput_Click(object? sender, EventArgs e)
    {
        using FolderBrowserDialog dlg = new() { SelectedPath = _cmOutputPath.Text };
        if (dlg.ShowDialog() == DialogResult.OK) _cmOutputPath.Text = dlg.SelectedPath;
    }

    private void ColorMapUnrealScript_Click(object? sender, EventArgs e)
    {
        string rawFolder = _cmOutputPath.Text.Trim();
        if (string.IsNullOrEmpty(rawFolder)) rawFolder = _mapOutputPath.Text.Trim();

        if (!TryPrepareOutputFolder(rawFolder, out string folder, out string folderError))
        {
            CmSetStatus("✘  " + folderError, success: false);
            return;
        }

        string tilesDir = _tilesFolder.Text.Trim();
        if (string.IsNullOrEmpty(tilesDir))
        {
            CmSetStatus("✘  Set the Map tab's tile folder first (it holds the PNGs to import).", success: false);
            return;
        }

        try
        {
            WriteUnrealImportScript(folder, tilesDir);
            CmSetStatus($"✔  Wrote import_tiles_to_unreal.py → {folder}.  Run it in Unreal "
                + "(Tools → Execute Python Script).", success: true);
        }
        catch (Exception ex)
        {
            CmSetStatus($"✘  {ex.Message}", success: false);
        }
    }

    private void ColorMapZoom_Changed(object? sender, EventArgs e)
    {
        _cmZoom = _cmZoomBar.Value;
        _cmZoomLabel.Text = $"Zoom: {_cmZoom}px / cell";
        if (_cmIndex != null) RenderColorMapPreview();   // re-render only if a map exists
    }

    // ═══════════════════════════════════════════════════════
    //  Generate
    // ═══════════════════════════════════════════════════════

    private void ColorMapGenerate_Click(object? sender, EventArgs e)
    {
        // Run the generator with THIS tab's settings/effects (same core the
        // Map Generator uses), filling _inMap and _lastRoomRects.
        GenerateMapCore(new MapGenSettings
        {
            Level = (int)_cmLevel.Value,
            SymX = _cmSymX.Checked,
            SymY = _cmSymY.Checked,
            BorderWall = _cmBorderWall.Checked,
            OpenArena = _cmOpenArena.Checked,
            ScatteredPillars = _cmScatteredPillars.Checked,
            ChokePoints = _cmChokePoints.Checked,
            DeadEndPockets = _cmDeadEndPockets.Checked,
            MazeLike = _cmMazeLike.Checked
        });

        Bitmap[]? regionTiles = null;
        try
        {
            Cursor = Cursors.WaitCursor;
            int bw = (int)_cmBlendWidth.Value;

            // 1. Snapshot layout.
            bool[,] valid = (bool[,])_inMap.Clone();
            List<Rectangle> rooms = new(_lastRoomRects);

            // 2. Region ids: rooms first (precedence), then flood corridors.
            int[,] region = new int[MapCells, MapCells];
            for (int y = 0; y < MapCells; y++)
                for (int x = 0; x < MapCells; x++)
                    region[x, y] = -1;

            for (int r = 0; r < rooms.Count; r++)
            {
                Rectangle rect = rooms[r];
                for (int y = rect.Top; y < rect.Bottom; y++)
                    for (int x = rect.Left; x < rect.Right; x++)
                        if (InBounds(x, y) && valid[x, y])
                            region[x, y] = r;
            }

            int nextRegion = rooms.Count;
            for (int y = 0; y < MapCells; y++)
                for (int x = 0; x < MapCells; x++)
                    if (valid[x, y] && region[x, y] == -1)
                        CmFloodRegion(valid, region, x, y, nextRegion++);

            int hallCount = nextRegion - rooms.Count;

            // 3. One seamless NOISE texture per region (its own algorithm,
            //    level and seed), coloured in the region's hue.
            Array algos = Enum.GetValues(typeof(NoiseAlgorithm));
            regionTiles = new Bitmap[nextRegion == 0 ? 1 : nextRegion];
            for (int i = 0; i < nextRegion; i++)
            {
                Color hue = i < rooms.Count
                    ? BrightRoomColors[_rng.Next(BrightRoomColors.Length)]
                    : MutedHallColors[_rng.Next(MutedHallColors.Length)];

                NoiseAlgorithm algo = (NoiseAlgorithm)algos.GetValue(_rng.Next(algos.Length))!;
                int level = _rng.Next(30, 80);
                int seed = _rng.Next();

                regionTiles[i] = TileGenerator.Generate(
                    Darker(hue), Lighter(hue), 100, level, algo, seed);
            }

            // 4. Build per-cell tiles by blending each region texture toward
            //    differently-region neighbours.  De-duplicated by the
            //    region-adjacency key (interior cells of a region share one
            //    tile; same-region neighbours are not blended, so the texture
            //    tiles seamlessly within a region).
            DisposeColorMapTiles();
            _cmTiles = new List<Bitmap>();
            Dictionary<string, int> keyToIndex = new();
            ushort[,] index = new ushort[MapCells, MapCells];

            for (int y = 0; y < MapCells; y++)
            {
                for (int x = 0; x < MapCells; x++)
                {
                    if (!valid[x, y]) { index[x, y] = 0; continue; }

                    int selfR = region[x, y];
                    int rN  = NeighbourRegion(region, valid, x,     y - 1, selfR);
                    int rS  = NeighbourRegion(region, valid, x,     y + 1, selfR);
                    int rE  = NeighbourRegion(region, valid, x + 1, y,     selfR);
                    int rW  = NeighbourRegion(region, valid, x - 1, y,     selfR);
                    int rNE = NeighbourRegion(region, valid, x + 1, y - 1, selfR);
                    int rNW = NeighbourRegion(region, valid, x - 1, y - 1, selfR);
                    int rSE = NeighbourRegion(region, valid, x + 1, y + 1, selfR);
                    int rSW = NeighbourRegion(region, valid, x - 1, y + 1, selfR);

                    string key = $"{selfR}|{rN}|{rS}|{rE}|{rW}|{rNE}|{rNW}|{rSE}|{rSW}";
                    if (!keyToIndex.TryGetValue(key, out int idx))
                    {
                        if (_cmTiles.Count >= MaxTiles)
                        {
                            Cursor = Cursors.Default;
                            CmSetStatus($"✘  This map needs more than {MaxTiles} unique tiles "
                                + "(noise makes each region unique). Generate a simpler map "
                                + "(lower level / fewer effects).", success: false);
                            return;
                        }

                        // 3×3 of texture tiles; [1,1] = centre; a direction is
                        // filled only when that neighbour is a DIFFERENT region.
                        Bitmap?[,] cells = new Bitmap?[3, 3];
                        cells[1, 1] = regionTiles[selfR];
                        cells[1, 0] = rN  >= 0 ? regionTiles[rN]  : null;
                        cells[1, 2] = rS  >= 0 ? regionTiles[rS]  : null;
                        cells[2, 1] = rE  >= 0 ? regionTiles[rE]  : null;
                        cells[0, 1] = rW  >= 0 ? regionTiles[rW]  : null;
                        cells[2, 0] = rNE >= 0 ? regionTiles[rNE] : null;
                        cells[0, 0] = rNW >= 0 ? regionTiles[rNW] : null;
                        cells[2, 2] = rSE >= 0 ? regionTiles[rSE] : null;
                        cells[0, 2] = rSW >= 0 ? regionTiles[rSW] : null;

                        Bitmap?[,] blended = TileBlender.BlendPatch(cells, bw);
                        _cmTiles.Add(blended[1, 1]!);   // keep the centre result
                        DisposeAllExceptCentre(blended);

                        idx = _cmTiles.Count;           // 1-based
                        keyToIndex[key] = idx;
                    }
                    index[x, y] = (ushort)idx;
                }
            }

            _cmValid = valid;
            _cmIndex = index;

            RenderColorMapPreview();

            CmSetStatus($"✔  {rooms.Count} rooms, {hallCount} hallways, {_cmTiles.Count} unique "
                + "noise tiles.  Press Save to keep them.", success: true);
        }
        catch (Exception ex)
        {
            CmSetStatus($"✘  Generate error: {ex.Message}", success: false);
        }
        finally
        {
            if (regionTiles != null)
                foreach (Bitmap? b in regionTiles) b?.Dispose();   // inputs no longer needed
            Cursor = Cursors.Default;
        }
    }

    /// <summary>Walks the BSP tree and records each leaf room rectangle.</summary>
    private void CollectRoomRects(BspNode? node)
    {
        if (node == null) return;
        if (node.IsLeaf)
        {
            if (node.Room.HasValue) _lastRoomRects.Add(node.Room.Value);
            return;
        }
        CollectRoomRects(node.Left);
        CollectRoomRects(node.Right);
    }

    /// <summary>Region id of a neighbour if it is valid AND a different region; otherwise -1 (no blend).</summary>
    private static int NeighbourRegion(int[,] region, bool[,] valid, int x, int y, int selfR)
    {
        if (!InBounds(x, y) || !valid[x, y]) return -1;
        int r = region[x, y];
        return r == selfR ? -1 : r;
    }

    private static void DisposeAllExceptCentre(Bitmap?[,] grid)
    {
        for (int cy = 0; cy < 3; cy++)
            for (int cx = 0; cx < 3; cx++)
                if (!(cx == 1 && cy == 1)) grid[cx, cy]?.Dispose();
    }

    private static void CmFloodRegion(bool[,] valid, int[,] region, int sx, int sy, int id)
    {
        Queue<(int x, int y)> q = new();
        q.Enqueue((sx, sy));
        region[sx, sy] = id;

        while (q.Count > 0)
        {
            (int x, int y) = q.Dequeue();
            Visit(x - 1, y); Visit(x + 1, y); Visit(x, y - 1); Visit(x, y + 1);

            void Visit(int nx, int ny)
            {
                if (!InBounds(nx, ny)) return;
                if (!valid[nx, ny] || region[nx, ny] != -1) return;
                region[nx, ny] = id;
                q.Enqueue((nx, ny));
            }
        }
    }

    private static bool InBounds(int x, int y) => x >= 0 && x < MapCells && y >= 0 && y < MapCells;

    // ═══════════════════════════════════════════════════════
    //  Preview (matches the Map tab's grid + 1px borders, scaled up so the
    //  texture and blended edges are visible)
    // ═══════════════════════════════════════════════════════

    private void RenderColorMapPreview()
    {
        if (_cmIndex == null || _cmValid == null) return;

        int cell = _cmZoom;
        int side = MapCells * cell;
        Bitmap canvas = new(side, side, PixelFormat.Format32bppArgb);

        using (Graphics g = Graphics.FromImage(canvas))
        {
            g.SmoothingMode = SmoothingMode.None;
            g.PixelOffsetMode = PixelOffsetMode.Half;
            g.InterpolationMode = InterpolationMode.HighQualityBicubic;
            g.Clear(Color.FromArgb(235, 235, 235));

            using Pen blackPen = new(Color.Black, 1f);

            for (int y = 0; y < MapCells; y++)
            {
                for (int x = 0; x < MapCells; x++)
                {
                    if (!_cmValid[x, y]) continue;
                    int idx = _cmIndex[x, y];
                    if (idx == 0) continue;

                    int px = x * cell;
                    int py = y * cell;
                    g.DrawImage(_cmTiles[idx - 1], px, py, cell, cell);
                    g.DrawRectangle(blackPen, px, py, cell - 1, cell - 1);
                }
            }
        }

        _cmPreviewBmp?.Dispose();
        _cmPreviewBmp = canvas;
        _cmPreview.Size = new Size(side, side);
        _cmPreview.Image = _cmPreviewBmp;
        // Drive the scrollable area explicitly so BOTH bars appear reliably.
        _cmScroll.AutoScrollMinSize = new Size(side, side);
    }

    // ═══════════════════════════════════════════════════════
    //  Save
    // ═══════════════════════════════════════════════════════

    private void ColorMapSave_Click(object? sender, EventArgs e)
    {
        if (_cmIndex == null || _cmValid == null || _cmTiles.Count == 0)
        {
            CmSetStatus("✘  Generate a map first.", success: false);
            return;
        }

        string folder = _tilesFolder.Text.Trim();
        if (string.IsNullOrEmpty(folder))
        {
            CmSetStatus("✘  Set the Map tab's tile folder first (it is reused here).", success: false);
            return;
        }

        try
        {
            Cursor = Cursors.WaitCursor;
            Directory.CreateDirectory(folder);

            string stamp = DateTime.Now.ToString("yyyyMMdd_HHmmss");
            List<string> files = new();
            for (int i = 0; i < _cmTiles.Count; i++)
            {
                string name = $"cmap_{stamp}_{i + 1:D3}.png";
                _cmTiles[i].Save(Path.Combine(folder, name), ImageFormat.Png);
                files.Add(name);
            }

            LoadTiles();

            Dictionary<string, int> nameToIndex = new();
            foreach (TileInfo t in _palette) nameToIndex[t.Filename] = t.Index;

            int missing = 0;
            for (int y = 0; y < MapCells; y++)
            {
                for (int x = 0; x < MapCells; x++)
                {
                    _decoData[x, y] = 0;
                    _collisionData[x, y] = 0;

                    if (!_cmValid[x, y] || _cmIndex[x, y] == 0)
                    {
                        _inMap[x, y] = false;
                        _mapData[x, y] = 0;
                        continue;
                    }

                    _inMap[x, y] = true;
                    string file = files[_cmIndex[x, y] - 1];
                    if (nameToIndex.TryGetValue(file, out int palIdx) && palIdx <= 65535)
                    {
                        _mapData[x, y] = (ushort)palIdx;
                    }
                    else
                    {
                        _mapData[x, y] = 0;
                        missing++;
                    }
                }
            }

            RedrawMap();
            // Drive the map save with the Colorizer's own path + level.
            _mapOutputPath.Text = _cmOutputPath.Text;
            _mapLevel.Value = _cmLevel.Value;
            bool mapSaved = TrySaveMap();
            // Stay on the Map Colorizer tab; the map is also loaded into the Map
            // Generator tab if you want to switch over and edit it.

            if (!mapSaved)
            {
                // TrySaveMap already showed the reason on the Map tab status; mirror it here.
                CmSetStatus($"⚠  Tiles saved, but the map file did not save — check the output "
                    + "Path (use a local folder outside Documents/OneDrive).", success: false);
            }
            else if (missing > 0)
            {
                CmSetStatus($"⚠  Saved {_cmTiles.Count} tiles, but {missing} cells could not be "
                    + $"mapped — the folder likely exceeds the {65535}-tile palette limit. "
                    + "Use a folder with fewer existing tiles.", success: false);
            }
            else
            {
                CmSetStatus($"✔  Saved {_cmTiles.Count} tiles and the map. Tiles added to the "
                    + "palette; the map is also loaded on the Map Generator tab.", success: true);
            }
        }
        catch (Exception ex)
        {
            CmSetStatus($"✘  Save error: {ex.Message}", success: false);
        }
        finally
        {
            Cursor = Cursors.Default;
        }
    }

    // ═══════════════════════════════════════════════════════
    //  Helpers
    // ═══════════════════════════════════════════════════════

    private void CmSetStatus(string text, bool success)
    {
        _cmStatus.Text = text;
        _cmStatus.ForeColor = success ? Color.FromArgb(40, 110, 40) : Color.FromArgb(170, 0, 0);
    }

    /// <summary>A darker shade of a colour (texture low point).</summary>
    private static Color Darker(Color c, double f = 0.62)
        => Color.FromArgb(255, (int)(c.R * f), (int)(c.G * f), (int)(c.B * f));

    /// <summary>A lighter shade of a colour (texture high point).</summary>
    private static Color Lighter(Color c, double f = 0.40)
        => Color.FromArgb(255,
            c.R + (int)((255 - c.R) * f),
            c.G + (int)((255 - c.G) * f),
            c.B + (int)((255 - c.B) * f));

    /// <summary>HSV (h 0-360, s/v 0-1) to an opaque Color.</summary>
    private static Color Hsv(double h, double s, double v)
    {
        double c = v * s;
        double hp = h / 60.0;
        double x = c * (1 - Math.Abs(hp % 2 - 1));
        double r = 0, g = 0, b = 0;

        if (hp < 1) { r = c; g = x; }
        else if (hp < 2) { r = x; g = c; }
        else if (hp < 3) { g = c; b = x; }
        else if (hp < 4) { g = x; b = c; }
        else if (hp < 5) { r = x; b = c; }
        else { r = c; b = x; }

        double m = v - c;
        return Color.FromArgb(
            Math.Clamp((int)Math.Round((r + m) * 255), 0, 255),
            Math.Clamp((int)Math.Round((g + m) * 255), 0, 255),
            Math.Clamp((int)Math.Round((b + m) * 255), 0, 255));
    }

    private void DisposeColorMapTiles()
    {
        foreach (Bitmap b in _cmTiles) b.Dispose();
        _cmTiles.Clear();
    }

    private void DisposeColorMapResources()
    {
        DisposeColorMapTiles();
        _cmPreviewBmp?.Dispose();
    }
}
