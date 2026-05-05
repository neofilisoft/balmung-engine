using System;

namespace Balmung.Gameplay;

[Serializable]
public sealed class BalmungItem
{
    public string Name { get; set; } = "";
    public int Value { get; set; }
    public string Type { get; set; } = "generic";
}
