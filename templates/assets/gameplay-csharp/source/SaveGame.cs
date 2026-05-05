using System;
using System.Collections.Generic;

namespace Balmung.Gameplay;

[Serializable]
public sealed class BalmungSaveData
{
    public int Level { get; set; }
    public int Experience { get; set; }
    public float PlayerX { get; set; }
    public float PlayerY { get; set; }
    public float PlayerZ { get; set; }
    public List<BalmungItem> Inventory { get; set; } = [];
}

public sealed class BalmungSaveGameService
{
    public BalmungSaveData CreateSave(BalmungPlayerStats playerStats, float playerX, float playerY, float playerZ)
        => new()
        {
            Level = playerStats.Level,
            Experience = playerStats.Experience,
            PlayerX = playerX,
            PlayerY = playerY,
            PlayerZ = playerZ,
        };

    public void ApplySave(BalmungSaveData data, BalmungPlayerStats playerStats)
    {
        playerStats.Level = data.Level;
        playerStats.Experience = data.Experience;
    }
}
