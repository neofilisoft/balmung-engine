#pragma once

#include <cstdint>
#include <cstddef>

namespace Balmung {

using BlockId = std::uint16_t;

constexpr int kChunkSizeX = 16;
constexpr int kChunkSizeZ = 16;
constexpr int kChunkSizeY = 256;
constexpr int kSeaLevel = 62;
constexpr std::size_t kChunkVoxelCount = static_cast<std::size_t>(kChunkSizeX) * static_cast<std::size_t>(kChunkSizeZ) * static_cast<std::size_t>(kChunkSizeY);

enum class RenderBackend : std::uint8_t {
    Vulkan,
    DirectX12,
    OpenGL
};

struct ChunkCoord {
    int x = 0;
    int z = 0;

    friend bool operator==(const ChunkCoord&, const ChunkCoord&) = default;
};

struct ChunkCoordHash {
    std::size_t operator()(const ChunkCoord& coord) const noexcept {
        const auto hx = static_cast<std::uint64_t>(static_cast<std::uint32_t>(coord.x));
        const auto hz = static_cast<std::uint64_t>(static_cast<std::uint32_t>(coord.z));
        return static_cast<std::size_t>((hx << 32) ^ hz);
    }
};

struct WorldPos {
    int x = 0;
    int y = 0;
    int z = 0;
};

struct ChunkBuildStats {
    int solid_blocks = 0;
    int air_blocks = 0;
    int water_blocks = 0;
    int emissive_blocks = 0;
    int cave_cells = 0;
    int ore_blocks = 0;
};

inline int floor_div(int value, int divisor) {
    int q = value / divisor;
    int r = value % divisor;
    if (r != 0 && ((r < 0) != (divisor < 0))) {
        --q;
    }
    return q;
}

inline int positive_mod(int value, int divisor) {
    int r = value % divisor;
    return r < 0 ? r + divisor : r;
}

inline ChunkCoord world_to_chunk(int world_x, int world_z) {
    return {floor_div(world_x, kChunkSizeX), floor_div(world_z, kChunkSizeZ)};
}

inline int world_to_local_x(int world_x) {
    return positive_mod(world_x, kChunkSizeX);
}

inline int world_to_local_z(int world_z) {
    return positive_mod(world_z, kChunkSizeZ);
}

inline WorldPos chunk_origin(const ChunkCoord& coord) {
    return {coord.x * kChunkSizeX, 0, coord.z * kChunkSizeZ};
}

} // namespace Balmung


