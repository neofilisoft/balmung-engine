# VoxelBlock Engine v0.4

A Minecraft-inspired voxel engine built with [Ursina](https://www.ursinaengine.org/).

## Quick Start

```bash
pip install -r requirements.txt
python main.py
```

## Controls

| Key | Action |
|-----|--------|
| WASD | Move |
| Mouse | Look |
| Left Click | Place block |
| Right Click | Remove block |
| 1–6 | Select block type |
| Scroll Wheel | Cycle block type |
| E | Open / close inventory |
| Escape | Close inventory / quit |

## Block Types

| # | Block |
|---|-------|
| 1 | Grass |
| 2 | Dirt |
| 3 | Stone |
| 4 | Wood |
| 5 | Leaves |
| 6 | Sand |

## Project Structure

```
VoxelBlock-Engine/
├── main.py              # Entry point
├── requirements.txt
├── README.md
├── assets/
│   └── textures/        # Drop custom PNGs here
├── mods/                # Lua mods go here
│   └── example.lua
└── core/
    ├── __init__.py
    ├── voxel.py         # Voxel block class + color registry
    ├── world.py         # Perlin terrain + chunk generation
    ├── player.py        # First-person controller + input
    ├── inventory.py     # Hotbar & inventory UI
    └── mod_loader.py    # Lua modding system
```

## Adding Mods

Drop any `.lua` file into the `mods/` folder. The following API is exposed to Lua:

```lua
print("hello from lua")
place_block(x, y, z, "stone")   -- place a block at world coords
```

Requires `lupa` (`pip install lupa`). If not installed, mods are silently skipped.

## Adding Textures

Place `.png` files in `assets/textures/`. To use a custom texture on a block,
update `BLOCK_COLORS` in `core/voxel.py` and pass the texture name to `Voxel`.
