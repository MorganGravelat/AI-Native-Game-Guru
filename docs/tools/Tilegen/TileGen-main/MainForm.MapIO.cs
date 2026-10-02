using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.Json;
using System.Windows.Forms;

namespace EntitledLogicTileGen;

/// <summary>
/// Portable map data I/O.  Complements the engine-facing C++ export with:
///   • JSON  — a single file holding all layers + metadata (full round-trip), and
///   • CSV   — one plain grid file per layer (ground / decoration / collision /
///             valid), easy to diff or open in a spreadsheet.
///
/// All grids are written row-major: the outer array/line index is the row
/// (y, top → bottom) and the inner index is the column (x, left → right).
/// </summary>
public sealed partial class MainForm
{
    // -------------------------------------------------------
    //  JSON
    // -------------------------------------------------------

    private sealed class MapDto
    {
        public int Width { get; set; }
        public int Height { get; set; }
        public int Level { get; set; }
        public string Note { get; set; } = "";
        public string[] Tileset { get; set; } = System.Array.Empty<string>();
        public int[][] Ground { get; set; } = System.Array.Empty<int[]>();
        public int[][] Decoration { get; set; } = System.Array.Empty<int[]>();
        public int[][] Collision { get; set; } = System.Array.Empty<int[]>();
        public int[][] Valid { get; set; } = System.Array.Empty<int[]>();
    }

    private void ExportJson_Click(object? sender, System.EventArgs e)
    {
        string folder = _mapOutputPath.Text.Trim();
        if (string.IsNullOrEmpty(folder))
        {
            ShowMapStatus("✘  Map output path is empty", success: false);
            return;
        }

        try
        {
            Directory.CreateDirectory(folder);

            int level = (int)_mapLevel.Value;
            string[] tileset = new string[_palette.Count];
            for (int i = 0; i < _palette.Count; i++) tileset[i] = _palette[i].Filename;

            MapDto dto = new()
            {
                Width = MapCells,
                Height = MapCells,
                Level = level,
                Note = "Grids are row-major [y][x]. Ground/Decoration are tile indices "
                       + "(0 = none); Collision is 0/1; Valid is 0/1. "
                       + "Tileset[i] is the filename for tile index i+1.",
                Tileset = tileset,
                Ground = ToRows(_mapData),
                Decoration = ToRows(_decoData),
                Collision = ToRows(_collisionData),
                Valid = ToRowsBool(_inMap)
            };

            string stamp = System.DateTime.Now.ToString("yyyyMMdd_HHmmss");
            string filename = $"Map{level}_{stamp}.json";
            string path = Path.Combine(folder, filename);

            JsonSerializerOptions opts = new() { WriteIndented = true };
            File.WriteAllText(path, JsonSerializer.Serialize(dto, opts));

            ShowMapStatus($"✔  Exported JSON: {filename}  →  {folder}", success: true);
        }
        catch (System.Exception ex)
        {
            ShowMapStatus($"✘  JSON export error: {ex.Message}", success: false);
        }
    }

    // -------------------------------------------------------
    //  CSV (one file per layer)
    // -------------------------------------------------------

    private void ExportCsv_Click(object? sender, System.EventArgs e)
    {
        string folder = _mapOutputPath.Text.Trim();
        if (string.IsNullOrEmpty(folder))
        {
            ShowMapStatus("✘  Map output path is empty", success: false);
            return;
        }

        try
        {
            Directory.CreateDirectory(folder);

            int level = (int)_mapLevel.Value;
            string stamp = System.DateTime.Now.ToString("yyyyMMdd_HHmmss");
            string baseName = $"Map{level}_{stamp}";

            WriteCsv(Path.Combine(folder, baseName + "_ground.csv"), _mapData);
            WriteCsv(Path.Combine(folder, baseName + "_decoration.csv"), _decoData);
            WriteCsv(Path.Combine(folder, baseName + "_collision.csv"), _collisionData);
            WriteCsvBool(Path.Combine(folder, baseName + "_valid.csv"), _inMap);

            ShowMapStatus($"✔  Exported 4 CSV layers: {baseName}_*.csv  →  {folder}", success: true);
        }
        catch (System.Exception ex)
        {
            ShowMapStatus($"✘  CSV export error: {ex.Message}", success: false);
        }
    }

    private static void WriteCsv(string path, ushort[,] arr)
    {
        StringBuilder sb = new();
        for (int y = 0; y < MapCells; y++)
        {
            for (int x = 0; x < MapCells; x++)
            {
                sb.Append(arr[x, y]);
                if (x < MapCells - 1) sb.Append(',');
            }
            sb.Append('\n');
        }
        File.WriteAllText(path, sb.ToString());
    }

    private static void WriteCsvBool(string path, bool[,] arr)
    {
        StringBuilder sb = new();
        for (int y = 0; y < MapCells; y++)
        {
            for (int x = 0; x < MapCells; x++)
            {
                sb.Append(arr[x, y] ? 1 : 0);
                if (x < MapCells - 1) sb.Append(',');
            }
            sb.Append('\n');
        }
        File.WriteAllText(path, sb.ToString());
    }

    // -------------------------------------------------------
    //  Import (JSON = full; CSV = into the active layer)
    // -------------------------------------------------------

    private void ImportMap_Click(object? sender, System.EventArgs e)
    {
        using OpenFileDialog dlg = new()
        {
            Title = "Import map data",
            Filter = "Map data (*.json;*.csv)|*.json;*.csv|JSON (*.json)|*.json|CSV (*.csv)|*.csv",
            InitialDirectory = Directory.Exists(_mapOutputPath.Text.Trim())
                ? _mapOutputPath.Text.Trim()
                : ""
        };

        if (dlg.ShowDialog(this) != DialogResult.OK) return;

        try
        {
            string ext = Path.GetExtension(dlg.FileName).ToLowerInvariant();
            if (ext == ".json")
            {
                ImportJson(dlg.FileName);
            }
            else if (ext == ".csv")
            {
                ImportCsvIntoActiveLayer(dlg.FileName);
            }
            else
            {
                ShowMapStatus("✘  Unsupported file type", success: false);
            }
        }
        catch (System.Exception ex)
        {
            ShowMapStatus($"✘  Import error: {ex.Message}", success: false);
        }
    }

    private void ImportJson(string path)
    {
        string json = File.ReadAllText(path);
        MapDto? dto = JsonSerializer.Deserialize<MapDto>(json);
        if (dto == null)
        {
            ShowMapStatus("✘  Could not read JSON", success: false);
            return;
        }
        if (dto.Width != MapCells || dto.Height != MapCells)
        {
            ShowMapStatus($"✘  Map is {dto.Width}×{dto.Height}; expected {MapCells}×{MapCells}", success: false);
            return;
        }

        PushUndo();
        FromRows(dto.Ground, _mapData);
        FromRows(dto.Decoration, _decoData);
        FromRows(dto.Collision, _collisionData);
        FromRowsBool(dto.Valid, _inMap);

        if (dto.Level >= _mapLevel.Minimum && dto.Level <= _mapLevel.Maximum)
        {
            _mapLevel.Value = dto.Level;
        }

        RedrawMap();
        UpdateUndoButtons();
        ShowMapStatus($"✔  Imported {Path.GetFileName(path)} (all layers)", success: true);
    }

    private void ImportCsvIntoActiveLayer(string path)
    {
        string[] lines = File.ReadAllLines(path);
        // Collect non-empty rows.
        List<int[]> rows = new();
        foreach (string raw in lines)
        {
            string line = raw.Trim();
            if (line.Length == 0) continue;
            string[] parts = line.Split(',');
            int[] row = new int[parts.Length];
            for (int i = 0; i < parts.Length; i++)
            {
                int.TryParse(parts[i].Trim(), out row[i]);
            }
            rows.Add(row);
        }

        if (rows.Count < MapCells)
        {
            ShowMapStatus($"✘  CSV has {rows.Count} rows; expected {MapCells}", success: false);
            return;
        }

        ushort[,] target = ActiveArray();
        bool collision = _activeLayer == MapLayer.Collision;

        PushUndo();
        for (int y = 0; y < MapCells; y++)
        {
            int[] row = rows[y];
            for (int x = 0; x < MapCells && x < row.Length; x++)
            {
                int v = row[x];
                if (collision) v = v != 0 ? 1 : 0;
                target[x, y] = (ushort)System.Math.Clamp(v, 0, 65535);
            }
        }

        RedrawMap();
        UpdateUndoButtons();
        ShowMapStatus($"✔  Imported {Path.GetFileName(path)} into the {_activeLayer} layer", success: true);
    }

    // -------------------------------------------------------
    //  Grid <-> jagged-array helpers (row-major [y][x])
    // -------------------------------------------------------

    private static int[][] ToRows(ushort[,] arr)
    {
        int[][] rows = new int[MapCells][];
        for (int y = 0; y < MapCells; y++)
        {
            rows[y] = new int[MapCells];
            for (int x = 0; x < MapCells; x++) rows[y][x] = arr[x, y];
        }
        return rows;
    }

    private static int[][] ToRowsBool(bool[,] arr)
    {
        int[][] rows = new int[MapCells][];
        for (int y = 0; y < MapCells; y++)
        {
            rows[y] = new int[MapCells];
            for (int x = 0; x < MapCells; x++) rows[y][x] = arr[x, y] ? 1 : 0;
        }
        return rows;
    }

    private static void FromRows(int[][]? rows, ushort[,] dest)
    {
        if (rows == null) return;
        for (int y = 0; y < MapCells && y < rows.Length; y++)
        {
            int[] row = rows[y];
            if (row == null) continue;
            for (int x = 0; x < MapCells && x < row.Length; x++)
            {
                dest[x, y] = (ushort)System.Math.Clamp(row[x], 0, 65535);
            }
        }
    }

    private static void FromRowsBool(int[][]? rows, bool[,] dest)
    {
        if (rows == null) return;
        for (int y = 0; y < MapCells && y < rows.Length; y++)
        {
            int[] row = rows[y];
            if (row == null) continue;
            for (int x = 0; x < MapCells && x < row.Length; x++)
            {
                dest[x, y] = row[x] != 0;
            }
        }
    }
}
