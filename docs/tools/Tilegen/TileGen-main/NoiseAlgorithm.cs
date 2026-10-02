namespace EntitledLogicTileGen;

/// <summary>
/// Identifies the procedural noise algorithm used when generating a tile.
/// </summary>
public enum NoiseAlgorithm
{
    Perlin                = 0,
    Simplex               = 1,
    Value                 = 2,
    FractalBrownianMotion = 3,
    Worley                = 4,
    Turbulence            = 5
}
