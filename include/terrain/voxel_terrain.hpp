#pragma once

#include "terrain/chunk_data.hpp"
#include "terrain/voxel_noise.hpp"

namespace Balmung {

struct TerrainSettings {
    std::uint32_t seed = 1337u;
    float continental_frequency = 0.0032f;
    float erosion_frequency = 0.0085f;
    float cave_frequency = 0.045f;
    float ore_frequency = 0.08f;
    int min_height = 24;
    int max_height = 144;
    int sea_level = kSeaLevel;
};

class TerrainGenerator {
public:
    explicit TerrainGenerator(TerrainSettings settings = {});

    const TerrainSettings& settings() const { return settings_; }
    int sample_height(int world_x, int world_z) const;
    void generate_chunk(ChunkData& chunk, ChunkBuildStats* stats = nullptr) const;

private:
    TerrainSettings settings_;
    NoiseSampler continental_;
    NoiseSampler erosion_;
    NoiseSampler caves_;
    NoiseSampler ores_;

    BlockId pick_block(int world_x, int world_y, int world_z, int surface_height, bool cave) const;
};

} // namespace Balmung


