# Balmung Engine 2.2.1

Balmung is a hybrid game engine/tooling project with:

Version target: 2.2.1
Focus: build real games across 2D, 2.5D (HD-2D), and 3D from one evolving engine/toolchain.

- `C++` native engine modules (renderer/runtime/physics/material/scene scaffolds)
- `C#` Avalonia editor (`Balmung.Editor`)
- `Lua` modding scripts (`mods/`)
- `Python` tooling scripts (`tools/`)

Current direction:
- `Tier A` Lightweight 2D pipeline for Terraria-scale, modern pixel-art, stylized side-scrolling, and hybrid 2D/2.5D action workflows
- `Tier B` Unified pipeline for 3D + HD-2D / 2.5D
- `Tier C` Reusable voxel/world framework for Overworld-style terrain, chunk streaming, lighting, and tool-driven game creation

## Build (Windows)

### Requirements
- `.NET SDK 8`
- `CMake >= 3.22`
- `Visual Studio 2022 Build Tools` (Desktop development with C++)
- `Ninja` (optional)

Notes:
- Prefer `Visual Studio 2022` generator for CMake on Windows.
- `MSYS/MinGW + CMake` may be slow or hang during configure on some machines.

## Build Editor EXE (Recommended)

Debug build:

```powershell
dotnet build Balmung.Editor\Balmung.Editor.csproj -c Debug
```

Publish Release EXE:

```powershell
dotnet publish Balmung.Editor\Balmung.Editor.csproj `
  -c Release `
  -r win-x64 `
  --self-contained false
```

Output:
- `Balmung.Editor\bin\Release\net8.0\win-x64\publish\Balmung.Editor.exe`

## Build Native C++ Targets

Configure (MSVC / Visual Studio generator):

```powershell
cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64 `
  -DBM_BUILD_RENDERER=OFF `
  -DBM_BUILD_CAPI=OFF `
  -DBM_BUILD_PYTHON=OFF `
  -DBM_BUILD_TESTS=OFF `
  -DBM_BUILD_GAME=OFF
```

Build the native scaffold framework target:

```powershell
cmake --build build-msvc --config Release --target balmung_native_framework
```

Notes:
- `balmung_native_framework` is a native C++ target for validating new modules (`physics/material/scene/runtime`) without requiring a separate engine checkout.
- Some targets intentionally `skip` in CMake if required files are missing (placeholder targets are used to keep configure/build behavior explicit).

## Project Structure

```text
Balmung/
├─ CMakeLists.txt
├─ Balmung.sln
├─ README.md
├─ .gitattributes
│
├─ thirdparty/                        # External SDK/backend drop-ins (e.g. physics)
│  ├─ README.md
│  └─ physics/
│     └─ README.md
│
├─ include/                           # Native public headers (C++)
│  ├─ engine.hpp
│  ├─ api/
│  ├─ ecs/
│  ├─ material/                       # Material / PBR-ready scaffolds
│  ├─ physics/                        # Physics abstraction + null backend
│  ├─ renderer/
│  ├─ runtime/                        # 2D/3D/post/ECS runtime pipeline scaffolds
│  ├─ scene/                          # Scene document / serializer scaffolds
│  └─ scripting/
│
├─ src/                               # Native sources (C++)
│  ├─ engine.cpp
│  ├─ api/
│  ├─ material/
│  ├─ physics/
│  │  └─ backends/
│  ├─ renderer/
│  ├─ runtime/
│  ├─ scene/
│  └─ scripting/
│
├─ bindings/
│  └─ python/
│     └─ balmung_py.cpp            # C++ binding layer for Python (pybind11)
│
├─ Balmung.Bridge/                 # C# <-> native bridge (P/Invoke)
│  ├─ Balmung.Bridge.csproj
│  ├─ BalmungNative.cs
│  └─ BalmungEngine.cs
│
├─ Balmung.Editor/                 # Avalonia Editor
│  ├─ Balmung.Editor.csproj
│  ├─ Program.cs
│  ├─ App.axaml
│  ├─ App.axaml.cs
│  ├─ MainWindow.axaml
│  ├─ MainWindow.axaml.cs
│  ├─ Controls.cs
│  ├─ SceneEditing.cs
│  └─ ProjectPipelines.cs
│
├─ mods/                              # Lua mods / examples
│  ├─ mystuff.lua
│  └─ examples/
│     ├─ init.lua
│     ├─ rpg_events.lua
│     ├─ hd2d_lighting.lua
│     └─ pbr_lite_2d.lua
│
├─ tools/                             # Python tooling / automation
│  ├─ README.md
│  ├─ balmung_tool.py
│  ├─ bm_common.py
│  ├─ bm_assets.py
│  ├─ bm_scene.py
│  └─ bm_export.py
│
├─ experiments/                       # Non-production / legacy prototypes
│  ├─ README.md
│  └─ avalonia/
│     └─ OpenGlViewport.cs
│
└─ docs/
   ├─ API_CSharp_Quickstart_TH.md
   ├─ API_CSharp_Quickstart_EN.md
   ├─ EDITOR_WORKFLOW_TH.md
   ├─ EDITOR_WORKFLOW_EN.md
   ├─ PIPELINE_TIERS_MATRIX_TH.md
   └─ PIPELINE_TIERS_MATRIX_EN.md
```

## Lua Mods

Drop `.lua` files into `mods/` (or `mods/examples/` as references).

Examples included:
- RPG event hooks
- HD-2D lighting preset helpers
- PBR-lite 2D lit-sprite material registration examples

Notes:
- Lua in this repo is intended for runtime scripting / modding.
- It is **not** the old Python `lupa` flow.

## Python Tools

`tools/` contains lightweight Python scripts for automation and content workflows.

Examples:

```powershell
python tools\balmung_tool.py info
python tools\balmung_tool.py validate-layout
python tools\bm_scene.py demo_scene --width 32 --height 32 --block stone
python tools\\bm_assets.py --dry-run
python tools\\bm_export.py Build1
```

## Docs

See `docs/` for:
- C# API quickstart (TH/EN)
- Editor workflow (TH/EN)
- Tier A / Tier B pipeline architecture matrix (TH/EN)

## Troubleshooting

### CMake configure is very slow or hangs (Windows/MSYS)

Use Visual Studio generator instead of MSYS/MinGW:

```powershell
cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64
```

### NuGet warning `NU1900`

This is usually a vulnerability-feed/network issue and does not necessarily block `dotnet build`.


## Voxel Framework

New native module: `balmung_voxel_framework`

Included systems:
- Noise-based terrain generation with caves, shoreline handling, and ore/emissive placement
- Chunk storage using nibble-packed block light / sky light data
- Flood-fill voxel lighting
- Chunk visibility meshing with 32-bit packed vertices designed for Vulkan / DirectX style input layouts
- Multithreaded chunk generation and load / unload management

This module is intended to power `Overworld` while keeping `Balmung` usable as a reusable engine/framework.


## Overworld Integration

`Overworld` is a game project built with `Balmung Engine 2.2.1`.

Short answer for external questions:
- `Overworld uses Balmung Engine`
- `Balmung` is a custom engine / framework currently evolving toward a fuller game engine

Tooling command:

```powershell
python tools\\balmung_tool.py scan-overworld-pack --pack-root C:\\Users\\BEST\\Desktop\\Repo\\Overworld
python tools\\balmung_tool.py scan-source-pack --pack-root C:\\Users\\BEST\\Desktop\\Overworlder --project-name Overworlder
` 

This generates manifests for the Overworld game repo and for external game-owned asset sources such as Overworlder, so the engine/editor/tooling side can reason about textures, behavior packs, blockstates, shaders, and other imported resources without claiming ownership of those assets.










