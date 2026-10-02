using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;
using System.Windows.Forms;

namespace EntitledLogicTileGen;

/// <summary>
/// Tile Blender tab.  Lets the user pick a central tile plus up to eight
/// neighbours (N, S, E, W and the four diagonals) and blend their shared
/// edges/corners into a seamless set, saving every blended tile as a new
/// PNG while leaving the originals untouched.
///
/// All blending maths lives in <see cref="TileBlender"/>; this file is only
/// UI, palette handling, preview rendering and saving.
/// </summary>
public sealed partial class MainForm
{
    // ═══════════════════════════════════════════════════════
    //  Blend tab — state & control references
    // ═══════════════════════════════════════════════════════

    private TabPage _tabBlend = null!;

    // Palette (independent from the Map tab's palette)
    private TextBox _blendFolder = null!;
    private Label   _blendStatus = null!;
    private Panel   _blendScroll = null!;

    private readonly List<TileInfo>  _blendPalette = new();
    private TileInfo?       _blendBrush      = null;   // tile currently "held"
    private TilePaletteEntry? _blendBrushEntry = null;

    // 3 × 3 assignment grid.  [col, row]; [1, 1] is the centre.
    private readonly TileInfo?[,] _blendCells = new TileInfo?[3, 3];
    private readonly BlendCell[,] _blendCellCtrls = new BlendCell[3, 3];

    private NumericUpDown _blendWidth = null!;
    private PictureBox    _blendPreview = null!;
    private ComboBox      _blendView = null!;
    private Bitmap?       _blendPreviewBmp = null;
    private Label         _blendOpStatus = null!;

    private const int BlendThumb = 48;   // palette thumbnail size (matches map tab)

    // Role labels for the nine cells, indexed [col, row].
    private static readonly string[,] CellRoles =
    {
        { "NW", "W", "SW" },   // col 0
        { "N",  "C", "S"  },   // col 1
        { "NE", "E", "SE" }    // col 2
    };

    // ═══════════════════════════════════════════════════════
    //  Construction
    // ═══════════════════════════════════════════════════════

    private void BuildBlendTab()
    {
        BuildBlendPalette();
        BuildBlendGrid();
        BuildBlendSettings();
        BuildBlendPreview();

        UpdateBlendPreview();
    }

    // -------------------------------------------------------
    //  Palette panel
    // -------------------------------------------------------

    private void BuildBlendPalette()
    {
        Panel panel = new Panel
        {
            Location = new Point(10, 10),
            Size = new Size(325, 700),
            BorderStyle = BorderStyle.FixedSingle,
            BackColor = Color.White
        };

        Label header = new Label
        {
            Text = "Tile Palette",
            Location = new Point(8, 6),
            AutoSize = true,
            Font = new Font("Segoe UI", 9f, FontStyle.Bold),
            ForeColor = Color.FromArgb(60, 60, 60)
        };

        _blendFolder = new TextBox
        {
            Location = new Point(8, 28),
            Size = new Size(240, 23),
            Text = @"D:\work\The AI-Native Game Guru\Tiles",
            Font = new Font("Segoe UI", 9f)
        };

        Button btnBrowse = new Button
        {
            Text = "…",
            Location = new Point(252, 27),
            Size = new Size(28, 25),
            Font = new Font("Segoe UI", 9f, FontStyle.Bold),
            Cursor = Cursors.Hand
        };
        btnBrowse.Click += BlendBrowseFolder_Click;

        Button btnRefresh = new Button
        {
            Text = "↺",
            Location = new Point(284, 27),
            Size = new Size(28, 25),
            Font = new Font("Segoe UI", 10f, FontStyle.Bold),
            Cursor = Cursors.Hand
        };
        btnRefresh.Click += (_, _) => LoadBlendTiles();

        _blendStatus = new Label
        {
            Location = new Point(8, 57),
            Size = new Size(305, 16),
            AutoSize = false,
            Font = new Font("Segoe UI", 8f, FontStyle.Italic),
            ForeColor = Color.FromArgb(120, 120, 120),
            Text = ""
        };

        Label hint = new Label
        {
            Location = new Point(8, 74),
            Size = new Size(305, 16),
            AutoSize = false,
            Font = new Font("Segoe UI", 8f, FontStyle.Italic),
            ForeColor = Color.FromArgb(120, 120, 120),
            Text = "Click a tile, then click a grid cell to place it."
        };

        _blendScroll = new Panel
        {
            Location = new Point(6, 94),
            Size = new Size(313, 598),
            AutoScroll = true,
            BorderStyle = BorderStyle.None,
            BackColor = Color.White
        };

        panel.Controls.Add(header);
        panel.Controls.Add(_blendFolder);
        panel.Controls.Add(btnBrowse);
        panel.Controls.Add(btnRefresh);
        panel.Controls.Add(_blendStatus);
        panel.Controls.Add(hint);
        panel.Controls.Add(_blendScroll);

        _tabBlend.Controls.Add(panel);

        LoadBlendTiles();
    }

    private void BlendBrowseFolder_Click(object? sender, EventArgs e)
    {
        using FolderBrowserDialog dlg = new FolderBrowserDialog
        {
            Description = "Select folder containing tile PNG files",
            SelectedPath = _blendFolder.Text,
            ShowNewFolderButton = false
        };

        if (dlg.ShowDialog(this) == DialogResult.OK)
        {
            _blendFolder.Text = dlg.SelectedPath;
            LoadBlendTiles();
        }
    }

    /// <summary>
    /// (Re)scans the palette folder.  Clears the brush and any grid
    /// assignments, since reloading disposes the previous TileInfo objects.
    /// </summary>
    private void LoadBlendTiles()
    {
        // Clear grid assignments before disposing the tiles they point at.
        for (int cy = 0; cy < 3; cy++)
        {
            for (int cx = 0; cx < 3; cx++)
            {
                _blendCells[cx, cy] = null;
                _blendCellCtrls[cx, cy]?.SetTile(null);
            }
        }

        _blendBrush = null;
        _blendBrushEntry = null;

        foreach (TileInfo t in _blendPalette) t.Dispose();
        _blendPalette.Clear();
        _blendScroll.Controls.Clear();

        string folder = _blendFolder.Text.Trim();
        if (string.IsNullOrEmpty(folder) || !Directory.Exists(folder))
        {
            _blendStatus.Text = "(folder not found)";
            _blendStatus.ForeColor = Color.FromArgb(160, 0, 0);
            UpdateBlendPreview();
            return;
        }

        string[] files;
        try
        {
            files = Directory.GetFiles(folder, "*.png");
        }
        catch (Exception ex)
        {
            _blendStatus.Text = $"(error: {ex.Message})";
            _blendStatus.ForeColor = Color.FromArgb(160, 0, 0);
            UpdateBlendPreview();
            return;
        }

        Array.Sort(files, StringComparer.OrdinalIgnoreCase);

        int loaded = 0;
        int skipped = 0;
        for (int i = 0; i < files.Length && loaded < MaxTiles; i++)
        {
            try
            {
                using Bitmap source = new Bitmap(files[i]);
                TileInfo info = new TileInfo
                {
                    Index = loaded + 1,
                    Filename = Path.GetFileName(files[i]),
                    Original = new Bitmap(source),
                    Thumbnail = MakeThumbnail(source, BlendThumb),
                    MiniThumb = MakeThumbnail(source, MapCellSize)
                };
                _blendPalette.Add(info);
                loaded++;
            }
            catch
            {
                skipped++;
            }
        }

        if (_blendPalette.Count == 0)
        {
            _blendStatus.Text = "(no tiles in folder)";
            _blendStatus.ForeColor = Color.FromArgb(120, 120, 120);
        }
        else
        {
            string suffix = _blendPalette.Count == 1 ? "" : "s";
            string skipStr = skipped > 0 ? $"  ({skipped} skipped)" : "";
            string truncStr = files.Length > MaxTiles
                ? $"  (max {MaxTiles}, {files.Length - MaxTiles} unused)"
                : "";
            _blendStatus.Text = $"{_blendPalette.Count} tile{suffix} loaded{skipStr}{truncStr}";
            _blendStatus.ForeColor = Color.FromArgb(70, 110, 70);
        }

        BuildBlendPaletteEntries();
        UpdateBlendPreview();
    }

    private void BuildBlendPaletteEntries()
    {
        _blendScroll.SuspendLayout();
        _blendScroll.Controls.Clear();

        int entrySize = BlendThumb + 4;
        int step = entrySize + PaletteGap;

        int col = 0;
        int x = PaletteGap;
        int y = PaletteGap;

        foreach (TileInfo tile in _blendPalette)
        {
            TilePaletteEntry entry = new TilePaletteEntry(tile)
            {
                Location = new Point(x, y),
                Size = new Size(entrySize, entrySize)
            };
            entry.Click += BlendPaletteEntry_Click;
            _blendScroll.Controls.Add(entry);

            col++;
            if (col >= PaletteCols)
            {
                col = 0;
                x = PaletteGap;
                y += step;
            }
            else
            {
                x += step;
            }
        }

        _blendScroll.ResumeLayout();
    }

    private void BlendPaletteEntry_Click(object? sender, EventArgs e)
    {
        if (sender is not TilePaletteEntry entry) return;

        if (_blendBrushEntry != null && _blendBrushEntry != entry)
        {
            _blendBrushEntry.IsSelected = false;
            _blendBrushEntry.Invalidate();
        }

        entry.IsSelected = true;
        entry.Invalidate();

        _blendBrushEntry = entry;
        _blendBrush = entry.Tile;
    }

    // -------------------------------------------------------
    //  3 × 3 assignment grid
    // -------------------------------------------------------

    private void BuildBlendGrid()
    {
        GroupBox grp = MakeGroup("Neighbours", 348, 10, 330, 360);

        const int cell = 96;
        const int gap  = 6;
        const int x0   = 18;
        const int y0   = 28;

        for (int cy = 0; cy < 3; cy++)
        {
            for (int cx = 0; cx < 3; cx++)
            {
                BlendCell c = new BlendCell(cx, cy, CellRoles[cx, cy])
                {
                    Location = new Point(x0 + cx * (cell + gap), y0 + cy * (cell + gap)),
                    Size = new Size(cell, cell)
                };
                c.MouseDown += BlendCell_MouseDown;
                _blendCellCtrls[cx, cy] = c;
                grp.Controls.Add(c);
            }
        }

        Label note = new Label
        {
            Location = new Point(18, y0 + 2 * (cell + gap) + cell + 6),
            Size = new Size(294, 28),
            Font = new Font("Segoe UI", 8f, FontStyle.Italic),
            ForeColor = Color.FromArgb(120, 120, 120),
            Text = "Centre (C) is required. Right-click a cell to clear it."
        };
        grp.Controls.Add(note);

        _tabBlend.Controls.Add(grp);
    }

    private void BlendCell_MouseDown(object? sender, MouseEventArgs e)
    {
        if (sender is not BlendCell cell) return;

        if (e.Button == MouseButtons.Right)
        {
            _blendCells[cell.Col, cell.Row] = null;
            cell.SetTile(null);
            UpdateBlendPreview();
            return;
        }

        if (e.Button == MouseButtons.Left)
        {
            if (_blendBrush == null)
            {
                SetBlendStatus("Pick a tile from the palette first.", success: false);
                return;
            }

            _blendCells[cell.Col, cell.Row] = _blendBrush;
            cell.SetTile(_blendBrush);
            UpdateBlendPreview();
        }
    }

    // -------------------------------------------------------
    //  Settings + actions
    // -------------------------------------------------------

    private void BuildBlendSettings()
    {
        GroupBox grp = MakeGroup("Blend Settings", 348, 380, 330, 150);

        Label lblW = MakeLabel("Blend width (px into each tile):", 14, 32);
        _blendWidth = new NumericUpDown
        {
            Location = new Point(230, 29),
            Size = new Size(60, 23),
            Minimum = 1,
            Maximum = 32,
            Value = 10,
            Font = new Font("Segoe UI", 9f)
        };
        _blendWidth.ValueChanged += (_, _) => UpdateBlendPreview();

        Button btnClear = MakeButton("Clear All", 14, 70, 120, 32);
        btnClear.Click += BlendClear_Click;

        Button btnSave = MakeButton("Save Blended Tiles", 150, 70, 162, 32);
        btnSave.BackColor = Color.FromArgb(70, 130, 70);
        btnSave.ForeColor = Color.White;
        btnSave.FlatStyle = FlatStyle.Flat;
        btnSave.FlatAppearance.BorderSize = 0;
        btnSave.Click += BlendSave_Click;

        _blendOpStatus = new Label
        {
            Location = new Point(14, 112),
            Size = new Size(300, 30),
            AutoSize = false,
            Font = new Font("Segoe UI", 8.5f),
            ForeColor = Color.FromArgb(90, 90, 90),
            Text = ""
        };

        grp.Controls.Add(lblW);
        grp.Controls.Add(_blendWidth);
        grp.Controls.Add(btnClear);
        grp.Controls.Add(btnSave);
        grp.Controls.Add(_blendOpStatus);

        _tabBlend.Controls.Add(grp);
    }

    private void BlendClear_Click(object? sender, EventArgs e)
    {
        for (int cy = 0; cy < 3; cy++)
        {
            for (int cx = 0; cx < 3; cx++)
            {
                _blendCells[cx, cy] = null;
                _blendCellCtrls[cx, cy]?.SetTile(null);
            }
        }
        UpdateBlendPreview();
        SetBlendStatus("", success: true);
    }

    // -------------------------------------------------------
    //  Preview
    // -------------------------------------------------------

    private void BuildBlendPreview()
    {
        GroupBox grp = MakeGroup("Preview", 688, 10, 315, 360);

        _blendPreview = new PictureBox
        {
            Location = new Point(15, 28),
            Size = new Size(285, 258),
            BorderStyle = BorderStyle.FixedSingle,
            BackColor = Color.FromArgb(245, 245, 245),
            SizeMode = PictureBoxSizeMode.Zoom
        };

        Label viewLbl = new Label
        {
            Text = "View:",
            Location = new Point(15, 295),
            AutoSize = true,
            Font = new Font("Segoe UI", 8.5f)
        };
        _blendView = new ComboBox
        {
            Location = new Point(58, 292),
            Size = new Size(160, 23),
            DropDownStyle = ComboBoxStyle.DropDownList,
            Font = new Font("Segoe UI", 8.5f)
        };
        _blendView.Items.AddRange(new object[]
        {
            "Blended 3 × 3 patch", "Centre tiled 2×2", "Centre tiled 3×3", "Centre tiled 4×4"
        });
        _blendView.SelectedIndex = 0;
        _blendView.SelectedIndexChanged += (_, _) => UpdateBlendPreview();

        Label note = new Label
        {
            Location = new Point(15, 322),
            Size = new Size(285, 30),
            Font = new Font("Segoe UI", 8f, FontStyle.Italic),
            ForeColor = Color.FromArgb(120, 120, 120),
            Text = "Centre-tiled views show how the blended centre repeats."
        };

        grp.Controls.Add(_blendPreview);
        grp.Controls.Add(viewLbl);
        grp.Controls.Add(_blendView);
        grp.Controls.Add(note);
        _tabBlend.Controls.Add(grp);
    }

    /// <summary>
    /// Rebuilds the preview.  In "patch" view it lays the (up to) nine
    /// blended results out as a 192 × 192 image; in a "centre tiled" view it
    /// repeats the blended centre tile N × N so its own tiling can be judged.
    /// Empty cells render as a neutral placeholder.
    /// </summary>
    private void UpdateBlendPreview()
    {
        if (_blendPreview == null) return;

        Bitmap?[,] blended = BuildBlendedGrid();

        int view = _blendView?.SelectedIndex ?? 0;
        Bitmap canvas = view >= 1
            ? RenderCentreTiled(blended, view + 1)   // 2,3,4
            : RenderPatch(blended);

        // Dispose the freshly-blended preview tiles; the canvas now owns the pixels.
        DisposeGrid(blended);

        _blendPreviewBmp?.Dispose();
        _blendPreviewBmp = canvas;
        _blendPreview.Image = _blendPreviewBmp;
    }

    private static Bitmap RenderPatch(Bitmap?[,] blended)
    {
        const int t = TileBlender.TileSize;   // 64
        int side = t * 3;                      // 192

        Bitmap canvas = new Bitmap(side, side, PixelFormat.Format32bppArgb);
        using Graphics g = Graphics.FromImage(canvas);
        g.InterpolationMode = InterpolationMode.NearestNeighbor;
        g.PixelOffsetMode = PixelOffsetMode.Half;
        g.Clear(Color.FromArgb(245, 245, 245));

        for (int cy = 0; cy < 3; cy++)
        {
            for (int cx = 0; cx < 3; cx++)
            {
                int px = cx * t;
                int py = cy * t;

                Bitmap? cellImg = blended[cx, cy];
                if (cellImg != null)
                {
                    g.DrawImage(cellImg, px, py, t, t);
                }
                else
                {
                    using SolidBrush back = new(Color.FromArgb(232, 232, 232));
                    g.FillRectangle(back, px, py, t, t);
                    using Pen dash = new(Color.FromArgb(205, 205, 205)) { DashStyle = DashStyle.Dot };
                    g.DrawRectangle(dash, px + 1, py + 1, t - 3, t - 3);
                }
            }
        }

        return canvas;
    }

    private Bitmap RenderCentreTiled(Bitmap?[,] blended, int n)
    {
        Bitmap? centre = blended[1, 1];
        if (centre == null)
        {
            const int t = TileBlender.TileSize;
            Bitmap empty = new Bitmap(t, t, PixelFormat.Format32bppArgb);
            using Graphics g = Graphics.FromImage(empty);
            g.Clear(Color.FromArgb(232, 232, 232));
            using Pen dash = new(Color.FromArgb(205, 205, 205)) { DashStyle = DashStyle.Dot };
            g.DrawRectangle(dash, 1, 1, t - 3, t - 3);
            return empty;
        }

        return TileBlender.Tile(centre, n);
    }

    /// <summary>
    /// Runs <see cref="TileBlender.BlendPatch"/> on the current assignment
    /// grid using the live blend-width value.  Returns a 3 × 3 grid of new
    /// bitmaps (null where no tile is assigned).  Caller owns the bitmaps.
    /// </summary>
    private Bitmap?[,] BuildBlendedGrid()
    {
        Bitmap?[,] sources = new Bitmap?[3, 3];
        for (int cy = 0; cy < 3; cy++)
        {
            for (int cx = 0; cx < 3; cx++)
            {
                sources[cx, cy] = _blendCells[cx, cy]?.Original;
            }
        }

        return TileBlender.BlendPatch(sources, (int)_blendWidth.Value);
    }

    // -------------------------------------------------------
    //  Save
    // -------------------------------------------------------

    private void BlendSave_Click(object? sender, EventArgs e)
    {
        if (_blendCells[1, 1] == null)
        {
            SetBlendStatus("✘  Set a centre tile (C) before saving.", success: false);
            return;
        }

        if (CountNeighbours() == 0)
        {
            SetBlendStatus("✘  Select at least one neighbour to blend.", success: false);
            return;
        }

        string folder = _blendFolder.Text.Trim();
        if (string.IsNullOrEmpty(folder))
        {
            SetBlendStatus("✘  No output folder set.", success: false);
            return;
        }

        Bitmap?[,] blended = BuildBlendedGrid();

        try
        {
            Cursor = Cursors.WaitCursor;
            Directory.CreateDirectory(folder);

            string stamp = DateTime.Now.ToString("yyyyMMdd_HHmmss_fff");
            int saved = 0;

            for (int cy = 0; cy < 3; cy++)
            {
                for (int cx = 0; cx < 3; cx++)
                {
                    Bitmap? img = blended[cx, cy];
                    TileInfo? src = _blendCells[cx, cy];
                    if (img == null || src == null) continue;

                    string baseName = Path.GetFileNameWithoutExtension(src.Filename);
                    string role = CellRoles[cx, cy];
                    string filename = $"blend_{baseName}_{role}_{stamp}.png";
                    img.Save(Path.Combine(folder, filename), ImageFormat.Png);
                    saved++;
                }
            }

            SetBlendStatus($"✔  Saved {saved} blended tile{(saved == 1 ? "" : "s")} to: {folder}",
                           success: true);

            // Refresh the Blend palette so the new tiles appear (this clears the grid).
            LoadBlendTiles();

            // If the Map tab's palette points at the same folder, refresh it too
            // so the new blend_* tiles show up there without a manual Refresh.
            TryRefreshMapPalette(folder);
        }
        catch (Exception ex)
        {
            SetBlendStatus($"✘  Error: {ex.Message}", success: false);
        }
        finally
        {
            DisposeGrid(blended);
            Cursor = Cursors.Default;
        }
    }

    private int CountNeighbours()
    {
        int n = 0;
        for (int cy = 0; cy < 3; cy++)
        {
            for (int cx = 0; cx < 3; cx++)
            {
                if (cx == 1 && cy == 1) continue;   // skip centre
                if (_blendCells[cx, cy] != null) n++;
            }
        }
        return n;
    }

    // -------------------------------------------------------
    //  Helpers / cleanup
    // -------------------------------------------------------

    private void SetBlendStatus(string text, bool success)
    {
        _blendOpStatus.Text = text;
        _blendOpStatus.ForeColor = success
            ? Color.FromArgb(40, 110, 40)
            : Color.FromArgb(170, 0, 0);
    }

    /// <summary>
    /// Reloads the Map tab's palette, but only when it is pointed at the same
    /// folder the blended tiles were just written to — so the new tiles appear
    /// there too.  When the folders differ a refresh would show nothing new
    /// (and would needlessly reset the Map tab's selection), so it is skipped.
    /// </summary>
    private void TryRefreshMapPalette(string blendFolder)
    {
        string mapFolder = _tilesFolder.Text.Trim();
        if (string.IsNullOrEmpty(mapFolder) || string.IsNullOrEmpty(blendFolder)) return;

        try
        {
            char[] trim = { Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar };
            string a = Path.GetFullPath(blendFolder).TrimEnd(trim);
            string b = Path.GetFullPath(mapFolder).TrimEnd(trim);

            if (string.Equals(a, b, StringComparison.OrdinalIgnoreCase))
            {
                LoadTiles();
            }
        }
        catch
        {
            // Best-effort: a bad path just means no Map-tab refresh.
        }
    }

    private static void DisposeGrid(Bitmap?[,] grid)
    {
        for (int cy = 0; cy < 3; cy++)
        {
            for (int cx = 0; cx < 3; cx++)
            {
                grid[cx, cy]?.Dispose();
                grid[cx, cy] = null;
            }
        }
    }

    private void DisposeBlendResources()
    {
        _blendPreviewBmp?.Dispose();
        foreach (TileInfo t in _blendPalette) t.Dispose();
        _blendPalette.Clear();
    }

    // ═══════════════════════════════════════════════════════
    //  Nested type — one cell of the 3 × 3 assignment grid
    // ═══════════════════════════════════════════════════════

    /// <summary>
    /// A clickable cell in the neighbour grid.  Paints its assigned tile
    /// (or an empty placeholder), the role label (N, NE, C…) and a border
    /// that highlights the required centre cell.
    /// </summary>
    private sealed class BlendCell : Panel
    {
        public int    Col  { get; }
        public int    Row  { get; }
        public string Role { get; }

        private bool IsCentre => Col == 1 && Row == 1;

        private TileInfo? _tile;

        public BlendCell(int col, int row, string role)
        {
            Col  = col;
            Row  = row;
            Role = role;
            DoubleBuffered = true;
            BackColor = Color.White;
            Cursor = Cursors.Hand;
        }

        public void SetTile(TileInfo? tile)
        {
            _tile = tile;
            Invalidate();
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            Graphics g = e.Graphics;
            g.InterpolationMode = InterpolationMode.NearestNeighbor;
            g.PixelOffsetMode = PixelOffsetMode.Half;

            if (_tile?.Thumbnail != null)
            {
                g.DrawImage(_tile.Thumbnail, 3, 3, Width - 6, Height - 6);
            }
            else
            {
                using SolidBrush back = new(Color.FromArgb(243, 243, 243));
                g.FillRectangle(back, 0, 0, Width, Height);
            }

            // Role label — top-left on a translucent plate
            using Font roleFont = new Font("Segoe UI", 8f, FontStyle.Bold);
            using SolidBrush plate = new(Color.FromArgb(150, 0, 0, 0));
            SizeF rs = g.MeasureString(Role, roleFont);
            g.FillRectangle(plate, 3, 3, rs.Width + 4, rs.Height);
            using SolidBrush rb = new(Color.White);
            g.DrawString(Role, roleFont, rb, 5, 3);

            // Index number — bottom-right, if a tile is assigned
            if (_tile != null)
            {
                string num = _tile.Index.ToString();
                using Font numFont = new Font("Segoe UI", 7.5f, FontStyle.Bold);
                SizeF ts = g.MeasureString(num, numFont);
                float tx = Width - ts.Width - 4;
                float ty = Height - ts.Height - 2;
                using SolidBrush np = new(Color.FromArgb(170, 0, 0, 0));
                g.FillRectangle(np, tx - 1, ty + 1, ts.Width + 2, ts.Height - 2);
                using SolidBrush nb = new(Color.White);
                g.DrawString(num, numFont, nb, tx, ty);
            }

            // Border: the centre is always emphasised; filled cells get a
            // mid-grey edge; empty cells a light dashed edge.
            Color borderColor;
            int   borderWidth;
            if (IsCentre)
            {
                borderColor = Color.FromArgb(240, 130, 0);
                borderWidth = 2;
            }
            else if (_tile != null)
            {
                borderColor = Color.FromArgb(150, 150, 150);
                borderWidth = 1;
            }
            else
            {
                borderColor = Color.FromArgb(205, 205, 205);
                borderWidth = 1;
            }

            using Pen pen = new Pen(borderColor, borderWidth);
            if (_tile == null && !IsCentre) pen.DashStyle = DashStyle.Dash;
            int inset = borderWidth - 1;
            g.DrawRectangle(pen, inset, inset, Width - 1 - inset * 2, Height - 1 - inset * 2);
        }
    }
}
