#include "runtime/runtime_pipeline.hpp"

#include <unordered_set>

namespace Balmung::Runtime {
namespace {

Physics::BodyType to_physics_body_type(ECS::BodyType type) noexcept
{
    switch (type) {
    case ECS::BodyType::Dynamic: return Physics::BodyType::Dynamic;
    case ECS::BodyType::Kinematic: return Physics::BodyType::Kinematic;
    case ECS::BodyType::Static:
    default:
        return Physics::BodyType::Static;
    }
}

Physics::Vec3 to_physics_vec3(const ECS::Vec3& value) noexcept
{
    return Physics::Vec3{value.x, value.y, value.z};
}

Physics::Transform to_physics_transform(const ECS::TransformComponent& tx) noexcept
{
    return Physics::Transform{
        .position = to_physics_vec3(tx.position),
        .rotation_euler_deg = to_physics_vec3(tx.rotation_euler_deg),
        .scale = to_physics_vec3(tx.scale),
    };
}

void apply_body_state_to_transform(const Physics::BodyState& state, ECS::TransformComponent& tx) noexcept
{
    tx.position.x = state.transform.position.x;
    tx.position.y = state.transform.position.y;
    tx.position.z = state.transform.position.z;
    tx.rotation_euler_deg.x = state.transform.rotation_euler_deg.x;
    tx.rotation_euler_deg.y = state.transform.rotation_euler_deg.y;
    tx.rotation_euler_deg.z = state.transform.rotation_euler_deg.z;
}

} // namespace

RuntimePipeline::RuntimePipeline()
    : _bridge(&_materials)
{
    _post_graph.set_default_tier_a_2d_stack();
}

std::uint64_t RuntimePipeline::entity_key(ECS::Entity entity) noexcept
{
    return (static_cast<std::uint64_t>(entity.generation) << 32) | entity.index;
}

bool RuntimePipeline::binding_matches(const PhysicsBinding& binding,
                                      const ECS::RigidBodyComponent& rigid_body,
                                      const ECS::BoxColliderComponent* box_collider) noexcept
{
    const ECS::Vec3 expected_half_extents = box_collider ? box_collider->half_extents : ECS::Vec3{0.5f, 0.5f, 0.5f};
    return binding.body_type == rigid_body.body_type &&
           binding.mass == rigid_body.mass &&
           binding.use_gravity == rigid_body.use_gravity &&
           binding.has_box_collider == (box_collider != nullptr) &&
           binding.half_extents.x == expected_half_extents.x &&
           binding.half_extents.y == expected_half_extents.y &&
           binding.half_extents.z == expected_half_extents.z;
}

void RuntimePipeline::sync_physics_bindings(ECS::Registry& registry)
{
    std::unordered_set<std::uint64_t> seen;

    registry.for_each<ECS::TransformComponent, ECS::RigidBodyComponent>(
        [&](ECS::Entity entity, ECS::TransformComponent& tx, const ECS::RigidBodyComponent& rigid_body) {
            const auto key = entity_key(entity);
            seen.insert(key);

            const auto* box_collider = registry.try_get<ECS::BoxColliderComponent>(entity);
            const ECS::Vec3 half_extents = box_collider ? box_collider->half_extents : ECS::Vec3{0.5f, 0.5f, 0.5f};

            auto binding_it = _physics_bindings.find(key);
            if (binding_it != _physics_bindings.end() && !binding_matches(binding_it->second, rigid_body, box_collider)) {
                _physics.destroy_body(binding_it->second.body_id);
                _physics_bindings.erase(binding_it);
                binding_it = _physics_bindings.end();
            }

            if (binding_it == _physics_bindings.end()) {
                Physics::RigidBodyDesc desc{};
                desc.type = to_physics_body_type(rigid_body.body_type);
                desc.mass = rigid_body.mass;
                desc.use_gravity = rigid_body.use_gravity;
                desc.box_half_extents = to_physics_vec3(half_extents);

                const auto body_id = _physics.create_body(desc, to_physics_transform(tx));
                if (body_id == 0) {
                    return;
                }

                binding_it = _physics_bindings.emplace(
                    key,
                    PhysicsBinding{
                        .body_id = body_id,
                        .body_type = rigid_body.body_type,
                        .mass = rigid_body.mass,
                        .use_gravity = rigid_body.use_gravity,
                        .has_box_collider = box_collider != nullptr,
                        .half_extents = half_extents,
                    }).first;
            }

            if (rigid_body.body_type != ECS::BodyType::Dynamic) {
                _physics.set_body_transform(binding_it->second.body_id, to_physics_transform(tx));
            }
        });

    for (auto it = _physics_bindings.begin(); it != _physics_bindings.end();) {
        if (seen.contains(it->first)) {
            ++it;
            continue;
        }

        _physics.destroy_body(it->second.body_id);
        it = _physics_bindings.erase(it);
    }
}

void RuntimePipeline::step(ECS::Registry& registry, float dt)
{
    sync_physics_bindings(registry);
    _physics.step(dt);

    registry.for_each<ECS::TransformComponent, ECS::RigidBodyComponent>(
        [&](ECS::Entity entity, ECS::TransformComponent& tx, const ECS::RigidBodyComponent& rigid_body) {
            const auto binding_it = _physics_bindings.find(entity_key(entity));
            if (binding_it == _physics_bindings.end()) {
                return;
            }

            const auto state = _physics.get_body_state(binding_it->second.body_id);
            if (!state) {
                return;
            }

            if (rigid_body.body_type == ECS::BodyType::Dynamic) {
                apply_body_state_to_transform(*state, tx);
            }
        }
    );

    rebuild_render_scenes(registry);
}

void RuntimePipeline::rebuild_render_scenes(const ECS::Registry& registry)
{
    switch (_mode) {
    case ProjectRenderMode::Mode2D:
        _post_graph.set_default_tier_a_2d_stack();
        break;
    case ProjectRenderMode::ModeHD2D:
    case ProjectRenderMode::Mode3D:
        _post_graph.set_default_tier_b_unified_stack();
        break;
    }
    _bridge.build_from_ecs(registry, _scene_2d, _scene_3d);
}

} // namespace Balmung::Runtime


