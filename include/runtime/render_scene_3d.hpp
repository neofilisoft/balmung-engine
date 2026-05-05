#pragma once

#include "material/material_system.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace Balmung::Runtime {

struct Camera3D {
    float position[3]{0.0f, 0.0f, 0.0f};
    float pitch_deg = 0.0f;
    float yaw_deg = 0.0f;
    float fov_deg = 60.0f;
    float near_plane = 0.1f;
    float far_plane = 2000.0f;
};

struct Mesh3DInstance {
    float position[3]{0.0f, 0.0f, 0.0f};
    float rotation_euler_deg[3]{0.0f, 0.0f, 0.0f};
    float scale[3]{1.0f, 1.0f, 1.0f};
    Material::MaterialHandle material = 0;
    std::string mesh_asset;
    bool visible = true;
};

struct RenderScene3D {
    Camera3D camera{};
    std::vector<Mesh3DInstance> opaque_meshes;
    std::vector<Mesh3DInstance> transparent_meshes;

    void clear()
    {
        opaque_meshes.clear();
        transparent_meshes.clear();
    }
};

} // namespace Balmung::Runtime



