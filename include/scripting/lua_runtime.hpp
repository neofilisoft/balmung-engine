#pragma once
/**
 * Balmung — Lua Scripting (sol2)
 * Phase 2: replaces Python mod_loader.
 * Mirrors Python mod_loader.py API surface exactly:
 *   register_block, place_block, destroy_block,
 *   give_item, register_recipe, on(), print
 *
 * sol2 usage: header-only, drop sol/sol.hpp into third_party/sol2/
 * Fallback: if sol2 not present, mod loading prints a warning.
 */

#include <string>
#include <memory>
#include <vector>

namespace Balmung {

class World;
class Inventory;

class LuaRuntime {
public:
    LuaRuntime();
    ~LuaRuntime();

    // init must be called before load_mods
    // world/inv pointers are not owned; must outlive LuaRuntime
    bool init(World* world, Inventory* inv);

    // Load all .lua files from mods_dir
    int load_mods(const std::string& mods_dir = "mods");

    // Execute a script string directly (for editor console)
    bool exec(const std::string& code, std::string& error_out);

    // Call a named Lua function (returns false if not found)
    bool call(const std::string& fn_name);

    bool available() const { return _available; }

private:
    bool        _available = false;
    World*      _world     = nullptr;
    Inventory*  _inv       = nullptr;

    struct Impl;
    std::unique_ptr<Impl> _impl;

    void _expose_api();
};

} // namespace Balmung


