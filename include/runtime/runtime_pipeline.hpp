#pragma once

#include "material/material_system.hpp"
#include "physics/physics_world.hpp"
#include "runtime/post_process_graph.hpp"
#include "runtime/render_scene_2d.hpp"
#include "runtime/render_scene_3d.hpp"
#include "runtime/scene_render_bridge.hpp"

namespace VoxelBlock::Runtime {

enum class ProjectRenderMode {
    Mode2D,
    ModeHD2D,
    Mode3D,
};

class RuntimePipeline {
public:
    RuntimePipeline();

    void set_project_render_mode(ProjectRenderMode mode) noexcept { _mode = mode; }
    [[nodiscard]] ProjectRenderMode project_render_mode() const noexcept { return _mode; }

    void step(ECS::Registry& registry, float dt);
    void rebuild_render_scenes(const ECS::Registry& registry);

    [[nodiscard]] const RenderScene2D& scene_2d() const noexcept { return _scene_2d; }
    [[nodiscard]] const RenderScene3D& scene_3d() const noexcept { return _scene_3d; }
    [[nodiscard]] const PostProcessGraph& post_graph() const noexcept { return _post_graph; }
    [[nodiscard]] Material::MaterialSystem& materials() noexcept { return _materials; }
    [[nodiscard]] Physics::PhysicsWorld& physics() noexcept { return _physics; }

private:
    ProjectRenderMode _mode = ProjectRenderMode::Mode2D;
    Material::MaterialSystem _materials;
    Physics::PhysicsWorld _physics;
    PostProcessGraph _post_graph;
    RenderScene2D _scene_2d;
    RenderScene3D _scene_3d;
    SceneRenderBridge _bridge;
};

} // namespace VoxelBlock::Runtime

