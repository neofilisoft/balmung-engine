# Balmung Editor Workflow (Scene / Assets / Export)

เอกสารนี้อธิบาย workflow ฝั่ง Editor แบบใช้งานจริง

## 1) Visual Building (Scene Builder)
- เปิดแท็บ `Scene Builder`
- เลือก block จาก panel ซ้าย
- วาดลง grid ด้วยเมาส์ซ้าย
- ลบด้วย `Erase` หรือคลิกขวาลาก
- ตั้งชื่อ scene ที่ช่อง `Scene`
- กด `Save Scene`

ไฟล์ scene จะถูกบันทึกที่:
- `Scenes/<scene-name>.json`

## 2) Asset Pipeline (Auto Import)
โฟลเดอร์ที่ใช้:
- ใส่ไฟล์ต้นฉบับ: `Assets/Raw/`
- ผลลัพธ์อยู่ที่: `Assets/Processed/`

รองรับอัตโนมัติ (stage/import):
- `.png` (texture)
- `.wav` (audio)
- `.fbx` (model stage)
- `.cs` (script template / gameplay asset ฝั่ง C#)
- `.txt` (note / prompt / design doc)
- `.json`, `.tmj`, `.tsx` (ข้อมูล tilemap/tileset สำหรับ 2D)
- `.aseprite` (stage sprite animation สำหรับ 2D)

หมายเหตุ:
- ตอนนี้ `.fbx` อยู่ในสถานะ `staged` (คัดลอก + ลง manifest แล้ว)
- ขั้นตอน convert mesh/runtime optimization ยังเป็น backend task รอบถัดไป

กดปุ่ม:
- `Process Assets`

ผลลัพธ์:
- สร้าง/อัปเดต `Assets/Processed/asset_manifest.json`

## 3) Standalone Export (One-click Pack)
แท็บ `Export`
- ตั้ง `Build Name`
- เลือก `Target Platform`: `Windows`, `Linux`, `macOS`, `Android`, `iOS`, หรือ `Web`
- กด `Export Standalone`

ระบบจะ pack:
- `Scenes/`
- `Assets/Processed/`
- `mods/`
- และพยายามหา artifact ของ target platform ที่เลือกเพื่อใส่ให้

ผลลัพธ์อยู่ที่:
- `Exports/<BuildName>/`

ถ้ายังไม่มี artifact ของ target platform ที่เลือก
- ระบบจะ export package ให้ก่อน
- พร้อมสร้าง `README-EXPORT.txt` แจ้งขั้นตอน build/handoff ของ platform นั้น

## 4) Asset Templates

กด `Install Gameplay Template` ใน Content Browser เพื่อคัดลอกไฟล์ C# gameplay starter ของ Balmung เข้า `Assets/Raw/Scripts/BalmungGameplay` ของโปรเจกต์ปัจจุบัน. ถ้าต้องการเปลี่ยนที่เก็บ template ให้ตั้งค่า `BALMUNG_TEMPLATE_ROOT`; ไม่งั้น Balmung จะใช้ `templates/assets` ที่มากับเอนจิ้น.

## 5) Checklist ก่อนปล่อยงาน (Bug / Logic / Runtime)
- Scene save/load แล้วข้อมูลกลับมาตรง
- Asset pipeline ไม่มี error ใน manifest
- Export package มีไฟล์ scene + assets ครบ
- ถ้ามี `.exe` ให้ทดสอบเปิดจากโฟลเดอร์ export จริง
- ตรวจข้อความ error ที่แสดงใน status bar ว่าอ่านรู้เรื่อง




