namespace EntitledLogicTileGen.Noise;

/// <summary>
/// Fractal Brownian Motion — layered Perlin noise across multiple octaves.
///
/// Tiling works because each octave k uses period P × 2^k, which is always
/// an integer multiple of the base period P.  Every octave therefore loops
/// at exactly the same tile boundary, so the sum of octaves also loops.
/// </summary>
public sealed class FractalBrownianMotion
{
    // -------------------------------------------------------
    //  Constants
    // -------------------------------------------------------

    private const int    Octaves    = 6;
    private const double Lacunarity = 2.0;   // frequency multiplier per octave
    private const double Gain       = 0.5;   // amplitude multiplier per octave

    // -------------------------------------------------------
    //  Fields
    // -------------------------------------------------------

    private readonly PerlinNoise _perlin;

    // -------------------------------------------------------
    //  Constructor
    // -------------------------------------------------------

    public FractalBrownianMotion(int seed)
    {
        _perlin = new PerlinNoise(seed);
    }

    // -------------------------------------------------------
    //  Non-tiling sample
    // -------------------------------------------------------

    /// <summary>Returns an fBm value in approximately [-1, 1].</summary>
    public double Sample(double x, double y)
    {
        double value     = 0.0;
        double amplitude = 0.5;
        double frequency = 1.0;
        double maxValue  = 0.0;

        for (int i = 0; i < Octaves; i++)
        {
            value    += _perlin.Sample(x * frequency, y * frequency) * amplitude;
            maxValue += amplitude;
            frequency *= Lacunarity;
            amplitude *= Gain;
        }

        return value / maxValue;
    }

    /// <summary>Returns a value in [0, 1].</summary>
    public double Sample01(double x, double y)
    {
        return (Sample(x, y) + 1.0) * 0.5;
    }

    // -------------------------------------------------------
    //  Tiling sample
    // -------------------------------------------------------

    /// <summary>
    /// fBm in [0, 1] that tiles seamlessly at the given base period.
    /// Octave k uses period = basePeriod × lacunarity^k (always an integer
    /// multiple) so all octaves loop at exactly the same tile edges.
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
            value    += _perlin.SampleTiling(x * frequency, y * frequency, period) * amplitude;
            maxValue += amplitude;
            frequency *= Lacunarity;
            period    *= (int)Lacunarity;   // period doubles → still tiles at base
            amplitude *= Gain;
        }

        double raw = value / maxValue;   // [-1, 1]
        return (raw + 1.0) * 0.5;
    }
}
