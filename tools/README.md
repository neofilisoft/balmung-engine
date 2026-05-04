# Balmung Tools

Python tooling scripts for automation, asset processing, scene utilities, export helpers, and Overworld content integration.

Suggested uses:
- asset validation / batch import
- scene JSON generation
- export packaging helpers
- smoke tests for project structure
- Overworld content-pack scanning and manifest generation
- scanning external game-owned source packs such as `Overworlder`

Useful commands:

```powershell
python tools\balmung_tool.py info
python tools\balmung_tool.py validate-layout
python tools\balmung_tool.py process-assets --dry-run
python tools\balmung_tool.py scan-overworld-pack --pack-root D:\Projects\Overworld
python tools\balmung_tool.py scan-source-pack --pack-root D:\Projects\Overworlder --project-name Overworlder --consumer-project Overworld
```

`scan-overworld-pack` writes a JSON manifest for the game-side Overworld repo.

`scan-source-pack` writes a JSON manifest for external asset sources such as Bedrock-style resource/behavior packs. This is the current path for integrating `Overworlder` as an Overworld-owned asset/source pack that Balmung tools can consume.




