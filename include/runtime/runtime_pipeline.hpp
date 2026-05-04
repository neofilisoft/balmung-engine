#pragma once

#include "ecs/components.hpp"
#include "material/material_system.hpp"
#include "physics/physics_world.hpp"
#include "runtime/post_process_graph.hpp"
#include "runtime/render_scene_2d.hpp"
#include "runtime/render_scene_3d.hpp"
#include "runtime/scene_render_bridge.hpp"

#include <cstdint>
#include <unordered_map>

namespace Balmung::Runtime {

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
    struct PhysicsBinding {
        std::uint32_t body_id = 0;
        ECS::BodyType body_type = ECS::BodyType::Static;
        float mass = 1.0f;
        bool use_gravity = true;
        bool has_box_collider = false;
        ECS::Vec3 half_extents{0.5f, 0.5f, 0.5f};
    };

    static std::uint64_t entity_key(ECS::Entity entity) noexcept;
    static bool binding_matches(const PhysicsBinding& binding,
                                const ECS::RigidBodyComponent& rigid_body,
                                const ECS::BoxColliderComponent* box_collider) noexcept;
    void sync_physics_bindings(ECS::Registry& registry);

    ProjectRenderMode _mode = ProjectRenderMode::Mode2D;
    Material::MaterialSystem _materials;
    Physics::PhysicsWorld _physics;
    PostProcessGraph _post_graph;
    RenderScene2D _scene_2d;
    RenderScene3D _scene_3d;
    SceneRenderBridge _bridge;
    std::unordered_map<std::uint64_t, PhysicsBinding> _physics_bindings;
};

} // namespace Balmung::Runtime



