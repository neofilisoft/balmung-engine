#include "terrain/chunk_data.hpp"
#include <algorithm>
#include <cassert>

namespace Balmung {

ChunkData::ChunkData(ChunkCoord coord)
    : coord_(coord),
      blocks_(kChunkVoxelCount, static_cast<BlockId>(BlockKind::Air)),
      block_light_((kChunkVoxelCount + 1u) / 2u, 0u),
      sky_light_((kChunkVoxelCount + 1u) / 2u, 0u) {}

bool ChunkData::contains(int x, int y, int z) const {
    return x >= 0 && x < kChunkSizeX && z >= 0 && z < kChunkSizeZ && y >= 0 && y < kChunkSizeY;
}

std::size_t ChunkData::linear_index(int x, int y, int z) {
    return static_cast<std::size_t>((y * kChunkSizeZ + z) * kChunkSizeX + x);
}

BlockId ChunkData::block_at(int x, int y, int z) const {
    if (!contains(x, y, z)) {
        return static_cast<BlockId>(BlockKind::Air);
    }
    return blocks_[linear_index(x, y, z)];
}

void ChunkData::set_block(int x, int y, int z, BlockId block) {
    if (!contains(x, y, z)) {
        return;
    }
    blocks_[linear_index(x, y, z)] = block;
}

std::uint8_t ChunkData::read_nibble(const std::vector<std::uint8_t>& packed, std::size_t index) {
    const auto byte = packed[index / 2u];
    return static_cast<std::uint8_t>((index % 2u == 0u) ? (byte & 0x0fu) : ((byte >> 4u) & 0x0fu));
}

void ChunkData::write_nibble(std::vector<std::uint8_t>& packed, std::size_t index, std::uint8_t value) {
    value &= 0x0fu;
    auto& byte = packed[index / 2u];
    if (index % 2u == 0u) {
        byte = static_cast<std::uint8_t>((byte & 0xf0u) | value);
    } else {
        byte = static_cast<std::uint8_t>((byte & 0x0fu) | (value << 4u));
    }
}

std::uint8_t ChunkData::block_light_at(int x, int y, int z) const {
    if (!contains(x, y, z)) {
        return 0;
    }
    return read_nibble(block_light_, linear_index(x, y, z));
}

std::uint8_t ChunkData::sky_light_at(int x, int y, int z) const {
    if (!contains(x, y, z)) {
        return 0;
    }
    return read_nibble(sky_light_, linear_index(x, y, z));
}

void ChunkData::set_block_light(int x, int y, int z, std::uint8_t value) {
    if (!contains(x, y, z)) {
        return;
    }
    write_nibble(block_light_, linear_index(x, y, z), value);
}

void ChunkData::set_sky_light(int x, int y, int z, std::uint8_t value) {
    if (!contains(x, y, z)) {
        return;
    }
    write_nibble(sky_light_, linear_index(x, y, z), value);
}

bool ChunkData::is_air(int x, int y, int z) const {
    return block_at(x, y, z) == static_cast<BlockId>(BlockKind::Air);
}

bool ChunkData::is_opaque(int x, int y, int z) const {
    return BlockPalette::is_opaque(block_at(x, y, z));
}

void ChunkData::clear_lighting() {
    std::fill(block_light_.begin(), block_light_.end(), 0u);
    std::fill(sky_light_.begin(), sky_light_.end(), 0u);
}

void ChunkData::fill(BlockId block) {
    std::fill(blocks_.begin(), blocks_.end(), block);
}

} // namespace Balmung


