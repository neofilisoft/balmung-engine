#pragma once

#include "ecs/components.hpp"
#include "ecs/registry.hpp"
#include "material/material_system.hpp"
#include "runtime/render_scene_2d.hpp"
#include "runtime/render_scene_3d.hpp"

namespace VoxelBlock::Runtime {

class SceneRenderBridge {
public:
    explicit SceneRenderBridge(Material::MaterialSystem* material_system = nullptr)
        : _materials(material_system)
    {
    }

    void set_material_system(Material::MaterialSystem* material_system) { _materials = material_system; }

    void build_from_ecs(const ECS::Registry& registry, RenderScene2D& out_2d, RenderScene3D& out_3d) const;

private:
    Material::MaterialSystem* _materials = nullptr;
};

} // namespace VoxelBlock::Runtime

