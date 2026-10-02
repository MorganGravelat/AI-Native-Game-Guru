using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace EntitledLogicTileGen;

/// <summary>
/// Main application window for the Entitled Logic procedural tile generator.
/// The entire UI is built in code; no Designer.cs is required.
/// </summary>
public sealed partial class MainForm : Form
{
    // ═══════════════════════════════════════════════════════
    //  State
    // ═══════════════════════════════════════════════════════

    private Color _color1 = Color.FromArgb(34, 100, 34);     // forest green
    private Color _color2 = Color.FromArgb(194, 178, 128);   // tan / sand
    private Bitmap? _lastTile = null;
    private Bitmap? _previewBmp = null;
    private bool _suppressRgbEvent = false;

    private static readonly int[] PreviewScales = { 1, 2, 3, 4, 5, 6, 7 };

    // ═══════════════════════════════════════════════════════
    //  Control references  (assigned in BuildUI)
    // ═══════════════════════════════════════════════════════

    // Tabs
    private TabControl _tabs = null!;
    private TabPage _tabTile = null!;
    private TabPage _tabMap = null!;

    // Color 1
    private Panel panelColor1 = null!;
    private Label lblRgb1 = null!;
    private NumericUpDown nudR1 = null!;
    private NumericUpDown nudG1 = null!;
    private NumericUpDown nudB1 = null!;

    // Color 2
    private Panel panelColor2 = null!;
    private Label lblRgb2 = null!;
    private NumericUpDown nudR2 = null!;
    private NumericUpDown nudG2 = null!;
    private NumericUpDown nudB2 = null!;

    // Generation settings
    private ComboBox cmbAlgorithm = null!;
    private TrackBar trackSat = null!;
    private Label lblSatVal = null!;
    private TrackBar trackNoise = null!;
    private Label lblNoiseVal = null!;
    private CheckBox chkRandSeed = null!;
    private NumericUpDown nudSeed = null!;

    // Output
    private TextBox txtOutputPath = null!;

    // Status
    private Label lblStatus = null!;

    // Preview
    private PictureBox pictureBox = null!;
    private ComboBox cmbScale = null!;

    // ═══════════════════════════════════════════════════════
    //  Map tab — state
    // ═══════════════════════════════════════════════════════

    private const int MapCells = 64;
    private const int MapCellSize = 10;
    private const int MapPixels = MapCells * MapCellSize;   // 640

    // _mapData[x, y]: the byte value that will eventually be saved.
    //   0       = not in map / blank
    //   1..255  = tile index
    private readonly ushort[,] _mapData = new ushort[MapCells, MapCells];

    // _inMap[x, y]: editor-only flag controlling whether the cell is
    // drawn (white + black border) or hidden (matches form background).
    // The whole grid starts true so the user sees a full 64×64 grid.
    private readonly bool[,] _inMap = new bool[MapCells, MapCells];

    private Bitmap? _mapBitmap = null;

    // ═══════════════════════════════════════════════════════
    //  Map tab — control references  (assigned in BuildMapTab)
    // ═══════════════════════════════════════════════════════

    private PictureBox _mapGrid = null!;
    private Panel _palettePanel = null!;

    // Settings
    private NumericUpDown _mapLevel = null!;
    private TextBox _mapOutputPath = null!;
    private CheckBox _chkStructureMode = null!;
    private Label _mapStatus = null!;

    // Effects
    private CheckBox _chkSymX = null!;
    private CheckBox _chkSymY = null!;
    private CheckBox _chkBorderWall = null!;
    private CheckBox _chkOpenArena = null!;
    private CheckBox _chkScatteredPillars = null!;
    private CheckBox _chkChokePoints = null!;
    private CheckBox _chkDeadEndPockets = null!;
    private CheckBox _chkMazeLike = null!;

    // ═══════════════════════════════════════════════════════
    //  Tile palette — state & controls
    // ═══════════════════════════════════════════════════════

    private const int ThumbSize = 48;   // px per palette thumbnail
    private const int PaletteCols = 5;
    private const int PaletteGap = 6;
    private const int MaxTiles = 65535;  // ushort upper bound (16-bit tile indices)

    private TextBox _tilesFolder = null!;
    private Label _tilesStatus = null!;
    private Panel _tilesScroll = null!;   // scrollable container for entries

    private readonly List<TileInfo> _palette = new();
    private int _selectedTileIndex = 0;     // 0 = nothing selected
    private TilePaletteEntry? _selectedEntry = null;

    // RNG for the BSP map generator.  Re-seeded on every Generate click.
    private Random _rng = new Random();

    // ═══════════════════════════════════════════════════════
    //  Constructor
    // ═══════════════════════════════════════════════════════

    public MainForm()
    {
        Text = "Entitled Logic  ·  Procedural Tile Generator";
        ClientSize = new Size(1230, 770);  // widened for map-tab layers/tools/data column
        MinimumSize = new Size(1230, 770);
        FormBorderStyle = FormBorderStyle.FixedSingle;
        MaximizeBox = false;
        BackColor = Color.FromArgb(235, 235, 235);
        Font = new Font("Segoe UI", 9f, FontStyle.Regular, GraphicsUnit.Point);

        SuspendLayout();
        BuildUI();
        ApplyDefaults();
        LoadSettings();
        ResumeLayout(performLayout: true);
    }

    // ═══════════════════════════════════════════════════════
    //  Settings persistence — remembers folder paths between runs
    //  (stored at %LocalAppData%\EntitledLogicTileGen\settings.json)
    // ═══════════════════════════════════════════════════════

    private sealed class AppSettings
    {
        public string TilesFolder { get; set; } = string.Empty;
        public string MapOutputPath { get; set; } = string.Empty;
        public string ColorMapOutputPath { get; set; } = string.Empty;
    }

    private static string SettingsFilePath => Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "EntitledLogicTileGen", "settings.json");

    private void LoadSettings()
    {
        try
        {
            if (!File.Exists(SettingsFilePath)) return;
            AppSettings? s = System.Text.Json.JsonSerializer.Deserialize<AppSettings>(
                File.ReadAllText(SettingsFilePath));
            if (s == null) return;
            if (!string.IsNullOrWhiteSpace(s.TilesFolder)) _tilesFolder.Text = s.TilesFolder;
            if (!string.IsNullOrWhiteSpace(s.MapOutputPath)) _mapOutputPath.Text = s.MapOutputPath;
            if (!string.IsNullOrWhiteSpace(s.ColorMapOutputPath)) _cmOutputPath.Text = s.ColorMapOutputPath;
        }
        catch
        {
            // Ignore unreadable / corrupt settings — fall back to blank fields.
        }
    }

    private void SaveSettings()
    {
        try
        {
            AppSettings s = new()
            {
                TilesFolder = _tilesFolder.Text,
                MapOutputPath = _mapOutputPath.Text,
                ColorMapOutputPath = _cmOutputPath.Text
            };
            Directory.CreateDirectory(Path.GetDirectoryName(SettingsFilePath)!);
            File.WriteAllText(SettingsFilePath,
                System.Text.Json.JsonSerializer.Serialize(
                    s, new System.Text.Json.JsonSerializerOptions { WriteIndented = true }));
        }
        catch
        {
            // Settings are best-effort; never block exit on a write failure.
        }
    }

    protected override void OnFormClosing(FormClosingEventArgs e)
    {
        SaveSettings();
        base.OnFormClosing(e);
    }

    // ═══════════════════════════════════════════════════════
    //  UI construction
    // ═══════════════════════════════════════════════════════

    private void BuildUI()
    {
        BuildTabs();
        BuildColor1Group();
        BuildColor2Group();
        BuildSettingsGroup();
        BuildOutputGroup();
        BuildActionButtons();
        BuildStatusLabel();
        BuildPreviewGroup();
        BuildGeneratorExtras();
        BuildMapTab();
        BuildMapToolsColumn();
        BuildBlendTab();
        BuildColorMapTab();
    }

    // -------------------------------------------------------

    private void BuildTabs()
    {
        _tabs = new TabControl
        {
            Dock = DockStyle.Fill,
            Font = new Font("Segoe UI", 9f, FontStyle.Regular)
        };

        _tabTile = new TabPage("Tile Generator")
        {
            // Match the form's grey so the existing UI looks identical inside the tab.
            UseVisualStyleBackColor = false,
            BackColor = Color.FromArgb(235, 235, 235)
        };

        _tabMap = new TabPage("Map Generator")
        {
            UseVisualStyleBackColor = false,
            BackColor = Color.FromArgb(235, 235, 235)
            // Deliberately empty — UI for this tab will be added later.
        };

        _tabBlend = new TabPage("Tile Blender")
        {
            UseVisualStyleBackColor = false,
            BackColor = Color.FromArgb(235, 235, 235)
            // UI built in BuildBlendTab (see MainForm.Blend.cs).
        };

        _tabColorMap = new TabPage("Map Colorizer")
        {
            UseVisualStyleBackColor = false,
            BackColor = Color.FromArgb(235, 235, 235)
            // UI built in BuildColorMapTab (see MainForm.ColorMap.cs).
        };

        _tabs.TabPages.Add(_tabTile);
        _tabs.TabPages.Add(_tabMap);
        _tabs.TabPages.Add(_tabBlend);
        _tabs.TabPages.Add(_tabColorMap);

        Controls.Add(_tabs);
    }

    // -------------------------------------------------------

    private void BuildColor1Group()
    {
        GroupBox grp = MakeGroup("Color 1", 10, 10, 220, 215);

        panelColor1 = new Panel
        {
            Location = new Point(10, 22),
            Size = new Size(72, 72),
            BorderStyle = BorderStyle.FixedSingle
        };

        Button btnPick = MakeButton("Pick…", 92, 22, 118, 28);
        btnPick.Click += (_, _) => PickColor(1);

        lblRgb1 = new Label
        {
            Location = new Point(92, 56),
            Size = new Size(118, 36),
            AutoSize = false,
            ForeColor = Color.FromArgb(60, 60, 60)
        };

        Label lR = MakeLabel("R:", 10, 108);
        nudR1 = MakeNud(32, 105, 0, 255);

        Label lG = MakeLabel("G:", 10, 136);
        nudG1 = MakeNud(32, 133, 0, 255);

        Label lB = MakeLabel("B:", 10, 164);
        nudB1 = MakeNud(32, 161, 0, 255);

        Label lHint = new Label
        {
            Text = "(edit then press Enter)",
            Location = new Point(92, 175),
            Size = new Size(120, 30),
            AutoSize = false,
            ForeColor = Color.Gray,
            Font = new Font("Segoe UI", 7.5f)
        };

        // Wire events — live update swatch while editing
        nudR1.ValueChanged += (_, _) => OnNudChanged(1);
        nudG1.ValueChanged += (_, _) => OnNudChanged(1);
        nudB1.ValueChanged += (_, _) => OnNudChanged(1);

        grp.Controls.AddRange(new Control[]
        {
            panelColor1, btnPick, lblRgb1,
            lR, nudR1, lG, nudG1, lB, nudB1, lHint
        });

        _tabTile.Controls.Add(grp);
    }

    // -------------------------------------------------------

    private void BuildColor2Group()
    {
        GroupBox grp = MakeGroup("Color 2", 240, 10, 220, 215);

        panelColor2 = new Panel
        {
            Location = new Point(10, 22),
            Size = new Size(72, 72),
            BorderStyle = BorderStyle.FixedSingle
        };

        Button btnPick = MakeButton("Pick…", 92, 22, 118, 28);
        btnPick.Click += (_, _) => PickColor(2);

        lblRgb2 = new Label
        {
            Location = new Point(92, 56),
            Size = new Size(118, 36),
            AutoSize = false,
            ForeColor = Color.FromArgb(60, 60, 60)
        };

        Label lR = MakeLabel("R:", 10, 108);
        nudR2 = MakeNud(32, 105, 0, 255);

        Label lG = MakeLabel("G:", 10, 136);
        nudG2 = MakeNud(32, 133, 0, 255);

        Label lB = MakeLabel("B:", 10, 164);
        nudB2 = MakeNud(32, 161, 0, 255);

        Label lHint = new Label
        {
            Text = "(edit then press Enter)",
            Location = new Point(92, 175),
            Size = new Size(120, 30),
            AutoSize = false,
            ForeColor = Color.Gray,
            Font = new Font("Segoe UI", 7.5f)
        };

        nudR2.ValueChanged += (_, _) => OnNudChanged(2);
        nudG2.ValueChanged += (_, _) => OnNudChanged(2);
        nudB2.ValueChanged += (_, _) => OnNudChanged(2);

        grp.Controls.AddRange(new Control[]
        {
            panelColor2, btnPick, lblRgb2,
            lR, nudR2, lG, nudG2, lB, nudB2, lHint
        });

        _tabTile.Controls.Add(grp);
    }

    // -------------------------------------------------------

    private void BuildSettingsGroup()
    {
        GroupBox grp = MakeGroup("Generation Settings", 10, 235, 450, 215);

        // Algorithm
        Label lAlg = MakeLabel("Algorithm:", 10, 30);
        cmbAlgorithm = new ComboBox
        {
            Location = new Point(110, 27),
            Size = new Size(328, 23),
            DropDownStyle = ComboBoxStyle.DropDownList
        };
        cmbAlgorithm.Items.AddRange(new object[]
        {
            "Perlin Noise",
            "Simplex Noise",
            "Value Noise",
            "Fractal Brownian Motion (fBm)",
            "Worley / Cellular Noise",
            "Turbulence"
        });

        // Saturation
        Label lSat = MakeLabel("Saturation:", 10, 74);
        trackSat = new TrackBar
        {
            Location = new Point(110, 64),
            Size = new Size(285, 45),
            Minimum = 1,
            Maximum = 100,
            TickFrequency = 10,
            SmallChange = 1,
            LargeChange = 10
        };
        lblSatVal = new Label
        {
            Location = new Point(400, 74),
            Size = new Size(38, 20),
            TextAlign = ContentAlignment.MiddleRight
        };
        trackSat.Scroll += (_, _) => lblSatVal.Text = trackSat.Value.ToString();

        // Noise level
        Label lNoise = MakeLabel("Noise Level:", 10, 122);
        trackNoise = new TrackBar
        {
            Location = new Point(110, 112),
            Size = new Size(285, 45),
            Minimum = 0,
            Maximum = 100,
            TickFrequency = 10,
            SmallChange = 1,
            LargeChange = 10
        };
        lblNoiseVal = new Label
        {
            Location = new Point(400, 122),
            Size = new Size(38, 20),
            TextAlign = ContentAlignment.MiddleRight
        };
        trackNoise.Scroll += (_, _) => lblNoiseVal.Text = trackNoise.Value.ToString();

        // Seed
        Label lSeedLbl = MakeLabel("Seed:", 10, 170);
        chkRandSeed = new CheckBox
        {
            Text = "Random seed",
            Location = new Point(68, 168),
            AutoSize = true,
            Checked = true
        };
        nudSeed = new NumericUpDown
        {
            Location = new Point(185, 166),
            Size = new Size(100, 23),
            Minimum = 0,
            Maximum = int.MaxValue,
            Increment = 1,
            Enabled = false,
            Value = 12345
        };
        chkRandSeed.CheckedChanged += (_, _) =>
        {
            nudSeed.Enabled = !chkRandSeed.Checked;
        };

        grp.Controls.AddRange(new Control[]
        {
            lAlg,   cmbAlgorithm,
            lSat,   trackSat,   lblSatVal,
            lNoise, trackNoise, lblNoiseVal,
            lSeedLbl, chkRandSeed, nudSeed
        });

        _tabTile.Controls.Add(grp);
    }

    // -------------------------------------------------------

    private void BuildOutputGroup()
    {
        GroupBox grp = MakeGroup("Output Path", 10, 460, 450, 72);

        Label lPath = MakeLabel("Path:", 10, 32);
        txtOutputPath = new TextBox
        {
            Location = new Point(55, 29),
            Size = new Size(290, 23)
        };

        Button btnBrowse = MakeButton("Browse…", 354, 27, 82, 27);
        btnBrowse.Click += BrowseOutput_Click;

        grp.Controls.AddRange(new Control[] { lPath, txtOutputPath, btnBrowse });
        _tabTile.Controls.Add(grp);
    }

    // -------------------------------------------------------

    private void BuildActionButtons()
    {
        Button btnGen = new Button
        {
            Text = "⚙  Generate && Save Tile",
            Location = new Point(10, 545),
            Size = new Size(218, 48),
            BackColor = Color.FromArgb(48, 120, 48),
            ForeColor = Color.White,
            FlatStyle = FlatStyle.Flat,
            Font = new Font("Segoe UI", 10f, FontStyle.Bold),
            Cursor = Cursors.Hand
        };
        btnGen.FlatAppearance.BorderSize = 0;
        btnGen.Click += GenerateTile_Click;

        Button btnBatch = new Button
        {
            Text = "⚡  Batch Generate",
            Location = new Point(240, 545),
            Size = new Size(218, 48),
            BackColor = Color.FromArgb(48, 80, 160),
            ForeColor = Color.White,
            FlatStyle = FlatStyle.Flat,
            Font = new Font("Segoe UI", 10f, FontStyle.Bold),
            Cursor = Cursors.Hand
        };
        btnBatch.FlatAppearance.BorderSize = 0;
        btnBatch.Click += GenerateBatch_Click;

        _tabTile.Controls.Add(btnGen);
        _tabTile.Controls.Add(btnBatch);
    }

    // -------------------------------------------------------

    private void BuildStatusLabel()
    {
        lblStatus = new Label
        {
            Location = new Point(10, 603),
            Size = new Size(460, 75),
            AutoSize = false,
            ForeColor = Color.FromArgb(70, 70, 70),
            Font = new Font("Segoe UI", 8.5f)
        };
        _tabTile.Controls.Add(lblStatus);
    }

    // -------------------------------------------------------

    private void BuildPreviewGroup()
    {
        GroupBox grp = MakeGroup("Preview", 475, 10, 490, 660);

        pictureBox = new PictureBox
        {
            Location = new Point(10, 28),
            Size = new Size(466, 466),
            BorderStyle = BorderStyle.FixedSingle,
            BackColor = Color.FromArgb(28, 28, 28),
            SizeMode = PictureBoxSizeMode.Normal
        };

        Label lTileLbl = MakeLabel("Tiled preview:", 10, 509);
        cmbTiling = new ComboBox
        {
            Location = new Point(110, 506),
            Size = new Size(120, 23),
            DropDownStyle = ComboBoxStyle.DropDownList
        };
        cmbTiling.Items.AddRange(new object[] { "1×1 (single)", "2×2", "3×3", "4×4" });
        cmbTiling.SelectedIndex = 1;   // default 2×2
        cmbTiling.SelectedIndexChanged += (_, _) => RefreshPreview();

        Label lScaleLbl = MakeLabel("Preview scale:", 10, 537);
        cmbScale = new ComboBox
        {
            Location = new Point(115, 534),
            Size = new Size(75, 23),
            DropDownStyle = ComboBoxStyle.DropDownList
        };
        foreach (int s in PreviewScales) cmbScale.Items.Add($"{s}×");
        cmbScale.SelectedIndex = 2;   // default 3×
        cmbScale.SelectedIndexChanged += (_, _) => RefreshPreview();

        Button btnRefresh = MakeButton("↺  Refresh", 205, 532, 100, 27);
        btnRefresh.Click += (_, _) => RefreshPreview();

        Label lblInfo = new Label
        {
            Text =
                "Tile is 64×64 px — seamlessly tileable.\n" +
                "Each pixel is mapped onto a 4-D torus, so opposite\n" +
                "edges match exactly with no mirroring or fold seam.",
            Location = new Point(10, 575),
            Size = new Size(466, 72),
            AutoSize = false,
            ForeColor = Color.FromArgb(110, 110, 110),
            Font = new Font("Segoe UI", 8f)
        };

        grp.Controls.AddRange(new Control[]
        {
            pictureBox, lTileLbl, cmbTiling, lScaleLbl, cmbScale, btnRefresh, lblInfo
        });

        _tabTile.Controls.Add(grp);
    }

    // ═══════════════════════════════════════════════════════
    //  Map tab — shell only (no interactivity wired yet)
    // ═══════════════════════════════════════════════════════

    private void BuildMapTab()
    {
        InitializeSampleMap();

        BuildMapGrid();
        BuildPalettePanel();
        BuildMapSettingsGroup();
        BuildMapEffectsGroup();
        BuildMapActionButtons();
    }

    /// <summary>
    /// Seeds the default map with three rooms connected by two
    /// corridors — placeholder content until the Generate button
    /// is wired up.  Every other cell is invalid (renders as light
    /// grey background), so the user sees the kind of "rooms with
    /// passages" shape that BSP generation will eventually produce.
    /// </summary>
    private void InitializeSampleMap()
    {
        for (int y = 0; y < MapCells; y++)
        {
            for (int x = 0; x < MapCells; x++)
            {
                _inMap[x, y] = false;
                _mapData[x, y] = 0;
            }
        }

        FillSampleRect(0, 0, 24, 22);   // Room A  (upper-left, reaches (0,0))
        FillSampleRect(28, 4, 46, 30);   // Room B  (middle, larger)
        FillSampleRect(26, 36, 63, 63);   // Room C  (lower-right, reaches (63,63))
        FillSampleRect(24, 12, 28, 14);   // Corridor A → B
        FillSampleRect(36, 30, 38, 36);   // Corridor B → C
    }

    private void FillSampleRect(int x1, int y1, int x2, int y2)
    {
        for (int y = y1; y <= y2; y++)
        {
            for (int x = x1; x <= x2; x++)
            {
                if (x >= 0 && x < MapCells && y >= 0 && y < MapCells)
                {
                    _inMap[x, y] = true;
                }
            }
        }
    }

    // -------------------------------------------------------

    private void BuildMapGrid()
    {
        _mapBitmap = new Bitmap(MapPixels, MapPixels, PixelFormat.Format32bppArgb);

        _mapGrid = new PictureBox
        {
            Location = new Point(10, 10),
            Size = new Size(MapPixels + 2, MapPixels + 2),   // +2 for FixedSingle border
            BorderStyle = BorderStyle.FixedSingle,
            BackColor = Color.FromArgb(235, 235, 235),
            SizeMode = PictureBoxSizeMode.Normal,
            Image = _mapBitmap
        };
        _mapGrid.MouseDown += MapGrid_MouseDown;

        RedrawMap();

        _tabMap.Controls.Add(_mapGrid);
    }

    // -------------------------------------------------------

    private void BuildPalettePanel()
    {
        _palettePanel = new Panel
        {
            Location = new Point(662, 10),
            Size = new Size(325, 360),
            BorderStyle = BorderStyle.FixedSingle,
            BackColor = Color.White
        };

        Label lblHeader = new Label
        {
            Text = "Tile Palette",
            Location = new Point(8, 6),
            AutoSize = true,
            Font = new Font("Segoe UI", 9f, FontStyle.Bold),
            ForeColor = Color.FromArgb(60, 60, 60)
        };

        // Folder picker row
        _tilesFolder = new TextBox
        {
            Location = new Point(8, 28),
            Size = new Size(240, 23),
            Text = @"D:\work\The AI-Native Game Guru\Tiles",
            Font = new Font("Segoe UI", 9f)
        };

        Button btnBrowseTiles = new Button
        {
            Text = "…",
            Location = new Point(252, 27),
            Size = new Size(28, 25),
            Font = new Font("Segoe UI", 9f, FontStyle.Bold),
            Cursor = Cursors.Hand
        };
        btnBrowseTiles.Click += BrowseTilesFolder_Click;

        Button btnRefreshTiles = new Button
        {
            Text = "↺",
            Location = new Point(284, 27),
            Size = new Size(28, 25),
            Font = new Font("Segoe UI", 10f, FontStyle.Bold),
            Cursor = Cursors.Hand
        };
        btnRefreshTiles.Click += RefreshTiles_Click;

        _tilesStatus = new Label
        {
            Location = new Point(8, 57),
            Size = new Size(305, 16),
            AutoSize = false,
            Font = new Font("Segoe UI", 8f, FontStyle.Italic),
            ForeColor = Color.FromArgb(120, 120, 120),
            Text = ""
        };

        // Scrollable thumbnail grid
        _tilesScroll = new Panel
        {
            Location = new Point(6, 76),
            Size = new Size(313, 278),
            AutoScroll = true,
            BorderStyle = BorderStyle.None,
            BackColor = Color.White
        };

        _palettePanel.Controls.Add(lblHeader);
        _palettePanel.Controls.Add(_tilesFolder);
        _palettePanel.Controls.Add(btnBrowseTiles);
        _palettePanel.Controls.Add(btnRefreshTiles);
        _palettePanel.Controls.Add(_tilesStatus);
        _palettePanel.Controls.Add(_tilesScroll);

        _tabMap.Controls.Add(_palettePanel);

        // Initial scan of the default folder
        LoadTiles();
    }

    // -------------------------------------------------------
    //  Tile loading & palette population
    // -------------------------------------------------------

    private void LoadTiles()
    {
        // Drop any previously-loaded tiles
        foreach (TileInfo t in _palette) t.Dispose();
        _palette.Clear();
        _tilesScroll.Controls.Clear();
        _selectedEntry = null;
        _selectedTileIndex = 0;

        // Blend-tool caches are keyed by palette index, so they go stale the
        // moment the palette is rebuilt — drop them here (see MainForm.BlendLine.cs).
        _avgColorCache.Clear();
        _blendTileCache.Clear();
        _sessionBlendTiles.Clear();
        _savedBlendFiles.Clear();

        string folder = _tilesFolder.Text.Trim();
        if (string.IsNullOrEmpty(folder) || !Directory.Exists(folder))
        {
            _tilesStatus.Text = "(folder not found)";
            _tilesStatus.ForeColor = Color.FromArgb(160, 0, 0);
            BuildPaletteEntries();
            return;
        }

        string[] files;
        try
        {
            files = Directory.GetFiles(folder, "*.png");
        }
        catch (Exception ex)
        {
            _tilesStatus.Text = $"(error: {ex.Message})";
            _tilesStatus.ForeColor = Color.FromArgb(160, 0, 0);
            BuildPaletteEntries();
            return;
        }

        Array.Sort(files, StringComparer.OrdinalIgnoreCase);

        int loaded = 0;
        int skipped = 0;
        for (int i = 0; i < files.Length && loaded < MaxTiles; i++)
        {
            try
            {
                // Bitmap(filename) locks the file; copy into a fresh Bitmap
                // so the source file isn't held open for the editor's lifetime.
                using Bitmap source = new Bitmap(files[i]);
                TileInfo info = new TileInfo
                {
                    Index = loaded + 1,
                    Filename = Path.GetFileName(files[i]),
                    Original = new Bitmap(source),
                    Thumbnail = MakeThumbnail(source, ThumbSize),
                    MiniThumb = MakeThumbnail(source, MapCellSize)
                };
                _palette.Add(info);
                loaded++;
            }
            catch
            {
                skipped++;
            }
        }

        if (_palette.Count == 0)
        {
            _tilesStatus.Text = "(no tiles in folder)";
            _tilesStatus.ForeColor = Color.FromArgb(120, 120, 120);
        }
        else
        {
            string suffix = _palette.Count == 1 ? "" : "s";
            string skipStr = skipped > 0 ? $"  ({skipped} skipped)" : "";
            string truncStr = files.Length > MaxTiles ? $"  (max {MaxTiles}, {files.Length - MaxTiles} unused)" : "";
            _tilesStatus.Text = $"{_palette.Count} tile{suffix} loaded{skipStr}{truncStr}";
            _tilesStatus.ForeColor = Color.FromArgb(70, 110, 70);
        }

        BuildPaletteEntries();

        // Reloading tiles may invalidate map cells that referenced now-missing
        // indices, so repaint the map.
        RedrawMap();
    }

    /// <summary>
    /// Looks up a tile by its assigned index.  _palette[0] holds the
    /// tile with Index == 1, _palette[1] holds Index == 2, and so on,
    /// so this is just an array lookup with a sanity check.
    /// </summary>
    private TileInfo? FindTileByIndex(int index)
    {
        if (index < 1 || index > _palette.Count) return null;
        TileInfo t = _palette[index - 1];
        return t.Index == index ? t : null;
    }

    private static Bitmap MakeThumbnail(Bitmap source, int size)
    {
        Bitmap thumb = new Bitmap(size, size, PixelFormat.Format32bppArgb);
        using Graphics g = Graphics.FromImage(thumb);
        g.InterpolationMode = InterpolationMode.NearestNeighbor;
        g.PixelOffsetMode = PixelOffsetMode.Half;
        g.DrawImage(source, 0, 0, size, size);
        return thumb;
    }

    private void BuildPaletteEntries()
    {
        _tilesScroll.SuspendLayout();
        _tilesScroll.Controls.Clear();

        // Each entry is the thumbnail plus a 2-pixel border on each side
        int entrySize = ThumbSize + 4;
        int step = entrySize + PaletteGap;

        int col = 0;
        int x = PaletteGap;
        int y = PaletteGap;

        foreach (TileInfo tile in _palette)
        {
            TilePaletteEntry entry = new TilePaletteEntry(tile)
            {
                Location = new Point(x, y),
                Size = new Size(entrySize, entrySize)
            };
            entry.Click += PaletteEntry_Click;
            _tilesScroll.Controls.Add(entry);

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

        _tilesScroll.ResumeLayout();
    }

    // -------------------------------------------------------
    //  Palette event handlers
    // -------------------------------------------------------

    private void BrowseTilesFolder_Click(object? sender, EventArgs e)
    {
        using FolderBrowserDialog dlg = new FolderBrowserDialog
        {
            Description = "Select folder containing tile PNG files",
            SelectedPath = _tilesFolder.Text,
            ShowNewFolderButton = false
        };

        if (dlg.ShowDialog(this) == DialogResult.OK)
        {
            _tilesFolder.Text = dlg.SelectedPath;
            LoadTiles();
        }
    }

    private void RefreshTiles_Click(object? sender, EventArgs e)
    {
        LoadTiles();
    }

    private void PaletteEntry_Click(object? sender, EventArgs e)
    {
        if (sender is not TilePaletteEntry entry) return;

        if (_selectedEntry != null && _selectedEntry != entry)
        {
            _selectedEntry.IsSelected = false;
            _selectedEntry.Invalidate();
        }

        entry.IsSelected = true;
        entry.Invalidate();

        _selectedEntry = entry;
        _selectedTileIndex = entry.Tile.Index;
    }

    // -------------------------------------------------------

    private void BuildMapSettingsGroup()
    {
        GroupBox grp = MakeGroup("Map Settings", 662, 380, 325, 130);

        Label lLevel = MakeLabel("Level:", 10, 30);
        _mapLevel = new NumericUpDown
        {
            Location = new Point(60, 27),
            Size = new Size(60, 23),
            Minimum = 1,
            Maximum = 25,
            Value = 1,
            Font = new Font("Segoe UI", 9f)
        };

        Label lPath = MakeLabel("Path:", 10, 64);
        _mapOutputPath = new TextBox
        {
            Location = new Point(60, 61),
            Size = new Size(170, 23)
        };

        Button btnBrowse = MakeButton("Browse…", 236, 60, 78, 25);
        btnBrowse.Click += BrowseMapOutput_Click;

        _chkStructureMode = new CheckBox
        {
            Text = "Structure mode (add/remove cells)",
            Location = new Point(10, 96),
            AutoSize = true,
            Checked = false
        };

        grp.Controls.AddRange(new Control[]
        {
            lLevel, _mapLevel,
            lPath,  _mapOutputPath, btnBrowse,
            _chkStructureMode
        });

        _tabMap.Controls.Add(grp);
    }

    // -------------------------------------------------------

    private void BuildMapEffectsGroup()
    {
        GroupBox grp = MakeGroup("Map Effects", 662, 520, 325, 190);

        // Two columns, all unchecked by default.
        _chkSymX = MakeEffectCheck("Symmetry — X axis", 10, 28);
        _chkSymY = MakeEffectCheck("Symmetry — Y axis", 168, 28);

        _chkBorderWall = MakeEffectCheck("Border wall", 10, 56);
        _chkOpenArena = MakeEffectCheck("Open arena", 168, 56);

        _chkScatteredPillars = MakeEffectCheck("Scattered pillars", 10, 84);
        _chkChokePoints = MakeEffectCheck("Choke points", 168, 84);

        _chkDeadEndPockets = MakeEffectCheck("Dead-end pockets", 10, 112);
        _chkMazeLike = MakeEffectCheck("Maze-like", 168, 112);

        grp.Controls.AddRange(new Control[]
        {
            _chkSymX, _chkSymY,
            _chkBorderWall, _chkOpenArena,
            _chkScatteredPillars, _chkChokePoints,
            _chkDeadEndPockets, _chkMazeLike
        });

        _tabMap.Controls.Add(grp);
    }

    private static CheckBox MakeEffectCheck(string text, int x, int y)
    {
        return new CheckBox
        {
            Text = text,
            Location = new Point(x, y),
            AutoSize = true,
            Checked = false,
            Font = new Font("Segoe UI", 9f)
        };
    }

    // -------------------------------------------------------

    private void BuildMapActionButtons()
    {
        Button btnGenMap = new Button
        {
            Text = "⚙  Generate Map",
            Location = new Point(10, 654),
            Size = new Size(315, 40),
            BackColor = Color.FromArgb(48, 120, 48),
            ForeColor = Color.White,
            FlatStyle = FlatStyle.Flat,
            Font = new Font("Segoe UI", 10f, FontStyle.Bold),
            Cursor = Cursors.Hand
        };
        btnGenMap.FlatAppearance.BorderSize = 0;
        btnGenMap.Click += GenerateMap_Click;

        Button btnSaveMap = new Button
        {
            Text = "💾  Save Map",
            Location = new Point(337, 654),
            Size = new Size(315, 40),
            BackColor = Color.FromArgb(48, 80, 160),
            ForeColor = Color.White,
            FlatStyle = FlatStyle.Flat,
            Font = new Font("Segoe UI", 10f, FontStyle.Bold),
            Cursor = Cursors.Hand
        };
        btnSaveMap.FlatAppearance.BorderSize = 0;
        btnSaveMap.Click += SaveMap_Click;

        _mapStatus = new Label
        {
            Location = new Point(10, 698),
            Size = new Size(642, 18),
            AutoSize = false,
            ForeColor = Color.FromArgb(70, 70, 70),
            Font = new Font("Segoe UI", 8.5f),
            TextAlign = ContentAlignment.MiddleLeft,
            Text = ""
        };

        _tabMap.Controls.Add(btnGenMap);
        _tabMap.Controls.Add(btnSaveMap);
        _tabMap.Controls.Add(_mapStatus);
    }

    // -------------------------------------------------------

    private void BrowseMapOutput_Click(object? sender, EventArgs e)
    {
        using FolderBrowserDialog dlg = new FolderBrowserDialog
        {
            Description = "Select folder to save generated map .cpp files",
            SelectedPath = _mapOutputPath.Text,
            ShowNewFolderButton = true
        };

        if (dlg.ShowDialog(this) == DialogResult.OK)
        {
            _mapOutputPath.Text = dlg.SelectedPath;
        }
    }

    // -------------------------------------------------------
    //  Map saving — emits Map{level}_{variant}.cpp / .h
    // -------------------------------------------------------

    private void SaveMap_Click(object? sender, EventArgs e) => TrySaveMap();

    /// <summary>Saves the map (.cpp/.h/_TileAssets.h). Returns true on success.</summary>
    private bool TrySaveMap()
    {
        try
        {
            Cursor = Cursors.WaitCursor;

            if (!TryPrepareOutputFolder(_mapOutputPath.Text, out string folder, out string folderError))
            {
                ShowMapStatus("✘  " + folderError, success: false);
                return false;
            }

            int level = (int)_mapLevel.Value;
            int variant = FindNextVariant(level, folder);
            string baseName = $"Map{level}_{variant}";

            string cppPath = Path.Combine(folder, baseName + ".cpp");
            string hPath = Path.Combine(folder, baseName + ".h");
            string tilesPath = Path.Combine(folder, baseName + "_TileAssets.h");

            File.WriteAllText(cppPath, BuildCppFile(baseName, level, variant));
            File.WriteAllText(hPath, BuildHeaderFile(baseName));
            File.WriteAllText(tilesPath, BuildTileAssetsHeader(baseName));

            // Make sure every blend tile referenced by _TileAssets.h is on disk.
            (bool blendOk, int unsaved) = EnsureBlendTilesSaved();

            if (blendOk)
            {
                ShowMapStatus($"✔  Saved: {baseName}.cpp + .h + _TileAssets.h  →  {folder}", success: true);
            }
            else
            {
                ShowMapStatus(
                    $"⚠  Saved {baseName}.cpp + .h + _TileAssets.h, but {unsaved} blend tile(s) couldn't be "
                    + "written — _TileAssets.h will reference missing PNGs. Set a valid Tiles folder and re-save.",
                    success: false);
            }
            return true;
        }
        catch (DirectoryNotFoundException)
        {
            ShowMapStatus("✘  The output folder couldn't be written to. Choose a local folder that "
                + "exists (e.g. inside your project on C:), not a OneDrive-synced location.", success: false);
            return false;
        }
        catch (UnauthorizedAccessException)
        {
            ShowMapStatus("✘  No permission to write to the output folder. Pick a different folder.",
                success: false);
            return false;
        }
        catch (Exception ex)
        {
            ShowMapStatus($"✘  Error: {ex.Message}", success: false);
            return false;
        }
        finally
        {
            Cursor = Cursors.Default;
        }
    }

    private void ShowMapStatus(string message, bool success)
    {
        _mapStatus.ForeColor = success
            ? Color.FromArgb(0, 110, 0)
            : Color.FromArgb(160, 0, 0);
        _mapStatus.Text = message;
    }

    /// <summary>
    /// Normalises a path, creates the folder, and write-tests it (a temp file).
    /// Returns false with a plain-English reason when the folder can't be
    /// written to — which catches OneDrive-redirected Documents and Windows
    /// "Controlled folder access" blocks that otherwise fail mid-write.
    /// </summary>
    private static bool TryPrepareOutputFolder(string raw, out string fullPath, out string error)
    {
        fullPath = string.Empty;
        error = string.Empty;

        raw = (raw ?? string.Empty).Trim();
        if (string.IsNullOrEmpty(raw))
        {
            error = "The output folder is empty — set a Path first.";
            return false;
        }

        try
        {
            fullPath = Path.GetFullPath(raw);
            Directory.CreateDirectory(fullPath);

            string probe = Path.Combine(fullPath, ".write_test_" + Guid.NewGuid().ToString("N") + ".tmp");
            File.WriteAllText(probe, string.Empty);
            File.Delete(probe);
            return true;
        }
        catch (Exception ex) when (ex is UnauthorizedAccessException
                                      or DirectoryNotFoundException
                                      or IOException
                                      or System.Security.SecurityException)
        {
            error = $"Can't write to '{fullPath}'. Pick a local folder outside Documents/OneDrive "
                  + "(for example C:\\TileGen\\Maps). Windows folder protection or OneDrive sync "
                  + "can block writes to Documents.";
            return false;
        }
    }

    /// <summary>
    /// Scans the output folder for files matching Map{level}_*.cpp and
    /// returns one more than the highest existing variant number.
    /// Gaps are intentionally not filled — if Map3_1 and Map3_4 exist,
    /// the next save becomes Map3_5.
    /// </summary>
    private int FindNextVariant(int level, string folder)
    {
        if (!Directory.Exists(folder)) return 1;

        string prefix = $"Map{level}_";
        string[] files;
        try
        {
            files = Directory.GetFiles(folder, $"{prefix}*.cpp");
        }
        catch
        {
            return 1;
        }

        int max = 0;
        foreach (string file in files)
        {
            string name = Path.GetFileNameWithoutExtension(file);
            if (!name.StartsWith(prefix, StringComparison.OrdinalIgnoreCase)) continue;
            string suffix = name.Substring(prefix.Length);
            if (int.TryParse(suffix, out int variant) && variant > max)
            {
                max = variant;
            }
        }
        return max + 1;
    }

    private string BuildHeaderFile(string baseName)
    {
        StringBuilder sb = new();
        sb.AppendLine($"// {baseName}.h  —  generated by Entitled Logic map editor");
        sb.AppendLine();
        sb.AppendLine("#pragma once");
        sb.AppendLine();
        sb.AppendLine($"extern const unsigned short {baseName}[64][64];");
        sb.AppendLine($"extern const unsigned char {baseName}_Valid[64][64];");
        sb.AppendLine($"extern const unsigned short {baseName}_Deco[64][64];");
        sb.AppendLine($"extern const unsigned char {baseName}_Collision[64][64];");
        return sb.ToString();
    }

    /// <summary>Unreal asset path (mount point) the tiles import to. On disk this
    /// is &lt;Project&gt;/Content/Tiles; in code it is referenced as /Game/Tiles.</summary>
    private const string UnrealTilesContentPath = "/Game/Tiles";

    /// <summary>
    /// Builds the Unreal join table: a static, source-level lookup from tile
    /// index → imported UTexture2D asset path, indexed the same way as the map
    /// arrays (0 = empty, 1..N = palette tiles).  Drop this header in next to
    /// the map .cpp/.h and resolve a cell with a single LoadObject / FSoftObjectPath.
    /// </summary>
    private string BuildTileAssetsHeader(string baseName)
    {
        StringBuilder sb = new();
        sb.AppendLine($"// {baseName}_TileAssets.h  —  generated by Entitled Logic map editor");
        sb.AppendLine("//");
        sb.AppendLine($"// Tile index → imported Unreal asset (UTexture2D). Index 0 = empty cell.");
        sb.AppendLine($"// Assets are expected at {UnrealTilesContentPath}/  (on disk: <Project>/Content/Tiles/).");
        sb.AppendLine("// Import the PNGs with the generated import_tiles_to_unreal.py script.");
        sb.AppendLine("//");
        sb.AppendLine("// Packaging note: string-path references are invisible to the cooker. Add");
        sb.AppendLine($"//   {UnrealTilesContentPath}  under Project Settings → Packaging →");
        sb.AppendLine("//   \"Additional Asset Directories to Cook\" so the tiles ship in packaged builds.");
        sb.AppendLine("//");
        sb.AppendLine("// Example:");
        sb.AppendLine($"//   const TCHAR* Path = {baseName}_TileAssets[{baseName}[x][y]];");
        sb.AppendLine("//   UTexture2D* Tex = Path[0] ? LoadObject<UTexture2D>(nullptr, Path) : nullptr;");
        sb.AppendLine();
        sb.AppendLine("#pragma once");
        sb.AppendLine();
        sb.AppendLine("#include \"CoreMinimal.h\"");
        sb.AppendLine();
        sb.AppendLine($"// inline (C++17+) → one definition shared across translation units.");
        sb.AppendLine($"inline const TCHAR* const {baseName}_TileAssets[] =");
        sb.AppendLine("{");
        sb.AppendLine("    TEXT(\"\"),  // 0 = empty");

        foreach (TileInfo tile in _palette)
        {
            string asset = Path.GetFileNameWithoutExtension(tile.Filename);
            sb.AppendLine($"    TEXT(\"{UnrealTilesContentPath}/{asset}\"),  // {tile.Index}");
        }

        sb.AppendLine("};");
        sb.AppendLine();
        sb.AppendLine($"inline constexpr int32 {baseName}_TileAssetCount = {_palette.Count + 1};");
        return sb.ToString();
    }

    /// <summary>Writes a Python script that batch-imports the tile PNGs into Unreal
    /// (/Game/Tiles) with crisp pixel-art settings: Nearest filter, sRGB on, no mips.</summary>
    private void WriteUnrealImportScript(string folder, string sourceTilesDir)
    {
        StringBuilder sb = new();
        sb.AppendLine("# ============================================================================");
        sb.AppendLine("#  import_tiles_to_unreal.py");
        sb.AppendLine("#  Auto-generated by EntitledLogicTileGen. Imports the tile PNGs that sit");
        sb.AppendLine("#  next to this script into your Unreal project as pixel-art textures.");
        sb.AppendLine("# ============================================================================");
        sb.AppendLine("#");
        sb.AppendLine("#  WHAT THIS DOES");
        sb.AppendLine("#    - Imports every .png in SOURCE_DIR (set below) into the Unreal content");
        sb.AppendLine("#      path DEST_PATH (default /Game/Tiles) as UTexture2D assets.");
        sb.AppendLine("#    - Configures each one for crisp pixel art: nearest-neighbor filtering,");
        sb.AppendLine("#      sRGB on, no mipmaps, uncompressed (TC_EDITOR_ICON).");
        sb.AppendLine("#    - Saves the assets so they persist in your project.");
        sb.AppendLine("#    These are the same tiles referenced by index in the generated");
        sb.AppendLine("#    Map<level>_<variant>.cpp / .h, and by name in <base>_TileAssets.h.");
        sb.AppendLine("#");
        sb.AppendLine("#  PREREQUISITES (one-time)");
        sb.AppendLine("#    1. Unreal Engine 5.7 with your project open.");
        sb.AppendLine("#    2. Enable the Python plugin: Edit > Plugins > search 'Python Editor");
        sb.AppendLine("#       Script Plugin' > tick Enabled > restart the editor.");
        sb.AppendLine("#    3. (Recommended) Enable 'Editor Scripting Utilities' the same way.");
        sb.AppendLine("#");
        sb.AppendLine("#  HOW TO RUN (pick one)");
        sb.AppendLine("#    A) Menu:  Tools > Execute Python Script...  then choose this file.");
        sb.AppendLine("#    B) Output Log: open Window > Output Log, switch the command box");
        sb.AppendLine("#       dropdown from 'Cmd' to 'Python', and run:");
        sb.AppendLine("#           exec(open(r'FULL/PATH/TO/import_tiles_to_unreal.py').read())");
        sb.AppendLine("#    C) Headless / command line:");
        sb.AppendLine("#           UnrealEditor-Cmd.exe \"C:/Path/MyProject.uproject\" -run=pythonscript \\");
        sb.AppendLine("#               -script=\"C:/Path/import_tiles_to_unreal.py\"");
        sb.AppendLine("#");
        sb.AppendLine("#  SOURCE_DIR and DEST_PATH below are already filled in for this export.");
        sb.AppendLine("#    - SOURCE_DIR points at the folder these PNGs were written to. If you");
        sb.AppendLine("#      move the PNGs, update SOURCE_DIR to match.");
        sb.AppendLine("#    - DEST_PATH is an Unreal content path: '/Game/...' maps to your");
        sb.AppendLine("#      project's Content/ folder on disk.  /Game/Tiles -> Content/Tiles.");
        sb.AppendLine("#");
        sb.AppendLine("#  RE-RUNNING");
        sb.AppendLine("#    Safe to run again. replace_existing=True overwrites existing tiles of");
        sb.AppendLine("#    the same name in place, so indices stay stable. New PNGs are added.");
        sb.AppendLine("#");
        sb.AppendLine("#  PACKAGING NOTE (important for cooked / shipping builds)");
        sb.AppendLine("#    The map files reference tiles by string path, which the cooker cannot");
        sb.AppendLine("#    see automatically. Add the tile directory to:");
        sb.AppendLine("#        Project Settings > Packaging > Additional Asset Directories to Cook");
        sb.AppendLine("#    and add  /Game/Tiles  so the textures ship with the build.");
        sb.AppendLine("#");
        sb.AppendLine("#  TROUBLESHOOTING");
        sb.AppendLine("#    - 'name unreal is not defined'  -> Python plugin not enabled (see above),");
        sb.AppendLine("#      or you ran it in 'Cmd' mode instead of 'Python' in the Output Log.");
        sb.AppendLine("#    - 'SOURCE_DIR not found'        -> the PNGs moved; fix the path below.");
        sb.AppendLine("#    - Tiles look blurry in-game     -> confirm the texture Filter is Nearest");
        sb.AppendLine("#      and Mip Gen Settings is NoMipmaps (this script sets both).");
        sb.AppendLine("# ============================================================================");
        sb.AppendLine();
        sb.AppendLine("import os");
        sb.AppendLine("import unreal");
        sb.AppendLine();
        sb.AppendLine($"SOURCE_DIR = r\"{sourceTilesDir}\"");
        sb.AppendLine($"DEST_PATH  = \"{UnrealTilesContentPath}\"");
        sb.AppendLine();
        sb.AppendLine("tools = unreal.AssetToolsHelpers.get_asset_tools()");
        sb.AppendLine("tasks = []");
        sb.AppendLine("for f in os.listdir(SOURCE_DIR):");
        sb.AppendLine("    if not f.lower().endswith('.png'):");
        sb.AppendLine("        continue");
        sb.AppendLine("    t = unreal.AssetImportTask()");
        sb.AppendLine("    t.set_editor_property('filename', os.path.join(SOURCE_DIR, f))");
        sb.AppendLine("    t.set_editor_property('destination_path', DEST_PATH)");
        sb.AppendLine("    t.set_editor_property('automated', True)");
        sb.AppendLine("    t.set_editor_property('replace_existing', True)");
        sb.AppendLine("    t.set_editor_property('save', True)");
        sb.AppendLine("    tasks.append(t)");
        sb.AppendLine();
        sb.AppendLine("tools.import_asset_tasks(tasks)");
        sb.AppendLine();
        sb.AppendLine("# Configure each imported texture for crisp pixel art.");
        sb.AppendLine("for t in tasks:");
        sb.AppendLine("    for path in t.get_editor_property('imported_object_paths'):");
        sb.AppendLine("        tex = unreal.load_asset(path)");
        sb.AppendLine("        if isinstance(tex, unreal.Texture2D):");
        sb.AppendLine("            tex.set_editor_property('srgb', True)");
        sb.AppendLine("            tex.set_editor_property('filter', unreal.TextureFilter.TF_NEAREST)");
        sb.AppendLine("            tex.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)");
        sb.AppendLine("            tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)");
        sb.AppendLine("            unreal.EditorAssetLibrary.save_asset(path)");
        sb.AppendLine();
        sb.AppendLine("unreal.log('Tile import complete.')");

        File.WriteAllText(Path.Combine(folder, "import_tiles_to_unreal.py"), sb.ToString());
    }

    private string BuildCppFile(string baseName, int level, int variant)
    {
        StringBuilder sb = new();

        // Header comment block
        sb.AppendLine($"// {baseName}.cpp  —  Level {level}, variant {variant}");
        sb.AppendLine($"// Generated {DateTime.Now:yyyy-MM-dd HH:mm:ss} by Entitled Logic map editor.");
        sb.AppendLine("//");
        sb.AppendLine($"// 64 × 64 map.  Indexed {baseName}[x][y] where x is the column and y the row.");
        sb.AppendLine("//");
        sb.AppendLine("// Tile values (16-bit, unsigned short):");
        sb.AppendLine("//   0         = no tile painted in this cell");
        sb.AppendLine("//   1..65535  = tile index — see lookup table below");
        sb.AppendLine("//");
        sb.AppendLine($"// Validity ({baseName}_Valid):");
        sb.AppendLine("//   0 = cell is not part of the playable map (wall / void)");
        sb.AppendLine("//   1 = cell is a valid map square");
        sb.AppendLine("//");
        sb.AppendLine($"// Decoration ({baseName}_Deco): overlay tile index (0 = none), same index space as ground.");
        sb.AppendLine($"// Collision ({baseName}_Collision): 0 = passable, 1 = blocked.");
        sb.AppendLine("//");

        // Tile index → asset lookup
        sb.AppendLine("// Tile index → asset lookup (index : tile asset filename):");
        if (_palette.Count == 0)
        {
            sb.AppendLine("//   (no tiles loaded at save time)");
        }
        else
        {
            foreach (TileInfo tile in _palette)
            {
                sb.AppendLine($"//   {tile.Index,5} = {tile.Filename}");
            }
        }
        sb.AppendLine("//");

        // Cell statistics
        int validCount = 0;
        int tiledCount = 0;
        for (int y = 0; y < MapCells; y++)
        {
            for (int x = 0; x < MapCells; x++)
            {
                if (_inMap[x, y])
                {
                    validCount++;
                    if (_mapData[x, y] != 0) tiledCount++;
                }
            }
        }
        int totalCells = MapCells * MapCells;
        double validPct = (validCount * 100.0) / totalCells;
        double tiledPct = validCount > 0 ? (tiledCount * 100.0) / validCount : 0.0;
        sb.AppendLine("// Cell statistics:");
        sb.AppendLine($"//   Valid: {validCount} / {totalCells} ({validPct:F1}%)");
        sb.AppendLine($"//   Tiled: {tiledCount} / {validCount} ({tiledPct:F1}% of valid)");
        sb.AppendLine();

        sb.AppendLine($"#include \"{baseName}.h\"");
        sb.AppendLine();

        AppendArray2D(sb, $"const unsigned short {baseName}[64][64]",
                      (x, y) => _mapData[x, y]);
        sb.AppendLine();
        AppendArray2D(sb, $"const unsigned char {baseName}_Valid[64][64]",
                      (x, y) => _inMap[x, y] ? 1 : 0);
        sb.AppendLine();
        AppendArray2D(sb, $"const unsigned short {baseName}_Deco[64][64]",
                      (x, y) => _decoData[x, y]);
        sb.AppendLine();
        AppendArray2D(sb, $"const unsigned char {baseName}_Collision[64][64]",
                      (x, y) => _collisionData[x, y]);

        return sb.ToString();
    }

    /// <summary>
    /// Writes one of the 2-D arrays.  Outer index is x (column), inner is y.
    /// Each cell is emitted right-aligned in a 3-character field separated
    /// by ", " — lines are long but very readable.
    /// </summary>
    private void AppendArray2D(StringBuilder sb, string declaration, Func<int, int, int> get)
    {
        sb.AppendLine(declaration + " =");
        sb.AppendLine("{");

        for (int x = 0; x < MapCells; x++)
        {
            sb.AppendLine($"    // x = {x}");
            sb.Append("    { ");
            for (int y = 0; y < MapCells; y++)
            {
                sb.Append($"{get(x, y),5}");
                if (y < MapCells - 1) sb.Append(", ");
            }
            sb.Append(" }");
            sb.AppendLine(x < MapCells - 1 ? "," : "");
        }

        sb.AppendLine("};");
    }

    // -------------------------------------------------------
    //  Map generation — Binary Space Partitioning
    // -------------------------------------------------------

    /// <summary>
    /// Generate button click.  Confirms before destroying any painted
    /// tiles, then re-seeds the RNG from TickCount and rebuilds the
    /// _inMap structure (tile values are left at 0 throughout).
    /// </summary>
    private void GenerateMap_Click(object? sender, EventArgs e)
    {
        if (HasPaintedCells())
        {
            DialogResult result = MessageBox.Show(
                this,
                "Generating will replace the current map and discard any painted tiles.\n\nContinue?",
                "Replace current map",
                MessageBoxButtons.YesNo,
                MessageBoxIcon.Question);

            if (result != DialogResult.Yes) return;
        }

        PushUndo();
        GenerateMap();
    }

    private bool HasPaintedCells()
    {
        for (int y = 0; y < MapCells; y++)
        {
            for (int x = 0; x < MapCells; x++)
            {
                if (_mapData[x, y] != 0) return true;
            }
        }
        return false;
    }

    private void GenerateMap()
    {
        GenerateMapCore(new MapGenSettings
        {
            Level = (int)_mapLevel.Value,
            SymX = _chkSymX.Checked,
            SymY = _chkSymY.Checked,
            BorderWall = _chkBorderWall.Checked,
            OpenArena = _chkOpenArena.Checked,
            ScatteredPillars = _chkScatteredPillars.Checked,
            ChokePoints = _chkChokePoints.Checked,
            DeadEndPockets = _chkDeadEndPockets.Checked,
            MazeLike = _chkMazeLike.Checked
        });
    }

    /// <summary>Generation parameters, so either the Map Generator tab or the
    /// Map Colorizer tab can drive the same generator with its own controls.</summary>
    private struct MapGenSettings
    {
        public int Level;
        public bool SymX, SymY, BorderWall, OpenArena;
        public bool ScatteredPillars, ChokePoints, DeadEndPockets, MazeLike;
    }

    private void GenerateMapCore(MapGenSettings s)
    {
        try
        {
            Cursor = Cursors.WaitCursor;

            int seed = Environment.TickCount;
            _rng = new Random(seed);

            // ── Effect flags (from the calling tab's controls) ─────────────
            bool symX = s.SymX;
            bool symY = s.SymY;
            bool border = s.BorderWall;
            bool arena = s.OpenArena;
            bool pillars = s.ScatteredPillars;
            bool chokes = s.ChokePoints;
            bool deadEnds = s.DeadEndPockets;
            bool maze = s.MazeLike;

            // ── Clear both state arrays ────────────────────────────────────
            for (int y = 0; y < MapCells; y++)
            {
                for (int x = 0; x < MapCells; x++)
                {
                    _inMap[x, y] = false;
                    _mapData[x, y] = 0;
                }
            }

            _lastRoomRects.Clear();
            int level = s.Level;
            BspNode? bspRoot = null;

            // ── Build the structure ────────────────────────────────────────
            if (arena)
            {
                // Open arena replaces BSP entirely.  Maze-like and choke
                // points are ignored in this branch (no corridor system).
                GenerateOpenArena();
            }
            else
            {
                int maxDepth = Math.Clamp(3 + (level - 1) / 8, 3, 6);
                int minRoom = Math.Clamp(8 - (level - 1) / 6, 4, 8);

                if (maze)
                {
                    maxDepth = Math.Min(8, maxDepth + 2);
                    minRoom = Math.Max(3, minRoom - 2);
                }

                int corridorWidth = chokes ? 1 : 2;

                bspRoot = new BspNode { X = 0, Y = 0, W = MapCells, H = MapCells };
                SplitNode(bspRoot, depth: 0, maxDepth: maxDepth, minRoom: minRoom);
                CarveAllRooms(bspRoot, minRoom);
                CollectRoomRects(bspRoot);
                CarveAllCorridors(bspRoot, corridorWidth);
            }

            // ── Post-pass modifications ───────────────────────────────────
            if (pillars) AddScatteredPillars(bspRoot);
            if (deadEnds) AddDeadEndPockets();

            // ── Symmetry mirror (applied to the full result) ──────────────
            // After all structural work; the top-left quadrant is the master
            // and the others become its reflection.
            if (symX) MirrorX();
            if (symY) MirrorY();

            // ── Border wall enforcement (final) ───────────────────────────
            // Applied last so it can't be undone by another pass.
            if (border) ApplyBorderWall();

            RedrawMap();

            // ── Status feedback ───────────────────────────────────────────
            int validCount = 0;
            for (int y = 0; y < MapCells; y++)
            {
                for (int x = 0; x < MapCells; x++)
                {
                    if (_inMap[x, y]) validCount++;
                }
            }
            double pct = validCount * 100.0 / (MapCells * MapCells);
            string effectsLabel = BuildActiveEffectsLabel(arena, maze, pillars, chokes, deadEnds, symX, symY, border);
            ShowMapStatus(
                $"Generated  ·  level {level}, seed {seed}  ·  {validCount} valid cells ({pct:F1}%){effectsLabel}",
                success: true);
        }
        catch (Exception ex)
        {
            ShowMapStatus($"✘  Generation error: {ex.Message}", success: false);
        }
        finally
        {
            Cursor = Cursors.Default;
        }
    }

    private static string BuildActiveEffectsLabel(
        bool arena, bool maze, bool pillars, bool chokes,
        bool deadEnds, bool symX, bool symY, bool border)
    {
        List<string> active = new();
        if (arena) active.Add("arena");
        if (maze && !arena) active.Add("maze");
        if (chokes && !arena) active.Add("chokes");
        if (pillars) active.Add("pillars");
        if (deadEnds) active.Add("dead-ends");
        if (symX) active.Add("sym-X");
        if (symY) active.Add("sym-Y");
        if (border) active.Add("border");
        return active.Count == 0 ? "" : "  ·  " + string.Join(", ", active);
    }

    /// <summary>
    /// Recursively splits a BSP node.  The split direction is biased by
    /// aspect ratio so long-thin nodes are split across their long axis;
    /// otherwise the direction is random.  Splitting stops at maxDepth or
    /// when both dimensions fall below the minimum size needed to host
    /// two children that can each fit a minRoom-sized room with margin.
    /// </summary>
    private void SplitNode(BspNode node, int depth, int maxDepth, int minRoom)
    {
        if (depth >= maxDepth) return;

        int minChildSize = minRoom + 2;   // room + 1-cell margin each side
        int minSplitSize = 2 * minChildSize;

        bool canSplitTopBottom = node.H >= minSplitSize;
        bool canSplitLeftRight = node.W >= minSplitSize;

        if (!canSplitTopBottom && !canSplitLeftRight) return;

        bool splitTopBottom;
        if (canSplitTopBottom && !canSplitLeftRight) splitTopBottom = true;
        else if (canSplitLeftRight && !canSplitTopBottom) splitTopBottom = false;
        else if (node.W > node.H * 1.25) splitTopBottom = false;  // wide, split L/R
        else if (node.H > node.W * 1.25) splitTopBottom = true;   // tall, split T/B
        else splitTopBottom = _rng.Next(2) == 0;

        if (splitTopBottom)
        {
            int splitOffset = _rng.Next(minChildSize, node.H - minChildSize + 1);
            node.Left = new BspNode { X = node.X, Y = node.Y, W = node.W, H = splitOffset };
            node.Right = new BspNode { X = node.X, Y = node.Y + splitOffset, W = node.W, H = node.H - splitOffset };
        }
        else
        {
            int splitOffset = _rng.Next(minChildSize, node.W - minChildSize + 1);
            node.Left = new BspNode { X = node.X, Y = node.Y, W = splitOffset, H = node.H };
            node.Right = new BspNode { X = node.X + splitOffset, Y = node.Y, W = node.W - splitOffset, H = node.H };
        }

        SplitNode(node.Left, depth + 1, maxDepth, minRoom);
        SplitNode(node.Right, depth + 1, maxDepth, minRoom);
    }

    /// <summary>
    /// Walks the tree, carving a room into each leaf with at least a
    /// 1-cell margin on every side so adjacent rooms don't touch.
    /// </summary>
    private void CarveAllRooms(BspNode node, int minRoom)
    {
        if (node.IsLeaf)
        {
            CarveRoom(node, minRoom);
            return;
        }
        if (node.Left != null) CarveAllRooms(node.Left, minRoom);
        if (node.Right != null) CarveAllRooms(node.Right, minRoom);
    }

    private void CarveRoom(BspNode leaf, int minRoom)
    {
        int slackX = leaf.W - minRoom;
        int slackY = leaf.H - minRoom;

        int left, top, rw, rh;
        if (slackX < 2 || slackY < 2)
        {
            // Defensive: shouldn't happen with correct split logic, but
            // place a minimal centred room if it ever does.
            left = (leaf.W - minRoom) / 2;
            top = (leaf.H - minRoom) / 2;
            rw = minRoom;
            rh = minRoom;
        }
        else
        {
            // Each margin gets at least 1; the rest of the slack is distributed randomly
            left = 1 + _rng.Next(slackX - 1);   // 1..slackX-1
            top = 1 + _rng.Next(slackY - 1);
            int right = slackX - left;
            int bottom = slackY - top;
            rw = leaf.W - left - right;
            rh = leaf.H - top - bottom;
        }

        int rx = leaf.X + left;
        int ry = leaf.Y + top;
        leaf.Room = new Rectangle(rx, ry, rw, rh);

        for (int y = ry; y < ry + rh; y++)
        {
            for (int x = rx; x < rx + rw; x++)
            {
                MarkValid(x, y);
            }
        }
    }

    /// <summary>
    /// Connects every non-leaf node's two subtrees by carving an L-shaped
    /// corridor between a representative room in each.  Because every
    /// subtree contains at least one room and every parent gets a
    /// corridor, all rooms in the final tree are reachable from each
    /// other.
    /// </summary>
    private void CarveAllCorridors(BspNode node, int corridorWidth)
    {
        if (node.IsLeaf) return;

        if (node.Left != null) CarveAllCorridors(node.Left, corridorWidth);
        if (node.Right != null) CarveAllCorridors(node.Right, corridorWidth);

        if (node.Left != null && node.Right != null)
        {
            Point? a = GetSomeRoomCenter(node.Left);
            Point? b = GetSomeRoomCenter(node.Right);
            if (a.HasValue && b.HasValue)
            {
                CarveCorridor(a.Value, b.Value, corridorWidth);
            }
        }
    }

    private Point? GetSomeRoomCenter(BspNode node)
    {
        if (node.IsLeaf && node.Room.HasValue)
        {
            Rectangle r = node.Room.Value;
            return new Point(r.X + r.Width / 2, r.Y + r.Height / 2);
        }
        if (node.Left != null) { Point? p = GetSomeRoomCenter(node.Left); if (p.HasValue) return p; }
        if (node.Right != null) { Point? p = GetSomeRoomCenter(node.Right); if (p.HasValue) return p; }
        return null;
    }

    private void CarveCorridor(Point a, Point b, int width)
    {
        // For width W centred on coord c: cells span [c - (W-1)/2 .. c + W/2]
        int half1 = (width - 1) / 2;
        int half2 = width / 2;

        bool horizontalFirst = _rng.Next(2) == 0;
        Point bend = horizontalFirst ? new Point(b.X, a.Y) : new Point(a.X, b.Y);

        // First leg
        DrawCorridorLine(a, bend, half1, half2);
        // Second leg
        DrawCorridorLine(bend, b, half1, half2);

        // Fill the full width×width block at the bend.  Each leg's band is
        // offset (half1=0, half2=width/2), so at some turn orientations the
        // outer corner cell isn't reached by either leg; this guarantees the
        // corner is solid for every orientation.
        for (int dx = -half1; dx <= half2; dx++)
            for (int dy = -half1; dy <= half2; dy++)
                MarkValid(bend.X + dx, bend.Y + dy);
    }

    private void DrawCorridorLine(Point a, Point b, int half1, int half2)
    {
        if (a.X == b.X)
        {
            int y1 = Math.Min(a.Y, b.Y);
            int y2 = Math.Max(a.Y, b.Y);
            for (int y = y1; y <= y2; y++)
            {
                for (int dx = -half1; dx <= half2; dx++)
                {
                    MarkValid(a.X + dx, y);
                }
            }
        }
        else
        {
            int x1 = Math.Min(a.X, b.X);
            int x2 = Math.Max(a.X, b.X);
            for (int x = x1; x <= x2; x++)
            {
                for (int dy = -half1; dy <= half2; dy++)
                {
                    MarkValid(x, a.Y + dy);
                }
            }
        }
    }

    private void MarkValid(int x, int y)
    {
        if (x < 0 || x >= MapCells || y < 0 || y >= MapCells) return;
        _inMap[x, y] = true;
    }

    // -------------------------------------------------------
    //  Effect: Open arena (replaces BSP entirely)
    // -------------------------------------------------------

    /// <summary>
    /// Builds an organic open-arena layout: a large central region with
    /// jagged edges produced by randomly "biting" chunks from the
    /// perimeter of an inset rectangle.  A central core is force-validated
    /// at the end to guarantee the arena isn't accidentally split in two.
    /// </summary>
    private void GenerateOpenArena()
    {
        const int inset = 4;
        int x1 = inset;
        int y1 = inset;
        int x2 = MapCells - 1 - inset;
        int y2 = MapCells - 1 - inset;

        // Fill the bulk rectangle
        for (int y = y1; y <= y2; y++)
        {
            for (int x = x1; x <= x2; x++)
            {
                _inMap[x, y] = true;
            }
        }

        // Bite small chunks out of the perimeter
        int biteCount = 25 + _rng.Next(15);   // 25..39 bites (conservative)
        for (int i = 0; i < biteCount; i++)
        {
            int side = _rng.Next(4);
            int bx, by;
            switch (side)
            {
                case 0: bx = x1 + _rng.Next(x2 - x1 + 1); by = y1; break;   // top
                case 1: bx = x1 + _rng.Next(x2 - x1 + 1); by = y2; break;   // bottom
                case 2: bx = x1; by = y1 + _rng.Next(y2 - y1 + 1); break;   // left
                default: bx = x2; by = y1 + _rng.Next(y2 - y1 + 1); break;   // right
            }

            int size = 1 + _rng.Next(3);   // 1..3
            for (int dy = -size; dy <= size; dy++)
            {
                for (int dx = -size; dx <= size; dx++)
                {
                    int x = bx + dx;
                    int y = by + dy;
                    if (x >= 0 && x < MapCells && y >= 0 && y < MapCells)
                    {
                        _inMap[x, y] = false;
                    }
                }
            }
        }

        // Force-validate a central core so the arena can't be split in half
        int cx = MapCells / 2;
        int cy = MapCells / 2;
        const int coreRadius = 10;
        for (int y = cy - coreRadius; y <= cy + coreRadius; y++)
        {
            for (int x = cx - coreRadius; x <= cx + coreRadius; x++)
            {
                MarkValid(x, y);
            }
        }
    }

    // -------------------------------------------------------
    //  Effect: Scattered pillars
    // -------------------------------------------------------

    /// <summary>
    /// Drops a small number of single-cell invalid spots inside each large
    /// room (BSP case) or scattered across the playable area (arena case).
    /// Pillars only land on truly interior cells — those with all four
    /// cardinal neighbours valid — so they can't accidentally block a
    /// corridor or sit on a room edge.
    /// </summary>
    private void AddScatteredPillars(BspNode? bspRoot)
    {
        if (bspRoot != null)
        {
            AddPillarsInBspLeaves(bspRoot);
        }
        else
        {
            AddPillarsInArena();
        }
    }

    private void AddPillarsInBspLeaves(BspNode node)
    {
        if (node.IsLeaf)
        {
            if (!node.Room.HasValue) return;
            Rectangle r = node.Room.Value;
            int area = r.Width * r.Height;
            if (area < 25) return;

            int pillarCount = 1 + _rng.Next(3);   // 1..3
            for (int i = 0; i < pillarCount; i++)
            {
                for (int attempt = 0; attempt < 10; attempt++)
                {
                    int px = r.X + 1 + _rng.Next(Math.Max(1, r.Width - 2));
                    int py = r.Y + 1 + _rng.Next(Math.Max(1, r.Height - 2));
                    if (TryPlacePillar(px, py)) break;
                }
            }
            return;
        }
        if (node.Left != null) AddPillarsInBspLeaves(node.Left);
        if (node.Right != null) AddPillarsInBspLeaves(node.Right);
    }

    private void AddPillarsInArena()
    {
        int pillarCount = 8 + _rng.Next(8);   // 8..15
        for (int i = 0; i < pillarCount; i++)
        {
            for (int attempt = 0; attempt < 20; attempt++)
            {
                int px = _rng.Next(MapCells);
                int py = _rng.Next(MapCells);
                if (TryPlacePillar(px, py)) break;
            }
        }
    }

    private bool TryPlacePillar(int x, int y)
    {
        if (x <= 0 || x >= MapCells - 1 || y <= 0 || y >= MapCells - 1) return false;
        if (!_inMap[x, y]) return false;
        // Must be a strict interior cell — all four cardinal neighbours valid
        if (!_inMap[x - 1, y] || !_inMap[x + 1, y]) return false;
        if (!_inMap[x, y - 1] || !_inMap[x, y + 1]) return false;
        _inMap[x, y] = false;
        return true;
    }

    // -------------------------------------------------------
    //  Effect: Dead-end pockets
    // -------------------------------------------------------

    /// <summary>
    /// Adds 2..4 short stubs that branch off existing corridors or rooms
    /// into formerly-invalid space, ending in a small 2×2 nook.  Each
    /// stub is a dead end — it doesn't reconnect to anything.
    /// </summary>
    private void AddDeadEndPockets()
    {
        int target = 2 + _rng.Next(3);   // 2..4
        for (int i = 0; i < target; i++)
        {
            TryAddOnePocket();
        }
    }

    private bool TryAddOnePocket()
    {
        // Shuffle direction order so we don't always pick North first
        int[] dirX = { 0, 0, -1, 1 };
        int[] dirY = { -1, 1, 0, 0 };

        for (int attempt = 0; attempt < 60; attempt++)
        {
            int x = _rng.Next(MapCells);
            int y = _rng.Next(MapCells);
            if (!_inMap[x, y]) continue;

            // Look for a direction where the neighbour is invalid — that's where the stub will go
            int[] order = { 0, 1, 2, 3 };
            for (int k = 3; k > 0; k--)
            {
                int j = _rng.Next(k + 1);
                (order[k], order[j]) = (order[j], order[k]);
            }

            int dx = 0, dy = 0;
            bool found = false;
            foreach (int idx in order)
            {
                int nx = x + dirX[idx];
                int ny = y + dirY[idx];
                if (nx < 0 || nx >= MapCells || ny < 0 || ny >= MapCells) continue;
                if (!_inMap[nx, ny])
                {
                    dx = dirX[idx];
                    dy = dirY[idx];
                    found = true;
                    break;
                }
            }
            if (!found) continue;

            // Carve a 2..4-cell stub away from the starting cell
            int stubLen = 2 + _rng.Next(3);   // 2..4
            int sx = x, sy = y;
            bool ok = true;
            for (int s = 0; s < stubLen; s++)
            {
                sx += dx;
                sy += dy;
                if (sx < 0 || sx >= MapCells || sy < 0 || sy >= MapCells) { ok = false; break; }
                _inMap[sx, sy] = true;
            }
            if (!ok) continue;

            // 2×2 nook around the stub end
            for (int ddy = -1; ddy <= 0; ddy++)
            {
                for (int ddx = -1; ddx <= 0; ddx++)
                {
                    MarkValid(sx + ddx, sy + ddy);
                }
            }

            return true;
        }
        return false;
    }

    // -------------------------------------------------------
    //  Effect: Symmetry mirrors
    // -------------------------------------------------------

    /// <summary>
    /// Mirrors the left half across the vertical centre line.  The right
    /// half becomes a reflection of the left.
    /// </summary>
    private void MirrorX()
    {
        int half = MapCells / 2;
        for (int y = 0; y < MapCells; y++)
        {
            for (int x = 0; x < half; x++)
            {
                int mx = MapCells - 1 - x;
                _inMap[mx, y] = _inMap[x, y];
                _mapData[mx, y] = _mapData[x, y];
            }
        }
    }

    /// <summary>
    /// Mirrors the top half across the horizontal centre line.  The
    /// bottom half becomes a reflection of the top.
    /// </summary>
    private void MirrorY()
    {
        int half = MapCells / 2;
        for (int y = 0; y < half; y++)
        {
            int my = MapCells - 1 - y;
            for (int x = 0; x < MapCells; x++)
            {
                _inMap[x, my] = _inMap[x, y];
                _mapData[x, my] = _mapData[x, y];
            }
        }
    }

    // -------------------------------------------------------
    //  Effect: Border wall
    // -------------------------------------------------------

    /// <summary>
    /// Forces a 1-cell ring around the entire 64×64 grid to be invalid.
    /// Applied last so it overrides anything else.
    /// </summary>
    private void ApplyBorderWall()
    {
        for (int i = 0; i < MapCells; i++)
        {
            _inMap[0, i] = false; _mapData[0, i] = 0;
            _inMap[MapCells - 1, i] = false; _mapData[MapCells - 1, i] = 0;
            _inMap[i, 0] = false; _mapData[i, 0] = 0;
            _inMap[i, MapCells - 1] = false; _mapData[i, MapCells - 1] = 0;
        }
    }

    // -------------------------------------------------------
    //  Map rendering
    // -------------------------------------------------------

    /// <summary>
    /// Repaints _mapBitmap from the current _inMap / _mapData state.
    /// Cells with _inMap == true are drawn either as the tile's
    /// 10×10 mini-thumbnail (if _mapData > 0) or as a white square
    /// (if _mapData == 0), always with a 1-pixel black border.
    /// Cells with _inMap == false are skipped, leaving the form's
    /// light-grey background showing through.
    /// </summary>
    private void RedrawMap()
    {
        if (_mapBitmap == null) return;

        using Graphics g = Graphics.FromImage(_mapBitmap);
        g.SmoothingMode = SmoothingMode.None;
        g.PixelOffsetMode = PixelOffsetMode.Half;
        g.InterpolationMode = InterpolationMode.NearestNeighbor;

        g.Clear(Color.FromArgb(235, 235, 235));

        using SolidBrush whiteBrush = new(Color.White);
        using Pen blackPen = new(Color.Black, 1f);
        using Pen redPen = new(Color.Red, 1f);

        for (int y = 0; y < MapCells; y++)
        {
            for (int x = 0; x < MapCells; x++)
            {
                if (!_inMap[x, y]) continue;

                int px = x * MapCellSize;
                int py = y * MapCellSize;

                int tileIdx = _mapData[x, y];
                if (tileIdx == 0)
                {
                    g.FillRectangle(whiteBrush, px, py, MapCellSize, MapCellSize);
                }
                else
                {
                    TileInfo? tile = FindTileByIndex(tileIdx);
                    if (tile?.MiniThumb != null)
                    {
                        g.DrawImage(tile.MiniThumb, px, py, MapCellSize, MapCellSize);
                    }
                    else
                    {
                        // Tile index points at nothing — palette was reloaded
                        // and this cell now references a missing tile.  Mark
                        // it visibly so the user can repaint it.
                        g.FillRectangle(whiteBrush, px, py, MapCellSize, MapCellSize);
                        g.DrawLine(redPen, px, py, px + MapCellSize - 1, py + MapCellSize - 1);
                        g.DrawLine(redPen, px + MapCellSize - 1, py, px, py + MapCellSize - 1);
                    }
                }

                DrawCellOverlays(g, x, y, px, py);
                g.DrawRectangle(blackPen, px, py, MapCellSize - 1, MapCellSize - 1);
            }
        }

        DrawToolPreview(g);

        _mapGrid?.Invalidate();
    }

    // -------------------------------------------------------
    //  Map interaction
    // -------------------------------------------------------

    /// <summary>
    /// Left-click and right-click on the map grid.
    ///   Paint mode:
    ///     L on valid cell  + tile selected → write that tile into the cell
    ///     Shift+L on valid + tile selected → flood-fill connected cells of
    ///                                        the same source value with the
    ///                                        selected tile
    ///     L on valid cell, nothing selected → no-op
    ///     L on invalid cell                 → no-op
    ///     R on valid cell with tile > 0    → show tile dialog
    ///     R anywhere else                  → no-op
    ///   Structure mode:
    ///     L on invalid cell → make valid (value resets to 0, draws white)
    ///     L on valid cell   → make invalid (draws light grey)
    ///     R on valid cell   → make invalid
    ///     R on invalid cell → no-op
    /// </summary>
    private void MapGrid_MouseDown(object? sender, MouseEventArgs e)
    {
        int x = e.X / MapCellSize;
        int y = e.Y / MapCellSize;
        if (x < 0 || x >= MapCells || y < 0 || y >= MapCells) return;

        bool structureMode = _chkStructureMode.Checked;

        // Paint tools (paint / fill / rectangle / line) on the active layer.
        if (!structureMode)
        {
            OnMapToolMouseDown(x, y, e.Button);
            return;
        }

        // ---- Structure mode: toggle cell validity (ground only) ----
        if (e.Button == MouseButtons.Left)
        {
            PushUndo();
            _inMap[x, y] = !_inMap[x, y];
            _mapData[x, y] = 0;
            RedrawMap();
        }
        else if (e.Button == MouseButtons.Right)
        {
            if (!_inMap[x, y]) return;
            PushUndo();
            _inMap[x, y] = false;
            _mapData[x, y] = 0;
            RedrawMap();
        }
    }

    /// <summary>
    /// Pops a small modal dialog showing the clicked tile at 4× scale
    /// (256×256), with its index, filename, and map coordinates.
    /// </summary>
    private void ShowTileDialog(int tileIndex, int cellX, int cellY)
    {
        TileInfo? tile = FindTileByIndex(tileIndex);
        if (tile?.Original == null) return;

        using Form dlg = new Form
        {
            Text = $"Tile {tile.Index}  ·  ({cellX}, {cellY})",
            FormBorderStyle = FormBorderStyle.FixedDialog,
            StartPosition = FormStartPosition.CenterParent,
            MaximizeBox = false,
            MinimizeBox = false,
            ShowInTaskbar = false,
            ClientSize = new Size(280, 322),
            BackColor = Color.FromArgb(235, 235, 235)
        };

        // 4× upscale of the 64×64 original
        Bitmap scaled = new Bitmap(256, 256, PixelFormat.Format32bppArgb);
        using (Graphics g = Graphics.FromImage(scaled))
        {
            g.InterpolationMode = InterpolationMode.NearestNeighbor;
            g.PixelOffsetMode = PixelOffsetMode.Half;
            g.DrawImage(tile.Original, 0, 0, 256, 256);
        }

        PictureBox pb = new PictureBox
        {
            Location = new Point(12, 12),
            Size = new Size(256, 256),
            BorderStyle = BorderStyle.FixedSingle,
            SizeMode = PictureBoxSizeMode.Normal,
            BackColor = Color.White,
            Image = scaled
        };

        Label lblInfo = new Label
        {
            Location = new Point(12, 274),
            Size = new Size(256, 38),
            AutoSize = false,
            Font = new Font("Segoe UI", 8.5f),
            ForeColor = Color.FromArgb(60, 60, 60),
            Text = $"Index: {tile.Index}\nFile: {tile.Filename}"
        };

        dlg.Controls.Add(pb);
        dlg.Controls.Add(lblInfo);

        try
        {
            dlg.ShowDialog(this);
        }
        finally
        {
            pb.Image = null;
            scaled.Dispose();
        }
    }

    // ═══════════════════════════════════════════════════════
    //  Default values
    // ═══════════════════════════════════════════════════════

    private void ApplyDefaults()
    {
        // Color 1
        _suppressRgbEvent = true;
        nudR1.Value = _color1.R;
        nudG1.Value = _color1.G;
        nudB1.Value = _color1.B;
        _suppressRgbEvent = false;
        panelColor1.BackColor = _color1;
        UpdateRgbLabel(1);

        // Color 2
        _suppressRgbEvent = true;
        nudR2.Value = _color2.R;
        nudG2.Value = _color2.G;
        nudB2.Value = _color2.B;
        _suppressRgbEvent = false;
        panelColor2.BackColor = _color2;
        UpdateRgbLabel(2);

        // Algorithm
        cmbAlgorithm.SelectedIndex = 0;

        // Saturation
        trackSat.Value = 100;
        lblSatVal.Text = "100";

        // Noise level
        trackNoise.Value = 15;
        lblNoiseVal.Text = "15";

        // Output path
        txtOutputPath.Text = @"D:\work\The AI-Native Game Guru\Tiles";

        // Map output path
        _mapOutputPath.Text = @"D:\work\The AI-Native Game Guru\Maps";
    }

    // ═══════════════════════════════════════════════════════
    //  RGB / colour helpers
    // ═══════════════════════════════════════════════════════

    private void OnNudChanged(int which)
    {
        if (_suppressRgbEvent) return;

        if (which == 1)
        {
            _color1 = Color.FromArgb((int)nudR1.Value, (int)nudG1.Value, (int)nudB1.Value);
            panelColor1.BackColor = _color1;
            UpdateRgbLabel(1);
        }
        else
        {
            _color2 = Color.FromArgb((int)nudR2.Value, (int)nudG2.Value, (int)nudB2.Value);
            panelColor2.BackColor = _color2;
            UpdateRgbLabel(2);
        }
    }

    private void UpdateRgbLabel(int which)
    {
        if (which == 1)
        {
            lblRgb1.Text = $"RGB({(int)nudR1.Value}, {(int)nudG1.Value}, {(int)nudB1.Value})";
        }
        else
        {
            lblRgb2.Text = $"RGB({(int)nudR2.Value}, {(int)nudG2.Value}, {(int)nudB2.Value})";
        }
    }

    private void PickColor(int which)
    {
        using ColorDialog dlg = new ColorDialog
        {
            Color = which == 1 ? _color1 : _color2,
            FullOpen = true,
            AllowFullOpen = true
        };

        if (dlg.ShowDialog(this) != DialogResult.OK) return;

        _suppressRgbEvent = true;

        if (which == 1)
        {
            _color1 = dlg.Color;
            nudR1.Value = _color1.R;
            nudG1.Value = _color1.G;
            nudB1.Value = _color1.B;
            panelColor1.BackColor = _color1;
        }
        else
        {
            _color2 = dlg.Color;
            nudR2.Value = _color2.R;
            nudG2.Value = _color2.G;
            nudB2.Value = _color2.B;
            panelColor2.BackColor = _color2;
        }

        _suppressRgbEvent = false;
        UpdateRgbLabel(which);
    }

    // ═══════════════════════════════════════════════════════
    //  Output / browse
    // ═══════════════════════════════════════════════════════

    private void BrowseOutput_Click(object? sender, EventArgs e)
    {
        using FolderBrowserDialog dlg = new FolderBrowserDialog
        {
            Description = "Select folder to save generated tiles",
            SelectedPath = txtOutputPath.Text,
            ShowNewFolderButton = true
        };

        if (dlg.ShowDialog(this) == DialogResult.OK)
        {
            txtOutputPath.Text = dlg.SelectedPath;
        }
    }

    // ═══════════════════════════════════════════════════════
    //  Generate single tile
    // ═══════════════════════════════════════════════════════

    private void GenerateTile_Click(object? sender, EventArgs e)
    {
        try
        {
            Cursor = Cursors.WaitCursor;
            _lastTile?.Dispose();
            _lastTile = DoGenerate();

            string savedPath = SaveTile(_lastTile);
            RefreshPreview();

            SetStatus($"✔  Saved: {savedPath}", success: true);
        }
        catch (Exception ex)
        {
            SetStatus($"✘  Error: {ex.Message}", success: false);
        }
        finally
        {
            Cursor = Cursors.Default;
        }
    }

    // ═══════════════════════════════════════════════════════
    //  Generate batch of 10 tiles
    // ═══════════════════════════════════════════════════════

    private void GenerateBatch_Click(object? sender, EventArgs e)
    {
        int batchSize = (int)nudBatch.Value;

        try
        {
            Cursor = Cursors.WaitCursor;

            string dir = EnsureOutputDir();
            int baseSeed = chkRandSeed.Checked
                ? Environment.TickCount
                : (int)nudSeed.Value;

            for (int i = 0; i < batchSize; i++)
            {
                int seed = (int)(baseSeed ^ (i * 0x9E3779B9));   // spread seeds well
                using Bitmap tile = DoGenerate(seed);

                string timestamp = DateTime.Now.ToString("yyyyMMdd_HHmmss_fff");
                string filename = $"tile_{timestamp}_{i:D2}.png";
                tile.Save(Path.Combine(dir, filename), ImageFormat.Png);

                // Display the last tile in the preview
                if (i == batchSize - 1)
                {
                    _lastTile?.Dispose();
                    _lastTile = (Bitmap)tile.Clone();
                    RefreshPreview();
                }
            }

            SetStatus($"✔  Saved {batchSize} tiles to: {dir}", success: true);
        }
        catch (Exception ex)
        {
            SetStatus($"✘  Error: {ex.Message}", success: false);
        }
        finally
        {
            Cursor = Cursors.Default;
        }
    }

    // ═══════════════════════════════════════════════════════
    //  Core generation
    // ═══════════════════════════════════════════════════════

    private Bitmap DoGenerate(int? overrideSeed = null)
    {
        int seed = overrideSeed ?? (chkRandSeed.Checked
            ? Environment.TickCount
            : (int)nudSeed.Value);

        if (chkUseRamp != null && chkUseRamp.Checked)
        {
            return TileGenerator.Generate(
                ramp: _ramp,
                saturation: trackSat.Value,
                noiseLevel: trackNoise.Value,
                algorithm: (NoiseAlgorithm)cmbAlgorithm.SelectedIndex,
                seed: seed);
        }

        return TileGenerator.Generate(
            color1: _color1,
            color2: _color2,
            saturation: trackSat.Value,
            noiseLevel: trackNoise.Value,
            algorithm: (NoiseAlgorithm)cmbAlgorithm.SelectedIndex,
            seed: seed);
    }

    private string SaveTile(Bitmap tile)
    {
        string dir = EnsureOutputDir();
        string filename = $"tile_{DateTime.Now:yyyyMMdd_HHmmss_fff}.png";
        string fullPath = Path.Combine(dir, filename);
        tile.Save(fullPath, ImageFormat.Png);
        return fullPath;
    }

    private string EnsureOutputDir()
    {
        string dir = txtOutputPath.Text.Trim();
        if (string.IsNullOrWhiteSpace(dir))
        {
            dir = @"D:\work\The AI-Native Game Guru\Tiles";
        }
        Directory.CreateDirectory(dir);
        return dir;
    }

    // ═══════════════════════════════════════════════════════
    //  Preview rendering
    // ═══════════════════════════════════════════════════════

    private void RefreshPreview()
    {
        if (_lastTile == null) return;

        int n = cmbTiling.SelectedIndex + 1;   // 0→1×1, 1→2×2, 2→3×3, 3→4×4
        bool tiled = n > 1;
        int srcSide = 64 * n;
        int scale = PreviewScales[cmbScale.SelectedIndex];

        // Clamp scale so the image fits in the PictureBox
        int boxW = pictureBox.ClientSize.Width;
        int boxH = pictureBox.ClientSize.Height;
        int maxScale = Math.Max(1, Math.Min(boxW, boxH) / srcSide);
        if (scale > maxScale) scale = maxScale;

        Bitmap? src = null;
        Bitmap? canvas = null;

        try
        {
            src = tiled ? TileBlender.Tile(_lastTile, n) : _lastTile;

            int dstW = srcSide * scale;
            int dstH = srcSide * scale;
            int offX = (boxW - dstW) / 2;
            int offY = (boxH - dstH) / 2;

            canvas = new Bitmap(boxW, boxH, PixelFormat.Format32bppArgb);

            using Graphics g = Graphics.FromImage(canvas);
            g.Clear(Color.FromArgb(28, 28, 28));
            g.InterpolationMode = InterpolationMode.NearestNeighbor;
            g.PixelOffsetMode = PixelOffsetMode.Half;
            g.DrawImage(src, offX, offY, dstW, dstH);

            // Draw a subtle grid overlay to show tile boundaries
            //if (tiled)
            //{
            //    using Pen pen = new Pen(Color.FromArgb(60, 255, 255, 255), 1f);
            //    int mid = offX + 64 * scale;
            //    g.DrawLine(pen, mid, offY, mid, offY + dstH);
            //    mid = offY + 64 * scale;
            //    g.DrawLine(pen, offX, mid, offX + dstW, mid);
            //}
        }
        finally
        {
            if (tiled && src != null) src.Dispose();
        }

        // Swap canvas into the PictureBox
        Bitmap? old = pictureBox.Image as Bitmap;
        pictureBox.Image = canvas;
        _previewBmp?.Dispose();
        _previewBmp = canvas;
        old?.Dispose();
    }

    // ═══════════════════════════════════════════════════════
    //  Status helper
    // ═══════════════════════════════════════════════════════

    private void SetStatus(string message, bool success)
    {
        lblStatus.ForeColor = success
            ? Color.FromArgb(0, 110, 0)
            : Color.FromArgb(160, 0, 0);
        lblStatus.Text = message;
    }

    // ═══════════════════════════════════════════════════════
    //  Control factory helpers
    // ═══════════════════════════════════════════════════════

    private static GroupBox MakeGroup(string text, int x, int y, int w, int h)
    {
        return new GroupBox
        {
            Text = text,
            Location = new Point(x, y),
            Size = new Size(w, h),
            Font = new Font("Segoe UI", 9f, FontStyle.Bold)
        };
    }

    private static Label MakeLabel(string text, int x, int y)
    {
        return new Label
        {
            Text = text,
            Location = new Point(x, y),
            AutoSize = true,
            Font = new Font("Segoe UI", 9f)
        };
    }

    private static NumericUpDown MakeNud(int x, int y, decimal min, decimal max)
    {
        return new NumericUpDown
        {
            Location = new Point(x, y),
            Size = new Size(56, 23),
            Minimum = min,
            Maximum = max,
            Increment = 1,
            Font = new Font("Segoe UI", 9f)
        };
    }

    private static Button MakeButton(string text, int x, int y, int w, int h)
    {
        return new Button
        {
            Text = text,
            Location = new Point(x, y),
            Size = new Size(w, h),
            Font = new Font("Segoe UI", 9f),
            Cursor = Cursors.Hand
        };
    }

    // ═══════════════════════════════════════════════════════
    //  Cleanup
    // ═══════════════════════════════════════════════════════

    protected override void Dispose(bool disposing)
    {
        if (disposing)
        {
            _lastTile?.Dispose();
            _previewBmp?.Dispose();
            _mapBitmap?.Dispose();

            foreach (TileInfo t in _palette) t.Dispose();
            _palette.Clear();

            DisposeBlendResources();
            DisposeColorMapResources();
        }
        base.Dispose(disposing);
    }

    // ═══════════════════════════════════════════════════════
    //  Nested types — palette data + visual entry
    // ═══════════════════════════════════════════════════════

    /// <summary>
    /// Holds the data for one tile in the palette: its assigned index
    /// (1..255), the source filename, the full-size original bitmap,
    /// the 48×48 palette thumbnail, and the 10×10 mini-thumbnail used
    /// for in-map rendering.
    /// </summary>
    private sealed class TileInfo : IDisposable
    {
        public int Index { get; init; }
        public string Filename { get; init; } = "";
        public Bitmap? Original { get; init; }
        public Bitmap? Thumbnail { get; init; }
        public Bitmap? MiniThumb { get; init; }

        public void Dispose()
        {
            Original?.Dispose();
            Thumbnail?.Dispose();
            MiniThumb?.Dispose();
        }
    }

    /// <summary>
    /// A clickable Panel subclass that paints one tile entry in the
    /// palette: the thumbnail, the index number overlaid in the
    /// bottom-right corner, and a selection-highlight border.
    /// </summary>
    private sealed class TilePaletteEntry : Panel
    {
        public TileInfo Tile { get; }
        public bool IsSelected { get; set; }

        public TilePaletteEntry(TileInfo tile)
        {
            Tile = tile;
            DoubleBuffered = true;
            BackColor = Color.White;
            Cursor = Cursors.Hand;
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            Graphics g = e.Graphics;
            g.InterpolationMode = InterpolationMode.NearestNeighbor;
            g.PixelOffsetMode = PixelOffsetMode.Half;

            if (Tile.Thumbnail != null)
            {
                // Thumbnail centred with a 2-px gutter for the border
                g.DrawImage(Tile.Thumbnail, 2, 2, Width - 4, Height - 4);
            }

            // Index number — small white text on a translucent black plate
            string num = Tile.Index.ToString();
            using Font numFont = new Font("Segoe UI", 7f, FontStyle.Bold);
            SizeF tsize = g.MeasureString(num, numFont);
            float tx = Width - tsize.Width - 3;
            float ty = Height - tsize.Height - 1;

            using SolidBrush plate = new(Color.FromArgb(170, 0, 0, 0));
            g.FillRectangle(plate, tx - 1, ty + 1, tsize.Width + 2, tsize.Height - 2);
            using SolidBrush textBrush = new(Color.White);
            g.DrawString(num, numFont, textBrush, tx, ty);

            // Selection / idle border
            Color borderColor = IsSelected
                ? Color.FromArgb(240, 130, 0)
                : Color.FromArgb(190, 190, 190);
            int borderWidth = IsSelected ? 2 : 1;

            using Pen pen = new Pen(borderColor, borderWidth);
            // Inset by half the pen width so the line lands fully inside the control
            int inset = borderWidth - 1;
            g.DrawRectangle(pen, inset, inset, Width - 1 - inset * 2, Height - 1 - inset * 2);
        }
    }

    /// <summary>
    /// A node in the binary-space-partitioning tree used by the map
    /// generator.  Internal nodes have Left/Right children; leaves have
    /// a Room rectangle carved within their bounds.
    /// </summary>
    private sealed class BspNode
    {
        public int X, Y, W, H;
        public BspNode? Left;
        public BspNode? Right;
        public Rectangle? Room;
        public bool IsLeaf => Left == null && Right == null;
    }
}
