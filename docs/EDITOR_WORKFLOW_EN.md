# Balmung Editor Workflow (Scene / Assets / Export)

This guide describes the current editor workflow.

## 1) Visual Building (Scene Builder)
- Open the `Scene Builder` tab
- Select a block from the left palette
- Paint onto the grid with left mouse drag
- Erase with `Erase` mode or right mouse drag
- Set the scene name in the `Scene` field
- Click `Save Scene`

Scene files are saved to:
- `Scenes/<scene-name>.json`

## 2) Asset Pipeline (Auto Import)
Folders:
- Source files: `Assets/Raw/`
- Processed output: `Assets/Processed/`

Current supported file types:
- `.png` (textures)
- `.wav` (audio)
- `.fbx` (model staging)
- `.cs` (script templates / C# gameplay assets)
- `.txt` (notes / prompts / design docs)
- `.json`, `.tmj`, `.tsx` (2D tilemap and tileset data)
- `.aseprite` (2D sprite animation staging)

Notes:
- `.fbx` is currently marked as `staged` (copied + indexed in manifest)
- Runtime mesh conversion/optimization is still a backend follow-up task

Action:
- Click `Process Assets`

Output:
- `Assets/Processed/asset_manifest.json`

## 3) Standalone Export (One-click Pack)
In the `Export` tab:
- Set `Build Name`
- Choose `Target Platform`: `Windows`, `Linux`, `macOS`, `Android`, `iOS`, or `Web`
- Click `Export Standalone`

The exporter packs:
- `Scenes/`
- `Assets/Processed/`
- `mods/`
- The selected target platform artifact if one is found

Output folder:
- `Exports/<BuildName>/`

If no target artifact is found:
- A package is still created
- `README-EXPORT.txt` is generated with guidance

## 4) Asset Templates

Use `Install Gameplay Template` in the Content Browser to copy the bundled Balmung C# gameplay starter files into `Assets/Raw/Scripts/BalmungGameplay`. The template root can be overridden with `BALMUNG_TEMPLATE_ROOT`; otherwise Balmung uses the shipped `templates/assets` folder.

## 5) Pre-release Checklist (Bug / Logic / Runtime)
- Save/load a scene and verify content matches
- Confirm asset pipeline finishes without manifest errors
- Verify exported package contains scenes + assets
- If a game `.exe` exists, run it from the export folder
- Check status/error messages are understandable for end users




