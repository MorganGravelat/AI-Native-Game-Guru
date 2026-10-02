namespace EntitledLogicTileGen.Noise;

/// <summary>
/// Classic 2-D gradient (Perlin) noise with seamless-tiling support.
///
/// SampleTiling01(x, y, period) guarantees that the noise value at
/// x == period exactly equals the value at x == 0, so tiles placed
/// side-by-side have perfectly matching edges with no fold artefact.
/// </summary>
public sealed class PerlinNoise
{
    // -------------------------------------------------------
    //  Fields
    // -------------------------------------------------------

    private readonly int[] _perm = new int[512];

    // -------------------------------------------------------
    //  Constructor
    // -------------------------------------------------------

    public PerlinNoise(int seed)
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

    private static double Fade(double t)
    {
        return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
    }

    private static double Lerp(double a, double b, double t)
    {
        return a + t * (b - a);
    }

    private static double Grad(int hash, double x, double y)
    {
        int    h = hash & 7;
        double u = h < 4 ? x : y;
        double v = h < 4 ? y : x;
        return ((h & 1) != 0 ? -u : u) + ((h & 2) != 0 ? -v : v);
    }

    // -------------------------------------------------------
    //  Non-tiling sample (general reference)
    // -------------------------------------------------------

    /// <summary>Returns a noise value in approximately [-1, 1].</summary>
    public double Sample(double x, double y)
    {
        int xi = (int)Math.Floor(x) & 255;
        int yi = (int)Math.Floor(y) & 255;

        double xf = x - Math.Floor(x);
        double yf = y - Math.Floor(y);

        double u = Fade(xf);
        double v = Fade(yf);

        int aa = _perm[_perm[xi    ] + yi    ];
        int ab = _perm[_perm[xi    ] + yi + 1];
        int ba = _perm[_perm[xi + 1] + yi    ];
        int bb = _perm[_perm[xi + 1] + yi + 1];

        double row0 = Lerp(Grad(aa, xf,       yf      ),
                           Grad(ba, xf - 1.0, yf      ), u);
        double row1 = Lerp(Grad(ab, xf,       yf - 1.0),
                           Grad(bb, xf - 1.0, yf - 1.0), u);

        return Lerp(row0, row1, v);
    }

    // -------------------------------------------------------
    //  Tiling sample
    // -------------------------------------------------------

    /// <summary>
    /// Noise in [-1, 1] that repeats exactly every <paramref name="period"/> units.
    /// Integer cell indices are wrapped modulo the period so that the gradient
    /// at cell 0 is reused at cell P, giving a seamless loop.
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

        int aa = _perm[_perm[xi ] + yi ];
        int ab = _perm[_perm[xi ] + yi1];
        int ba = _perm[_perm[xi1] + yi ];
        int bb = _perm[_perm[xi1] + yi1];

        double row0 = Lerp(Grad(aa, xf,       yf      ),
                           Grad(ba, xf - 1.0, yf      ), u);
        double row1 = Lerp(Grad(ab, xf,       yf - 1.0),
                           Grad(bb, xf - 1.0, yf - 1.0), u);

        return Lerp(row0, row1, v);
    }

    /// <summary>Tiling noise remapped to [0, 1].</summary>
    public double SampleTiling01(double x, double y, int period)
    {
        return (SampleTiling(x, y, period) + 1.0) * 0.5;
    }
}
