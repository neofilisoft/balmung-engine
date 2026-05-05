# scene/2d
Owns production-facing 2D runtime features for pixel games, side-scrollers, farming RPGs, and HD-2D composition.

Current supported surface:
- `Sprite2DComponent` in ECS for texture/material/layer ordered sprites.
- `Tilemap2DComponent` in ECS for chunked tilemap batches.
- `RenderScene2D` static tile batches and dynamic sprite batches.
- Asset staging for `.png`, `.tmj`, `.tsx`, `.aseprite`, `.json`, `.wav`, and C# gameplay scripts.
- Export handoff to desktop platforms and WebAssembly artifacts.


