using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Windows.Forms;

namespace EntitledLogicTileGen;

/// <summary>
/// Modal editor for a <see cref="ColorRamp"/>.  Shows a live gradient bar,
/// an editable list of colour stops (position + colour), a stepped/indexed
/// toggle, and a couple of presets.  On OK, <see cref="Result"/> holds the
/// edited ramp; the ramp passed in is never mutated (a clone is edited).
/// </summary>
public sealed class RampEditorDialog : Form
{
    private readonly ColorRamp _ramp;

    private PictureBox _gradientBar = null!;
    private ListBox _stopList = null!;
    private NumericUpDown _position = null!;
    private Panel _colorSwatch = null!;
    private CheckBox _stepped = null!;
    private Bitmap? _gradientBmp;

    /// <summary>The edited ramp.  Valid when ShowDialog returns OK.</summary>
    public ColorRamp Result => _ramp;

    public RampEditorDialog(ColorRamp initial)
    {
        _ramp = initial.Clone();

        Text = "Edit Color Ramp";
        FormBorderStyle = FormBorderStyle.FixedDialog;
        StartPosition = FormStartPosition.CenterParent;
        MaximizeBox = false;
        MinimizeBox = false;
        ShowInTaskbar = false;
        ClientSize = new Size(470, 372);
        BackColor = Color.FromArgb(240, 240, 240);
        Font = new Font("Segoe UI", 9f);

        BuildControls();
        RebuildList();
        RenderGradient();
        if (_stopList.Items.Count > 0) _stopList.SelectedIndex = 0;
    }

    private void BuildControls()
    {
        Label lblBar = new() { Text = "Gradient (0 → 1):", Location = new Point(12, 10), AutoSize = true };

        _gradientBar = new PictureBox
        {
            Location = new Point(12, 30),
            Size = new Size(446, 44),
            BorderStyle = BorderStyle.FixedSingle,
            SizeMode = PictureBoxSizeMode.StretchImage
        };

        Label lblStops = new() { Text = "Stops:", Location = new Point(12, 84), AutoSize = true };

        _stopList = new ListBox
        {
            Location = new Point(12, 104),
            Size = new Size(250, 200),
            DrawMode = DrawMode.OwnerDrawFixed,
            ItemHeight = 24,
            IntegralHeight = false
        };
        _stopList.DrawItem += StopList_DrawItem;
        _stopList.SelectedIndexChanged += (_, _) => LoadSelectedIntoEditors();

        // ---- right-hand editor column ----
        int rx = 280;

        Label lblPos = new() { Text = "Position (%):", Location = new Point(rx, 104), AutoSize = true };
        _position = new NumericUpDown
        {
            Location = new Point(rx, 124),
            Size = new Size(80, 23),
            Minimum = 0,
            Maximum = 100,
            Value = 0
        };
        _position.ValueChanged += (_, _) => ApplyPositionToSelected();

        Label lblColor = new() { Text = "Color:", Location = new Point(rx, 156), AutoSize = true };
        _colorSwatch = new Panel
        {
            Location = new Point(rx, 176),
            Size = new Size(40, 24),
            BorderStyle = BorderStyle.FixedSingle,
            BackColor = Color.White
        };
        Button btnColor = new()
        {
            Text = "Pick…",
            Location = new Point(rx + 48, 175),
            Size = new Size(70, 26),
            Cursor = Cursors.Hand
        };
        btnColor.Click += (_, _) => PickColorForSelected();

        Button btnAdd = new()
        {
            Text = "Add Stop",
            Location = new Point(rx, 214),
            Size = new Size(85, 28),
            Cursor = Cursors.Hand
        };
        btnAdd.Click += (_, _) => AddStop();

        Button btnRemove = new()
        {
            Text = "Remove",
            Location = new Point(rx + 93, 214),
            Size = new Size(85, 28),
            Cursor = Cursors.Hand
        };
        btnRemove.Click += (_, _) => RemoveSelected();

        _stepped = new CheckBox
        {
            Text = "Stepped (hard indexed bands)",
            Location = new Point(rx, 250),
            AutoSize = true,
            Checked = _ramp.Stepped
        };
        _stepped.CheckedChanged += (_, _) =>
        {
            _ramp.Stepped = _stepped.Checked;
            RenderGradient();
        };

        Label lblPreset = new() { Text = "Presets:", Location = new Point(rx, 276), AutoSize = true };
        Button btnTerrain = new()
        {
            Text = "Terrain",
            Location = new Point(rx, 294),
            Size = new Size(85, 26),
            Cursor = Cursors.Hand
        };
        btnTerrain.Click += (_, _) => ApplyPreset(ColorRamp.DefaultTerrain());

        Button btnGray = new()
        {
            Text = "Grayscale",
            Location = new Point(rx + 93, 294),
            Size = new Size(85, 26),
            Cursor = Cursors.Hand
        };
        btnGray.Click += (_, _) => ApplyPreset(ColorRamp.Grayscale());

        // ---- OK / Cancel ----
        Button btnOk = new()
        {
            Text = "OK",
            DialogResult = DialogResult.OK,
            Location = new Point(294, 334),
            Size = new Size(80, 28),
            Cursor = Cursors.Hand
        };
        Button btnCancel = new()
        {
            Text = "Cancel",
            DialogResult = DialogResult.Cancel,
            Location = new Point(378, 334),
            Size = new Size(80, 28),
            Cursor = Cursors.Hand
        };

        AcceptButton = btnOk;
        CancelButton = btnCancel;

        Controls.AddRange(new Control[]
        {
            lblBar, _gradientBar, lblStops, _stopList,
            lblPos, _position, lblColor, _colorSwatch, btnColor,
            btnAdd, btnRemove, _stepped,
            lblPreset, btnTerrain, btnGray,
            btnOk, btnCancel
        });
    }

    // -------------------------------------------------------
    //  Rendering
    // -------------------------------------------------------

    private void RenderGradient()
    {
        int w = Math.Max(1, _gradientBar.ClientSize.Width);
        int h = Math.Max(1, _gradientBar.ClientSize.Height);

        Bitmap bmp = new(w, h, PixelFormat.Format32bppArgb);
        using (Graphics g = Graphics.FromImage(bmp))
        {
            for (int x = 0; x < w; x++)
            {
                double t = w == 1 ? 0.0 : (double)x / (w - 1);
                using Pen pen = new(_ramp.Evaluate(t));
                g.DrawLine(pen, x, 0, x, h);
            }

            // Mark each stop with a thin tick.
            using Pen tick = new(Color.FromArgb(180, 0, 0, 0));
            foreach (ColorRamp.Stop s in _ramp.Stops)
            {
                int sx = (int)Math.Round(s.Position * (w - 1));
                g.DrawLine(tick, sx, h - 8, sx, h - 1);
            }
        }

        _gradientBar.Image = bmp;
        _gradientBmp?.Dispose();
        _gradientBmp = bmp;
    }

    private void RebuildList()
    {
        // Preserve selection by remembering the underlying stop reference.
        ColorRamp.Stop? selected = _stopList.SelectedIndex >= 0
            && _stopList.SelectedIndex < _ramp.Count
                ? _ramp.Stops[_stopList.SelectedIndex]
                : null;

        _stopList.BeginUpdate();
        _stopList.Items.Clear();
        for (int i = 0; i < _ramp.Count; i++)
        {
            _stopList.Items.Add(i);   // index proxy; DrawItem reads the ramp
        }
        _stopList.EndUpdate();

        if (selected != null)
        {
            int idx = IndexOfStop(selected);
            if (idx >= 0) _stopList.SelectedIndex = idx;
        }
    }

    private void StopList_DrawItem(object? sender, DrawItemEventArgs e)
    {
        e.DrawBackground();
        if (e.Index < 0 || e.Index >= _ramp.Count) return;

        ColorRamp.Stop s = _ramp.Stops[e.Index];

        Rectangle bounds = e.Bounds;
        Rectangle swatch = new(bounds.Left + 4, bounds.Top + 3, 30, bounds.Height - 6);

        using (SolidBrush sb = new(s.Color)) e.Graphics.FillRectangle(sb, swatch);
        e.Graphics.DrawRectangle(Pens.Black, swatch);

        string text = $"{s.Position * 100.0:F0}%   RGB({s.Color.R}, {s.Color.G}, {s.Color.B})";
        using SolidBrush textBrush = new(e.ForeColor);
        e.Graphics.DrawString(text, e.Font ?? Font, textBrush,
            swatch.Right + 8, bounds.Top + 4);

        e.DrawFocusRectangle();
    }

    // -------------------------------------------------------
    //  Editing
    // -------------------------------------------------------

    private ColorRamp.Stop? Selected =>
        _stopList.SelectedIndex >= 0 && _stopList.SelectedIndex < _ramp.Count
            ? _ramp.Stops[_stopList.SelectedIndex]
            : null;

    private bool _loading;

    private void LoadSelectedIntoEditors()
    {
        ColorRamp.Stop? s = Selected;
        if (s == null) return;

        _loading = true;
        _position.Value = (decimal)Math.Round(s.Position * 100.0);
        _colorSwatch.BackColor = s.Color;
        _loading = false;
    }

    private void ApplyPositionToSelected()
    {
        if (_loading) return;
        ColorRamp.Stop? s = Selected;
        if (s == null) return;

        s.Position = (double)_position.Value / 100.0;
        _ramp.Sort();
        RebuildList();
        RenderGradient();
    }

    private void PickColorForSelected()
    {
        ColorRamp.Stop? s = Selected;
        if (s == null) return;

        using ColorDialog dlg = new() { Color = s.Color, FullOpen = true };
        if (dlg.ShowDialog(this) == DialogResult.OK)
        {
            s.Color = dlg.Color;
            _colorSwatch.BackColor = s.Color;
            _stopList.Invalidate();
            RenderGradient();
        }
    }

    private void AddStop()
    {
        double pos = (double)_position.Value / 100.0;
        Color c = _colorSwatch.BackColor;
        _ramp.Add(pos, c);
        RebuildList();
        RenderGradient();

        int idx = IndexOfStopAtPosition(pos);
        if (idx >= 0) _stopList.SelectedIndex = idx;
    }

    private void RemoveSelected()
    {
        if (_ramp.Count <= 2)
        {
            MessageBox.Show(this, "A ramp needs at least two stops.",
                "Edit Color Ramp", MessageBoxButtons.OK, MessageBoxIcon.Information);
            return;
        }

        int idx = _stopList.SelectedIndex;
        if (idx < 0) return;

        _ramp.RemoveAt(idx);
        RebuildList();
        RenderGradient();

        if (_stopList.Items.Count > 0)
        {
            _stopList.SelectedIndex = Math.Min(idx, _stopList.Items.Count - 1);
        }
    }

    private void ApplyPreset(ColorRamp preset)
    {
        _ramp.Clear();
        foreach (ColorRamp.Stop s in preset.Stops) _ramp.Add(s.Position, s.Color);
        _ramp.Stepped = preset.Stepped;
        _stepped.Checked = preset.Stepped;
        RebuildList();
        RenderGradient();
        if (_stopList.Items.Count > 0) _stopList.SelectedIndex = 0;
    }

    private int IndexOfStop(ColorRamp.Stop stop)
    {
        for (int i = 0; i < _ramp.Count; i++)
        {
            if (ReferenceEquals(_ramp.Stops[i], stop)) return i;
        }
        return -1;
    }

    private int IndexOfStopAtPosition(double pos)
    {
        int best = -1;
        double bestDelta = double.MaxValue;
        for (int i = 0; i < _ramp.Count; i++)
        {
            double d = Math.Abs(_ramp.Stops[i].Position - pos);
            if (d < bestDelta)
            {
                bestDelta = d;
                best = i;
            }
        }
        return best;
    }

    protected override void Dispose(bool disposing)
    {
        if (disposing)
        {
            _gradientBmp?.Dispose();
        }
        base.Dispose(disposing);
    }
}
