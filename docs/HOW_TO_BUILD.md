# How To Build Balmung

## Overview

This document covers the practical build paths for the current Balmung checkout.

What works today:
- `editor` desktop app
- `libbalmung.dll` native runtime library
- native framework static libraries such as `balmung_native_framework`
- `balmung_core`, `balmung_engine`, and `balmung_game`
- the fallback runtime path used by the editor, including persistent save/load and declarative mod metadata loading

Runtime model:
- Balmung is intended to behave like an engine runtime, not just an asset viewer. The native/runtime side is expected to own the main loop, scene stepping, and rendering flow in the same broad spirit as `LOVE` or `Bevy`, while `editor` provides the visual workflow on top.
- `balmung.exe` is the main shell/runtime entry point.
- `bal-editor.exe` opens the visual editor directly.

What is still scaffold-level:
- the shipping-grade Visibility Buffer renderer backend
- production GI / volumetric weather
- full asset streaming and automatic LOD cooking

## Dependencies

### Required

- Windows 10/11
- `.NET SDK 10`
- `CMake 3.22+`
- a C++ compiler toolchain

Recommended Windows toolchains:
- `Visual Studio 2022 Build Tools` with Desktop C++ workload
- `Ninja`

Also used in this repo:
- `MSYS2 UCRT64` or compatible MinGW toolchain for the current `build-balmung-gpu2` flow

### Optional

- `Vulkan SDK` if experimenting with Vulkan compile flags
- `Lua` development package for future scripting builds
- `pybind11` if enabling the Python binding

### Bundled Third-Party Code

Bundled in-repo:
- `AsterCorePhysics` at `thirdparty/physics/AsterCorePhysics-3.1`

The current native build pulls this physics code through CMake when present. To use an external checkout without hardcoding a machine path, pass `-DBM_ASTERCORE_ROOT=...` to CMake or set the `BM_ASTERCORE_ROOT` environment variable.

## Build Matrix

### 1. Build Balmung Editor

Debug:

```powershell
dotnet build editor\editor.csproj -c Debug
```

Release publish:

```powershell
dotnet publish editor\editor.csproj `
  -c Release `
  -r win-x64 `
  --self-contained false `
  -o .\release\balmung
```

Expected output:
- `release\balmung\bal-editor.exe`
- `release\balmung\balmung.exe`

### 2. Build Native Runtime DLL

Configure:

```powershell
cmake -S . -B build-2.4.0 -G Ninja `
  -DBM_BUILD_RENDERER=OFF `
  -DBM_BUILD_PYTHON=OFF `
  -DBM_BUILD_TESTS=OFF `
  -DBM_BUILD_GAME=OFF
```

Build:

```powershell
cmake --build build-2.4.0 --target balmung
```

Expected output:
- `build-2.4.0\balmung.dll` or `build-2.4.0\libbalmung.dll`

### 3. Build Native Framework Only

If you only want to validate native scaffolds:

```powershell
cmake --build build-balmung-gpu2 --target balmung_native_framework
```

### 4. Build Full Native Game

Configure:

```powershell
cmake -S . -B build-production -G Ninja `
  -DBM_BUILD_RENDERER=ON `
  -DBM_BUILD_GAME=ON `
  -DBM_BUILD_PYTHON=OFF `
  -DBM_BUILD_TESTS=OFF
```

Build:

```powershell
cmake --build build-production --target balmung_game balmung
```

Expected output:
- `build-production\balmung_game.exe`
- `build-production\balmung.dll` or `build-production\libbalmung.dll`

## File Formats

Balmung currently uses:
- `.balmung` for project manifests
- `.bmsc` for scene files
- `.balpack` for processed asset / export bundle manifests

Legacy `.json` scene files are still readable by the editor as fallback.

## Recommended Workflow

1. Build `balmung.dll` / `libbalmung.dll`
2. Build or publish `editor/`
3. Work in the editor using `Project.balmung`, `Scenes/*.bmsc`, and `Assets/Processed/assets.balpack`

## Troubleshooting

### Editor builds but native runtime is missing

Check that one of these exists:
- `build-balmung-gpu2\libbalmung.dll`
- `build-voxel\libbalmung.dll`

The editor copies native runtime files during build if a matching DLL is found.

### CMake says engine/game target is skipped

That is expected if the repo still lacks `core/*` implementation files.

### Vulkan requested but not detected

If `BM_RENDER_BACKEND=VULKAN` is set, you need a valid Vulkan SDK installation or `VULKAN_SDK` environment variable.

### AsterCore physics integration

The CMake build automatically includes AsterCore when `thirdparty/physics/AsterCorePhysics-3.1` exists.

## Cleanup

Common cleanup targets:
- `build-balmung-gpu2`
- `editor\bin`
- `editor\obj`
- `bridge\bin`
- `bridge\obj`
- temporary publish folders such as `release\balmung`

Be careful not to delete the release folder if you want to keep the published executable.
