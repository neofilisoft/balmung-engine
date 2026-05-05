# Scene Modules
scene/2d and scene/3d split runtime responsibilities so Balmung can support:
- 2D pixel-art games
- 2.5D / HD-2D games
- 3D action or voxel games
Runtime systems should prefer shared abstractions for serialization, components, rendering hooks, and scene authoring, while keeping dimension-specific logic inside the matching module folder.

