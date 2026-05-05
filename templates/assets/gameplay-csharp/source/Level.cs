using System;

namespace Balmung.Gameplay;

[Serializable]
public sealed class BalmungPlayerStats
{
    public int Level { get; set; } = 1;
    public int Experience { get; set; }
    public int MaxExperience { get; set; } = 100;
    public int Health { get; set; } = 100;
    public int Attack { get; set; } = 10;
}
