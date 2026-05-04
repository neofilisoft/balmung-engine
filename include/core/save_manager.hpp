#pragma once

#include "core/inventory.hpp"
#include "core/world.hpp"

#include <filesystem>
#include <string>

namespace Balmung {

class SaveManager {
public:
    explicit SaveManager(std::filesystem::path root = "saves");

    bool save(const std::string& name, const World& world, const Inventory& inventory) const;
    bool load_into_engine(const std::string& name, World& world, Inventory& inventory, int render_dist = 5) const;

private:
    std::filesystem::path _root;
    [[nodiscard]] std::filesystem::path world_file(const std::string& name) const;
};

} // namespace Balmung

