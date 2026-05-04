#pragma once

#include "core/inventory.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace Balmung {

struct CraftingResult {
    std::string block_type;
    int count = 1;
};

struct CraftingRecipe {
    std::array<std::string, 9> pattern{};
    CraftingResult result;
};

class CraftingSystem {
public:
    static void register_defaults();
    static void register_recipe(std::array<std::string, 9> pattern, std::string result, int count);
    [[nodiscard]] static bool can_craft(const std::array<std::string, 9>& pattern, const Inventory& inventory);
    [[nodiscard]] static std::optional<CraftingResult> craft(const std::array<std::string, 9>& pattern, Inventory& inventory);
    [[nodiscard]] static const std::vector<CraftingRecipe>& recipes();
    static void clear();
};

} // namespace Balmung

