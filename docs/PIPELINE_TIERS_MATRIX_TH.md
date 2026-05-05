# Balmung Pipeline Tiers Matrix (Tier A / Tier B)

เอกสารนี้สรุปการแบ่งสถาปัตยกรรมสำหรับ Balmung Engine ให้รองรับ:
- เกม 2D ล้วน (pixel art / RPG maker-like)
- เกม HD-2D / 2.5D
- เกม 3D

แนวคิดหลัก:
- `Tier A` = 2D Pipeline แบบ Lightweight
- `Tier B` = Unified Pipeline แบบ Scalable (2D + 3D + HD-2D/2.5D)

## เป้าหมายแต่ละ Tier

### Tier A: 2D Pipeline (Lightweight)
- เหมาะกับเกม 2D ล้วน
- เน้น performance, latency ต่ำ, workflow ทำเกมไว
- ใช้ทรัพยากรต่ำ (อินดี้/เครื่องสเปกกลาง/มือถือ)

### Tier B: Unified Pipeline (Scalable)
- เหมาะกับเกม 3D และ HD-2D/2.5D
- ใช้ render/material/post-processing ร่วมกัน
- ขยาย visual quality ได้มากกว่า (PBR-ready, richer lighting, layered FX)

## Feature Matrix

| หมวด | Tier A: 2D Lightweight | Tier B: Unified Scalable |
|---|---|---|
| เป้าหมายเกม | 2D ล้วน, pixel RPG, platformer | 3D, HD-2D, 2.5D, hybrid |
| Render Mode | 2D renderer เฉพาะทาง | 2D + 3D ใช้ pipeline ร่วมกัน |
| Camera | Orthographic, pixel-perfect | Ortho + Perspective, shared camera framework |
| Batching | Sprite batching (static + dynamic) | Sprite batching + 3D batching/instancing |
| Tilemap | Chunk batching, collision layers | Tier A features + depth-aware layering |
| Lighting | Canvas/2D lights, optional normal map | 2D lights + 3D lights + shared post FX |
| Materials | Unlit / Lit2D (simple) | Material system แบบ scalable, PBR-ready |
| Post-processing | Lightweight stack (bloom-lite, grading) | Shared post stack (bloom, tone map, grading, stylized FX) |
| Physics | 2D/simple collision หรือ backend abstraction | Physics backend abstraction (Jolt/PhysX/etc.) |
| ECS | ใช้ได้เต็มสำหรับ 2D entities | ใช้ร่วมกันทั้ง 2D/3D entities |
| Asset Pipeline | png/wav/fbx staging + 2D atlas/tileset | Tier A + 3D material/mesh workflows |
| Editor | 2D scene builder / tile tools | Unified editor + mode-aware tools |
| Runtime UI | Pixel UI/HUD | Pixel UI + 3D/HD UI variants |
| Export | Standalone package สำหรับ 2D games | Standalone package สำหรับ 2D/3D/HD-2D |
| เหมาะกับทีม | Solo / small indie | Small-medium team / content-heavy projects |

## Renderer Design Guideline (สำคัญ)

### Tier A (2D)
- ใช้ `SpriteBatcher` เป็นแกน
  - `StaticBatch`: tilemap/background/props ที่ไม่ขยับ
  - `DynamicBatch`: actor/VFX/UI dynamic sprites
- Sort by:
  - texture / atlas
  - material
  - blend mode
  - layer / order
- เป้าหมาย:
  - ลด draw calls
  - ลด state changes
  - ลด per-frame allocations

### Tier B (Unified)
- ใช้ `RenderGraph`/pass-based design (หรือโครงเทียบเท่า)
- แยก pass ชัดเจน:
  - 2D opaque/lit
  - 3D opaque
  - transparent
  - UI
  - post-processing
- ใช้ material abstraction เดียวกันเท่าที่เป็นไปได้
- รองรับ HD-2D:
  - 3D environment + 2D sprite characters + shared post FX

## PBR Strategy (3D + HD-2D)

แนะนำให้ Balmung รองรับ `PBR-ready material system` ใน Tier B:
- BaseColor (Albedo)
- Normal
- Metallic
- Roughness
- AO (optional)
- Emissive

สำหรับ 2D/HD-2D:
- ทำ `Lit Sprite Material` (normal map + emissive)
- ใช้ post-processing ร่วมกับ 3D ได้ (bloom / grading / tone map)

## API / Tooling Implication

### Tier A Focus API
- Scene2D / Tilemap APIs
- Sprite animation APIs
- 2D lighting APIs
- Save/load game state APIs

### Tier B Focus API
- Material APIs
- Render layer/pass APIs
- 3D scene + hybrid scene APIs
- Physics backend integration APIs

ทั้งสอง Tier ควรแชร์:
- ECS
- Asset pipeline
- Export pipeline
- Lua scripting
- Python tooling

## Milestone Roadmap (แนะนำ)

### M1: Tier A Core (Playable 2D)
- Sprite batching (static + dynamic)
- Tilemap chunk render
- 2D camera + pixel-perfect
- Scene save/load (2D map entities)
- Basic 2D lighting on canvas
- Lightweight post FX

### M2: Tier A Tooling (Production-ready 2D)
- Tile editor tools (paint/fill/erase/layers)
- Animation editor basics
- Tileset/atlas pipeline
- Runtime UI widgets for RPG workflows
- 2D export polish + validation

### M3: Tier B Foundation (Unified)
- Material abstraction
- Shared post-processing stack
- 3D scene integration with ECS
- Hybrid render ordering (2D+3D)
- Physics abstraction (`none/jolt/...`)

### M4: Tier B HD-2D / 2.5D
- 3D environment + 2D character pipeline
- Lit sprites + depth-aware FX
- PBR for 3D assets
- Hybrid editor tools / previews

## Bug / Runtime Checklist (ใช้ทุกครั้ง)

- Batching:
  - ไม่มี per-frame heap allocations สูงผิดปกติ
  - vertex/index buffer update ไม่ overflow
- Lighting:
  - invalid light params ไม่ทำให้ render crash
  - fallback เมื่อ shader/normal map หาย
- Post FX:
  - pass order ถูกต้อง (2D/3D/UI)
  - disable feature แล้วไม่พัง
- ECS:
  - stale entity handle ไม่ใช้ต่อได้
  - create/destroy loop ไม่ทำให้ iterator พัง
- Export:
  - ไฟล์ assets/scenes ครบ
  - run จาก export folder ได้จริง




