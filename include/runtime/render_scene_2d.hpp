#pragma once

#include "material/material_system.hpp"

#include <algorithm>
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

    void sort_for_render()
    {
        std::stable_sort(dynamic_sprites.begin(), dynamic_sprites.end(), [](const auto& a, const auto& b) {
            if (a.sort_layer != b.sort_layer) return a.sort_layer < b.sort_layer;
            if (a.order_in_layer != b.order_in_layer) return a.order_in_layer < b.order_in_layer;
            return a.z < b.z;
        });
    }

    [[nodiscard]] std::uint32_t sprite_count() const noexcept
    {
        return static_cast<std::uint32_t>(dynamic_sprites.size());
    }

    [[nodiscard]] std::uint32_t tile_count() const noexcept
    {
        std::uint32_t count = 0;
        for (const auto& batch : static_batches) {
            count += batch.tile_count;
        }
        return count;
    }
};

} // namespace Balmung::Runtime



