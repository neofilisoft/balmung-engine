# Balmung API (C#) สำหรับผู้เริ่มต้น

เอกสารนี้อธิบายการใช้ `Balmung.Bridge` ผ่าน C# โดยไม่ต้องรู้ C++ มาก่อน

## ภาพรวม
- `Balmung.Bridge.Native`
  - low-level P/Invoke (เรียก native DLL ตรงๆ)
- `BalmungEngine`
  - high-level wrapper ใช้งานง่ายกว่า (`Init`, `Tick`, `SaveWorld`, `LoadWorld`, `ExecLua`)

## เริ่มต้นใช้งาน
ตัวอย่างพื้นฐาน:

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

## เมธอดที่ใช้บ่อย
- `InitHeadless(width, height)`
  - ใช้กับ Editor/Tool ไม่มีหน้าต่าง native
- `InitWindowed(width, height, title)`
  - ใช้กับเกม standalone
- `Tick(dt)`
  - อัปเดต world/renderer หนึ่งเฟรม
- `PlaceVoxel(x,y,z,"block")`
  - วางบล็อก
- `DestroyVoxel(x,y,z)`
  - ลบบล็อก
- `SaveWorld(name)` / `LoadWorld(name)`
  - บันทึก/โหลดโลก
- `LoadMods("mods")`
  - โหลด Lua mods
- `ExecLua("print('hi')")`
  - รันคำสั่ง Lua
- `GetAllBlocks()`
  - ดึงรายการบล็อกทั้งหมดเพื่อสร้าง palette/UI
- `GetStats()`
  - อ่านสถิติ render (`DrawCalls`, `Triangles`, `FrameMs`)

## Event (ฝั่ง C#)
สามารถ subscribe อีเวนต์จาก engine ได้:

```csharp
engine.OnEngineEvent += (name, json) =>
{
    Console.WriteLine($"{name}: {json}");
};
```

ตัวอย่าง event ที่เจอได้:
- `on_place`
- `on_destroy`
- `on_craft`

## ข้อควรระวัง (Runtime)
- native DLL ต้องอยู่ใน path ที่แอปหาเจอ (`balmung.dll` ???? `libbalmung.dll`)
- ถ้า `InitHeadless()` คืน `false` ให้หยุดใช้งาน instance นั้น
- เรียก `Dispose()` หรือใช้ `using` เสมอ
- อย่าเรียก API จากหลาย thread พร้อมกัน (native side ยังไม่ออกแบบให้ re-entrant)

## ใช้กับ Editor แบบไม่ต้องแตะ C++
ฝั่ง `Balmung.Editor` ใช้ API นี้อยู่แล้วสำหรับ:
- วาง/ลบบล็อกใน Scene Builder
- save/load world + scene
- โหลด mods
- อ่านสถิติ

คุณสามารถขยายต่อใน C# ได้ก่อน โดยยังไม่ต้องแก้ C++:
- ปุ่มเครื่องมือใหม่ใน Editor
- import/export pipeline
- scene tools (fill/line/brush)






