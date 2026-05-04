# Balmung Engine Layout
engine/ is Balmung's canonical module root. It is organized so editor, runtime, and scene systems can grow without folding back into a single mixed source tree.
## Module Groups
- editor/: editor-facing systems and tools.
- scene/2d: runtime support for 2D scenes, HD-2D, pixel worlds, and side-scrollers.
- scene/3d: runtime support for 3D worlds, voxel scenes, and hybrid rendering.
- ont/: engine-owned default fonts and font manifests.
## Code Organization Rules
- Keep responsibilities local to a module.
- Aim for roughly 500-900 LOC per source file.
- Split files before they become mixed-responsibility or hard to navigate.
- Treat engine/ as the top-level map for new systems even while legacy source folders are being migrated.

