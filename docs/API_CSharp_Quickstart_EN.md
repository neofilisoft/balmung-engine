# Balmung API (C#) Quickstart

This guide explains how to use `Balmung.Bridge` from C# without requiring C++ knowledge.

## Overview
- `Balmung.Bridge.Native`
  - Low-level P/Invoke bindings (direct native DLL calls)
- `BalmungEngine`
  - High-level wrapper (recommended for most users)

## Getting Started
Basic example:

```csharp
using Balmung.Bridge;

using var engine = new BalmungEngine();
if (!engine.InitHeadless(1280, 720))
{
    Console.WriteLine("Engine init failed");
    return;
}

engine.AddItem("stone", 64);
engine.PlaceVoxel(0, 20, 0, "stone");
engine.Tick(1f / 60f);

Console.WriteLine($"Seed: {engine.WorldSeed}");
Console.WriteLine($"Chunks: {engine.ChunkCount}");

engine.SaveWorld("world1");
```

## Common Methods
- `InitHeadless(width, height)`
  - Use for editor/tools without a native game window
- `InitWindowed(width, height, title)`
  - Use for standalone game runtime
- `Tick(dt)`
  - Updates the engine for one frame
- `PlaceVoxel(x, y, z, "block")`
  - Places a block
- `DestroyVoxel(x, y, z)`
  - Removes a block
- `SaveWorld(name)` / `LoadWorld(name)`
  - Save/load world data
- `LoadMods("mods")`
  - Loads Lua mods
- `ExecLua("print('hi')")`
  - Runs a Lua snippet
- `GetAllBlocks()`
  - Returns block registry entries for UI/palette usage
- `GetStats()`
  - Returns render stats (`DrawCalls`, `Triangles`, `FrameMs`)

## Events (C#)
You can subscribe to engine events:

```csharp
engine.OnEngineEvent += (name, json) =>
{
    Console.WriteLine($"{name}: {json}");
};
```

Typical events:
- `on_place`
- `on_destroy`
- `on_craft`

## Runtime Notes / Pitfalls
- The native runtime (`balmung.dll` or `libbalmung.dll`) must be discoverable by your app.
- If `InitHeadless()` returns `false`, stop using that instance.
- Always call `Dispose()` (or use `using`).
- Do not call the native API from multiple threads at the same time (not designed as re-entrant yet).

## Using It Through the Editor (No C++ Required)
`Balmung.Editor` already uses this API for:
- Block placement/removal in the Scene Builder
- World + scene save/load
- Lua mod loading
- Render statistics

You can extend the editor in C# first, without touching C++:
- New editor tools/buttons
- Import/export pipeline features
- Scene tools (fill/line/brush)





