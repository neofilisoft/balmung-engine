#pragma once

#include "terrain/chunk_data.hpp"
#include <vector>

namespace Balmung {

enum class FaceOrientation : std::uint8_t {
    PosX = 0,
    NegX = 1,
    PosY = 2,
    NegY = 3,
    PosZ = 4,
    NegZ = 5
};

struct PackedVoxelVertex32 {
    std::uint32_t value = 0;

    static PackedVoxelVertex32 pack(
        std::uint8_t x,
        std::uint8_t y,
        std::uint8_t z,
        FaceOrientation face,
        std::uint8_t corner,
        std::uint8_t block_light,
        std::uint8_t sky_light,
        bool reserved = false);

    std::uint8_t x() const;
    std::uint8_t y() const;
    std::uint8_t z() const;
    FaceOrientation face() const;
    std::uint8_t corner() const;
    std::uint8_t block_light() const;
    std::uint8_t sky_light() const;
};

struct MaterialBatch {
    BlockId material = 0;
    std::uint32_t first_index = 0;
    std::uint32_t index_count = 0;
};

struct PackedChunkMesh {
    std::vector<PackedVoxelVertex32> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<MaterialBatch> batches;
    RenderBackend backend_hint = RenderBackend::Vulkan;

    std::size_t triangle_count() const { return indices.size() / 3; }
};

struct ChunkNeighborhood {
    const ChunkData* self = nullptr;
    const ChunkData* neg_x = nullptr;
    const ChunkData* pos_x = nullptr;
    const ChunkData* neg_z = nullptr;
    const ChunkData* pos_z = nullptr;
};

class ChunkMesher {
public:
    PackedChunkMesh build(const ChunkNeighborhood& neighborhood, RenderBackend backend_hint = RenderBackend::Vulkan) const;
};

} // namespace Balmung


