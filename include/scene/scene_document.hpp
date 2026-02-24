#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace VoxelBlock::Scene {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct SerializedTransform {
    Vec3 position{};
    Vec3 rotation_euler_deg{};
    Vec3 scale{1.0f, 1.0f, 1.0f};
};

struct SerializedRigidBody {
    std::string body_type;
    float mass = 1.0f;
    bool use_gravity = true;
};

struct SerializedBoxCollider {
    Vec3 half_extents{0.5f, 0.5f, 0.5f};
    bool is_trigger = false;
};

struct SerializedEntity {
    std::uint32_t entity_index = 0;
    std::uint32_t generation = 0;
    std::optional<std::string> name;
    std::optional<SerializedTransform> transform;
    std::optional<std::string> static_mesh_asset;
    std::optional<std::string> material_asset;
    std::optional<SerializedRigidBody> rigid_body;
    std::optional<SerializedBoxCollider> box_collider;
    std::optional<std::string> lua_script;
    std::optional<std::string> wav_asset;
    std::optional<std::string> voxel_block_type;
};

struct SceneDocument {
    std::string scene_name = "scene";
    std::uint32_t schema_version = 1;
    std::vector<SerializedEntity> entities;
};

} // namespace VoxelBlock::Scene

