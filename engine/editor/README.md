# Editor Modules
The editor tree mirrors the separation used by large production engines.
- nimation: timeline tooling, animation previews, retarget utilities.
- udio: waveform previews, bus routing, audio inspector tooling.
- debugger: live diagnostics, traces, perf overlays, remote hooks.
- docks: reusable dock panels, scene browser panes, content browsers.
- gui: editor chrome, widgets, style system, command routing.
- import: asset importers, metadata extraction, conversion passes.
- export: packaging, publish profiles, platform deploy presets.
- settings: editor preferences, keymaps, workspace profiles.
The active C++ and C# editor code still lives partly in legacy folders today, but this tree is now the canonical module map for future migrations.

