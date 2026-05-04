#pragma once
/**
 * Balmung — C API (extern "C")
 * Phase 3: P/Invoke bridge for C# Editor
 * All functions use plain C types only.
 * The engine instance is a singleton accessed by handle (int64).
 *
 * Naming convention: bm_<subsystem>_<action>
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// ── Engine lifecycle ──────────────────────────────────────────────────────────
typedef int64_t BMHandle;   // opaque engine instance

BMHandle bm_engine_create  (void);
void     bm_engine_destroy (BMHandle h);

// init headless (no window) — for editor
bool     bm_engine_init_headless(BMHandle h, int viewport_w, int viewport_h);
// init with window — for standalone game
bool     bm_engine_init_windowed(BMHandle h, int w, int hi, const char* title);

void     bm_engine_tick    (BMHandle h, float dt);
bool     bm_engine_running (BMHandle h);
void     bm_engine_quit    (BMHandle h);
void     bm_engine_set_menu_mode(BMHandle h, bool enabled);

// Minimal native overlay bridge (Phase 1 surface)
void     bm_ui_begin       (BMHandle handle);
void     bm_ui_add_rect    (BMHandle handle, float x, float y, float w, float height, float r, float g, float b, float a, int layer);
void     bm_ui_add_text    (BMHandle handle, float x, float y, float pixel_size, float r, float g, float b, float a, int layer, const char* text);

// Renderer: returns OpenGL texture ID of rendered scene (for editor viewport)
uint32_t bm_engine_scene_texture(BMHandle h);
void     bm_engine_resize_viewport(BMHandle h, int w, int hi);

void     bm_renderer_use_visibility_buffer(BMHandle h, bool enabled);
void     bm_renderer_set_automatic_lod(BMHandle h, bool enabled);
int      bm_renderer_pipeline_architecture(BMHandle h);

uint64_t bm_entity_create(BMHandle h, const char* name);
bool     bm_entity_destroy(BMHandle h, uint64_t entity_id);
int      bm_entity_count(BMHandle h);
bool     bm_entity_set_position(BMHandle h, uint64_t entity_id, float x, float y, float z);

// ── World ─────────────────────────────────────────────────────────────────────
uint32_t bm_world_get_seed   (BMHandle h);
int      bm_world_chunk_count(BMHandle h);

bool     bm_world_place_voxel  (BMHandle h, int x, int y, int z, const char* block_type);
bool     bm_world_destroy_voxel(BMHandle h, int x, int y, int z);
// Returns block_type string at pos, or empty string if none
// Caller provides buf[64]
bool     bm_world_get_voxel    (BMHandle h, int x, int y, int z,
                                 char* out_block_type, int buf_size);

bool     bm_world_save(BMHandle h, const char* name);
bool     bm_world_load(BMHandle h, const char* name);

// ── Block Registry ────────────────────────────────────────────────────────────
void     bm_block_register (BMHandle h, const char* name,
                             uint8_t r, uint8_t g, uint8_t b,
                             float hardness);
int      bm_block_count    (BMHandle h);
// Fills name_buf (caller provides); returns false if idx out of range
bool     bm_block_get_name (BMHandle h, int idx, char* name_buf, int buf_size);
// Fills r,g,b
bool     bm_block_get_color(BMHandle h, const char* name,
                             uint8_t* r, uint8_t* g, uint8_t* b);

// ── Inventory ─────────────────────────────────────────────────────────────────
void     bm_inv_add_item   (BMHandle h, const char* block_type, int count);
bool     bm_inv_remove_item(BMHandle h, const char* block_type, int count);
int      bm_inv_item_count (BMHandle h, const char* block_type);

// ── Crafting ──────────────────────────────────────────────────────────────────
// pattern: 9-element array of C strings (3x3 grid)
bool     bm_craft_can_craft(BMHandle h, const char** pattern9);
// Returns true + fills result_block/count if successful
bool     bm_craft_do_craft (BMHandle h, const char** pattern9,
                             char* result_block, int buf_size, int* result_count);

// ── Lua scripting ─────────────────────────────────────────────────────────────
int      bm_lua_load_mods  (BMHandle h, const char* mods_dir);
bool     bm_lua_exec       (BMHandle h, const char* code,
                             char* error_buf, int buf_size);

// ── Events ────────────────────────────────────────────────────────────────────
typedef void (*BMEventCallback)(const char* event_name, const char* payload_json);
void     bm_events_subscribe  (BMHandle h, BMEventCallback cb);
void     bm_events_unsubscribe (BMHandle h, BMEventCallback cb);

// ── Stats ─────────────────────────────────────────────────────────────────────
void     bm_stats_get(BMHandle h,
                      int* chunks_drawn, int* draw_calls,
                      int* triangles,   float* frame_ms);

#ifdef __cplusplus
} // extern "C"
#endif









