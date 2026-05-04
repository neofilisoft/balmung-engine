#pragma once
/**
 * Balmung — Engine
 * Top-level container. One instance = one game/editor session.
 * The C-API wraps this as an opaque handle.
 */

#include "core/world.hpp"
#include "core/inventory.hpp"
#include "core/crafting.hpp"
#include "core/entity_manager.hpp"
#include "core/game_loop.hpp"
#include "core/save_manager.hpp"
#include "core/player.hpp"
#include "renderer/camera.hpp"
#include "renderer/renderer.hpp"
#include "runtime/runtime_pipeline.hpp"
#include "scripting/lua_runtime.hpp"
#include <memory>
#include <string>
#include <functional>
#include <vector>

namespace Balmung {

class Window;

struct EngineConfig {
    int         viewport_w    = 1280;
    int         viewport_h    = 720;
    bool        headless      = false;
    std::string world_name    = "world1";
    int         render_dist   = 5;
    std::string mods_dir      = "mods";
    std::string saves_dir     = "saves";
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
    EntityManager& entities()  { return _entities; }
    Runtime::RuntimePipeline& runtime_pipeline() { return _runtime; }

    const World&     world()    const { return _world; }
    const EngineConfig& config()const { return _cfg;   }

    // Event bridge (for C-API callbacks → C# delegates)
    using EventBridgeCb = std::function<void(const std::string& name, const std::string& json)>;
    void add_event_bridge(EventBridgeCb cb);

    // Render-to-texture output (editor uses this)
    uint32_t scene_texture_id() const { return _renderer.scene_texture_id(); }
    void resize_viewport(int w, int h) { _renderer.resize(w, h); }

    struct OverlayRect {
        float x = 0.f;
        float y = 0.f;
        float w = 0.f;
        float h = 0.f;
        float r = 1.f;
        float g = 1.f;
        float b = 1.f;
        float a = 1.f;
        int layer = 0;
    };

    struct OverlayText {
        float x = 0.f;
        float y = 0.f;
        float pixel_size = 16.f;
        float r = 1.f;
        float g = 1.f;
        float b = 1.f;
        float a = 1.f;
        int layer = 0;
        std::string text;
    };

    void set_menu_mode(bool enabled);
    bool menu_mode() const { return _menu_mode; }
    void clear_overlay();
    void add_overlay_rect(float x, float y, float w, float h, float r, float g, float b, float a, int layer);
    void add_overlay_text(float x, float y, float pixel_size, float r, float g, float b, float a, int layer, std::string text);

private:
    EngineConfig   _cfg;
    World          _world;
    Inventory      _inv;
    Camera         _cam;
    Renderer       _renderer;
    LuaRuntime     _lua;
    SaveManager    _save;
    Player         _player;
    EntityManager  _entities;
    Runtime::RuntimePipeline _runtime;
    GameLoop       _loop;

    std::unique_ptr<Window> _window;  // null when headless
    bool _running = false;
    bool _menu_mode = false;

    float _auto_save_timer = 0.f;
    std::vector<OverlayRect> _overlay_rects;
    std::vector<OverlayText> _overlay_texts;

    void _handle_input(float dt);
    void _register_event_bridges();

    std::vector<EventBridgeCb> _bridges;
};

} // namespace Balmung



