#pragma once

#include "terrain/voxel_types.hpp"
#include <array>
#include <string_view>

namespace Balmung {

enum class BlockKind : BlockId {
    Air = 0,
    Grass = 1,
    Dirt = 2,
    Stone = 3,
    Sand = 4,
    Water = 5,
    Log = 6,
    Leaves = 7,
    CoalOre = 8,
    Glowstone = 9
};

enum BlockFlags : std::uint32_t {
    Block_None = 0,
    Block_Solid = 1u << 0,
    Block_Transparent = 1u << 1,
    Block_Emissive = 1u << 2,
    Block_Fluid = 1u << 3,
    Block_Surface = 1u << 4
};

struct BlockDefinition {
    BlockId id = 0;
    std::string_view name;
    std::uint32_t flags = Block_None;
    std::uint8_t emission = 0;
    std::uint8_t material_layer = 0;
};

class BlockPalette {
public:
    static constexpr std::size_t kBlockCount = 10;

    static const BlockDefinition& get(BlockId id);
    static bool is_solid(BlockId id);
    static bool is_transparent(BlockId id);
    static bool is_opaque(BlockId id);
    static bool emits_light(BlockId id);
    static std::uint8_t emission(BlockId id);
    static std::array<BlockDefinition, kBlockCount> all();
};

} // namespace Balmung


