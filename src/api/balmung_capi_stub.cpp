#include "api/balmung_capi.h"
#include "terrain/block_palette.hpp"
#include "terrain/chunk_manager.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

using namespace Balmung;

constexpr std::string_view kAirBlock = "air";
constexpr std::string_view kSaveHeader = "BM_WORLD_V2";

struct WorldKey {
    int x = 0;
    int y = 0;
    int z = 0;

    friend bool operator==(const WorldKey&, const WorldKey&) = default;
};

struct WorldKeyHash {
    std::size_t operator()(const WorldKey& key) const noexcept
    {
        const auto hx = static_cast<std::uint64_t>(static_cast<std::uint32_t>(key.x));
        const auto hy = static_cast<std::uint64_t>(static_cast<std::uint32_t>(key.y));
        const auto hz = static_cast<std::uint64_t>(static_cast<std::uint32_t>(key.z));
        return static_cast<std::size_t>(hx ^ (hy << 21) ^ (hz << 42));
    }
};

struct BlockRecord {
    std::string name;
    std::uint8_t r = 128;
    std::uint8_t g = 128;
    std::uint8_t b = 128;
    float hardness = 1.0f;
    bool builtin = false;
};

struct RecipeRecord {
    std::array<std::string, 9> pattern{};
    std::string result;
    int count = 1;
};

struct SavedCustomBlock {
    std::string name;
    std::uint8_t r = 128;
    std::uint8_t g = 128;
    std::uint8_t b = 128;
    float hardness = 1.0f;
};

struct SavedInventoryEntry {
    std::string block_name;
    int count = 0;
};

struct SavedVoxelOverride {
    WorldKey key{};
    std::string block_name;
};

bool copy_str(const std::string& src, char* buf, int size)
{
    if (!buf || size <= 0) {
        return false;
    }

    std::strncpy(buf, src.c_str(), static_cast<std::size_t>(size - 1));
    buf[size - 1] = '\0';
    return true;
}

std::uint32_t hash_seed(std::string_view text)
{
    std::uint32_t hash = 2166136261u;
    for (const unsigned char c : text) {
        hash ^= static_cast<std::uint32_t>(c);
        hash *= 16777619u;
    }
    return hash == 0 ? 1337u : hash;
}

bool try_parse_int(const std::string& text, int& out)
{
    try {
        std::size_t used = 0;
        out = std::stoi(text, &used);
        return used == text.size();
    } catch (...) {
        return false;
    }
}

bool try_parse_uint(const std::string& text, std::uint32_t& out)
{
    try {
        std::size_t used = 0;
        const auto value = std::stoul(text, &used);
        if (used != text.size()) {
            return false;
        }
        out = static_cast<std::uint32_t>(value);
        return true;
    } catch (...) {
        return false;
    }
}

bool try_parse_float(const std::string& text, float& out)
{
    try {
        std::size_t used = 0;
        out = std::stof(text, &used);
        return used == text.size();
    } catch (...) {
        return false;
    }
}

std::vector<std::string> split_tabs(const std::string& line)
{
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= line.size()) {
        const auto pos = line.find('\t', start);
        if (pos == std::string::npos) {
            parts.emplace_back(line.substr(start));
            break;
        }
        parts.emplace_back(line.substr(start, pos - start));
        start = pos + 1;
    }
    return parts;
}

std::array<std::string, 9> normalize_pattern(const std::array<std::string, 9>& pattern)
{
    int min_row = 3;
    int max_row = -1;
    int min_col = 3;
    int max_col = -1;

    for (int i = 0; i < 9; ++i) {
        if (pattern[static_cast<std::size_t>(i)].empty()) {
            continue;
        }

        const int row = i / 3;
        const int col = i % 3;
        min_row = std::min(min_row, row);
        max_row = std::max(max_row, row);
        min_col = std::min(min_col, col);
        max_col = std::max(max_col, col);
    }

    std::array<std::string, 9> normalized{};
    if (max_row < min_row || max_col < min_col) {
        return normalized;
    }

    for (int row = min_row; row <= max_row; ++row) {
        for (int col = min_col; col <= max_col; ++col) {
            normalized[static_cast<std::size_t>((row - min_row) * 3 + (col - min_col))] =
                pattern[static_cast<std::size_t>(row * 3 + col)];
        }
    }

    return normalized;
}

std::array<std::string, 9> read_pattern9(const char** pattern9)
{
    std::array<std::string, 9> pattern{};
    if (!pattern9) {
        return pattern;
    }

    for (int i = 0; i < 9; ++i) {
        pattern[static_cast<std::size_t>(i)] = pattern9[i] ? pattern9[i] : "";
    }
    return normalize_pattern(pattern);
}

std::string strip_lua_comments(const std::string& text)
{
    std::ostringstream cleaned;
    std::istringstream input(text);
    std::string line;
    bool first = true;
    while (std::getline(input, line)) {
        const auto comment = line.find("--");
        if (comment != std::string::npos) {
            line.erase(comment);
        }
        if (!first) {
            cleaned << '\n';
        }
        first = false;
        cleaned << line;
    }
    return cleaned.str();
}

std::vector<std::string> extract_quoted_strings(const std::string& text)
{
    static const std::regex kQuotedString(R"REGEX("([^"]*)")REGEX");

    std::vector<std::string> result;
    for (std::sregex_iterator it(text.begin(), text.end(), kQuotedString), end; it != end; ++it) {
        result.push_back((*it)[1].str());
    }
    return result;
}

struct StubEngine {
    bool running = false;
    bool menu_mode = false;
    int viewport_w = 1280;
    int viewport_h = 720;
    std::string window_title = "Balmung Engine 2.4";
    std::string world_name = "world1";
    std::string mods_dir = "mods";
    std::filesystem::path save_root = "saves";
    std::uint32_t seed = 1337u;
    std::unique_ptr<ChunkManager> chunks;
    std::unordered_map<WorldKey, std::string, WorldKeyHash> overrides;
    std::unordered_map<std::string, int> inventory;
    std::vector<BlockRecord> blocks;
    std::unordered_map<std::string, std::size_t> block_index;
    std::vector<RecipeRecord> recipes;
    std::vector<BMEventCallback> callbacks;
    int loaded_mod_count = 0;
    int chunks_drawn = 0;
    int draw_calls = 0;
    int triangles = 0;
    float frame_ms = 0.0f;
    float simulated_time_s = 0.0f;
    bool visibility_buffer = true;
    bool automatic_lod = true;
    std::uint64_t next_entity_id = 1;

    struct StubEntity {
        std::uint64_t id = 0;
        std::string name;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    std::unordered_map<std::uint64_t, StubEntity> entities;

    void register_or_update_block(std::string name, std::uint8_t r, std::uint8_t g, std::uint8_t b, float hardness, bool builtin)
    {
        if (name.empty()) {
            return;
        }

        const auto it = block_index.find(name);
        if (it != block_index.end()) {
            auto& record = blocks[it->second];
            record.r = r;
            record.g = g;
            record.b = b;
            record.hardness = hardness;
            record.builtin = record.builtin || builtin;
            return;
        }

        block_index.emplace(name, blocks.size());
        blocks.push_back(BlockRecord{
            .name = std::move(name),
            .r = r,
            .g = g,
            .b = b,
            .hardness = hardness,
            .builtin = builtin,
        });
    }

    void restore_default_blocks()
    {
        blocks.clear();
        block_index.clear();

        register_or_update_block("air", 0, 0, 0, 0.0f, true);
        register_or_update_block("grass", 92, 168, 74, 0.8f, true);
        register_or_update_block("dirt", 121, 85, 58, 0.7f, true);
        register_or_update_block("stone", 125, 125, 125, 1.6f, true);
        register_or_update_block("sand", 219, 211, 160, 0.6f, true);
        register_or_update_block("water", 64, 110, 255, 0.0f, true);
        register_or_update_block("log", 128, 94, 60, 1.0f, true);
        register_or_update_block("wood", 128, 94, 60, 1.0f, true);
        register_or_update_block("leaves", 86, 146, 78, 0.2f, true);
        register_or_update_block("coal_ore", 64, 64, 64, 2.0f, true);
        register_or_update_block("glowstone", 220, 190, 90, 0.4f, true);
        register_or_update_block("planks", 186, 153, 94, 0.8f, true);
        register_or_update_block("cobble", 110, 110, 110, 1.4f, true);
    }

    void restore_default_recipes()
    {
        recipes.clear();

        RecipeRecord wood_to_planks{};
        wood_to_planks.pattern[0] = "wood";
        wood_to_planks.result = "planks";
        wood_to_planks.count = 4;
        recipes.push_back(wood_to_planks);

        RecipeRecord log_to_planks{};
        log_to_planks.pattern[0] = "log";
        log_to_planks.result = "planks";
        log_to_planks.count = 4;
        recipes.push_back(log_to_planks);

        RecipeRecord stone_to_cobble{};
        stone_to_cobble.pattern = normalize_pattern({
            "stone", "stone", "",
            "stone", "stone", "",
            "", "", ""
        });
        stone_to_cobble.result = "cobble";
        stone_to_cobble.count = 1;
        recipes.push_back(stone_to_cobble);
    }

    void seed_default_inventory()
    {
        inventory.clear();
        inventory["grass"] = 64;
        inventory["dirt"] = 64;
        inventory["stone"] = 64;
        inventory["wood"] = 64;
        inventory["leaves"] = 64;
        inventory["sand"] = 64;
        inventory["planks"] = 64;
        inventory["cobble"] = 64;
    }

    void recreate_world(std::uint32_t new_seed)
    {
        seed = new_seed == 0 ? 1337u : new_seed;

        ChunkManager::Settings settings{};
        settings.seed = seed;
        settings.backend = RenderBackend::Vulkan;
        settings.load_radius = 5;
        settings.unload_radius = 7;
        settings.commit_budget = 32;

        chunks = std::make_unique<ChunkManager>(settings);
        chunks->update_focus(0.0f, 0.0f);
        for (int i = 0; i < 16; ++i) {
            chunks->commit_ready(64);
        }
        overrides.clear();
        simulated_time_s = 0.0f;
        update_stats(0.0f);
    }

    void reset_to_defaults(const std::string& new_world_name, std::uint32_t new_seed, bool with_starter_inventory)
    {
        world_name = new_world_name.empty() ? "world1" : new_world_name;
        restore_default_blocks();
        restore_default_recipes();
        inventory.clear();
        if (with_starter_inventory) {
            seed_default_inventory();
        }
        recreate_world(new_seed);
        menu_mode = false;
        loaded_mod_count = 0;
        running = true;
    }

    void ensure_block_exists(const std::string& block_name)
    {
        if (block_name.empty() || block_name == kAirBlock) {
            return;
        }
        if (block_index.contains(block_name)) {
            return;
        }

        register_or_update_block(block_name, 128, 128, 128, 1.0f, false);
    }

    std::optional<std::string> sample_block_name(int x, int y, int z) const
    {
        const auto override_it = overrides.find({x, y, z});
        if (override_it != overrides.end()) {
            if (override_it->second == kAirBlock) {
                return std::nullopt;
            }
            return override_it->second;
        }

        if (!chunks || y < 0 || y >= kChunkSizeY) {
            return std::nullopt;
        }

        const auto coord = world_to_chunk(x, z);
        const auto* chunk = chunks->find_chunk(coord);
        if (!chunk) {
            return std::nullopt;
        }

        const BlockId id = chunk->block_at(world_to_local_x(x), y, world_to_local_z(z));
        const auto& block = BlockPalette::get(id);
        if (block.id == static_cast<BlockId>(BlockKind::Air)) {
            return std::nullopt;
        }
        return std::string(block.name);
    }

    std::filesystem::path world_save_path(const std::string& name) const
    {
        const std::string sanitized = name.empty() ? "world1" : name;
        return save_root / (sanitized + ".bmworld");
    }

    void emit(const std::string& event_name, const std::string& payload_json) const
    {
        for (const auto callback : callbacks) {
            if (callback) {
                callback(event_name.c_str(), payload_json.c_str());
            }
        }
    }

    int load_mods_from_dir(const std::filesystem::path& dir, bool apply_runtime_effects)
    {
        restore_default_blocks();
        restore_default_recipes();

        const std::regex register_block_pattern(R"REGEX(register_block\s*\(\s*"([^"]+)"\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*([0-9]+(?:\.[0-9]+)?)\s*\))REGEX");
        const std::regex give_item_pattern(R"REGEX(give_item\s*\(\s*"([^"]+)"\s*,\s*(-?\d+)\s*\))REGEX");
        const std::regex register_recipe_pattern(R"REGEX(register_recipe\s*\(\s*\{([\s\S]*?)\}\s*,\s*"([^"]+)"\s*,\s*(-?\d+)\s*\))REGEX");

        int file_count = 0;
        if (!std::filesystem::exists(dir)) {
            loaded_mod_count = 0;
            return 0;
        }

        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".lua") {
                continue;
            }

            ++file_count;

            std::ifstream input(entry.path(), std::ios::binary);
            if (!input) {
                continue;
            }

            std::ostringstream buffer;
            buffer << input.rdbuf();
            const std::string script = strip_lua_comments(buffer.str());

            for (std::sregex_iterator it(script.begin(), script.end(), register_block_pattern), end; it != end; ++it) {
                int r = 128;
                int g = 128;
                int b = 128;
                float hardness = 1.0f;
                if (!try_parse_int((*it)[2].str(), r) ||
                    !try_parse_int((*it)[3].str(), g) ||
                    !try_parse_int((*it)[4].str(), b) ||
                    !try_parse_float((*it)[5].str(), hardness)) {
                    continue;
                }

                register_or_update_block(
                    (*it)[1].str(),
                    static_cast<std::uint8_t>(std::clamp(r, 0, 255)),
                    static_cast<std::uint8_t>(std::clamp(g, 0, 255)),
                    static_cast<std::uint8_t>(std::clamp(b, 0, 255)),
                    hardness,
                    false);
            }

            for (std::sregex_iterator it(script.begin(), script.end(), register_recipe_pattern), end; it != end; ++it) {
                RecipeRecord recipe{};
                const auto items = extract_quoted_strings((*it)[1].str());
                for (std::size_t i = 0; i < std::min<std::size_t>(items.size(), recipe.pattern.size()); ++i) {
                    recipe.pattern[i] = items[i];
                    ensure_block_exists(recipe.pattern[i]);
                }
                recipe.pattern = normalize_pattern(recipe.pattern);
                recipe.result = (*it)[2].str();
                ensure_block_exists(recipe.result);
                if (!try_parse_int((*it)[3].str(), recipe.count) || recipe.count <= 0) {
                    recipe.count = 1;
                }
                recipes.push_back(recipe);
            }

            if (apply_runtime_effects) {
                for (std::sregex_iterator it(script.begin(), script.end(), give_item_pattern), end; it != end; ++it) {
                    int count = 0;
                    if (!try_parse_int((*it)[2].str(), count) || count <= 0) {
                        continue;
                    }
                    const std::string item_name = (*it)[1].str();
                    ensure_block_exists(item_name);
                    inventory[item_name] += count;
                }
            }
        }

        loaded_mod_count = file_count;
        return file_count;
    }

    void update_stats(float dt)
    {
        chunks_drawn = chunks ? static_cast<int>(chunks->loaded_chunk_count()) : 0;
        draw_calls = std::max(1, chunks_drawn) + static_cast<int>(std::ceil(overrides.size() / 64.0));
        triangles = std::max(0, chunks_drawn) * 2048 + static_cast<int>(overrides.size()) * 12;
        if (dt > 0.0f) {
            frame_ms = std::max(dt * 1000.0f, 0.01f);
        }
    }

    bool save_world_named(const std::string& requested_name)
    {
        const std::string final_name = requested_name.empty() ? world_name : requested_name;
        const auto path = world_save_path(final_name);

        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec) {
            return false;
        }

        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) {
            return false;
        }

        output << kSaveHeader << '\n';
        output << "world_name\t" << final_name << '\n';
        output << "seed\t" << seed << '\n';
        output << "viewport\t" << viewport_w << '\t' << viewport_h << '\n';
        output << "menu_mode\t" << (menu_mode ? 1 : 0) << '\n';

        for (const auto& block : blocks) {
            if (block.builtin) {
                continue;
            }
            output << "custom_block\t"
                   << block.name << '\t'
                   << static_cast<int>(block.r) << '\t'
                   << static_cast<int>(block.g) << '\t'
                   << static_cast<int>(block.b) << '\t'
                   << block.hardness << '\n';
        }

        for (const auto& [name, count] : inventory) {
            if (count <= 0) {
                continue;
            }
            output << "inventory\t" << name << '\t' << count << '\n';
        }

        for (const auto& [key, block_name] : overrides) {
            output << "voxel\t"
                   << key.x << '\t'
                   << key.y << '\t'
                   << key.z << '\t'
                   << block_name << '\n';
        }

        if (!output.good()) {
            return false;
        }

        world_name = final_name;
        return true;
    }

    bool load_world_named(const std::string& requested_name, bool boot_mode)
    {
        const std::string final_name = requested_name.empty() ? "world1" : requested_name;
        const auto path = world_save_path(final_name);
        if (!std::filesystem::exists(path)) {
            reset_to_defaults(final_name, hash_seed(final_name), true);
            load_mods_from_dir(mods_dir, true);
            return true;
        }

        std::ifstream input(path, std::ios::binary);
        if (!input) {
            return false;
        }

        std::vector<std::string> lines;
        {
            std::string line;
            while (std::getline(input, line)) {
                lines.push_back(line);
            }
        }

        if (lines.empty() || lines.front() != kSaveHeader) {
            return false;
        }

        std::uint32_t loaded_seed = hash_seed(final_name);
        int loaded_viewport_w = viewport_w;
        int loaded_viewport_h = viewport_h;
        bool loaded_menu_mode = false;
        std::vector<SavedCustomBlock> custom_blocks;
        std::vector<SavedInventoryEntry> saved_inventory;
        std::vector<SavedVoxelOverride> saved_voxels;

        for (std::size_t i = 1; i < lines.size(); ++i) {
            const auto parts = split_tabs(lines[i]);
            if (parts.empty()) {
                continue;
            }

            if (parts[0] == "world_name" && parts.size() >= 2) {
                world_name = parts[1];
            } else if (parts[0] == "seed" && parts.size() >= 2) {
                (void)try_parse_uint(parts[1], loaded_seed);
            } else if (parts[0] == "viewport" && parts.size() >= 3) {
                (void)try_parse_int(parts[1], loaded_viewport_w);
                (void)try_parse_int(parts[2], loaded_viewport_h);
            } else if (parts[0] == "menu_mode" && parts.size() >= 2) {
                int parsed = 0;
                if (try_parse_int(parts[1], parsed)) {
                    loaded_menu_mode = parsed != 0;
                }
            } else if (parts[0] == "custom_block" && parts.size() >= 6) {
                int r = 128;
                int g = 128;
                int b = 128;
                float hardness = 1.0f;
                if (!try_parse_int(parts[2], r) ||
                    !try_parse_int(parts[3], g) ||
                    !try_parse_int(parts[4], b) ||
                    !try_parse_float(parts[5], hardness)) {
                    continue;
                }
                custom_blocks.push_back(SavedCustomBlock{
                    .name = parts[1],
                    .r = static_cast<std::uint8_t>(std::clamp(r, 0, 255)),
                    .g = static_cast<std::uint8_t>(std::clamp(g, 0, 255)),
                    .b = static_cast<std::uint8_t>(std::clamp(b, 0, 255)),
                    .hardness = hardness,
                });
            } else if (parts[0] == "inventory" && parts.size() >= 3) {
                int count = 0;
                if (!try_parse_int(parts[2], count)) {
                    continue;
                }
                saved_inventory.push_back(SavedInventoryEntry{
                    .block_name = parts[1],
                    .count = count,
                });
            } else if (parts[0] == "voxel" && parts.size() >= 5) {
                int x = 0;
                int y = 0;
                int z = 0;
                if (!try_parse_int(parts[1], x) ||
                    !try_parse_int(parts[2], y) ||
                    !try_parse_int(parts[3], z)) {
                    continue;
                }
                saved_voxels.push_back(SavedVoxelOverride{
                    .key = {x, y, z},
                    .block_name = parts[4],
                });
            }
        }

        reset_to_defaults(final_name, loaded_seed, false);
        load_mods_from_dir(mods_dir, false);
        viewport_w = loaded_viewport_w;
        viewport_h = loaded_viewport_h;
        menu_mode = loaded_menu_mode;
        if (!world_name.empty() && world_name != final_name) {
            world_name = final_name;
        }

        for (const auto& block : custom_blocks) {
            register_or_update_block(block.name, block.r, block.g, block.b, block.hardness, false);
        }

        inventory.clear();
        for (const auto& item : saved_inventory) {
            if (item.count <= 0) {
                continue;
            }
            ensure_block_exists(item.block_name);
            inventory[item.block_name] = item.count;
        }

        overrides.clear();
        for (const auto& item : saved_voxels) {
            if (item.block_name != kAirBlock) {
                ensure_block_exists(item.block_name);
            }
            overrides[item.key] = item.block_name;
        }

        running = true;
        update_stats(boot_mode ? 0.0f : 1.0f / 60.0f);
        return true;
    }
};

std::unordered_map<BMHandle, std::unique_ptr<StubEngine>> g_engines;
BMHandle g_next_handle = 1;
std::mutex g_mutex;

StubEngine* get_engine(BMHandle handle)
{
    const auto it = g_engines.find(handle);
    return it == g_engines.end() ? nullptr : it->second.get();
}

} // namespace

extern "C" {

BMHandle bm_engine_create(void)
{
    std::lock_guard lock(g_mutex);
    const BMHandle handle = g_next_handle++;
    g_engines[handle] = std::make_unique<StubEngine>();
    return handle;
}

void bm_engine_destroy(BMHandle h)
{
    std::lock_guard lock(g_mutex);
    g_engines.erase(h);
}

bool bm_engine_init_headless(BMHandle h, int viewport_w, int viewport_h)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine) {
        return false;
    }

    engine->viewport_w = std::max(1, viewport_w);
    engine->viewport_h = std::max(1, viewport_h);
    engine->window_title = "Balmung Engine 2.4";
    return engine->load_world_named(engine->world_name, true);
}

bool bm_engine_init_windowed(BMHandle h, int w, int hi, const char* title)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine) {
        return false;
    }

    engine->viewport_w = std::max(1, w);
    engine->viewport_h = std::max(1, hi);
    engine->window_title = (title && *title) ? title : "Balmung Engine 2.4";
    return engine->load_world_named(engine->world_name, true);
}

void bm_engine_tick(BMHandle h, float dt)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine || !engine->running || !engine->chunks) {
        return;
    }

    const auto tick_begin = std::chrono::steady_clock::now();
    const float step = dt > 0.0f ? dt : 1.0f / 60.0f;
    engine->simulated_time_s += step;

    engine->chunks->update_focus(0.0f, 0.0f);
    engine->chunks->commit_ready(64);

    const auto tick_end = std::chrono::steady_clock::now();
    const auto elapsed_ms = std::chrono::duration<float, std::milli>(tick_end - tick_begin).count();
    engine->update_stats(step);
    engine->frame_ms = std::max(engine->frame_ms, elapsed_ms);
}

bool bm_engine_running(BMHandle h)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    return engine && engine->running;
}

void bm_engine_quit(BMHandle h)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine) {
        return;
    }
    engine->save_world_named(engine->world_name);
    engine->running = false;
}

void bm_engine_set_menu_mode(BMHandle h, bool enabled)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (engine) {
        engine->menu_mode = enabled;
    }
}

void bm_ui_begin(BMHandle) {}
void bm_ui_add_rect(BMHandle, float, float, float, float, float, float, float, float, int) {}
void bm_ui_add_text(BMHandle, float, float, float, float, float, float, float, int, const char*) {}

uint32_t bm_engine_scene_texture(BMHandle)
{
    return 0;
}

void bm_engine_resize_viewport(BMHandle h, int w, int hi)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine) {
        return;
    }
    engine->viewport_w = std::max(1, w);
    engine->viewport_h = std::max(1, hi);
}

void bm_renderer_use_visibility_buffer(BMHandle h, bool enabled)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (engine) {
        engine->visibility_buffer = enabled;
    }
}

void bm_renderer_set_automatic_lod(BMHandle h, bool enabled)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (engine) {
        engine->automatic_lod = enabled;
    }
}

int bm_renderer_pipeline_architecture(BMHandle h)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    return engine && engine->visibility_buffer ? 1 : 0;
}

uint64_t bm_entity_create(BMHandle h, const char* name)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine) {
        return 0;
    }

    const auto id = engine->next_entity_id++;
    engine->entities.emplace(id, StubEngine::StubEntity{id, name ? name : ""});
    return id;
}

bool bm_entity_destroy(BMHandle h, uint64_t entity_id)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    return engine && engine->entities.erase(entity_id) > 0;
}

int bm_entity_count(BMHandle h)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    return engine ? static_cast<int>(engine->entities.size()) : 0;
}

bool bm_entity_set_position(BMHandle h, uint64_t entity_id, float x, float y, float z)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine) {
        return false;
    }

    auto it = engine->entities.find(entity_id);
    if (it == engine->entities.end()) {
        return false;
    }

    it->second.x = x;
    it->second.y = y;
    it->second.z = z;
    return true;
}

uint32_t bm_world_get_seed(BMHandle h)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    return engine ? engine->seed : 0u;
}

int bm_world_chunk_count(BMHandle h)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    return (engine && engine->chunks) ? static_cast<int>(engine->chunks->loaded_chunk_count()) : 0;
}

bool bm_world_place_voxel(BMHandle h, int x, int y, int z, const char* block_type)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine || !block_type || !*block_type) {
        return false;
    }

    const std::string name = block_type;
    engine->ensure_block_exists(name);
    engine->overrides[{x, y, z}] = name;
    engine->update_stats(0.0f);

    std::ostringstream payload;
    payload << "{\"x\":" << x
            << ",\"y\":" << y
            << ",\"z\":" << z
            << ",\"block\":\"" << name << "\"}";
    engine->emit("on_place", payload.str());
    return true;
}

bool bm_world_destroy_voxel(BMHandle h, int x, int y, int z)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine) {
        return false;
    }

    engine->overrides[{x, y, z}] = std::string(kAirBlock);
    engine->update_stats(0.0f);

    std::ostringstream payload;
    payload << "{\"x\":" << x
            << ",\"y\":" << y
            << ",\"z\":" << z << "}";
    engine->emit("on_destroy", payload.str());
    return true;
}

bool bm_world_get_voxel(BMHandle h, int x, int y, int z, char* out_block_type, int buf_size)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine) {
        return false;
    }

    const auto block_name = engine->sample_block_name(x, y, z);
    if (!block_name) {
        return false;
    }
    return copy_str(*block_name, out_block_type, buf_size);
}

bool bm_world_save(BMHandle h, const char* name)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine) {
        return false;
    }
    return engine->save_world_named(name ? name : engine->world_name);
}

bool bm_world_load(BMHandle h, const char* name)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine) {
        return false;
    }
    return engine->load_world_named(name ? name : engine->world_name, false);
}

void bm_block_register(BMHandle h, const char* name, uint8_t r, uint8_t g, uint8_t b, float hardness)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine || !name || !*name) {
        return;
    }
    engine->register_or_update_block(name, r, g, b, hardness, false);
}

int bm_block_count(BMHandle h)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    return engine ? static_cast<int>(engine->blocks.size()) : 0;
}

bool bm_block_get_name(BMHandle h, int index, char* name_buf, int buf_size)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine || index < 0 || index >= static_cast<int>(engine->blocks.size())) {
        return false;
    }
    return copy_str(engine->blocks[static_cast<std::size_t>(index)].name, name_buf, buf_size);
}

bool bm_block_get_color(BMHandle h, const char* name, uint8_t* r, uint8_t* g, uint8_t* b)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine || !name || !r || !g || !b) {
        return false;
    }

    const auto it = engine->block_index.find(name);
    if (it == engine->block_index.end()) {
        return false;
    }

    const auto& block = engine->blocks[it->second];
    *r = block.r;
    *g = block.g;
    *b = block.b;
    return true;
}

void bm_inv_add_item(BMHandle h, const char* block_type, int count)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine || !block_type || count <= 0) {
        return;
    }
    engine->ensure_block_exists(block_type);
    engine->inventory[block_type] += count;
}

bool bm_inv_remove_item(BMHandle h, const char* block_type, int count)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine || !block_type || count <= 0) {
        return false;
    }

    auto it = engine->inventory.find(block_type);
    if (it == engine->inventory.end() || it->second < count) {
        return false;
    }

    it->second -= count;
    if (it->second <= 0) {
        engine->inventory.erase(it);
    }
    return true;
}

int bm_inv_item_count(BMHandle h, const char* block_type)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine || !block_type) {
        return 0;
    }
    const auto it = engine->inventory.find(block_type);
    return it == engine->inventory.end() ? 0 : it->second;
}

bool bm_craft_can_craft(BMHandle h, const char** pattern9)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine) {
        return false;
    }

    const auto requested = read_pattern9(pattern9);
    for (const auto& recipe : engine->recipes) {
        if (recipe.pattern != requested) {
            continue;
        }

        std::unordered_map<std::string, int> required;
        for (const auto& item : recipe.pattern) {
            if (!item.empty()) {
                ++required[item];
            }
        }

        bool has_all = true;
        for (const auto& [name, count] : required) {
            const auto it = engine->inventory.find(name);
            if (it == engine->inventory.end() || it->second < count) {
                has_all = false;
                break;
            }
        }

        if (has_all) {
            return true;
        }
    }
    return false;
}

bool bm_craft_do_craft(BMHandle h, const char** pattern9, char* result_block, int buf_size, int* result_count)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine) {
        return false;
    }

    const auto requested = read_pattern9(pattern9);
    for (const auto& recipe : engine->recipes) {
        if (recipe.pattern != requested) {
            continue;
        }

        std::unordered_map<std::string, int> required;
        for (const auto& item : recipe.pattern) {
            if (!item.empty()) {
                ++required[item];
            }
        }

        for (const auto& [name, count] : required) {
            const auto it = engine->inventory.find(name);
            if (it == engine->inventory.end() || it->second < count) {
                return false;
            }
        }

        for (const auto& [name, count] : required) {
            auto& slot = engine->inventory[name];
            slot -= count;
            if (slot <= 0) {
                engine->inventory.erase(name);
            }
        }

        engine->ensure_block_exists(recipe.result);
        engine->inventory[recipe.result] += recipe.count;

        if (result_count) {
            *result_count = recipe.count;
        }
        copy_str(recipe.result, result_block, buf_size);

        std::ostringstream payload;
        payload << "{\"result\":\"" << recipe.result << "\",\"count\":" << recipe.count << "}";
        engine->emit("on_craft", payload.str());
        return true;
    }

    return false;
}

int bm_lua_load_mods(BMHandle h, const char* dir)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine) {
        return 0;
    }

    const std::string requested_dir = (dir && *dir) ? dir : engine->mods_dir;
    engine->mods_dir = requested_dir;
    return engine->load_mods_from_dir(requested_dir, false);
}

bool bm_lua_exec(BMHandle, const char*, char* error_buf, int buf_size)
{
    if (error_buf && buf_size > 0) {
        copy_str(
            "Lua VM is not linked in fallback runtime mode. Declarative mod metadata still loads via bm_lua_load_mods().",
            error_buf,
            buf_size);
    }
    return false;
}

void bm_events_subscribe(BMHandle h, BMEventCallback cb)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine || !cb) {
        return;
    }
    engine->callbacks.push_back(cb);
}

void bm_events_unsubscribe(BMHandle h, BMEventCallback cb)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine) {
        return;
    }
    engine->callbacks.erase(
        std::remove(engine->callbacks.begin(), engine->callbacks.end(), cb),
        engine->callbacks.end());
}

void bm_stats_get(BMHandle h, int* chunks_drawn, int* draw_calls, int* triangles, float* frame_ms)
{
    std::lock_guard lock(g_mutex);
    auto* engine = get_engine(h);
    if (!engine) {
        return;
    }

    if (chunks_drawn) {
        *chunks_drawn = engine->chunks_drawn;
    }
    if (draw_calls) {
        *draw_calls = engine->draw_calls;
    }
    if (triangles) {
        *triangles = engine->triangles;
    }
    if (frame_ms) {
        *frame_ms = engine->frame_ms;
    }
}

} // extern "C"
