using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Windows.Forms;

namespace EntitledLogicTileGen;

/// <summary>
/// Generator-tab additions:
///   • a "use colour ramp" toggle + ramp editor launcher (multi-stop
///     gradient / indexed-palette colouring),
///   • a configurable batch-variant count, and
///   • the shared N×N tiled-preview helper.
///
/// These hang off controls created in <see cref="BuildGeneratorExtras"/>
/// (called from BuildUI) plus a few surgical hooks in the original
/// generator methods (DoGenerate, RefreshPreview, GenerateBatch_Click).
/// </summary>
public sealed partial class MainForm
{
    // Colour ramp used when "use ramp" is ticked.  Starts as a terrain ramp;
    // the two-colour pickers still drive generation when the box is off.
    private ColorRamp _ramp = ColorRamp.DefaultTerrain();

    private CheckBox chkUseRamp = null!;
    private NumericUpDown nudBatch = null!;
    private ComboBox cmbTiling = null!;   // replaces the old 2×2-only checkbox

    private void BuildGeneratorExtras()
    {
        chkUseRamp = new CheckBox
        {
            Text = "Use color ramp",
            Location = new Point(10, 686),
            AutoSize = true,
            Checked = false,
            Font = new Font("Segoe UI", 9f)
        };
        chkUseRamp.CheckedChanged += (_, _) =>
        {
            // Regenerate the live preview so the effect is immediate.
            if (_lastTile != null)
            {
                _lastTile.Dispose();
                _lastTile = DoGenerate();
                RefreshPreview();
            }
        };

        Button btnEditRamp = new Button
        {
            Text = "Edit Ramp…",
            Location = new Point(150, 683),
            Size = new Size(95, 26),
            Cursor = Cursors.Hand,
            Font = new Font("Segoe UI", 9f)
        };
        btnEditRamp.Click += EditRamp_Click;

        Label lblBatch = new Label
        {
            Text = "Batch ×",
            Location = new Point(360, 686),
            AutoSize = true,
            Font = new Font("Segoe UI", 9f)
        };
        nudBatch = new NumericUpDown
        {
            Location = new Point(410, 683),
            Size = new Size(50, 23),
            Minimum = 2,
            Maximum = 99,
            Value = 10,
            Font = new Font("Segoe UI", 9f)
        };

        _tabTile.Controls.Add(chkUseRamp);
        _tabTile.Controls.Add(btnEditRamp);
        _tabTile.Controls.Add(lblBatch);
        _tabTile.Controls.Add(nudBatch);
    }

    private void EditRamp_Click(object? sender, EventArgs e)
    {
        using RampEditorDialog dlg = new(_ramp);
        if (dlg.ShowDialog(this) == DialogResult.OK)
        {
            _ramp = dlg.Result;

            // If ramp mode is on and we have a tile, refresh the preview.
            if (chkUseRamp.Checked && _lastTile != null)
            {
                _lastTile.Dispose();
                _lastTile = DoGenerate();
                RefreshPreview();
            }
        }
    }
}
