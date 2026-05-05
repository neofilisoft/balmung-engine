#include "core/save_manager.hpp"

#include <fstream>

namespace Balmung {

SaveManager::SaveManager(std::filesystem::path root)
    : _root(std::move(root))
{
}

bool SaveManager::save(const std::string& name, const World& world, const Inventory& inventory) const
{
    std::filesystem::create_directories(_root);
    std::ofstream out(world_file(name), std::ios::binary);
    if (!out) {
        return false;
    }

    out << "BM_WORLD_V3\n";
    out << "seed\t" << world.seed() << "\n";
    for (const auto& [item, count] : inventory.items()) {
        out << "inv\t" << item << "\t" << count << "\n";
    }
    world.for_each_voxel([&](const VoxelData& voxel) {
        out << "voxel\t" << voxel.x << "\t" << voxel.y << "\t" << voxel.z << "\t" << voxel.block_type << "\n";
    });
    return true;
}

bool SaveManager::load_into_engine(const std::string& name, World& world, Inventory& inventory, int render_dist) const
{
    std::ifstream in(world_file(name), std::ios::binary);
    if (!in) {
        world.update_chunks(0.0f, 0.0f, render_dist);
        return false;
    }

    world.clear();
    inventory.clear();

    std::string header;
    std::getline(in, header);
    if (header != "BM_WORLD_V3") {
        world.update_chunks(0.0f, 0.0f, render_dist);
        return false;
    }

    std::string tag;
    while (in >> tag) {
        if (tag == "seed") {
            std::uint32_t seed = 1337;
            in >> seed;
            world.set_seed(seed);
        } else if (tag == "inv") {
            std::string item;
            int count = 0;
            in >> item >> count;
            inventory.add_item(item, count);
        } else if (tag == "voxel") {
            int x = 0;
            int y = 0;
            int z = 0;
            std::string block;
            in >> x >> y >> z >> block;
            world.place_voxel(x, y, z, block);
        } else {
            std::string rest;
            std::getline(in, rest);
        }
    }

    world.update_chunks(0.0f, 0.0f, render_dist);
    return true;
}

std::filesystem::path SaveManager::world_file(const std::string& name) const
{
    const auto safe_name = name.empty() ? std::string("world1") : name;
    return _root / (safe_name + ".bmworld");
}

} // namespace Balmung

