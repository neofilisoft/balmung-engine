#include "core/crafting.hpp"

#include "core/event_system.hpp"

#include <algorithm>
#include <unordered_map>

namespace Balmung {
namespace {

std::vector<CraftingRecipe>& recipe_store()
{
    static std::vector<CraftingRecipe> value;
    return value;
}

std::array<std::string, 9> normalize(std::array<std::string, 9> pattern)
{
    for (auto& slot : pattern) {
        if (slot == "air") {
            slot.clear();
        }
    }
    return pattern;
}

const CraftingRecipe* find_recipe(const std::array<std::string, 9>& pattern)
{
    const auto normalized = normalize(pattern);
    for (const auto& recipe : recipe_store()) {
        if (recipe.pattern == normalized) {
            return &recipe;
        }
    }
    return nullptr;
}

} // namespace

void CraftingSystem::register_defaults()
{
    if (!recipe_store().empty()) {
        return;
    }

    std::array<std::string, 9> wood{};
    wood[0] = "wood";
    register_recipe(wood, "planks", 4);

    std::array<std::string, 9> cobble{};
    cobble[0] = "stone";
    cobble[1] = "stone";
    register_recipe(cobble, "cobble", 1);
}

void CraftingSystem::register_recipe(std::array<std::string, 9> pattern, std::string result, int count)
{
    if (result.empty() || count <= 0) {
        return;
    }

    CraftingRecipe recipe;
    recipe.pattern = normalize(std::move(pattern));
    recipe.result = CraftingResult{std::move(result), count};
    recipe_store().push_back(std::move(recipe));
}

bool CraftingSystem::can_craft(const std::array<std::string, 9>& pattern, const Inventory& inventory)
{
    register_defaults();
    const auto* recipe = find_recipe(pattern);
    if (!recipe) {
        return false;
    }

    std::unordered_map<std::string, int> required;
    for (const auto& item : recipe->pattern) {
        if (!item.empty()) {
            ++required[item];
        }
    }

    for (const auto& [item, count] : required) {
        if (inventory.item_count(item) < count) {
            return false;
        }
    }
    return true;
}

std::optional<CraftingResult> CraftingSystem::craft(const std::array<std::string, 9>& pattern, Inventory& inventory)
{
    register_defaults();
    const auto* recipe = find_recipe(pattern);
    if (!recipe || !can_craft(pattern, inventory)) {
        return std::nullopt;
    }

    for (const auto& item : recipe->pattern) {
        if (!item.empty()) {
            inventory.remove_item(item, 1);
        }
    }
    inventory.add_item(recipe->result.block_type, recipe->result.count);

    EventData data;
    data.set("result", recipe->result.block_type);
    data.set("count", recipe->result.count);
    EventSystem::emit("on_craft", data);
    return recipe->result;
}

const std::vector<CraftingRecipe>& CraftingSystem::recipes()
{
    register_defaults();
    return recipe_store();
}

void CraftingSystem::clear()
{
    recipe_store().clear();
}

} // namespace Balmung
