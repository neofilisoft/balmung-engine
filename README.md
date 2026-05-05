# Balmung Engine 2.4

Balmung is a hybrid general-purpose game engine and tooling workspace aimed at `2D`, `HD-2D`, `3D`, RPG, and open-world production, with terrain tooling as one subsystem rather than the whole identity of the engine. the engine owns the game loop, runtime orchestration, and rendering path, while still shipping with a visual editor for content, scene, and asset workflows.

Executable roles:
- `balmung.exe` is the main Balmung shell/runtime entry point and project-management surface.
- `bal-editor.exe` opens the visual editor directly.

Current stack:
- `C++` native runtime modules for scene, runtime, material, UI, terrain, and physics integration
- `C#` Avalonia editor in `editor/`
- `Lua` runtime scripting / modding in `mods/`
- `Python` automation and content tooling in `tools/`

Copyright:
- `© Neofilisoft`

Current direction:
- `Tier A` lightweight 2D production workflow
- `Tier B` unified 3D + HD-2D / 2.5D workflow
- `Tier C` terrain/world tooling for chunked terrain, streaming, lighting, and editor-assisted worldbuilding

## File Formats

Balmung now uses these engine-facing formats:
- `.balmung` for project manifests
- `.bmsc` for scene files
- `.balpack` for processed asset-pack / export-bundle manifests

Legacy `.json` scene files are still readable in the editor as a fallback during migration.

## Editor Vocabulary

Balmung intentionally uses its own editor language while keeping ECS + OOP workflows readable:

| Engine | Runtime Object | Reusable Object | Editor Tree |
| --- | --- | --- | --- |
| Unity | Game Object | Prefab | Hierarchy |
| Unreal | Actor | Blueprint | World Outliner |
| Godot | Node | Scene | Scene Tree |
| Flax | Actor | Prefab | Scene Tree |
| Balmung | Entity | Prefab | Outliner |

In Balmung Editor, an object in the world is called an `Entity`, whether it is backed by ECS data, high-level C# logic, or Lua gameplay logic.

## Build Summary

What builds cleanly in this checkout today:
- `editor` as a Windows desktop executable
- `libbalmung.dll` native runtime / bridge library
- `balmung_native_framework` and `balmung_voxel_framework`
- `balmung_core`, `balmung_engine`, and `balmung_game`
- production-hardened fallback C API/runtime with persistent world saves, mod metadata scanning, block registry, crafting, and editor-safe stats/events

What is not production-complete yet:
- Full AAA renderer features such as a shipping Visibility Buffer backend, hardware virtualized geometry, production GI, and volumetric weather are architectural stubs / roadmap items, not complete Highend-class implementations yet.

## Quick Start

Build the editor:

```powershell
dotnet build editor\editor.csproj -c Debug
```

Expected debug output:
- `editor\bin\Debug\net10.0\bal-editor.exe`
- `editor\bin\Debug\net10.0\balmung.exe`

Build the native runtime DLL:

```powershell
cmake -S . -B build-2.4.0 -G Ninja `
  -DBM_BUILD_RENDERER=OFF `
  -DBM_BUILD_PYTHON=OFF `
  -DBM_BUILD_TESTS=OFF `
  -DBM_BUILD_GAME=OFF

cmake --build build-2.4.0 --target balmung
```

Build the production-oriented native game/runtime path:

```powershell
cmake -S . -B build-production -G Ninja `
  -DBM_BUILD_RENDERER=ON `
  -DBM_BUILD_GAME=ON `
  -DBM_BUILD_PYTHON=OFF `
  -DBM_BUILD_TESTS=OFF

cmake --build build-production --target balmung_game balmung
```

Publish a reusable Windows editor executable:

```powershell
dotnet publish editor\editor.csproj `
  -c Release `
  -r win-x64 `
  --self-contained false `
  -o .\release\balmung
```

Expected publish output:
- `release\balmung\bal-editor.exe`
- `release\balmung\balmung.exe`

Detailed requirements, dependencies, and build paths are documented in `docs/HOW_TO_BUILD.md`.

## Project Structure

```text
Balmung/
|- CMakeLists.txt
|- Balmung.sln
|- README.md
|- thirdparty/
|  |- physics/
|- include/
|  |- api/
|  |- ecs/
|  |- material/
|  |- physics/
|  |- renderer/
|  |- runtime/
|  |- scene/
|  |- scripting/
|  `- terrain/
|- src/
|  |- api/
|  |- material/
|  |- physics/
|  |- renderer/
|  |- runtime/
|  |- scene/
|  |- scripting/
|  `- voxel/
|- bridge/
|- editor/
|- mods/
|- tools/
`- docs/
```

Note:
- `include/terrain` is the public include path for terrain headers
- `src/voxel` still contains the terrain implementation sources and has not been fully renamed yet

## Production Core Status

Balmung now has a buildable C++20 core path for:
- `EntityManager` over the ECS registry
- fixed-step `GameLoop`
- `balmung_engine` runtime composition
- `balmung_game` standalone executable
- C API and .NET 10 bridge calls for entity creation, entity transforms, Visibility Buffer policy, and automatic LOD policy

The renderer defaults its policy to Visibility Buffer + PBR/GI/automatic LOD, but the current OpenGL renderer remains a functional scaffold. The next large renderer milestone is a real GPU Visibility Buffer path with material resolve, shadow atlas, probe-grid GI, and streaming LOD assets.

## Beyond Games

Balmung 2 is a graphic engine + editor platform, so it is not limited to games. Good target uses include:
- real-time cinematic previs, virtual production blocking, and interactive storyboards
- architectural / urban planning visualization with traffic, crowd, weather, and day-night simulation
- education and simulation labs where scenarios need visual scripting and hot iteration
- digital twins for factories, buildings, campuses, and cities
- RPG/open-world narrative tools for quest graphs, dialogue staging, and cinematic triggers
- medical, robotics, and industrial training prototypes where C# tooling plus native simulation is useful
- UI-heavy interactive installations and control-room style visual apps

The strongest fit is still interactive worlds: 2D tools are supported, but Balmung is being shaped especially around RPG, open-world, streaming terrain, population simulation, and editor-driven cinematic workflows.

## Terrain Subsystem

The terrain subsystem is still explicitly voxel/chunk-oriented internally. Public include paths now live under `include/terrain`, but many types still use voxel-specific naming such as:
- `BlockId`
- `ChunkCoord`
- `TerrainGenerator`
- `ChunkManager`

So Balmung is no longer positioned as a voxel-only engine, but this subsystem is still clearly a terrain/voxel toolkit rather than a neutral world framework.

## Physics

Balmung integrates `AsterCorePhysics` through the physics backend abstraction. Set `BM_ASTERCORE_ROOT` as a CMake cache value or environment variable to point at an external AsterCore checkout; otherwise Balmung falls back to `thirdparty/physics/AsterCorePhysics-3.1`.

## Scripting And Tools

Lua examples live in `mods/`.

Python tools include:
- `balmung_tool.py`
- `bm_assets.py`
- `bm_scene.py`
- `bm_export.py`

Examples:

```powershell
python tools\balmung_tool.py info
python tools\balmung_tool.py validate-layout
python tools\bm_scene.py demo_scene --width 32 --height 32 --block stone
python tools\bm_assets.py --dry-run
python tools\bm_export.py Build1
```

## Docs

Available docs:
- `docs/HOW_TO_BUILD.md`
- `docs/API_CSharp_Quickstart_EN.md`
- `docs/API_CSharp_Quickstart_TH.md`
- `docs/EDITOR_WORKFLOW_EN.md`
- `docs/EDITOR_WORKFLOW_TH.md`

## Troubleshooting

If CMake configures slowly or inconsistently on Windows, prefer Ninja or Visual Studio generators over ad-hoc MSYS builds.

If `balmung_game` does not appear, configure with `-DBM_BUILD_RENDERER=ON -DBM_BUILD_GAME=ON` and build the `balmung_game` target.
[Dependency (dependency.txt)](./docs/dependency.txt)

