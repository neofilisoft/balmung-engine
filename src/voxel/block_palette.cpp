#include "terrain/block_palette.hpp"
#include <stdexcept>

namespace Balmung {

namespace {
constexpr std::array<BlockDefinition, BlockPalette::kBlockCount> kBlocks{{
    {static_cast<BlockId>(BlockKind::Air),       "air",       Block_Transparent,                           0, 0},
    {static_cast<BlockId>(BlockKind::Grass),     "grass",     Block_Solid | Block_Surface,                 0, 1},
    {static_cast<BlockId>(BlockKind::Dirt),      "dirt",      Block_Solid,                                 0, 2},
    {static_cast<BlockId>(BlockKind::Stone),     "stone",     Block_Solid,                                 0, 3},
    {static_cast<BlockId>(BlockKind::Sand),      "sand",      Block_Solid | Block_Surface,                 0, 4},
    {static_cast<BlockId>(BlockKind::Water),     "water",     Block_Transparent | Block_Fluid,             0, 5},
    {static_cast<BlockId>(BlockKind::Log),       "log",       Block_Solid,                                 0, 6},
    {static_cast<BlockId>(BlockKind::Leaves),    "leaves",    Block_Transparent,                           0, 7},
    {static_cast<BlockId>(BlockKind::CoalOre),   "coal_ore",  Block_Solid,                                 0, 8},
    {static_cast<BlockId>(BlockKind::Glowstone), "glowstone", Block_Solid | Block_Emissive,               14, 9},
}};
}

const BlockDefinition& BlockPalette::get(BlockId id) {
    if (id >= kBlocks.size()) {
        return kBlocks[0];
    }
    return kBlocks[id];
}

bool BlockPalette::is_solid(BlockId id) {
    return (get(id).flags & Block_Solid) != 0;
}

bool BlockPalette::is_transparent(BlockId id) {
    return (get(id).flags & Block_Transparent) != 0;
}

bool BlockPalette::is_opaque(BlockId id) {
    return is_solid(id) && !is_transparent(id);
}

bool BlockPalette::emits_light(BlockId id) {
    return (get(id).flags & Block_Emissive) != 0;
}

std::uint8_t BlockPalette::emission(BlockId id) {
    return get(id).emission;
}

std::array<BlockDefinition, BlockPalette::kBlockCount> BlockPalette::all() {
    return kBlocks;
}

} // namespace Balmung


