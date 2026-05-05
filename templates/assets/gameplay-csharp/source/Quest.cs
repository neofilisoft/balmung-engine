using System;
using System.Collections.Generic;

namespace Balmung.Gameplay;

[Serializable]
public sealed class BalmungQuest
{
    public string Name { get; set; } = "";
    public bool IsCompleted { get; set; }
    public List<string> Steps { get; set; } = [];
}
