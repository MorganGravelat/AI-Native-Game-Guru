using System.Drawing;
using System.Drawing.Imaging;

namespace EntitledLogicTileGen;

/// <summary>
/// Colour-ramp generation path.  Mirrors the two-colour
/// <see cref="TileGenerator.Generate(Color, Color, int, int, NoiseAlgorithm, int)"/>
/// overload but maps the normalised noise value through a multi-stop
/// <see cref="ColorRamp"/>, so a single tile can span several terrain bands.
///
/// The noise pipeline (4-D torus sampling, normalisation, contrast) is
/// shared with the original path via the existing private helpers — only
/// the final colour-assignment step differs.
/// </summary>
public static partial class TileGenerator
{
    /// <summary>
    /// Generates a 64 × 64 seamless tile, colouring each pixel by passing
    /// its normalised noise value through <paramref name="ramp"/>.
    /// </summary>
    public static Bitmap Generate(
        ColorRamp ramp,
        int saturation,
        int noiseLevel,
        NoiseAlgorithm algorithm,
        int seed)
    {
        double[,] noiseMap = BuildNoiseMap(algorithm, seed);
        ApplyNoiseContrast(noiseMap, noiseLevel);
        return BuildBitmap(noiseMap, ramp, saturation);
    }

    private static Bitmap BuildBitmap(double[,] noiseMap, ColorRamp ramp, int saturation)
    {
        Bitmap bmp = new(TileSize, TileSize, PixelFormat.Format32bppArgb);

        for (int py = 0; py < TileSize; py++)
        {
            for (int px = 0; px < TileSize; px++)
            {
                Color c = ramp.Evaluate(noiseMap[px, py]);

                if (saturation < 100)
                {
                    c = AdjustSaturation(c, saturation / 100.0);
                }

                bmp.SetPixel(px, py, c);
            }
        }

        return bmp;
    }
}
