#include "terrain/packed_mesh.hpp"
#include "terrain/block_palette.hpp"
#include <algorithm>
#include <array>
#include <map>
#include <vector>

namespace Balmung {

PackedVoxelVertex32 PackedVoxelVertex32::pack(
    std::uint8_t x,
    std::uint8_t y,
    std::uint8_t z,
    FaceOrientation face,
    std::uint8_t corner,
    std::uint8_t block_light,
    std::uint8_t sky_light,
    bool reserved) {
    PackedVoxelVertex32 vertex{};
    vertex.value = (static_cast<std::uint32_t>(x & 0x1fu) << 0u)
        | (static_cast<std::uint32_t>(y) << 5u)
        | (static_cast<std::uint32_t>(z & 0x1fu) << 13u)
        | (static_cast<std::uint32_t>(face) << 18u)
        | (static_cast<std::uint32_t>(corner & 0x03u) << 21u)
        | (static_cast<std::uint32_t>(block_light & 0x0fu) << 23u)
        | (static_cast<std::uint32_t>(sky_light & 0x0fu) << 27u)
        | (static_cast<std::uint32_t>(reserved ? 1u : 0u) << 31u);
    return vertex;
}

std::uint8_t PackedVoxelVertex32::x() const { return static_cast<std::uint8_t>((value >> 0u) & 0x1fu); }
std::uint8_t PackedVoxelVertex32::y() const { return static_cast<std::uint8_t>((value >> 5u) & 0xffu); }
std::uint8_t PackedVoxelVertex32::z() const { return static_cast<std::uint8_t>((value >> 13u) & 0x1fu); }
FaceOrientation PackedVoxelVertex32::face() const { return static_cast<FaceOrientation>((value >> 18u) & 0x07u); }
std::uint8_t PackedVoxelVertex32::corner() const { return static_cast<std::uint8_t>((value >> 21u) & 0x03u); }
std::uint8_t PackedVoxelVertex32::block_light() const { return static_cast<std::uint8_t>((value >> 23u) & 0x0fu); }
std::uint8_t PackedVoxelVertex32::sky_light() const { return static_cast<std::uint8_t>((value >> 27u) & 0x0fu); }

namespace {

BlockId sample_block(const ChunkNeighborhood& nb, int x, int y, int z) {
    if (y < 0 || y >= kChunkSizeY || nb.self == nullptr) {
        return static_cast<BlockId>(BlockKind::Air);
    }
    if (x >= 0 && x < kChunkSizeX && z >= 0 && z < kChunkSizeZ) {
        return nb.self->block_at(x, y, z);
    }
    if (x < 0 && nb.neg_x) {
        return nb.neg_x->block_at(kChunkSizeX - 1, y, z);
    }
    if (x >= kChunkSizeX && nb.pos_x) {
        return nb.pos_x->block_at(0, y, z);
    }
    if (z < 0 && nb.neg_z) {
        return nb.neg_z->block_at(x, y, kChunkSizeZ - 1);
    }
    if (z >= kChunkSizeZ && nb.pos_z) {
        return nb.pos_z->block_at(x, y, 0);
    }
    return static_cast<BlockId>(BlockKind::Air);
}

std::uint8_t sample_block_light(const ChunkNeighborhood& nb, int x, int y, int z) {
    if (y < 0 || y >= kChunkSizeY || nb.self == nullptr) {
        return 0;
    }
    if (x >= 0 && x < kChunkSizeX && z >= 0 && z < kChunkSizeZ) {
        return nb.self->block_light_at(x, y, z);
    }
    if (x < 0 && nb.neg_x) {
        return nb.neg_x->block_light_at(kChunkSizeX - 1, y, z);
    }
    if (x >= kChunkSizeX && nb.pos_x) {
        return nb.pos_x->block_light_at(0, y, z);
    }
    if (z < 0 && nb.neg_z) {
        return nb.neg_z->block_light_at(x, y, kChunkSizeZ - 1);
    }
    if (z >= kChunkSizeZ && nb.pos_z) {
        return nb.pos_z->block_light_at(x, y, 0);
    }
    return 0;
}

std::uint8_t sample_sky_light(const ChunkNeighborhood& nb, int x, int y, int z) {
    if (y < 0 || y >= kChunkSizeY || nb.self == nullptr) {
        return 0;
    }
    if (x >= 0 && x < kChunkSizeX && z >= 0 && z < kChunkSizeZ) {
        return nb.self->sky_light_at(x, y, z);
    }
    if (x < 0 && nb.neg_x) {
        return nb.neg_x->sky_light_at(kChunkSizeX - 1, y, z);
    }
    if (x >= kChunkSizeX && nb.pos_x) {
        return nb.pos_x->sky_light_at(0, y, z);
    }
    if (z < 0 && nb.neg_z) {
        return nb.neg_z->sky_light_at(x, y, kChunkSizeZ - 1);
    }
    if (z >= kChunkSizeZ && nb.pos_z) {
        return nb.pos_z->sky_light_at(x, y, 0);
    }
    return 0;
}

void append_face(std::vector<PackedVoxelVertex32>& vertices,
                 std::vector<std::uint32_t>& indices,
                 std::uint8_t x,
                 std::uint8_t y,
                 std::uint8_t z,
                 FaceOrientation face,
                 std::uint8_t block_light,
                 std::uint8_t sky_light) {
    const auto base = static_cast<std::uint32_t>(vertices.size());
    for (std::uint8_t corner = 0; corner < 4; ++corner) {
        vertices.push_back(PackedVoxelVertex32::pack(x, y, z, face, corner, block_light, sky_light));
    }
    indices.push_back(base + 0u);
    indices.push_back(base + 1u);
    indices.push_back(base + 2u);
    indices.push_back(base + 0u);
    indices.push_back(base + 2u);
    indices.push_back(base + 3u);
}

} // namespace

PackedChunkMesh ChunkMesher::build(const ChunkNeighborhood& neighborhood, RenderBackend backend_hint) const {
    PackedChunkMesh mesh{};
    mesh.backend_hint = backend_hint;
    if (neighborhood.self == nullptr) {
        return mesh;
    }

    struct Scratch {
        std::vector<PackedVoxelVertex32> vertices;
        std::vector<std::uint32_t> indices;
    };

    std::map<BlockId, Scratch> material_scratch;

    for (int y = 0; y < kChunkSizeY; ++y) {
        for (int z = 0; z < kChunkSizeZ; ++z) {
            for (int x = 0; x < kChunkSizeX; ++x) {
                const auto block = neighborhood.self->block_at(x, y, z);
                if (!BlockPalette::is_solid(block)) {
                    continue;
                }

                auto& scratch = material_scratch[block];
                const auto block_light = neighborhood.self->block_light_at(x, y, z);
                const auto sky_light = neighborhood.self->sky_light_at(x, y, z);

                const auto emit_if_visible = [&](int nx, int ny, int nz, FaceOrientation face) {
                    const auto neighbor = sample_block(neighborhood, nx, ny, nz);
                    if (BlockPalette::is_opaque(neighbor)) {
                        return;
                    }
                    const auto light = std::max(block_light, sample_block_light(neighborhood, nx, ny, nz));
                    const auto sky = std::max(sky_light, sample_sky_light(neighborhood, nx, ny, nz));
                    append_face(scratch.vertices, scratch.indices, static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y), static_cast<std::uint8_t>(z), face, light, sky);
                };

                emit_if_visible(x + 1, y, z, FaceOrientation::PosX);
                emit_if_visible(x - 1, y, z, FaceOrientation::NegX);
                emit_if_visible(x, y + 1, z, FaceOrientation::PosY);
                emit_if_visible(x, y - 1, z, FaceOrientation::NegY);
                emit_if_visible(x, y, z + 1, FaceOrientation::PosZ);
                emit_if_visible(x, y, z - 1, FaceOrientation::NegZ);
            }
        }
    }

    for (auto& [material, scratch] : material_scratch) {
        if (scratch.indices.empty()) {
            continue;
        }
        const auto base_vertex = static_cast<std::uint32_t>(mesh.vertices.size());
        const auto first_index = static_cast<std::uint32_t>(mesh.indices.size());
        mesh.vertices.insert(mesh.vertices.end(), scratch.vertices.begin(), scratch.vertices.end());
        for (auto index : scratch.indices) {
            mesh.indices.push_back(base_vertex + index);
        }
        mesh.batches.push_back({material, first_index, static_cast<std::uint32_t>(mesh.indices.size()) - first_index});
    }

    return mesh;
}

} // namespace Balmung


