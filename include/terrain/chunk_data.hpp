#pragma once

#include "terrain/voxel_types.hpp"
#include "terrain/block_palette.hpp"
#include <vector>

namespace Balmung {

class ChunkData {
public:
    explicit ChunkData(ChunkCoord coord = {});

    const ChunkCoord& coord() const { return coord_; }

    bool contains(int x, int y, int z) const;
    BlockId block_at(int x, int y, int z) const;
    void set_block(int x, int y, int z, BlockId block);

    std::uint8_t block_light_at(int x, int y, int z) const;
    std::uint8_t sky_light_at(int x, int y, int z) const;
    void set_block_light(int x, int y, int z, std::uint8_t value);
    void set_sky_light(int x, int y, int z, std::uint8_t value);

    bool is_air(int x, int y, int z) const;
    bool is_opaque(int x, int y, int z) const;

    void clear_lighting();
    void fill(BlockId block);

    const std::vector<BlockId>& raw_blocks() const { return blocks_; }

    static std::size_t linear_index(int x, int y, int z);

private:
    ChunkCoord coord_;
    std::vector<BlockId> blocks_;
    std::vector<std::uint8_t> block_light_;
    std::vector<std::uint8_t> sky_light_;

    static std::uint8_t read_nibble(const std::vector<std::uint8_t>& packed, std::size_t index);
    static void write_nibble(std::vector<std::uint8_t>& packed, std::size_t index, std::uint8_t value);
};

} // namespace Balmung


