/**
 * VoxelBlock — Lua Runtime (sol2)
 * Requires sol2 header: third_party/sol2/sol/sol.hpp
 * + Lua 5.4 library linked (lua54 / lua5.4)
 * Fallback: prints warning, mods disabled
 */

#if __has_include("sol2/sol/sol.hpp")
#  define SOL_ALL_SAFETIES_ON 1
#  include "sol2/sol/sol.hpp"
#  define VB_HAS_LUA 1
#else
#  define VB_HAS_LUA 0
#endif

#include "scripting/lua_runtime.hpp"
#include "core/world.hpp"
#include "core/inventory.hpp"
#include "core/voxel.hpp"
#include "core/crafting.hpp"
#include "core/event_system.hpp"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
namespace VoxelBlock {

// ── Pimpl ─────────────────────────────────────────────────────────────────────

struct LuaRuntime::Impl {
#if VB_HAS_LUA
    sol::state lua;
#endif
};

LuaRuntime::~LuaRuntime() = default;
LuaRuntime::LuaRuntime()  = default;

// ── Init & API exposure ───────────────────────────────────────────────────────

bool LuaRuntime::init(World* world, Inventory* inv) {
    _world = world;
    _inv   = inv;
    _impl  = std::make_unique<Impl>();

#if VB_HAS_LUA
    _impl->lua.open_libraries(
        sol::lib::base, sol::lib::math, sol::lib::string,
        sol::lib::table, sol::lib::io);
    _expose_api();
    _available = true;
    std::cout << "[Lua] Runtime ready (sol2 + Lua 5.4)\n";
    return true;
#else
    std::cout << "[Lua] sol2 not found — mods disabled.\n"
              << "      Drop sol2 into third_party/sol2/sol/sol.hpp\n";
    _available = false;
    return false;
#endif
}

void LuaRuntime::_expose_api() {
#if VB_HAS_LUA
    auto& lua = _impl->lua;

    // Override print → engine console
    lua["print"] = [](sol::variadic_args va) {
        for (auto v : va) std::cout << v.as<std::string>() << " ";
        std::cout << "\n";
    };

    // ── Block registry ────────────────────────────────────────────────────────
    // register_block(name, r, g, b, hardness)
    lua["register_block"] = [](std::string name,
                                int r, int g, int b,
                                sol::optional<float> hardness)
    {
        BlockRegistry::register_block(
            name,
            Color3{(uint8_t)r, (uint8_t)g, (uint8_t)b},
            "white_cube",
            hardness.value_or(1.f));
        std::cout << "[Mod] Registered block: " << name << "\n";
    };

    // ── World operations ──────────────────────────────────────────────────────
    World* world = _world;
    lua["place_block"] = [world](int x, int y, int z, std::string bt) {
        if (world) world->place_voxel(x, y, z, bt);
    };
    lua["destroy_block"] = [world](int x, int y, int z) {
        if (world) world->destroy_voxel(x, y, z);
    };

    // ── Inventory ─────────────────────────────────────────────────────────────
    Inventory* inv = _inv;
    lua["give_item"] = [inv](std::string bt, sol::optional<int> count) {
        if (inv) inv->add_item(bt, count.value_or(1));
    };

    // ── Crafting ──────────────────────────────────────────────────────────────
    // register_recipe({slots...}, result, count)
    lua["register_recipe"] = [](sol::table tbl, std::string result,
                                 sol::optional<int> count)
    {
        std::array<std::string, 9> pattern;
        pattern.fill("");
        int i = 0;
        for (auto& kv : tbl) {
            if (i >= 9) break;
            pattern[i++] = kv.second.as<std::string>();
        }
        CraftingSystem::register_recipe(pattern, result, count.value_or(1));
    };

    // ── Events ────────────────────────────────────────────────────────────────
    // on("event_name", function(data) ... end)
    lua["on"] = [](std::string event_name, sol::function fn) {
        EventSystem::on(event_name, [fn](const EventData& d) mutable {
            try {
                // Build Lua table from EventData
#if VB_HAS_LUA
                // call with empty table (full EventData→table in Phase 4)
                fn();
#endif
            } catch (const std::exception& e) {
                std::cerr << "[Lua Event] " << e.what() << "\n";
            }
        });
    };
#endif
}

// ── Load mods ─────────────────────────────────────────────────────────────────

int LuaRuntime::load_mods(const std::string& mods_dir) {
    if (!_available) return 0;
    fs::create_directories(mods_dir);
    int loaded = 0;

#if VB_HAS_LUA
    std::vector<fs::path> files;
    for (auto& e : fs::directory_iterator(mods_dir))
        if (e.path().extension() == ".lua") files.push_back(e.path());
    std::sort(files.begin(), files.end());

    for (auto& path : files) {
        auto result = _impl->lua.safe_script_file(path.string(),
            [](lua_State*, sol::protected_function_result pfr){ return pfr; });
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "[Mod] Error in " << path.filename() << ": " << err.what() << "\n";
        } else {
            std::cout << "[Mod] Loaded: " << path.filename() << "\n";
            ++loaded;
        }
    }
    std::cout << "[Lua] " << loaded << " mod(s) loaded.\n";
#endif
    return loaded;
}

bool LuaRuntime::exec(const std::string& code, std::string& err_out) {
    if (!_available) { err_out = "Lua not available"; return false; }
#if VB_HAS_LUA
    auto res = _impl->lua.safe_script(code,
        [](lua_State*, sol::protected_function_result pfr){ return pfr; });
    if (!res.valid()) { sol::error e = res; err_out = e.what(); return false; }
    return true;
#else
    return false;
#endif
}

bool LuaRuntime::call(const std::string& fn) {
    if (!_available) return false;
#if VB_HAS_LUA
    sol::optional<sol::function> f = _impl->lua[fn];
    if (!f) return false;
    (*f)();
    return true;
#else
    return false;
#endif
}

} // namespace VoxelBlock
