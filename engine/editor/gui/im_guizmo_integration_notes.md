# ImGuizmo Integration Notes

Reference source:
- `ImGuizmo` upstream reference checkout

Balmung's refreshed viewport now mirrors the core ImGuizmo workflow:
- translate / rotate / scale modes
- local / world space switching
- view presets such as perspective, top, front, and isometric
- overlay-first viewport chrome before native gizmo hookup

Next step is wiring native Dear ImGui + ImGuizmo against Balmung camera matrices and scene transforms.

