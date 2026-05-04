#include "terrain/light_propagation.hpp"
#include "terrain/block_palette.hpp"
#include <array>
#include <queue>

namespace Balmung {

namespace {
struct LightNode {
    int x = 0;
    int y = 0;
    int z = 0;
    std::uint8_t light = 0;
    bool sky = false;
};

constexpr std::array<WorldPos, 6> kOffsets{{
    { 1, 0, 0}, {-1, 0, 0},
    { 0, 1, 0}, { 0,-1, 0},
    { 0, 0, 1}, { 0, 0,-1}
}};
}

void LightingSolver::rebuild(ChunkData& chunk) const {
    chunk.clear_lighting();
    std::queue<LightNode> queue;

    for (int z = 0; z < kChunkSizeZ; ++z) {
        for (int x = 0; x < kChunkSizeX; ++x) {
            std::uint8_t sky = 15;
            for (int y = kChunkSizeY - 1; y >= 0; --y) {
                const auto block = chunk.block_at(x, y, z);
                if (BlockPalette::is_opaque(block)) {
                    sky = 0;
                }
                if (sky > 0) {
                    chunk.set_sky_light(x, y, z, sky);
                    queue.push({x, y, z, sky, true});
                }
                if (BlockPalette::emits_light(block)) {
                    const auto emission = BlockPalette::emission(block);
                    chunk.set_block_light(x, y, z, emission);
                    queue.push({x, y, z, emission, false});
                }
            }
        }
    }

    while (!queue.empty()) {
        const auto node = queue.front();
        queue.pop();
        if (node.light <= 1) {
            continue;
        }

        for (const auto& offset : kOffsets) {
            const int nx = node.x + offset.x;
            const int ny = node.y + offset.y;
            const int nz = node.z + offset.z;
            if (!chunk.contains(nx, ny, nz)) {
                continue;
            }
            const auto block = chunk.block_at(nx, ny, nz);
            if (BlockPalette::is_opaque(block)) {
                continue;
            }
            auto current = node.sky ? chunk.sky_light_at(nx, ny, nz) : chunk.block_light_at(nx, ny, nz);
            const auto next = static_cast<std::uint8_t>(node.light - 1);
            if (next <= current) {
                continue;
            }
            if (node.sky) {
                chunk.set_sky_light(nx, ny, nz, next);
            } else {
                chunk.set_block_light(nx, ny, nz, next);
            }
            queue.push({nx, ny, nz, next, node.sky});
        }
    }
}

} // namespace Balmung


