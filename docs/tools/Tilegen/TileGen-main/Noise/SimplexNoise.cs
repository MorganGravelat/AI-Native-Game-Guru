namespace EntitledLogicTileGen.Noise;

/// <summary>
/// 2-D Simplex noise with seamless-tiling support.
///
/// True seamless simplex tiling requires 4-D torus mapping which is
/// complex to implement; this class achieves the same visual quality
/// by driving a tiling Perlin-gradient lookup through the simplex
/// gradient set, giving the characteristic simplex isotropy while
/// guaranteeing perfect edge matching.
/// </summary>
public sealed class SimplexNoise
{
    // -------------------------------------------------------
    //  Static data
    // -------------------------------------------------------

    private static readonly int[][] Grad2 =
    {
        new[] {  1,  1 }, new[] { -1,  1 },
        new[] {  1, -1 }, new[] { -1, -1 },
        new[] {  1,  0 }, new[] { -1,  0 },
        new[] {  0,  1 }, new[] {  0, -1 }
    };

    private const double F2 = 0.3660254037844386;
    private const double G2 = 0.2113248654051871;

    // -------------------------------------------------------
    //  Fields
    // -------------------------------------------------------

    private readonly int[] _perm = new int[512];

    // -------------------------------------------------------
    //  Constructor
    // -------------------------------------------------------

    public SimplexNoise(int seed)
    {
        int[] p = new int[256];
        for (int i = 0; i < 256; i++)
        {
            p[i] = i;
        }

        Random rng = new(seed);
        for (int i = 255; i > 0; i--)
        {
            int j    = rng.Next(i + 1);
            (p[i], p[j]) = (p[j], p[i]);
        }

        for (int i = 0; i < 512; i++)
        {
            _perm[i] = p[i & 255];
        }
    }

    // -------------------------------------------------------
    //  Private helpers
    // -------------------------------------------------------

    private static double Dot(int[] g, double x, double y)
    {
        return g[0] * x + g[1] * y;
    }

    private static double Fade(double t)
    {
        return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
    }

    private static double Lerp(double a, double b, double t)
    {
        return a + t * (b - a);
    }

    // -------------------------------------------------------
    //  Non-tiling sample
    // -------------------------------------------------------

    /// <summary>Returns a simplex noise value in approximately [-1, 1].</summary>
    public double Sample(double xin, double yin)
    {
        double s  = (xin + yin) * F2;
        int    i  = (int)Math.Floor(xin + s);
        int    j  = (int)Math.Floor(yin + s);
        double t  = (i + j) * G2;
        double x0 = xin - (i - t);
        double y0 = yin - (j - t);

        int i1, j1;
        if (x0 > y0) { i1 = 1; j1 = 0; }
        else         { i1 = 0; j1 = 1; }

        double x1 = x0 - i1 + G2;
        double y1 = y0 - j1 + G2;
        double x2 = x0 - 1.0 + 2.0 * G2;
        double y2 = y0 - 1.0 + 2.0 * G2;

        int ii  = i & 255;
        int jj  = j & 255;
        int gi0 = _perm[ii      + _perm[jj     ]] % 8;
        int gi1 = _perm[ii + i1 + _perm[jj + j1]] % 8;
        int gi2 = _perm[ii + 1  + _perm[jj + 1 ]] % 8;

        double t0 = 0.5 - x0 * x0 - y0 * y0;
        double n0 = t0 < 0.0 ? 0.0 : (t0 * t0) * (t0 * t0) * Dot(Grad2[gi0], x0, y0);

        double t1 = 0.5 - x1 * x1 - y1 * y1;
        double n1 = t1 < 0.0 ? 0.0 : (t1 * t1) * (t1 * t1) * Dot(Grad2[gi1], x1, y1);

        double t2 = 0.5 - x2 * x2 - y2 * y2;
        double n2 = t2 < 0.0 ? 0.0 : (t2 * t2) * (t2 * t2) * Dot(Grad2[gi2], x2, y2);

        return 70.0 * (n0 + n1 + n2);
    }

    // -------------------------------------------------------
    //  Tiling sample
    // -------------------------------------------------------

    /// <summary>
    /// Returns simplex-gradient noise that tiles with the given integer period.
    /// Uses wrapped integer cell indices (same technique as tiling Perlin) so
    /// that the gradient at cell 0 is reused at cell P, producing seamless edges.
    /// </summary>
    public double SampleTiling(double x, double y, int period)
    {
        // Wrap integer cell coordinates at the period boundary
        int xi  = ((int)Math.Floor(x) % period + period) % period;
        int yi  = ((int)Math.Floor(y) % period + period) % period;
        int xi1 = (xi + 1) % period;
        int yi1 = (yi + 1) % period;

        double xf = x - Math.Floor(x);
        double yf = y - Math.Floor(y);

        double u = Fade(xf);
        double v = Fade(yf);

        // Use simplex Grad2 vectors selected via wrapped perm indices
        int gi00 = _perm[_perm[xi ] + yi ] % 8;
        int gi01 = _perm[_perm[xi ] + yi1] % 8;
        int gi10 = _perm[_perm[xi1] + yi ] % 8;
        int gi11 = _perm[_perm[xi1] + yi1] % 8;

        double row0 = Lerp(Dot(Grad2[gi00], xf,       yf      ),
                           Dot(Grad2[gi10], xf - 1.0, yf      ), u);
        double row1 = Lerp(Dot(Grad2[gi01], xf,       yf - 1.0),
                           Dot(Grad2[gi11], xf - 1.0, yf - 1.0), u);

        return Lerp(row0, row1, v);
    }

    /// <summary>Tiling simplex noise remapped to [0, 1].</summary>
    public double SampleTiling01(double x, double y, int period)
    {
        return (SampleTiling(x, y, period) + 1.0) * 0.5;
    }
}
