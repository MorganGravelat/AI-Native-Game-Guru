namespace EntitledLogicTileGen.Noise;

/// <summary>
/// Worley (cellular / F1) noise with seamless-tiling support.
///
/// Feature points are placed in a period × period grid.  When checking
/// neighbouring cells near a tile boundary, the points are "ghost-copied"
/// with the appropriate world-space offset, so the distance field wraps
/// continuously and the tile edges match without any visible seam.
/// </summary>
public sealed class WorleyNoise
{
    // -------------------------------------------------------
    //  Constants
    // -------------------------------------------------------

    private const int PointsPerCell = 2;

    // -------------------------------------------------------
    //  Fields
    // -------------------------------------------------------

    // _jitter[cellIndex][pointIndex * 2 + 0] = x jitter in [0, 1)
    // _jitter[cellIndex][pointIndex * 2 + 1] = y jitter in [0, 1)
    private readonly double[][] _jitter;
    private readonly int        _period;   // grid side length

    // -------------------------------------------------------
    //  Constructor
    // -------------------------------------------------------

    /// <summary>
    /// <paramref name="period"/> must match the tiling period passed to
    /// SampleTiling01.  A value of 4 is a good default.
    /// </summary>
    public WorleyNoise(int seed, int period = 4)
    {
        _period = period;
        Random rng = new(seed);

        int cells  = period * period;
        _jitter    = new double[cells][];

        for (int c = 0; c < cells; c++)
        {
            _jitter[c] = new double[PointsPerCell * 2];
            for (int k = 0; k < PointsPerCell; k++)
            {
                _jitter[c][k * 2    ] = rng.NextDouble();
                _jitter[c][k * 2 + 1] = rng.NextDouble();
            }
        }
    }

    // -------------------------------------------------------
    //  Non-tiling sample
    // -------------------------------------------------------

    /// <summary>Returns F1 Worley noise in [0, 1].</summary>
    public double Sample01(double x, double y)
    {
        int    cellX  = (int)Math.Floor(x);
        int    cellY  = (int)Math.Floor(y);
        double minDist = double.MaxValue;

        for (int dy = -1; dy <= 1; dy++)
        {
            for (int dx = -1; dx <= 1; dx++)
            {
                int nx = cellX + dx;
                int ny = cellY + dy;

                int wx = ((nx % _period) + _period) % _period;
                int wy = ((ny % _period) + _period) % _period;

                double[] pts = _jitter[wy * _period + wx];

                for (int k = 0; k < PointsPerCell; k++)
                {
                    // Ghost-copy position in world space
                    double px = nx + pts[k * 2    ];
                    double py = ny + pts[k * 2 + 1];
                    double ddx = x - px;
                    double ddy = y - py;
                    double d   = Math.Sqrt(ddx * ddx + ddy * ddy);
                    if (d < minDist) minDist = d;
                }
            }
        }

        return Math.Clamp(minDist * 1.6, 0.0, 1.0);
    }

    // -------------------------------------------------------
    //  Tiling sample
    // -------------------------------------------------------

    /// <summary>
    /// F1 Worley noise in [0, 1] that tiles seamlessly with the period
    /// that was supplied to the constructor.
    /// </summary>
    public double SampleTiling01(double x, double y, int period)
    {
        int    cellX   = (int)Math.Floor(x);
        int    cellY   = (int)Math.Floor(y);
        double minDist = double.MaxValue;

        for (int dy = -1; dy <= 1; dy++)
        {
            for (int dx = -1; dx <= 1; dx++)
            {
                int nx = cellX + dx;
                int ny = cellY + dy;

                // Wrap to canonical cell
                int wx = ((nx % period) + period) % period;
                int wy = ((ny % period) + period) % period;

                double[] pts = _jitter[wy * period + wx];

                for (int k = 0; k < PointsPerCell; k++)
                {
                    // Ghost-copy: the canonical point shifted into
                    // the neighbour's position in world space
                    double px  = nx + pts[k * 2    ];
                    double py  = ny + pts[k * 2 + 1];
                    double ddx = x - px;
                    double ddy = y - py;
                    double d   = Math.Sqrt(ddx * ddx + ddy * ddy);
                    if (d < minDist) minDist = d;
                }
            }
        }

        return Math.Clamp(minDist * 1.6, 0.0, 1.0);
    }
}
