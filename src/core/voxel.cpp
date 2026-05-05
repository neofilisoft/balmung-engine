#include "core/voxel.hpp"

#include <algorithm>
#include <unordered_map>

namespace Balmung {
namespace {

std::unordered_map<std::string, BlockDefinition>& registry()
{
    static std::unordered_map<std::string, BlockDefinition> value;
    return value;
}

} // namespace

void BlockRegistry::register_defaults()
{
    if (!registry().empty()) {
        return;
    }

    register_block("air", {0, 0, 0}, "none", 0.0f);
    registry()["air"].tags.erase("solid");
    registry()["air"].tags.insert("empty");

    register_block("grass", {86, 168, 74}, "white_cube", 0.6f);
    register_block("dirt", {116, 78, 48}, "white_cube", 0.5f);
    register_block("stone", {126, 130, 135}, "white_cube", 1.5f);
    register_block("sand", {214, 197, 128}, "white_cube", 0.4f);
    register_block("wood", {129, 83, 47}, "white_cube", 1.2f);
    register_block("leaves", {62, 142, 69}, "white_cube", 0.2f);
    register_block("planks", {172, 119, 67}, "white_cube", 0.8f);
    register_block("cobble", {101, 104, 108}, "white_cube", 1.8f);
    register_block("water", {56, 118, 210}, "white_cube", 100.0f);
    registry()["water"].tags.erase("solid");
    registry()["water"].tags.insert("liquid");
}

void BlockRegistry::register_block(std::string name, Color3 color, std::string mesh_asset, float hardness)
{
    if (name.empty()) {
        return;
    }

    BlockDefinition definition;
    definition.name = std::move(name);
    definition.color = color;
    definition.mesh_asset = std::move(mesh_asset);
    definition.hardness = hardness;
    definition.tags.insert("solid");
    registry()[definition.name] = std::move(definition);
}

const BlockDefinition* BlockRegistry::get_block(const std::string& name)
{
    register_defaults();
    const auto it = registry().find(name);
    return it == registry().end() ? nullptr : &it->second;
}

std::vector<std::string> BlockRegistry::all_blocks()
{
    register_defaults();
    std::vector<std::string> names;
    names.reserve(registry().size());
    for (const auto& [name, _] : registry()) {
        names.push_back(name);
    }
    std::sort(names.begin(), names.end());
    return names;
}

void BlockRegistry::clear()
{
    registry().clear();
}

} // namespace Balmung

