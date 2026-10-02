using System.Collections.Generic;
using System.Drawing;

namespace EntitledLogicTileGen;

/// <summary>
/// An ordered set of colour stops used to map a normalised noise value
/// (0…1) onto a colour.  Replaces the fixed two-colour blend in the tile
/// generator: two stops at 0 and 1 reproduce the old behaviour exactly,
/// while extra stops let you build terrain ramps (e.g. water → sand →
/// grass → rock → snow) so output reads as terrain rather than a height
/// field.
///
/// Set <see cref="Stepped"/> to true for a hard "indexed palette" look:
/// instead of interpolating between stops, each value snaps to the nearest
/// lower stop's colour, giving flat colour bands.
/// </summary>
public sealed class ColorRamp
{
    public sealed class Stop
    {
        public double Position;   // 0…1
        public Color Color;

        public Stop(double position, Color color)
        {
            Position = position;
            Color = color;
        }
    }

    private readonly List<Stop> _stops = new();

    /// <summary>When true, colours snap to bands instead of interpolating.</summary>
    public bool Stepped { get; set; }

    public ColorRamp()
    {
    }

    public ColorRamp(IEnumerable<Stop> stops, bool stepped = false)
    {
        _stops.AddRange(stops);
        Stepped = stepped;
        Sort();
    }

    /// <summary>Read-only, position-sorted view of the stops.</summary>
    public IReadOnlyList<Stop> Stops => _stops;

    public int Count => _stops.Count;

    public void Add(double position, Color color)
    {
        _stops.Add(new Stop(Math.Clamp(position, 0.0, 1.0), color));
        Sort();
    }

    public void RemoveAt(int index)
    {
        if (index >= 0 && index < _stops.Count) _stops.RemoveAt(index);
    }

    public void Clear() => _stops.Clear();

    public void Sort() => _stops.Sort((a, b) => a.Position.CompareTo(b.Position));

    /// <summary>
    /// Maps a value in [0, 1] to a colour.  Below the first stop returns the
    /// first stop's colour; above the last returns the last's.  Between two
    /// stops it interpolates each RGB channel linearly (or snaps to the lower
    /// stop when <see cref="Stepped"/> is set).
    /// </summary>
    public Color Evaluate(double t)
    {
        if (_stops.Count == 0) return Color.Black;
        if (_stops.Count == 1) return _stops[0].Color;

        t = Math.Clamp(t, 0.0, 1.0);

        if (t <= _stops[0].Position) return _stops[0].Color;
        if (t >= _stops[^1].Position) return _stops[^1].Color;

        for (int i = 0; i < _stops.Count - 1; i++)
        {
            Stop lo = _stops[i];
            Stop hi = _stops[i + 1];
            if (t < lo.Position || t > hi.Position) continue;

            if (Stepped) return lo.Color;

            double span = hi.Position - lo.Position;
            double f = span < 1e-9 ? 0.0 : (t - lo.Position) / span;

            int r = (int)Math.Round(lo.Color.R + (hi.Color.R - lo.Color.R) * f);
            int g = (int)Math.Round(lo.Color.G + (hi.Color.G - lo.Color.G) * f);
            int b = (int)Math.Round(lo.Color.B + (hi.Color.B - lo.Color.B) * f);
            return Color.FromArgb(
                Math.Clamp(r, 0, 255),
                Math.Clamp(g, 0, 255),
                Math.Clamp(b, 0, 255));
        }

        return _stops[^1].Color;
    }

    public ColorRamp Clone()
    {
        ColorRamp copy = new() { Stepped = Stepped };
        foreach (Stop s in _stops) copy._stops.Add(new Stop(s.Position, s.Color));
        return copy;
    }

    // -------------------------------------------------------
    //  Factory helpers / presets
    // -------------------------------------------------------

    /// <summary>Two-stop ramp equivalent to the classic Color 1 → Color 2 blend.</summary>
    public static ColorRamp FromTwoColors(Color a, Color b)
    {
        return new ColorRamp(new[]
        {
            new Stop(0.0, a),
            new Stop(1.0, b)
        });
    }

    /// <summary>Water → sand → grass → rock → snow.</summary>
    public static ColorRamp DefaultTerrain()
    {
        return new ColorRamp(new[]
        {
            new Stop(0.00, Color.FromArgb( 38,  70, 120)),   // deep water
            new Stop(0.32, Color.FromArgb( 70, 120, 170)),   // shallow water
            new Stop(0.40, Color.FromArgb(196, 178, 128)),   // sand
            new Stop(0.55, Color.FromArgb( 64, 132,  60)),   // grass
            new Stop(0.75, Color.FromArgb(110,  96,  78)),   // rock
            new Stop(1.00, Color.FromArgb(238, 238, 238))    // snow
        });
    }

    public static ColorRamp Grayscale()
    {
        return FromTwoColors(Color.Black, Color.White);
    }
}
