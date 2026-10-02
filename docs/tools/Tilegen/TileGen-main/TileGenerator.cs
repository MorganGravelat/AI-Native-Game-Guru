using System.Drawing;
using System.Drawing.Imaging;

namespace EntitledLogicTileGen;

/// <summary>
/// Generates a seamlessly-tileable 64 × 64 pixel tile.
///
/// Seamless tiling is achieved by mapping every pixel's 2-D tile
/// coordinate onto the surface of a 4-D torus and sampling a 4-D
/// noise function there.  Because the torus angles are derived from
/// cos/sin of the normalised pixel position, pixel 0 and the
/// hypothetical pixel 64 produce identical 4-D coordinates:
///
///   cos(2π × 0/64) = cos(2π × 64/64) = 1.0
///   sin(2π × 0/64) = sin(2π × 64/64) = 0.0
///
/// The tile edges therefore match perfectly with zero fold
/// artefacts and no mirroring.
///
/// Six distinct noise algorithms are supported — Perlin, Simplex,
/// Value, Worley, fBm (layered Perlin) and Turbulence (|layered
/// Perlin|).  Each is implemented natively in 4-D so all benefit
/// from the same torus-tiling guarantee.
/// </summary>
public static partial class TileGenerator
{
    // -------------------------------------------------------
    //  Constants
    // -------------------------------------------------------

    private const int TileSize = 64;

    // Controls pattern density (larger = more detail per tile)
    private const double BaseRadius = 2.8;

    private const int FbmOctaves = 6;
    private const double FbmLacunarity = 2.0;
    private const double FbmGain = 0.5;

    // Skew/unskew factors for 4-D Simplex
    private const double F4 = 0.30901699437494742;   // (sqrt(5) - 1) / 4
    private const double G4 = 0.13819660112501052;   // (5 - sqrt(5)) / 20

    // 32 gradient vectors for 4-D Perlin / Simplex noise.
    // Edge midpoints of the 4-cube — three ±1 components and one 0.
    private static readonly int[][] Grad4 = new int[][]
    {
        new int[] { 0, 1, 1, 1}, new int[] { 0, 1, 1,-1}, new int[] { 0, 1,-1, 1}, new int[] { 0, 1,-1,-1},
        new int[] { 0,-1, 1, 1}, new int[] { 0,-1, 1,-1}, new int[] { 0,-1,-1, 1}, new int[] { 0,-1,-1,-1},
        new int[] { 1, 0, 1, 1}, new int[] { 1, 0, 1,-1}, new int[] { 1, 0,-1, 1}, new int[] { 1, 0,-1,-1},
        new int[] {-1, 0, 1, 1}, new int[] {-1, 0, 1,-1}, new int[] {-1, 0,-1, 1}, new int[] {-1, 0,-1,-1},
        new int[] { 1, 1, 0, 1}, new int[] { 1, 1, 0,-1}, new int[] { 1,-1, 0, 1}, new int[] { 1,-1, 0,-1},
        new int[] {-1, 1, 0, 1}, new int[] {-1, 1, 0,-1}, new int[] {-1,-1, 0, 1}, new int[] {-1,-1, 0,-1},
        new int[] { 1, 1, 1, 0}, new int[] { 1, 1,-1, 0}, new int[] { 1,-1, 1, 0}, new int[] { 1,-1,-1, 0},
        new int[] {-1, 1, 1, 0}, new int[] {-1, 1,-1, 0}, new int[] {-1,-1, 1, 0}, new int[] {-1,-1,-1, 0}
    };

    // -------------------------------------------------------
    //  Public entry point
    // -------------------------------------------------------

    public static Bitmap Generate(
        Color color1,
        Color color2,
        int saturation,
        int noiseLevel,
        NoiseAlgorithm algorithm,
        int seed)
    {
        double[,] noiseMap = BuildNoiseMap(algorithm, seed);
        ApplyNoiseContrast(noiseMap, noiseLevel);
        return BuildBitmap(noiseMap, color1, color2, saturation);
    }

    // -------------------------------------------------------
    //  Noise map — 64 × 64, all values initially in raw range
    // -------------------------------------------------------

    private static double[,] BuildNoiseMap(NoiseAlgorithm algorithm, int seed)
    {
        double[,] map = new double[TileSize, TileSize];
        int[] perm = BuildPermutation(seed);

        // Pre-build per-octave permutations for the multi-octave algorithms.
        // (Without this we'd allocate FbmOctaves × TileSize² perm arrays per
        // generation — once per pixel, every pixel.)
        int[][]? octavePerms = null;
        if (algorithm == NoiseAlgorithm.FractalBrownianMotion
            || algorithm == NoiseAlgorithm.Turbulence)
        {
            octavePerms = new int[FbmOctaves][];
            for (int i = 0; i < FbmOctaves; i++)
            {
                octavePerms[i] = BuildPermutation(seed + i * 1013);
            }
        }

        double minV = double.MaxValue;
        double maxV = -double.MaxValue;

        for (int py = 0; py < TileSize; py++)
        {
            for (int px = 0; px < TileSize; px++)
            {
                double v = SampleAlgorithm(algorithm, px, py, perm, octavePerms);
                map[px, py] = v;
                if (v < minV) minV = v;
                if (v > maxV) maxV = v;
            }
        }

        // Normalise to [0, 1]
        double range = maxV - minV;
        if (range < 1e-9) range = 1.0;

        for (int py = 0; py < TileSize; py++)
        {
            for (int px = 0; px < TileSize; px++)
            {
                map[px, py] = (map[px, py] - minV) / range;
            }
        }

        return map;
    }

    // -------------------------------------------------------
    //  Algorithm dispatch
    // -------------------------------------------------------

    private static double SampleAlgorithm(
        NoiseAlgorithm algorithm,
        int px,
        int py,
        int[] perm,
        int[][]? octavePerms)
    {
        switch (algorithm)
        {
            case NoiseAlgorithm.Perlin:
                return TorusPerlin(px, py, BaseRadius, perm);

            case NoiseAlgorithm.Simplex:
                return TorusSimplex(px, py, BaseRadius, perm);

            case NoiseAlgorithm.Value:
                // Slightly smaller radius for bigger, blobbier value-noise cells
                return TorusValue(px, py, BaseRadius * 0.75, perm);

            case NoiseAlgorithm.FractalBrownianMotion:
                return TorusFbm(px, py, octavePerms!);

            case NoiseAlgorithm.Worley:
                return TorusWorley(px, py, perm);

            case NoiseAlgorithm.Turbulence:
                return TorusTurbulence(px, py, octavePerms!);

            default:
                return TorusValue(px, py, BaseRadius, perm);
        }
    }

    // -------------------------------------------------------
    //  Single-octave torus value noise
    // -------------------------------------------------------

    /// <summary>
    /// Maps pixel (px, py) onto a 4-D torus and samples value noise.
    /// The torus wraps perfectly so left==right and top==bottom.
    /// </summary>
    private static double TorusValue(int px, int py, double radius, int[] perm)
    {
        // Angles in [0, 2π)
        double ax = TwoPI * px / TileSize;
        double ay = TwoPI * py / TileSize;

        // 4-D torus coordinates
        double x4 = Math.Cos(ax) * radius;
        double y4 = Math.Sin(ax) * radius;
        double z4 = Math.Cos(ay) * radius;
        double w4 = Math.Sin(ay) * radius;

        return ValueNoise4D(x4, y4, z4, w4, perm);
    }

    private const double TwoPI = 2.0 * Math.PI;

    /// <summary>
    /// 4-D Perlin (gradient) noise on the torus.  Sharper, more
    /// directional structure than value noise.
    /// </summary>
    private static double TorusPerlin(int px, int py, double radius, int[] perm)
    {
        double ax = TwoPI * px / TileSize;
        double ay = TwoPI * py / TileSize;

        double x4 = Math.Cos(ax) * radius;
        double y4 = Math.Sin(ax) * radius;
        double z4 = Math.Cos(ay) * radius;
        double w4 = Math.Sin(ay) * radius;

        return PerlinNoise4D(x4, y4, z4, w4, perm);
    }

    /// <summary>
    /// 4-D Simplex noise on the torus.  Isotropic — visually free of
    /// the axis-aligned bias that Perlin can exhibit.
    /// </summary>
    private static double TorusSimplex(int px, int py, double radius, int[] perm)
    {
        double ax = TwoPI * px / TileSize;
        double ay = TwoPI * py / TileSize;

        double x4 = Math.Cos(ax) * radius;
        double y4 = Math.Sin(ax) * radius;
        double z4 = Math.Cos(ay) * radius;
        double w4 = Math.Sin(ay) * radius;

        return SimplexNoise4D(x4, y4, z4, w4, perm);
    }

    // -------------------------------------------------------
    //  Multi-octave algorithms — both layer 4-D Perlin, which is
    //  the classical fBm/turbulence formulation.  Octave perms are
    //  supplied by BuildNoiseMap so we don't reallocate per pixel.
    // -------------------------------------------------------

    private static double TorusFbm(int px, int py, int[][] octavePerms)
    {
        double value = 0.0;
        double amp = FbmGain;
        double total = 0.0;
        double radius = BaseRadius;

        for (int i = 0; i < FbmOctaves; i++)
        {
            value += TorusPerlin(px, py, radius, octavePerms[i]) * amp;
            total += amp;
            radius *= FbmLacunarity;
            amp *= FbmGain;
        }

        return value / total;   // roughly [-1, 1] — normalised later
    }

    private static double TorusTurbulence(int px, int py, int[][] octavePerms)
    {
        double value = 0.0;
        double amp = FbmGain;
        double total = 0.0;
        double radius = BaseRadius;

        for (int i = 0; i < FbmOctaves; i++)
        {
            // Perlin returns signed noise; |signed| gives the
            // characteristic sharp turbulence ridges.
            double sample = TorusPerlin(px, py, radius, octavePerms[i]);
            value += Math.Abs(sample) * amp;
            total += amp;
            radius *= FbmLacunarity;
            amp *= FbmGain;
        }

        return value / total;   // roughly [0, 1] — normalised later
    }

    // -------------------------------------------------------
    //  Worley (cellular) noise on a torus
    // -------------------------------------------------------

    private static double TorusWorley(int px, int py, int[] perm)
    {
        // Map pixel to torus 4-D coords in the range used for cell lookup
        double ax = TwoPI * px / TileSize;
        double ay = TwoPI * py / TileSize;
        double r = BaseRadius;

        double x4 = Math.Cos(ax) * r;
        double y4 = Math.Sin(ax) * r;
        double z4 = Math.Cos(ay) * r;
        double w4 = Math.Sin(ay) * r;

        // Grid-cell coordinates in 4-D space
        int ix = (int)Math.Floor(x4);
        int iy = (int)Math.Floor(y4);
        int iz = (int)Math.Floor(z4);
        int iw = (int)Math.Floor(w4);

        double minDist = double.MaxValue;

        // Search 3^4 = 81 neighbouring 4-D cells
        for (int dw = -1; dw <= 1; dw++)
        {
            for (int dz = -1; dz <= 1; dz++)
            {
                for (int dy = -1; dy <= 1; dy++)
                {
                    for (int dx = -1; dx <= 1; dx++)
                    {
                        int cx = ix + dx;
                        int cy = iy + dy;
                        int cz = iz + dz;
                        int cw = iw + dw;

                        // Two feature points per 4-D cell
                        for (int k = 0; k < 2; k++)
                        {
                            int h = HashCell4D(perm, cx, cy, cz, cw * 31 + k);
                            double jx = cx + (h & 255) / 255.0;
                            double jy = cy + ((h >> 8) & 255) / 255.0;
                            double jz = cz + ((h >> 16) & 255) / 255.0;
                            double jw = cw + ((h >> 24) & 255) / 255.0;

                            double ddx = x4 - jx;
                            double ddy = y4 - jy;
                            double ddz = z4 - jz;
                            double ddw = w4 - jw;
                            double d = Math.Sqrt(ddx * ddx + ddy * ddy + ddz * ddz + ddw * ddw);
                            if (d < minDist) minDist = d;
                        }
                    }
                }
            }
        }

        return Math.Clamp(minDist * 1.4, 0.0, 1.0);
    }

    // -------------------------------------------------------
    //  4-D value noise
    // -------------------------------------------------------

    private static double ValueNoise4D(
        double x, double y, double z, double w, int[] perm)
    {
        int ix = (int)Math.Floor(x);
        int iy = (int)Math.Floor(y);
        int iz = (int)Math.Floor(z);
        int iw = (int)Math.Floor(w);
        double fx = Fade(x - ix);
        double fy = Fade(y - iy);
        double fz = Fade(z - iz);
        double fw = Fade(w - iw);

        // 16 lattice point values
        double v0000 = H(perm, ix, iy, iz, iw);
        double v1000 = H(perm, ix + 1, iy, iz, iw);
        double v0100 = H(perm, ix, iy + 1, iz, iw);
        double v1100 = H(perm, ix + 1, iy + 1, iz, iw);
        double v0010 = H(perm, ix, iy, iz + 1, iw);
        double v1010 = H(perm, ix + 1, iy, iz + 1, iw);
        double v0110 = H(perm, ix, iy + 1, iz + 1, iw);
        double v1110 = H(perm, ix + 1, iy + 1, iz + 1, iw);
        double v0001 = H(perm, ix, iy, iz, iw + 1);
        double v1001 = H(perm, ix + 1, iy, iz, iw + 1);
        double v0101 = H(perm, ix, iy + 1, iz, iw + 1);
        double v1101 = H(perm, ix + 1, iy + 1, iz, iw + 1);
        double v0011 = H(perm, ix, iy, iz + 1, iw + 1);
        double v1011 = H(perm, ix + 1, iy, iz + 1, iw + 1);
        double v0111 = H(perm, ix, iy + 1, iz + 1, iw + 1);
        double v1111 = H(perm, ix + 1, iy + 1, iz + 1, iw + 1);

        // Quadrilinear interpolation
        double w0 = Lerp(
                        Lerp(Lerp(v0000, v1000, fx), Lerp(v0100, v1100, fx), fy),
                        Lerp(Lerp(v0010, v1010, fx), Lerp(v0110, v1110, fx), fy),
                        fz);
        double w1 = Lerp(
                        Lerp(Lerp(v0001, v1001, fx), Lerp(v0101, v1101, fx), fy),
                        Lerp(Lerp(v0011, v1011, fx), Lerp(v0111, v1111, fx), fy),
                        fz);

        return Lerp(w0, w1, fw);
    }

    // -------------------------------------------------------
    //  4-D Perlin (gradient) noise
    // -------------------------------------------------------

    /// <summary>
    /// Classic 4-D gradient noise.  Output range is roughly [-1, 1];
    /// post-hoc normalisation in BuildNoiseMap remaps to [0, 1].
    /// </summary>
    private static double PerlinNoise4D(
        double x, double y, double z, double w, int[] perm)
    {
        int ix = (int)Math.Floor(x);
        int iy = (int)Math.Floor(y);
        int iz = (int)Math.Floor(z);
        int iw = (int)Math.Floor(w);

        double fx = x - ix;
        double fy = y - iy;
        double fz = z - iz;
        double fw = w - iw;

        // Quintic Hermite fades for the lerp weights
        double u = Fade(fx);
        double v = Fade(fy);
        double s = Fade(fz);
        double tt = Fade(fw);

        // 16 corner gradients (one per vertex of the 4-D unit cell)
        int g0000 = HashCell4D(perm, ix, iy, iz, iw) & 31;
        int g1000 = HashCell4D(perm, ix + 1, iy, iz, iw) & 31;
        int g0100 = HashCell4D(perm, ix, iy + 1, iz, iw) & 31;
        int g1100 = HashCell4D(perm, ix + 1, iy + 1, iz, iw) & 31;
        int g0010 = HashCell4D(perm, ix, iy, iz + 1, iw) & 31;
        int g1010 = HashCell4D(perm, ix + 1, iy, iz + 1, iw) & 31;
        int g0110 = HashCell4D(perm, ix, iy + 1, iz + 1, iw) & 31;
        int g1110 = HashCell4D(perm, ix + 1, iy + 1, iz + 1, iw) & 31;
        int g0001 = HashCell4D(perm, ix, iy, iz, iw + 1) & 31;
        int g1001 = HashCell4D(perm, ix + 1, iy, iz, iw + 1) & 31;
        int g0101 = HashCell4D(perm, ix, iy + 1, iz, iw + 1) & 31;
        int g1101 = HashCell4D(perm, ix + 1, iy + 1, iz, iw + 1) & 31;
        int g0011 = HashCell4D(perm, ix, iy, iz + 1, iw + 1) & 31;
        int g1011 = HashCell4D(perm, ix + 1, iy, iz + 1, iw + 1) & 31;
        int g0111 = HashCell4D(perm, ix, iy + 1, iz + 1, iw + 1) & 31;
        int g1111 = HashCell4D(perm, ix + 1, iy + 1, iz + 1, iw + 1) & 31;

        // Dot products: gradient · vector-from-corner-to-point
        double n0000 = Dot4(Grad4[g0000], fx, fy, fz, fw);
        double n1000 = Dot4(Grad4[g1000], fx - 1.0, fy, fz, fw);
        double n0100 = Dot4(Grad4[g0100], fx, fy - 1.0, fz, fw);
        double n1100 = Dot4(Grad4[g1100], fx - 1.0, fy - 1.0, fz, fw);
        double n0010 = Dot4(Grad4[g0010], fx, fy, fz - 1.0, fw);
        double n1010 = Dot4(Grad4[g1010], fx - 1.0, fy, fz - 1.0, fw);
        double n0110 = Dot4(Grad4[g0110], fx, fy - 1.0, fz - 1.0, fw);
        double n1110 = Dot4(Grad4[g1110], fx - 1.0, fy - 1.0, fz - 1.0, fw);
        double n0001 = Dot4(Grad4[g0001], fx, fy, fz, fw - 1.0);
        double n1001 = Dot4(Grad4[g1001], fx - 1.0, fy, fz, fw - 1.0);
        double n0101 = Dot4(Grad4[g0101], fx, fy - 1.0, fz, fw - 1.0);
        double n1101 = Dot4(Grad4[g1101], fx - 1.0, fy - 1.0, fz, fw - 1.0);
        double n0011 = Dot4(Grad4[g0011], fx, fy, fz - 1.0, fw - 1.0);
        double n1011 = Dot4(Grad4[g1011], fx - 1.0, fy, fz - 1.0, fw - 1.0);
        double n0111 = Dot4(Grad4[g0111], fx, fy - 1.0, fz - 1.0, fw - 1.0);
        double n1111 = Dot4(Grad4[g1111], fx - 1.0, fy - 1.0, fz - 1.0, fw - 1.0);

        // Quadrilinear interpolation through the 4-D hypercube
        double w0 = Lerp(
                        Lerp(Lerp(n0000, n1000, u), Lerp(n0100, n1100, u), v),
                        Lerp(Lerp(n0010, n1010, u), Lerp(n0110, n1110, u), v),
                        s);
        double w1 = Lerp(
                        Lerp(Lerp(n0001, n1001, u), Lerp(n0101, n1101, u), v),
                        Lerp(Lerp(n0011, n1011, u), Lerp(n0111, n1111, u), v),
                        s);

        return Lerp(w0, w1, tt);
    }

    // -------------------------------------------------------
    //  4-D Simplex noise
    // -------------------------------------------------------

    /// <summary>
    /// 4-D Simplex noise using the rank-based simplex-selection method
    /// (no 64-entry simplex lookup table needed).  Output range is
    /// roughly [-1, 1]; post-hoc normalisation in BuildNoiseMap remaps
    /// to [0, 1].
    ///
    /// The 4-simplex (pentachoron) has 5 corners.  We walk through
    /// them in an order determined by ranking the relative coordinates,
    /// summing a falloff-weighted dot-product contribution from each.
    /// </summary>
    private static double SimplexNoise4D(
        double x, double y, double z, double w, int[] perm)
    {
        // Skew the input space to find which simplex cell contains the point
        double sk = (x + y + z + w) * F4;
        int i = (int)Math.Floor(x + sk);
        int j = (int)Math.Floor(y + sk);
        int k = (int)Math.Floor(z + sk);
        int l = (int)Math.Floor(w + sk);

        // Unskew the cell origin back to (x, y, z, w) space
        double t = (i + j + k + l) * G4;
        double X0 = i - t;
        double Y0 = j - t;
        double Z0 = k - t;
        double W0 = l - t;

        // Relative coordinates inside the simplex (from corner 0)
        double x0 = x - X0;
        double y0 = y - Y0;
        double z0 = z - Z0;
        double w0 = w - W0;

        // Rank the relative coordinates to determine traversal order.
        // Each rank ends up in [0, 3]; the largest gets rank 3.
        int rankx = 0, ranky = 0, rankz = 0, rankw = 0;
        if (x0 > y0) rankx++; else ranky++;
        if (x0 > z0) rankx++; else rankz++;
        if (x0 > w0) rankx++; else rankw++;
        if (y0 > z0) ranky++; else rankz++;
        if (y0 > w0) ranky++; else rankw++;
        if (z0 > w0) rankz++; else rankw++;

        // Corner 1 advances along the largest-rank axis (rank == 3)
        int i1 = rankx >= 3 ? 1 : 0;
        int j1 = ranky >= 3 ? 1 : 0;
        int k1 = rankz >= 3 ? 1 : 0;
        int l1 = rankw >= 3 ? 1 : 0;

        // Corner 2 advances along the two largest-rank axes (rank >= 2)
        int i2 = rankx >= 2 ? 1 : 0;
        int j2 = ranky >= 2 ? 1 : 0;
        int k2 = rankz >= 2 ? 1 : 0;
        int l2 = rankw >= 2 ? 1 : 0;

        // Corner 3 advances along the three largest-rank axes (rank >= 1)
        int i3 = rankx >= 1 ? 1 : 0;
        int j3 = ranky >= 1 ? 1 : 0;
        int k3 = rankz >= 1 ? 1 : 0;
        int l3 = rankw >= 1 ? 1 : 0;

        // Offsets to corners 1-4 in (unskewed) input space
        double x1 = x0 - i1 + G4;
        double y1 = y0 - j1 + G4;
        double z1 = z0 - k1 + G4;
        double w1c = w0 - l1 + G4;

        double x2 = x0 - i2 + 2.0 * G4;
        double y2 = y0 - j2 + 2.0 * G4;
        double z2 = z0 - k2 + 2.0 * G4;
        double w2 = w0 - l2 + 2.0 * G4;

        double x3 = x0 - i3 + 3.0 * G4;
        double y3 = y0 - j3 + 3.0 * G4;
        double z3 = z0 - k3 + 3.0 * G4;
        double w3 = w0 - l3 + 3.0 * G4;

        double x4 = x0 - 1.0 + 4.0 * G4;
        double y4 = y0 - 1.0 + 4.0 * G4;
        double z4 = z0 - 1.0 + 4.0 * G4;
        double w4 = w0 - 1.0 + 4.0 * G4;

        // Hashed gradient indices for each corner
        int gi0 = HashCell4D(perm, i, j, k, l) & 31;
        int gi1 = HashCell4D(perm, i + i1, j + j1, k + k1, l + l1) & 31;
        int gi2 = HashCell4D(perm, i + i2, j + j2, k + k2, l + l2) & 31;
        int gi3 = HashCell4D(perm, i + i3, j + j3, k + k3, l + l3) & 31;
        int gi4 = HashCell4D(perm, i + 1, j + 1, k + 1, l + 1) & 31;

        // Contribution from each corner: (max(0, 0.6 - r²))⁴ · dot(g, offset)
        double n0, n1, n2, n3, n4;

        double t0 = 0.6 - x0 * x0 - y0 * y0 - z0 * z0 - w0 * w0;
        n0 = t0 < 0.0 ? 0.0 : (t0 * t0) * (t0 * t0) * Dot4(Grad4[gi0], x0, y0, z0, w0);

        double t1 = 0.6 - x1 * x1 - y1 * y1 - z1 * z1 - w1c * w1c;
        n1 = t1 < 0.0 ? 0.0 : (t1 * t1) * (t1 * t1) * Dot4(Grad4[gi1], x1, y1, z1, w1c);

        double t2 = 0.6 - x2 * x2 - y2 * y2 - z2 * z2 - w2 * w2;
        n2 = t2 < 0.0 ? 0.0 : (t2 * t2) * (t2 * t2) * Dot4(Grad4[gi2], x2, y2, z2, w2);

        double t3 = 0.6 - x3 * x3 - y3 * y3 - z3 * z3 - w3 * w3;
        n3 = t3 < 0.0 ? 0.0 : (t3 * t3) * (t3 * t3) * Dot4(Grad4[gi3], x3, y3, z3, w3);

        double t4 = 0.6 - x4 * x4 - y4 * y4 - z4 * z4 - w4 * w4;
        n4 = t4 < 0.0 ? 0.0 : (t4 * t4) * (t4 * t4) * Dot4(Grad4[gi4], x4, y4, z4, w4);

        // Scale the sum to bring 4-D simplex output close to [-1, 1]
        return 27.0 * (n0 + n1 + n2 + n3 + n4);
    }

    private static double Dot4(int[] g, double x, double y, double z, double w)
    {
        return g[0] * x + g[1] * y + g[2] * z + g[3] * w;
    }

    // -------------------------------------------------------
    //  Hash / permutation utilities
    // -------------------------------------------------------

    private static double H(int[] perm, int x, int y, int z, int w)
    {
        return perm[HashCell4D(perm, x, y, z, w) & 511] / 511.0;
    }

    private static int HashCell4D(int[] perm, int x, int y, int z, int w)
    {
        return perm[(perm[(perm[(x & 255) + perm[y & 255]] + z) & 255] + w) & 255];
    }

    private static int[] BuildPermutation(int seed)
    {
        int[] p = new int[256];
        for (int i = 0; i < 256; i++) p[i] = i;

        Random rng = new(seed);
        for (int i = 255; i > 0; i--)
        {
            int j = rng.Next(i + 1);
            (p[i], p[j]) = (p[j], p[i]);
        }

        int[] perm = new int[512];
        for (int i = 0; i < 512; i++) perm[i] = p[i & 255];
        return perm;
    }

    // -------------------------------------------------------
    //  Step 2 — noise-level contrast
    // -------------------------------------------------------

    private static void ApplyNoiseContrast(double[,] map, int noiseLevel)
    {
        double factor = noiseLevel / 100.0;

        for (int py = 0; py < TileSize; py++)
        {
            for (int px = 0; px < TileSize; px++)
            {
                map[px, py] = 0.5 + (map[px, py] - 0.5) * factor;
            }
        }
    }

    // -------------------------------------------------------
    //  Step 3 — assemble final bitmap (NO folding — ever)
    // -------------------------------------------------------

    private static Bitmap BuildBitmap(
        double[,] noiseMap,
        Color color1,
        Color color2,
        int saturation)
    {
        Bitmap bmp = new(TileSize, TileSize, PixelFormat.Format32bppArgb);

        for (int py = 0; py < TileSize; py++)
        {
            for (int px = 0; px < TileSize; px++)
            {
                double t = noiseMap[px, py];
                Color blended = BlendAndSaturate(color1, color2, t, saturation);
                bmp.SetPixel(px, py, blended);
            }
        }

        return bmp;
    }

    // -------------------------------------------------------
    //  Colour helpers
    // -------------------------------------------------------

    private static Color BlendAndSaturate(Color c1, Color c2, double t, int saturation)
    {
        double r = c1.R + (c2.R - c1.R) * t;
        double g = c1.G + (c2.G - c1.G) * t;
        double b = c1.B + (c2.B - c1.B) * t;

        Color blended = Color.FromArgb(Round(r), Round(g), Round(b));

        if (saturation < 100)
        {
            blended = AdjustSaturation(blended, saturation / 100.0);
        }

        return blended;
    }

    private static Color AdjustSaturation(Color color, double factor)
    {
        RgbToHsl(
            color.R / 255.0,
            color.G / 255.0,
            color.B / 255.0,
            out double h,
            out double s,
            out double l);

        s = Math.Clamp(s * factor, 0.0, 1.0);
        HslToRgb(h, s, l, out double ro, out double go, out double bo);

        return Color.FromArgb(
            Round(ro * 255.0),
            Round(go * 255.0),
            Round(bo * 255.0));
    }

    // -------------------------------------------------------
    //  Math helpers
    // -------------------------------------------------------

    private static double Fade(double t)
    {
        return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
    }

    private static double Lerp(double a, double b, double t)
    {
        return a + t * (b - a);
    }

    private static void RgbToHsl(
        double r, double g, double b,
        out double h, out double s, out double l)
    {
        double cMin = Math.Min(r, Math.Min(g, b));
        double cMax = Math.Max(r, Math.Max(g, b));
        double delta = cMax - cMin;

        l = (cMax + cMin) / 2.0;

        if (delta < 1e-10) { h = 0; s = 0; return; }

        s = delta / (1.0 - Math.Abs(2.0 * l - 1.0));

        if (Math.Abs(cMax - r) < 1e-10) h = (g - b) / delta % 6.0;
        else if (Math.Abs(cMax - g) < 1e-10) h = (b - r) / delta + 2.0;
        else h = (r - g) / delta + 4.0;

        h /= 6.0;
        if (h < 0.0) h += 1.0;
    }

    private static void HslToRgb(
        double h, double s, double l,
        out double r, out double g, out double b)
    {
        if (s < 1e-10) { r = g = b = l; return; }

        double c = (1.0 - Math.Abs(2.0 * l - 1.0)) * s;
        double x = c * (1.0 - Math.Abs(h * 6.0 % 2.0 - 1.0));
        double m = l - c / 2.0;

        double r1, g1, b1;
        switch ((int)(h * 6.0) % 6)
        {
            case 0: r1 = c; g1 = x; b1 = 0; break;
            case 1: r1 = x; g1 = c; b1 = 0; break;
            case 2: r1 = 0; g1 = c; b1 = x; break;
            case 3: r1 = 0; g1 = x; b1 = c; break;
            case 4: r1 = x; g1 = 0; b1 = c; break;
            default: r1 = c; g1 = 0; b1 = x; break;
        }

        r = Math.Clamp(r1 + m, 0.0, 1.0);
        g = Math.Clamp(g1 + m, 0.0, 1.0);
        b = Math.Clamp(b1 + m, 0.0, 1.0);
    }

    private static int Round(double v)
    {
        return Math.Clamp((int)Math.Round(v), 0, 255);
    }
}
