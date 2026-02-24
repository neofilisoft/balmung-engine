#pragma once
/**
 * VoxelBlock — C API (extern "C")
 * Phase 3: P/Invoke bridge for C# Editor
 * All functions use plain C types only.
 * The engine instance is a singleton accessed by handle (int64).
 *
 * Naming convention: vb_<subsystem>_<action>
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// ── Engine lifecycle ──────────────────────────────────────────────────────────
typedef int64_t VBHandle;   // opaque engine instance

VBHandle vb_engine_create  (void);
void     vb_engine_destroy (VBHandle h);

// init headless (no window) — for editor
bool     vb_engine_init_headless(VBHandle h, int viewport_w, int viewport_h);
// init with window — for standalone game
bool     vb_engine_init_windowed(VBHandle h, int w, int hi, const char* title);

void     vb_engine_tick    (VBHandle h, float dt);
bool     vb_engine_running (VBHandle h);
void     vb_engine_quit    (VBHandle h);

// Renderer: returns OpenGL texture ID of rendered scene (for editor viewport)
uint32_t vb_engine_scene_texture(VBHandle h);
void     vb_engine_resize_viewport(VBHandle h, int w, int hi);

// ── World ─────────────────────────────────────────────────────────────────────
uint32_t vb_world_get_seed   (VBHandle h);
int      vb_world_chunk_count(VBHandle h);

bool     vb_world_place_voxel  (VBHandle h, int x, int y, int z, const char* block_type);
bool     vb_world_destroy_voxel(VBHandle h, int x, int y, int z);
// Returns block_type string at pos, or empty string if none
// Caller provides buf[64]
bool     vb_world_get_voxel    (VBHandle h, int x, int y, int z,
                                 char* out_block_type, int buf_size);

bool     vb_world_save(VBHandle h, const char* name);
bool     vb_world_load(VBHandle h, const char* name);

// ── Block Registry ────────────────────────────────────────────────────────────
void     vb_block_register (VBHandle h, const char* name,
                             uint8_t r, uint8_t g, uint8_t b,
                             float hardness);
int      vb_block_count    (VBHandle h);
// Fills name_buf (caller provides); returns false if idx out of range
bool     vb_block_get_name (VBHandle h, int idx, char* name_buf, int buf_size);
// Fills r,g,b
bool     vb_block_get_color(VBHandle h, const char* name,
                             uint8_t* r, uint8_t* g, uint8_t* b);

// ── Inventory ─────────────────────────────────────────────────────────────────
void     vb_inv_add_item   (VBHandle h, const char* block_type, int count);
bool     vb_inv_remove_item(VBHandle h, const char* block_type, int count);
int      vb_inv_item_count (VBHandle h, const char* block_type);

// ── Crafting ──────────────────────────────────────────────────────────────────
// pattern: 9-element array of C strings (3x3 grid)
bool     vb_craft_can_craft(VBHandle h, const char** pattern9);
// Returns true + fills result_block/count if successful
bool     vb_craft_do_craft (VBHandle h, const char** pattern9,
                             char* result_block, int buf_size, int* result_count);

// ── Lua scripting ─────────────────────────────────────────────────────────────
int      vb_lua_load_mods  (VBHandle h, const char* mods_dir);
bool     vb_lua_exec       (VBHandle h, const char* code,
                             char* error_buf, int buf_size);

// ── Events ────────────────────────────────────────────────────────────────────
typedef void (*VBEventCallback)(const char* event_name, const char* payload_json);
void     vb_events_subscribe  (VBHandle h, VBEventCallback cb);
void     vb_events_unsubscribe (VBHandle h, VBEventCallback cb);

// ── Stats ─────────────────────────────────────────────────────────────────────
void     vb_stats_get(VBHandle h,
                      int* chunks_drawn, int* draw_calls,
                      int* triangles,   float* frame_ms);

#ifdef __cplusplus
} // extern "C"
#endif
