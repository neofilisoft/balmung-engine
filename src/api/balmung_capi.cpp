/**
 * Balmung — C-API Implementation
 * Maps BMHandle → Engine instance
 * Thread safety: not re-entrant; call from main thread only
 */

#include "api/balmung_capi.h"
#include "engine.hpp"

#include <algorithm>
#include <array>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <cstring>
#include <iostream>

// ── Handle Registry ───────────────────────────────────────────────────────────

namespace {

std::unordered_map<BMHandle, std::unique_ptr<Balmung::Engine>> g_engines;
BMHandle g_next_handle = 1;
std::mutex g_mutex;

Balmung::Engine* get(BMHandle h) {
    auto it = g_engines.find(h);
    return it != g_engines.end() ? it->second.get() : nullptr;
}

bool copy_str(const std::string& src, char* buf, int sz) {
    if (!buf || sz <= 0) return false;
    std::strncpy(buf, src.c_str(), (size_t)sz - 1);
    buf[sz-1] = '\0';
    return true;
}

} // anon ns

// ── Lifecycle ─────────────────────────────────────────────────────────────────

extern "C" {

BMHandle bm_engine_create() {
    std::lock_guard<std::mutex> lk(g_mutex);
    BMHandle h = g_next_handle++;
    // Engine created lazily in init_headless / init_windowed
    g_engines[h] = nullptr;
    return h;
}

void bm_engine_destroy(BMHandle h) {
    std::lock_guard<std::mutex> lk(g_mutex);
    g_engines.erase(h);
}

bool bm_engine_init_headless(BMHandle h, int vw, int vh) {
    std::lock_guard<std::mutex> lk(g_mutex);
    auto it = g_engines.find(h); if (it == g_engines.end()) return false;
    Balmung::EngineConfig cfg;
    cfg.headless    = true;
    cfg.viewport_w  = vw;
    cfg.viewport_h  = vh;
    it->second = std::make_unique<Balmung::Engine>(cfg);
    return it->second->init();
}

bool bm_engine_init_windowed(BMHandle h, int w, int hi, const char* title) {
    std::lock_guard<std::mutex> lk(g_mutex);
    auto it = g_engines.find(h); if (it == g_engines.end()) return false;
    Balmung::EngineConfig cfg;
    cfg.headless    = false;
    cfg.viewport_w  = w;
    cfg.viewport_h  = hi;
    it->second = std::make_unique<Balmung::Engine>(cfg);
    return it->second->init();
}

void bm_engine_tick(BMHandle h, float dt) {
    auto* e = get(h); if (e) e->tick(dt);
}

bool bm_engine_running(BMHandle h) {
    auto* e = get(h); return e && e->running();
}

void bm_engine_quit(BMHandle h) {
    auto* e = get(h); if (e) e->quit();
}

void bm_engine_set_menu_mode(BMHandle h, bool enabled) {
    auto* e = get(h); if (e) e->set_menu_mode(enabled);
}

void bm_ui_begin(BMHandle h) {
    auto* e = get(h); if (e) e->clear_overlay();
}

void bm_ui_add_rect(BMHandle h, float x, float y, float w, float height, float r, float g, float b, float a, int layer) {
    auto* e = get(h); if (e) e->add_overlay_rect(x, y, w, height, r, g, b, a, layer);
}

void bm_ui_add_text(BMHandle h, float x, float y, float pixel_size, float r, float g, float b, float a, int layer, const char* text) {
    auto* e = get(h); if (e) e->add_overlay_text(x, y, pixel_size, r, g, b, a, layer, text ? text : "");
}

uint32_t bm_engine_scene_texture(BMHandle h) {
    auto* e = get(h); return e ? e->scene_texture_id() : 0;
}

void bm_engine_resize_viewport(BMHandle h, int w, int hi) {
    auto* e = get(h); if (e) e->resize_viewport(w, hi);
}

void bm_renderer_use_visibility_buffer(BMHandle h, bool enabled) {
    auto* e = get(h); if (e) e->renderer().use_visibility_buffer(enabled);
}

void bm_renderer_set_automatic_lod(BMHandle h, bool enabled) {
    auto* e = get(h); if (e) e->renderer().set_automatic_lod(enabled);
}

int bm_renderer_pipeline_architecture(BMHandle h) {
    auto* e = get(h);
    return e ? static_cast<int>(e->renderer().pipeline_settings().architecture) : 0;
}

static uint64_t pack_entity(Balmung::ECS::Entity entity) {
    return (static_cast<uint64_t>(entity.generation) << 32) | entity.index;
}

static Balmung::ECS::Entity unpack_entity(uint64_t id) {
    return Balmung::ECS::Entity{
        static_cast<Balmung::ECS::EntityIndex>(id & 0xffffffffu),
        static_cast<Balmung::ECS::EntityGeneration>(id >> 32),
    };
}

uint64_t bm_entity_create(BMHandle h, const char* name) {
    auto* e = get(h); if (!e) return 0;
    return pack_entity(e->entities().create_entity(name ? name : ""));
}

bool bm_entity_destroy(BMHandle h, uint64_t entity_id) {
    auto* e = get(h); return e && e->entities().destroy_entity(unpack_entity(entity_id));
}

int bm_entity_count(BMHandle h) {
    auto* e = get(h); return e ? static_cast<int>(e->entities().alive_count()) : 0;
}

bool bm_entity_set_position(BMHandle h, uint64_t entity_id, float x, float y, float z) {
    auto* e = get(h); if (!e) return false;
    auto entity = unpack_entity(entity_id);
    if (!e->entities().is_alive(entity)) return false;
    auto& registry = e->entities().registry();
    auto* tx = registry.try_get<Balmung::ECS::TransformComponent>(entity);
    if (!tx) {
        tx = &registry.emplace_or_replace<Balmung::ECS::TransformComponent>(entity);
    }
    tx->position = {x, y, z};
    return true;
}

// ── World ─────────────────────────────────────────────────────────────────────

uint32_t bm_world_get_seed(BMHandle h) {
    auto* e = get(h); return e ? e->world().seed() : 0;
}

int bm_world_chunk_count(BMHandle h) {
    auto* e = get(h); return e ? e->world().loaded_chunk_count() : 0;
}

bool bm_world_place_voxel(BMHandle h, int x, int y, int z, const char* bt) {
    auto* e = get(h); if (!e) return false;
    return e->world().place_voxel(x, y, z, bt) != nullptr;
}

bool bm_world_destroy_voxel(BMHandle h, int x, int y, int z) {
    auto* e = get(h); return e && e->world().destroy_voxel(x, y, z);
}

bool bm_world_get_voxel(BMHandle h, int x, int y, int z, char* buf, int sz) {
    auto* e = get(h); if (!e) return false;
    auto* v = e->world().get_voxel(x, y, z);
    if (!v) return false;
    return copy_str(v->block_type, buf, sz);
}

bool bm_world_save(BMHandle h, const char* name) {
    auto* e = get(h); if (!e) return false;
    return e->save_manager().save(name, e->world(), e->inventory());
}

bool bm_world_load(BMHandle h, const char* name) {
    auto* e = get(h); if (!e) return false;
    return e->save_manager().load_into_engine(name, e->world(), e->inventory());
}

// ── Block Registry ────────────────────────────────────────────────────────────

void bm_block_register(BMHandle, const char* name,
                        uint8_t r, uint8_t g, uint8_t b, float hardness)
{
    Balmung::BlockRegistry::register_block(
        name, {r,g,b}, "white_cube", hardness);
}

int bm_block_count(BMHandle) {
    return (int)Balmung::BlockRegistry::all_blocks().size();
}

bool bm_block_get_name(BMHandle, int idx, char* buf, int sz) {
    auto blocks = Balmung::BlockRegistry::all_blocks();
    if (idx < 0 || idx >= (int)blocks.size()) return false;
    return copy_str(blocks[idx], buf, sz);
}

bool bm_block_get_color(BMHandle, const char* name,
                         uint8_t* r, uint8_t* g, uint8_t* b)
{
    auto* def = Balmung::BlockRegistry::get_block(name);
    if (!def) return false;
    *r = def->color.r; *g = def->color.g; *b = def->color.b;
    return true;
}

// ── Inventory ─────────────────────────────────────────────────────────────────

void bm_inv_add_item(BMHandle h, const char* bt, int count) {
    auto* e = get(h); if (e) e->inventory().add_item(bt, count);
}

bool bm_inv_remove_item(BMHandle h, const char* bt, int count) {
    auto* e = get(h); return e && e->inventory().remove_item(bt, count);
}

int bm_inv_item_count(BMHandle h, const char* bt) {
    auto* e = get(h); return e ? e->inventory().item_count(bt) : 0;
}

// ── Crafting ──────────────────────────────────────────────────────────────────

bool bm_craft_can_craft(BMHandle h, const char** pattern9) {
    auto* e = get(h); if (!e) return false;
    std::array<std::string,9> grid;
    for (int i=0; i<9; ++i) grid[i] = pattern9[i] ? pattern9[i] : "";
    return Balmung::CraftingSystem::can_craft(grid, e->inventory());
}

bool bm_craft_do_craft(BMHandle h, const char** pattern9,
                        char* result_block, int sz, int* result_count)
{
    auto* e = get(h); if (!e) return false;
    std::array<std::string,9> grid;
    for (int i=0; i<9; ++i) grid[i] = pattern9[i] ? pattern9[i] : "";
    auto res = Balmung::CraftingSystem::craft(grid, e->inventory());
    if (!res) return false;
    copy_str(res->block_type, result_block, sz);
    if (result_count) *result_count = res->count;
    return true;
}

// ── Lua ───────────────────────────────────────────────────────────────────────

int bm_lua_load_mods(BMHandle h, const char* dir) {
    auto* e = get(h); return e ? e->lua().load_mods(dir) : 0;
}

bool bm_lua_exec(BMHandle h, const char* code, char* err_buf, int sz) {
    auto* e = get(h); if (!e) return false;
    std::string err;
    bool ok = e->lua().exec(code, err);
    if (!ok && err_buf) copy_str(err, err_buf, sz);
    return ok;
}

// ── Events ────────────────────────────────────────────────────────────────────

static std::vector<BMEventCallback> g_callbacks;

void bm_events_subscribe(BMHandle h, BMEventCallback cb) {
    auto* e = get(h); if (!e || !cb) return;
    g_callbacks.push_back(cb);
    e->add_event_bridge([cb](const std::string& name, const std::string& json){
        cb(name.c_str(), json.c_str());
    });
}

void bm_events_unsubscribe(BMHandle, BMEventCallback cb) {
    g_callbacks.erase(
        std::remove(g_callbacks.begin(), g_callbacks.end(), cb),
        g_callbacks.end());
}

// ── Stats ─────────────────────────────────────────────────────────────────────

void bm_stats_get(BMHandle h, int* cd, int* dc, int* tri, float* ms) {
    auto* e = get(h); if (!e) return;
    auto& s = e->renderer().stats();
    if (cd)  *cd  = s.chunks_drawn;
    if (dc)  *dc  = s.draw_calls;
    if (tri) *tri = s.triangles;
    if (ms)  *ms  = s.frame_ms;
}

} // extern "C"






