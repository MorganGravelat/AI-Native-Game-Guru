namespace EntitledLogicTileGen.Noise;

/// <summary>
/// Value noise — random values at integer lattice points blended with a
/// smooth cubic curve.  Tiling variant wraps lattice lookups at the period
/// boundary so edges of adjacent tiles match perfectly.
/// </summary>
public sealed class ValueNoise
{
    // -------------------------------------------------------
    //  Fields
    // -------------------------------------------------------

    // 256 × 256 table; indexed as [x & 255, y & 255] via flat index
    private readonly double[] _lattice = new double[256 * 256];

    // -------------------------------------------------------
    //  Constructor
    // -------------------------------------------------------

    public ValueNoise(int seed)
    {
        Random rng = new(seed);
        for (int i = 0; i < _lattice.Length; i++)
        {
            _lattice[i] = rng.NextDouble();
        }
    }

    // -------------------------------------------------------
    //  Private helpers
    // -------------------------------------------------------

    private double At(int x, int y)
    {
        return _lattice[(x & 255) + (y & 255) * 256];
    }

    private double AtTiling(int x, int y, int period)
    {
        int wx = ((x % period) + period) % period;
        int wy = ((y % period) + period) % period;
        return _lattice[wx + wy * 256];
    }

    private static double Smooth(double t)
    {
        return t * t * (3.0 - 2.0 * t);
    }

    private static double Lerp(double a, double b, double t)
    {
        return a + t * (b - a);
    }

    // -------------------------------------------------------
    //  Non-tiling sample
    // -------------------------------------------------------

    /// <summary>Returns a noise value in [0, 1].</summary>
    public double Sample01(double x, double y)
    {
        int    ix = (int)Math.Floor(x);
        int    iy = (int)Math.Floor(y);
        double fx = x - Math.Floor(x);
        double fy = y - Math.Floor(y);

        double u = Smooth(fx);
        double v = Smooth(fy);

        return Lerp(
            Lerp(At(ix, iy    ), At(ix + 1, iy    ), u),
            Lerp(At(ix, iy + 1), At(ix + 1, iy + 1), u),
            v);
    }

    // -------------------------------------------------------
    //  Tiling sample
    // -------------------------------------------------------

    /// <summary>
    /// Value noise in [0, 1] that repeats exactly every
    /// <paramref name="period"/> units in both axes.
    /// </summary>
    public double SampleTiling01(double x, double y, int period)
    {
        int    ix = (int)Math.Floor(x);
        int    iy = (int)Math.Floor(y);
        double fx = x - Math.Floor(x);
        double fy = y - Math.Floor(y);

        double u = Smooth(fx);
        double v = Smooth(fy);

        return Lerp(
            Lerp(AtTiling(ix,     iy,     period), AtTiling(ix + 1, iy,     period), u),
            Lerp(AtTiling(ix,     iy + 1, period), AtTiling(ix + 1, iy + 1, period), u),
            v);
    }
}
