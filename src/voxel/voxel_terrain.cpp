#include "terrain/voxel_terrain.hpp"
#include <algorithm>
#include <cmath>

namespace Balmung {

TerrainGenerator::TerrainGenerator(TerrainSettings settings)
    : settings_(settings),
      continental_(settings.seed ^ 0x1234abcdu),
      erosion_(settings.seed ^ 0x9f37b51au),
      caves_(settings.seed ^ 0x5bd1e995u),
      ores_(settings.seed ^ 0x27d4eb2du) {}

int TerrainGenerator::sample_height(int world_x, int world_z) const {
    const float continents = continental_.fbm2d(world_x * settings_.continental_frequency, world_z * settings_.continental_frequency, 5, 2.0f, 0.5f);
    const float erosion = erosion_.fbm2d(world_x * settings_.erosion_frequency, world_z * settings_.erosion_frequency, 3, 2.2f, 0.55f);
    const float base = static_cast<float>(settings_.sea_level) + continents * 34.0f + erosion * 10.0f;
    return std::clamp(static_cast<int>(std::round(base)), settings_.min_height, settings_.max_height);
}

BlockId TerrainGenerator::pick_block(int world_x, int world_y, int world_z, int surface_height, bool cave) const {
    if (cave) {
        if (world_y <= 18 && ores_.fbm3d(world_x * settings_.ore_frequency, world_y * settings_.ore_frequency, world_z * settings_.ore_frequency, 3, 2.1f, 0.5f) > 0.68f) {
            return static_cast<BlockId>(BlockKind::Glowstone);
        }
        return static_cast<BlockId>(BlockKind::Air);
    }

    if (world_y > surface_height) {
        if (world_y <= settings_.sea_level) {
            return static_cast<BlockId>(BlockKind::Water);
        }
        return static_cast<BlockId>(BlockKind::Air);
    }

    const int depth = surface_height - world_y;
    const bool beach = surface_height <= settings_.sea_level + 2;

    if (depth == 0) {
        return static_cast<BlockId>(beach ? BlockKind::Sand : BlockKind::Grass);
    }
    if (depth < 4) {
        return static_cast<BlockId>(beach ? BlockKind::Sand : BlockKind::Dirt);
    }

    if (world_y > 8 && ores_.fbm3d(world_x * settings_.ore_frequency, world_y * settings_.ore_frequency, world_z * settings_.ore_frequency, 2, 2.0f, 0.6f) > 0.72f) {
        return static_cast<BlockId>(BlockKind::CoalOre);
    }

    return static_cast<BlockId>(BlockKind::Stone);
}

void TerrainGenerator::generate_chunk(ChunkData& chunk, ChunkBuildStats* stats) const {
    ChunkBuildStats local_stats{};
    const auto origin = chunk_origin(chunk.coord());
    for (int z = 0; z < kChunkSizeZ; ++z) {
        const int world_z = origin.z + z;
        for (int x = 0; x < kChunkSizeX; ++x) {
            const int world_x = origin.x + x;
            const int surface_height = sample_height(world_x, world_z);
            for (int y = 0; y < kChunkSizeY; ++y) {
                const int world_y = y;
                const bool below_surface = world_y <= surface_height;
                const float cave_noise = caves_.fbm3d(world_x * settings_.cave_frequency, world_y * settings_.cave_frequency, world_z * settings_.cave_frequency, 3, 2.0f, 0.5f);
                const bool cave = below_surface && world_y < surface_height - 6 && cave_noise > 0.48f;
                const BlockId block = pick_block(world_x, world_y, world_z, surface_height, cave);
                chunk.set_block(x, y, z, block);

                if (block == static_cast<BlockId>(BlockKind::Air)) {
                    ++local_stats.air_blocks;
                } else {
                    ++local_stats.solid_blocks;
                    if (block == static_cast<BlockId>(BlockKind::Water)) {
                        ++local_stats.water_blocks;
                    }
                    if (block == static_cast<BlockId>(BlockKind::CoalOre)) {
                        ++local_stats.ore_blocks;
                    }
                    if (BlockPalette::emits_light(block)) {
                        ++local_stats.emissive_blocks;
                    }
                }
                if (cave) {
                    ++local_stats.cave_cells;
                }
            }
        }
    }
    if (stats) {
        *stats = local_stats;
    }
}

} // namespace Balmung


