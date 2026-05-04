#pragma once

#include "material/material_system.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace Balmung::Runtime {

struct Sprite2DInstance {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float width = 16.0f;
    float height = 16.0f;
    float rotation_deg = 0.0f;
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 1.0f;
    float v1 = 1.0f;
    std::uint32_t sort_layer = 0;
    std::uint32_t order_in_layer = 0;
    Material::MaterialHandle material = 0;
    std::string texture_asset;
    bool static_batched = false;
};

struct TileChunk2DBatch {
    std::int32_t chunk_x = 0;
    std::int32_t chunk_y = 0;
    std::uint32_t tile_count = 0;
    Material::MaterialHandle material = 0;
    std::string atlas_asset;
};

struct RenderScene2D {
    std::vector<TileChunk2DBatch> static_batches;
    std::vector<Sprite2DInstance> dynamic_sprites;

    void clear()
    {
        static_batches.clear();
        dynamic_sprites.clear();
    }
};

} // namespace Balmung::Runtime



