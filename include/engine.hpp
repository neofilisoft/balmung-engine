#pragma once
/**
 * VoxelBlock — Engine
 * Top-level container. One instance = one game/editor session.
 * The C-API wraps this as an opaque handle.
 */

#include "core/world.hpp"
#include "core/inventory.hpp"
#include "core/crafting.hpp"
#include "core/save_manager.hpp"
#include "core/player.hpp"
#include "renderer/camera.hpp"
#include "renderer/renderer.hpp"
#include "scripting/lua_runtime.hpp"
#include <memory>
#include <string>
#include <functional>
#include <vector>

namespace VoxelBlock {

class Window;

struct EngineConfig {
    enum class RuntimeProfile {
        GameEngine,
        Framework,
    };

    int         viewport_w    = 1280;
    int         viewport_h    = 720;
    bool        headless      = false;
    std::string world_name    = "world1";
    int         render_dist   = 5;
    std::string mods_dir      = "mods";
    std::string saves_dir     = "saves";
    RuntimeProfile runtime_profile = RuntimeProfile::GameEngine;

    bool is_framework_mode() const { return runtime_profile == RuntimeProfile::Framework; }
};

class Engine {
public:
    explicit Engine(const EngineConfig& cfg = {});
    ~Engine();

    // init: creates window if not headless, boots all subsystems
    bool init();
    void tick(float dt);
    bool running() const;
    void quit();

    // Subsystem access
    World&       world()       { return _world; }
    Inventory&   inventory()   { return _inv;   }
    Camera&      camera()      { return _cam;   }
    Renderer&    renderer()    { return _renderer; }
    LuaRuntime&  lua()         { return _lua;   }
    SaveManager& save_manager(){ return _save;  }
    Player&      player()      { return _player; }

    const World&     world()    const { return _world; }
    const EngineConfig& config()const { return _cfg;   }
