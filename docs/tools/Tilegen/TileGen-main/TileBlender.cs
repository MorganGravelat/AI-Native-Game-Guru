using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;

namespace EntitledLogicTileGen;

/// <summary>
/// Blends a 3 × 3 patch of 64 × 64 tiles so that the central tile and each
/// of its selected neighbours join with no visible seam.
///
/// The caller supplies a 3 × 3 grid of source bitmaps (null = empty slot).
/// Grid layout, with [1, 1] always the centre:
///
///        col 0   col 1   col 2
///   row 0  NW      N      NE
///   row 1   W      C       E
///   row 2  SW      S      SE
///
/// BlendPatch returns a parallel 3 × 3 grid of NEW bitmaps — one for every
/// non-null source cell — leaving the originals untouched.
///
/// ── How the blend works ──────────────────────────────────────────────
/// Each output pixel is a weighted mix of up to four tiles: the tile
/// itself, its vertical neighbour (N or S), its horizontal neighbour
/// (E or W) and the diagonal neighbour in that corner.  Neighbour colours
/// are sampled at the *mirrored* position across the shared seam
/// (coordinate c → 63 − c), so a pixel one step inside the seam reads the
/// neighbour pixel one step on the other side.
///
/// The mix weight at any seam is exactly ½, so two adjacent tiles converge
/// to the same midpoint colour at their shared edge — that is what makes
/// the join seamless.  Because every tile in the patch applies the *same*
/// rule against the *same* neighbours, the four-way corner points where
/// four tiles meet also resolve to one common colour.
///
/// Corner behaviour:
///   • Both edges of a corner selected  → bilinear blend across self, the
///     two edge neighbours and the diagonal tile (a clean diagonal split).
///   • Only the diagonal selected        → a soft pull toward the diagonal
///     tile confined to the corner.
///   • A perpendicular edge with nothing on it stays pristine: any parallel
///     blend tapers to zero as it approaches that edge, so the falloff runs
///     diagonally and the untouched edge can still tile against other tiles.
/// </summary>
public static class TileBlender
{
    public const int TileSize = 64;

    private const int Max = TileSize - 1;   // 63

    // -------------------------------------------------------
    //  Public entry point
    // -------------------------------------------------------

    /// <summary>
    /// Blends every non-null cell of a 3 × 3 source grid against its
    /// selected neighbours.  Returns a new 3 × 3 grid of blended bitmaps
    /// (null where the source was null).  Source bitmaps are not modified.
    /// </summary>
    /// <param name="cells">3 × 3 grid [col, row]; [1, 1] is the centre.</param>
    /// <param name="blendWidth">
    /// How many pixels the blend reaches into each tile from a seam
    /// (clamped to 1 … 32).
    /// </param>
    public static Bitmap?[,] BlendPatch(Bitmap?[,] cells, int blendWidth)
    {
        int bw = Math.Clamp(blendWidth, 1, TileSize / 2);

        Bitmap?[,] result = new Bitmap?[3, 3];

        for (int cy = 0; cy < 3; cy++)
        {
            for (int cx = 0; cx < 3; cx++)
            {
                if (cells[cx, cy] != null)
                {
                    result[cx, cy] = BlendCell(cells, cx, cy, bw);
                }
            }
        }

        return result;
    }

    // -------------------------------------------------------
    //  Blend one cell against its present neighbours
    // -------------------------------------------------------

    private static Bitmap BlendCell(Bitmap?[,] cells, int cx, int cy, int bw)
    {
        Bitmap self = cells[cx, cy]!;

        // Orthogonal and diagonal neighbours (null if off-grid or empty)
        Bitmap? n  = At(cells, cx,     cy - 1);
        Bitmap? s  = At(cells, cx,     cy + 1);
        Bitmap? w  = At(cells, cx - 1, cy);
        Bitmap? e  = At(cells, cx + 1, cy);
        Bitmap? nw = At(cells, cx - 1, cy - 1);
        Bitmap? ne = At(cells, cx + 1, cy - 1);
        Bitmap? sw = At(cells, cx - 1, cy + 1);
        Bitmap? se = At(cells, cx + 1, cy + 1);

        Bitmap output = new Bitmap(TileSize, TileSize, PixelFormat.Format32bppArgb);

        for (int y = 0; y < TileSize; y++)
        {
            // Nearest vertical edge: top half pulls toward N, bottom toward S.
            bool    top  = y <= Max - y;
            Bitmap? vTile = top ? n : s;
            double  gV   = Ramp(top ? y : Max - y, bw);   // 1 at the edge → 0 at depth bw
            int     vMirrorY = Max - y;

            for (int x = 0; x < TileSize; x++)
            {
                // Nearest horizontal edge: left half pulls toward W, right toward E.
                bool    left = x <= Max - x;
                Bitmap? hTile = left ? w : e;
                double  gH   = Ramp(left ? x : Max - x, bw);
                int     hMirrorX = Max - x;

                // Diagonal tile in this corner direction
                Bitmap? dTile = top ? (left ? nw : ne)
                                    : (left ? sw : se);

                bool vPresent = vTile != null;
                bool hPresent = hTile != null;
                bool dPresent = dTile != null;

                // A blend axis is active when its edge neighbour OR the corner
                // diagonal is present (the diagonal needs both axes to engage).
                bool vActive = vPresent || dPresent;
                bool hActive = hPresent || dPresent;

                double vt = vActive ? 0.5 * gV : 0.0;
                double ht = hActive ? 0.5 * gH : 0.0;

                // Respect an unblended perpendicular edge: if nothing is selected
                // on the horizontal side, fade the vertical blend out as it nears
                // that edge (and vice-versa).  This is what makes the falloff run
                // diagonally and leaves the empty edge untouched.
                if (!(hPresent || dPresent)) vt *= 1.0 - gH;
                if (!(vPresent || dPresent)) ht *= 1.0 - gV;

                // Sample each contributor.  An absent neighbour reads back the
                // tile's own pixel, so its weight harmlessly returns to "self".
                (double a0, double r0, double g0, double b0) = Px(self, x, y);

                (double aV, double rV, double gV2, double bV) =
                    vPresent ? Px(vTile!, x, vMirrorY) : (a0, r0, g0, b0);

                (double aH, double rH, double gH2, double bH) =
                    hPresent ? Px(hTile!, hMirrorX, y) : (a0, r0, g0, b0);

                (double aD, double rD, double gD, double bD) =
                    dPresent ? Px(dTile!, hMirrorX, vMirrorY) : (a0, r0, g0, b0);

                // Bilinear partition of unity across the four contributors.
                double wSelf = (1.0 - vt) * (1.0 - ht);
                double wV    = vt         * (1.0 - ht);
                double wH    = (1.0 - vt) * ht;
                double wD    = vt         * ht;

                int a = Round(wSelf * a0 + wV * aV  + wH * aH  + wD * aD);
                int r = Round(wSelf * r0 + wV * rV  + wH * rH  + wD * rD);
                int g = Round(wSelf * g0 + wV * gV2 + wH * gH2 + wD * gD);
                int b = Round(wSelf * b0 + wV * bV  + wH * bH  + wD * bD);

                output.SetPixel(x, y, Color.FromArgb(a, r, g, b));
            }
        }

        return output;
    }

    // -------------------------------------------------------
    //  Helpers
    // -------------------------------------------------------

    /// <summary>Returns cells[col, row], or null if outside the 3 × 3 grid.</summary>
    private static Bitmap? At(Bitmap?[,] cells, int col, int row)
    {
        if (col < 0 || col > 2 || row < 0 || row > 2) return null;
        return cells[col, row];
    }

    /// <summary>
    /// Smooth proximity ramp: 1.0 exactly at the edge (distance 0), easing
    /// down to 0.0 at <paramref name="bw"/> pixels in (smoothstep curve).
    /// </summary>
    private static double Ramp(int distance, int bw)
    {
        if (distance >= bw) return 0.0;
        double s = 1.0 - (double)distance / bw;   // 1 at edge → 0 at depth bw
        return s * s * (3.0 - 2.0 * s);           // smoothstep for a soft taper
    }

    /// <summary>
    /// Reads a pixel as (A, R, G, B) doubles, clamping coordinates so a
    /// non-64 × 64 neighbour can never throw.
    /// </summary>
    private static (double a, double r, double g, double b) Px(Bitmap bmp, int x, int y)
    {
        int cx = Math.Clamp(x, 0, bmp.Width  - 1);
        int cy = Math.Clamp(y, 0, bmp.Height - 1);
        Color c = bmp.GetPixel(cx, cy);
        return (c.A, c.R, c.G, c.B);
    }

    private static int Round(double v)
    {
        return Math.Clamp((int)Math.Round(v), 0, 255);
    }

    /// <summary>
    /// Tiles a single <see cref="TileSize"/>×<see cref="TileSize"/> image into
    /// an n × n grid (nearest-neighbour, so pixel art stays crisp).  Shared by
    /// the Tile Blender and Generator previews.
    /// </summary>
    public static Bitmap Tile(Bitmap tile, int n)
    {
        int side = TileSize * n;
        Bitmap result = new(side, side, PixelFormat.Format32bppArgb);

        using Graphics g = Graphics.FromImage(result);
        g.InterpolationMode = InterpolationMode.NearestNeighbor;
        g.PixelOffsetMode = PixelOffsetMode.Half;

        for (int ty = 0; ty < n; ty++)
            for (int tx = 0; tx < n; tx++)
                g.DrawImage(tile, tx * TileSize, ty * TileSize, TileSize, TileSize);

        return result;
    }
}
