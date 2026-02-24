/**
 * VoxelBlock — C-API Implementation
 * Maps VBHandle → Engine instance
 * Thread safety: not re-entrant; call from main thread only
 */

#include "api/voxelblock_capi.h"
#include "engine.hpp"

#include <unordered_map>
#include <memory>
#include <mutex>
#include <cstring>
#include <iostream>

// ── Handle Registry ───────────────────────────────────────────────────────────

namespace {

std::unordered_map<VBHandle, std::unique_ptr<VoxelBlock::Engine>> g_engines;
VBHandle g_next_handle = 1;
std::mutex g_mutex;

VoxelBlock::Engine* get(VBHandle h) {
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

VBHandle vb_engine_create() {
    std::lock_guard<std::mutex> lk(g_mutex);
    VBHandle h = g_next_handle++;
    // Engine created lazily in init_headless / init_windowed
    g_engines[h] = nullptr;
    return h;
}

void vb_engine_destroy(VBHandle h) {
    std::lock_guard<std::mutex> lk(g_mutex);
    g_engines.erase(h);
}

bool vb_engine_init_headless(VBHandle h, int vw, int vh) {
    std::lock_guard<std::mutex> lk(g_mutex);
    auto it = g_engines.find(h); if (it == g_engines.end()) return false;
    VoxelBlock::EngineConfig cfg;
    cfg.headless    = true;
    cfg.viewport_w  = vw;
    cfg.viewport_h  = vh;
    it->second = std::make_unique<VoxelBlock::Engine>(cfg);
    return it->second->init();
}

bool vb_engine_init_windowed(VBHandle h, int w, int hi, const char* title) {
    std::lock_guard<std::mutex> lk(g_mutex);
    auto it = g_engines.find(h); if (it == g_engines.end()) return false;
    VoxelBlock::EngineConfig cfg;
    cfg.headless    = false;
    cfg.viewport_w  = w;
    cfg.viewport_h  = hi;
    it->second = std::make_unique<VoxelBlock::Engine>(cfg);
    return it->second->init();
}

void vb_engine_tick(VBHandle h, float dt) {
    auto* e = get(h); if (e) e->tick(dt);
}

bool vb_engine_running(VBHandle h) {
    auto* e = get(h); return e && e->running();
}

void vb_engine_quit(VBHandle h) {
    auto* e = get(h); if (e) e->quit();
}

uint32_t vb_engine_scene_texture(VBHandle h) {
    auto* e = get(h); return e ? e->scene_texture_id() : 0;
}

void vb_engine_resize_viewport(VBHandle h, int w, int hi) {
    auto* e = get(h); if (e) e->resize_viewport(w, hi);
}

// ── World ─────────────────────────────────────────────────────────────────────

uint32_t vb_world_get_seed(VBHandle h) {
    auto* e = get(h); return e ? e->world().seed() : 0;
}

int vb_world_chunk_count(VBHandle h) {
    auto* e = get(h); return e ? e->world().loaded_chunk_count() : 0;
}

bool vb_world_place_voxel(VBHandle h, int x, int y, int z, const char* bt) {
    auto* e = get(h); if (!e) return false;
    return e->world().place_voxel(x, y, z, bt) != nullptr;
}

bool vb_world_destroy_voxel(VBHandle h, int x, int y, int z) {
    auto* e = get(h); return e && e->world().destroy_voxel(x, y, z);
}

bool vb_world_get_voxel(VBHandle h, int x, int y, int z, char* buf, int sz) {
    auto* e = get(h); if (!e) return false;
    auto* v = e->world().get_voxel(x, y, z);
    if (!v) return false;
    return copy_str(v->block_type, buf, sz);
}

bool vb_world_save(VBHandle h, const char* name) {
    auto* e = get(h); if (!e) return false;
    return e->save_manager().save(name, e->world(), e->inventory());
}

bool vb_world_load(VBHandle h, const char* name) {
    auto* e = get(h); if (!e) return false;
    return e->save_manager().load_into_engine(name, e->world(), e->inventory());
}

// ── Block Registry ────────────────────────────────────────────────────────────

void vb_block_register(VBHandle, const char* name,
                        uint8_t r, uint8_t g, uint8_t b, float hardness)
{
    VoxelBlock::BlockRegistry::register_block(
        name, {r,g,b}, "white_cube", hardness);
}

int vb_block_count(VBHandle) {
    return (int)VoxelBlock::BlockRegistry::all_blocks().size();
}

bool vb_block_get_name(VBHandle, int idx, char* buf, int sz) {
    auto blocks = VoxelBlock::BlockRegistry::all_blocks();
    if (idx < 0 || idx >= (int)blocks.size()) return false;
    return copy_str(blocks[idx], buf, sz);
}

bool vb_block_get_color(VBHandle, const char* name,
                         uint8_t* r, uint8_t* g, uint8_t* b)
{
    auto* def = VoxelBlock::BlockRegistry::get_block(name);
    if (!def) return false;
    *r = def->color.r; *g = def->color.g; *b = def->color.b;
    return true;
}

// ── Inventory ─────────────────────────────────────────────────────────────────

void vb_inv_add_item(VBHandle h, const char* bt, int count) {
    auto* e = get(h); if (e) e->inventory().add_item(bt, count);
}

bool vb_inv_remove_item(VBHandle h, const char* bt, int count) {
    auto* e = get(h); return e && e->inventory().remove_item(bt, count);
}

int vb_inv_item_count(VBHandle h, const char* bt) {
    auto* e = get(h); return e ? e->inventory().item_count(bt) : 0;
}

// ── Crafting ──────────────────────────────────────────────────────────────────

bool vb_craft_can_craft(VBHandle h, const char** pattern9) {
    auto* e = get(h); if (!e) return false;
    std::array<std::string,9> grid;
    for (int i=0; i<9; ++i) grid[i] = pattern9[i] ? pattern9[i] : "";
    return VoxelBlock::CraftingSystem::can_craft(grid, e->inventory());
}

bool vb_craft_do_craft(VBHandle h, const char** pattern9,
                        char* result_block, int sz, int* result_count)
{
    auto* e = get(h); if (!e) return false;
    std::array<std::string,9> grid;
    for (int i=0; i<9; ++i) grid[i] = pattern9[i] ? pattern9[i] : "";
    auto res = VoxelBlock::CraftingSystem::craft(grid, e->inventory());
    if (!res) return false;
    copy_str(res->block_type, result_block, sz);
    if (result_count) *result_count = res->count;
    return true;
}

// ── Lua ───────────────────────────────────────────────────────────────────────

int vb_lua_load_mods(VBHandle h, const char* dir) {
    auto* e = get(h); return e ? e->lua().load_mods(dir) : 0;
}

bool vb_lua_exec(VBHandle h, const char* code, char* err_buf, int sz) {
    auto* e = get(h); if (!e) return false;
    std::string err;
    bool ok = e->lua().exec(code, err);
    if (!ok && err_buf) copy_str(err, err_buf, sz);
    return ok;
}

// ── Events ────────────────────────────────────────────────────────────────────

static std::vector<VBEventCallback> g_callbacks;

void vb_events_subscribe(VBHandle h, VBEventCallback cb) {
    auto* e = get(h); if (!e || !cb) return;
    g_callbacks.push_back(cb);
    e->add_event_bridge([cb](const std::string& name, const std::string& json){
        cb(name.c_str(), json.c_str());
    });
}

void vb_events_unsubscribe(VBHandle, VBEventCallback cb) {
    g_callbacks.erase(
        std::remove(g_callbacks.begin(), g_callbacks.end(), cb),
        g_callbacks.end());
}

// ── Stats ─────────────────────────────────────────────────────────────────────

void vb_stats_get(VBHandle h, int* cd, int* dc, int* tri, float* ms) {
    auto* e = get(h); if (!e) return;
    auto& s = e->renderer().stats();
    if (cd)  *cd  = s.chunks_drawn;
    if (dc)  *dc  = s.draw_calls;
    if (tri) *tri = s.triangles;
    if (ms)  *ms  = s.frame_ms;
}

} // extern "C"
