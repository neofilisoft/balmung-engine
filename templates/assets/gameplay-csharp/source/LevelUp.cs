using System;

namespace Balmung.Gameplay;

public sealed class BalmungLevelSystem
{
    public void GainExperience(BalmungPlayerStats playerStats, int amount)
    {
        playerStats.Experience += amount;
        while (playerStats.Experience >= playerStats.MaxExperience)
        {
            playerStats.Experience -= playerStats.MaxExperience;
            playerStats.Level++;
            playerStats.MaxExperience = Math.Max(playerStats.MaxExperience + 1, (int)(playerStats.MaxExperience * 1.5f));
            playerStats.Attack += 2;
            playerStats.Health += 10;
        }
    }
}
