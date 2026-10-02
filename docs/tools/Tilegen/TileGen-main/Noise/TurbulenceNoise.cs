namespace EntitledLogicTileGen.Noise;

/// <summary>
/// Turbulence noise — sum of |Perlin| across multiple octaves.
/// The sign-inversion near zero crossings creates sharp ridge-like
/// features, giving a fire, cloud, or marble-vein appearance.
///
/// Tiling uses the same period-doubling strategy as fBm.
/// </summary>
public sealed class TurbulenceNoise
{
    // -------------------------------------------------------
    //  Constants
    // -------------------------------------------------------

    private const int    Octaves    = 6;
    private const double Lacunarity = 2.0;
    private const double Gain       = 0.5;

    // -------------------------------------------------------
    //  Fields
    // -------------------------------------------------------

    private readonly PerlinNoise _perlin;

    // -------------------------------------------------------
    //  Constructor
    // -------------------------------------------------------

    public TurbulenceNoise(int seed)
    {
        _perlin = new PerlinNoise(seed);
    }

    // -------------------------------------------------------
    //  Non-tiling sample
    // -------------------------------------------------------

    /// <summary>Returns turbulence in approximately [0, 1].</summary>
    public double Sample(double x, double y)
    {
        double value     = 0.0;
        double amplitude = 0.5;
        double frequency = 1.0;
        double maxValue  = 0.0;

        for (int i = 0; i < Octaves; i++)
        {
            value    += Math.Abs(_perlin.Sample(x * frequency, y * frequency)) * amplitude;
            maxValue += amplitude;
            frequency *= Lacunarity;
            amplitude *= Gain;
        }

        return value / maxValue;
    }

    /// <summary>Returns a value in [0, 1].</summary>
    public double Sample01(double x, double y)
    {
        return Math.Clamp(Sample(x, y), 0.0, 1.0);
    }

    // -------------------------------------------------------
    //  Tiling sample
    // -------------------------------------------------------

    /// <summary>
    /// Turbulence in [0, 1] that tiles seamlessly at the given base period.
    /// </summary>
    public double SampleTiling01(double x, double y, int basePeriod)
    {
        double value     = 0.0;
        double amplitude = 0.5;
        double frequency = 1.0;
        double maxValue  = 0.0;
        int    period    = basePeriod;

        for (int i = 0; i < Octaves; i++)
        {
            value    += Math.Abs(_perlin.SampleTiling(x * frequency, y * frequency, period)) * amplitude;
            maxValue += amplitude;
            frequency *= Lacunarity;
            period    *= (int)Lacunarity;
            amplitude *= Gain;
        }

        return Math.Clamp(value / maxValue, 0.0, 1.0);
    }
}
