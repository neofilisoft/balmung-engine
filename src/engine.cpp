#else
#  define VB_HAS_GLFW 0
#endif

#include "engine.hpp"
#include "renderer/window.hpp"
#include "core/event_system.hpp"
#include <iostream>
#include <sstream>

namespace VoxelBlock {

Engine::Engine(const EngineConfig& cfg)
    : _cfg(cfg), _save(cfg.saves_dir), _player(&_inv) {}

Engine::~Engine() {
    _renderer.shutdown();
}

bool Engine::init() {
    // ── Core systems ──────────────────────────────────────────────────────────
    BlockRegistry::register_defaults();
    CraftingSystem::register_defaults();

    // ── World (load or generate) ──────────────────────────────────────────────
    bool loaded = true;
    if (!_cfg.is_framework_mode()) {
        loaded = _save.load_into_engine(_cfg.world_name, _world, _inv,
                                        _cfg.render_dist);
        if (!loaded) {
            // Give default starting items
            for (auto& bt : {"grass","dirt","stone","wood","leaves","sand","planks","cobble"})
                _inv.add_item(bt, 64);
        }
    }
    std::cout << "[Engine] World '" << _cfg.world_name
              << "' — seed=" << _world.seed()
              << " chunks=" << _world.loaded_chunk_count() << "\n";

    // ── Camera ────────────────────────────────────────────────────────────────
    _cam.position = {8.f, 25.f, 8.f};
    _cam.update_vectors();

    // ── Renderer ─────────────────────────────────────────────────────────────
    if (!_cfg.headless) {
        WindowConfig wcfg;
        wcfg.width  = _cfg.viewport_w;
        wcfg.height = _cfg.viewport_h;
        wcfg.title  = _cfg.is_framework_mode() ? "VoxelBlock Framework v2.0" : "VoxelBlock Engine v2.0";
        _window = std::make_unique<Window>(wcfg);

        // Wire window callbacks → engine
        _window->on_key = [this](int key, int action) {
#if VB_HAS_GLFW
            if (action == GLFW_PRESS || action == GLFW_REPEAT) {
                if (key == GLFW_KEY_ESCAPE) {
                    if (_player.inventory_open())
                        _player.set_inventory_open(false);
                    else quit();
                }
                if (key == GLFW_KEY_E)
                    _player.set_inventory_open(!_player.inventory_open());
                if (key == GLFW_KEY_F5)
                    _save.save(_cfg.world_name, _world, _inv);
                // Number keys
                if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9)
                    _player.set_block_index(key - GLFW_KEY_1);
            }
#endif
        };

        _window->on_mouse_button = [this](int btn, int action) {
#if VB_HAS_GLFW
            if (_player.inventory_open()) return;
            if (action == GLFW_PRESS) {
                if (btn == GLFW_MOUSE_BUTTON_LEFT)
                    _player.handle_input(Player::InputAction::PlaceBlock, &_world);
                else if (btn == GLFW_MOUSE_BUTTON_RIGHT)
                    _player.handle_input(Player::InputAction::BreakBlock, &_world);
            }
#endif
        };

        _window->on_scroll = [this](double, double yo) {
            if (yo > 0) _player.handle_input(Player::InputAction::ScrollUp);
            else        _player.handle_input(Player::InputAction::ScrollDown);
        };

        _window->on_resize = [this](int w, int h) { _renderer.resize(w, h); };
        _window->set_cursor_locked(true);
    }

    if (!_renderer.init(_cfg.viewport_w, _cfg.viewport_h))
        return false;

    // ── Lua mods ──────────────────────────────────────────────────────────────
    if (!_cfg.is_framework_mode()) {
        _lua.init(&_world, &_inv);
        _lua.load_mods(_cfg.mods_dir);
    }

    // ── Event bridge ──────────────────────────────────────────────────────────
    if (!_cfg.is_framework_mode())
        _register_event_bridges();

    _running = true;
    std::cout << "[Engine] Init complete.\n";
    return true;
}

void Engine::_register_event_bridges() {
    // Forward engine events to C# bridge callbacks
    auto forward = [this](const std::string& name, const EventData& d) {
        if (_bridges.empty()) return;
        // Serialize EventData to minimal JSON string
        std::ostringstream oss;
        oss << "{\"event\":\"" << name << "\"";
        // (Full serialization Phase 4; basic version here)
        oss << "}";
        for (auto& cb : _bridges) cb(name, oss.str());
    };

    for (auto& evt : {"on_place","on_destroy","on_craft"}) {
        EventSystem::on(evt, [evt, forward](const EventData& d){ forward(evt, d); });
    }
}

void Engine::add_event_bridge(EventBridgeCb cb) {
    _bridges.push_back(std::move(cb));
}

void Engine::tick(float dt) {
    if (!_running) return;

    if (_window) {
        if (_window->should_close()) { quit(); return; }
        _window->poll_events();
        _handle_input(dt);
    }

    if (!_cfg.is_framework_mode()) {
        // Chunk streaming
        static float chunk_timer = 0.f;
        chunk_timer += dt;
        if (chunk_timer >= 2.f) {
            chunk_timer = 0.f;
            _world.update_chunks(_cam.position.x, _cam.position.z);
        }

        // Auto-save
        _auto_save_timer += dt;
        if (_auto_save_timer >= 300.f) {
            _auto_save_timer = 0.f;
            _save.save(_cfg.world_name, _world, _inv);
        }
    }

    // Render
    _renderer.begin_frame();
    _renderer.render_world(_world, _cam);
    _renderer.end_frame();

    if (_window) _window->swap_buffers();
}

void Engine::_handle_input(float dt) {
    if (!_window || _player.inventory_open()) return;
#if VB_HAS_GLFW
    Vec3 move{};
    if (_window->key_down(GLFW_KEY_W)) { move.x += _cam.front.x; move.z += _cam.front.z; }
    if (_window->key_down(GLFW_KEY_S)) { move.x -= _cam.front.x; move.z -= _cam.front.z; }
    if (_window->key_down(GLFW_KEY_A)) { move.x -= _cam.right.x; move.z -= _cam.right.z; }
    if (_window->key_down(GLFW_KEY_D)) { move.x += _cam.right.x; move.z += _cam.right.z; }
    if (_window->key_down(GLFW_KEY_SPACE))      move.y += 1.f;
    if (_window->key_down(GLFW_KEY_LEFT_SHIFT)) move.y -= 1.f;
    _cam.move(move, dt);

    if (_window->cursor_locked()) {
        _cam.process_mouse(_window->mouse_dx(), _window->mouse_dy());
    }

    // Sync camera position to player
    _player.set_position(_cam.position);
#endif
}

bool Engine::running() const { return _running; }
void Engine::quit() {
    if (!_cfg.is_framework_mode())
        _save.save(_cfg.world_name, _world, _inv);
    _running = false;
    std::cout << "[Engine] Quit.\n";
}

} // namespace VoxelBlock
