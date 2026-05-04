#pragma once

#include "core/types.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

namespace Balmung {

struct VoxelData {
    int x = 0;
    int y = 0;
    int z = 0;
    std::string block_type;
};

struct VoxelPosHash {
    std::size_t operator()(const std::tuple<int, int, int>& p) const noexcept
    {
        const auto x = static_cast<std::uint64_t>(static_cast<std::uint32_t>(std::get<0>(p)));
        const auto y = static_cast<std::uint64_t>(static_cast<std::uint32_t>(std::get<1>(p)));
        const auto z = static_cast<std::uint64_t>(static_cast<std::uint32_t>(std::get<2>(p)));
        return static_cast<std::size_t>(x ^ (y << 21) ^ (z << 42));
    }
};

struct BlockDefinition {
    std::string name;
    Color3 color{};
    std::string mesh_asset = "white_cube";
    float hardness = 1.0f;
    std::unordered_set<std::string> tags;

    [[nodiscard]] bool has_tag(const std::string& tag) const
    {
        return tags.find(tag) != tags.end();
    }
};

class BlockRegistry {
public:
    static void register_defaults();
    static void register_block(std::string name, Color3 color, std::string mesh_asset, float hardness);
    [[nodiscard]] static const BlockDefinition* get_block(const std::string& name);
    [[nodiscard]] static std::vector<std::string> all_blocks();
    static void clear();
};

} // namespace Balmung

