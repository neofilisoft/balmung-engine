using System;

namespace Balmung.Gameplay;

public sealed class BalmungDialogueManager
{
    public string[] DialogueLines { get; set; } = [];
    public int CurrentLine { get; private set; }

    public string ShowDialogue(string[] lines)
    {
        DialogueLines = lines;
        if (DialogueLines.Length == 0)
            return string.Empty;

        CurrentLine = Math.Clamp(CurrentLine, 0, DialogueLines.Length - 1);
        return DialogueLines[CurrentLine++];
    }
}
