using System.Drawing;
using System.Windows.Forms;

namespace EntitledLogicTileGen;

/// <summary>
/// A deliberately plain "About" box for the tool.  It is intentionally
/// minimal so it can be enhanced later (logo, hyperlinks, build info,
/// credits, an icon, etc.).  The whole UI is built in code to match the
/// rest of the project — no Designer / no .resx.
///
/// Launch it with:
///     using AboutDialog dlg = new();
///     dlg.ShowDialog(this);
///
/// ── Enhance-me checklist (all optional) ────────────────────────────────
///   • Drop a logo into a PictureBox at the top.
///   • Turn the website / repo line into a clickable LinkLabel.
///   • Pull the real assembly version instead of the Version constant.
///   • Add a credits / third-party-licenses section or a scrollable panel.
/// The layout below leaves room on the right of the text column for an
/// icon, and the form can simply be made taller for extra sections.
/// </summary>
public sealed class AboutDialog : Form
{
    // Bump these as you like — kept as plain constants so there is one
    // obvious place to edit them.
    private const string AppName = "Procedural Tile Generator";
    private const string Version = "1.0";
    private const string Tagline =
        "Generates seamless 64×64 procedural tiles and grid maps for The AI-Native Game Guru series.";

    public AboutDialog()
    {
        Text = "About";
        FormBorderStyle = FormBorderStyle.FixedDialog;
        StartPosition = FormStartPosition.CenterParent;
        MaximizeBox = false;
        MinimizeBox = false;
        ShowInTaskbar = false;
        ClientSize = new Size(440, 196);
        BackColor = Color.FromArgb(240, 240, 240);
        Font = new Font("Segoe UI", 9f);

        BuildControls();
    }

    private void BuildControls()
    {
        Label lblName = new()
        {
            Text = AppName,
            Location = new Point(16, 18),
            AutoSize = true,
            Font = new Font("Segoe UI", 12f, FontStyle.Bold)
        };

        Label lblVersion = new()
        {
            Text = $"Version {Version}",
            Location = new Point(18, 50),
            AutoSize = true,
            ForeColor = Color.FromArgb(90, 90, 90),
            Font = new Font("Segoe UI", 9f, FontStyle.Italic)
        };

        Label lblTagline = new()
        {
            Text = Tagline,
            Location = new Point(18, 80),
            Size = new Size(404, 44),
            Font = new Font("Segoe UI", 9f)
        };

        Label lblCopyright = new()
        {
            // DateTime comes in via ImplicitUsings (System).
            Text = $"© {DateTime.Now.Year}  ·  The AI-Native",
            Location = new Point(18, 132),
            AutoSize = true,
            ForeColor = Color.FromArgb(120, 120, 120)
        };

        Button btnOk = new()
        {
            Text = "OK",
            DialogResult = DialogResult.OK,
            Location = new Point(344, 156),
            Size = new Size(80, 28),
            Cursor = Cursors.Hand
        };

        AcceptButton = btnOk;
        CancelButton = btnOk;

        Controls.AddRange(new Control[]
        {
            lblName, lblVersion, lblTagline, lblCopyright, btnOk
        });
    }
}
