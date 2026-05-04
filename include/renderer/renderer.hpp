#pragma once
/**
 * Balmung - Main Renderer
 * Owns shader programs, chunk mesh cache, and draw-call dispatch.
 * The editor accesses render output via render-to-texture
 * exposed through the C API (balmung_capi.h).
 */

#include "shader.hpp"
#include "camera.hpp"
#include "mesh.hpp"
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace Balmung {

class World;
class Window;

struct RenderStats {
    int chunks_drawn = 0;
    int draw_calls = 0;
    int triangles = 0;
    float frame_ms = 0.f;
};

enum class RenderArchitecture : std::uint8_t {
    ForwardPlus = 0,
    VisibilityBuffer = 1,
};

enum class GlobalIlluminationMode : std::uint8_t {
    Off = 0,
    ProbeGrid = 1,
    ScreenSpace = 2,
};

struct LODPolicy {
    bool automatic = true;
    float lod0_distance = 35.0f;
    float lod1_distance = 90.0f;
    float lod2_distance = 180.0f;
    float impostor_distance = 320.0f;
    int triangle_budget = 250000;
};

struct RenderPipelineSettings {
    RenderArchitecture architecture = RenderArchitecture::VisibilityBuffer;
    GlobalIlluminationMode gi = GlobalIlluminationMode::ProbeGrid;
    bool pbr_enabled = true;
    bool shadow_atlas_enabled = true;
    bool volumetric_lighting_enabled = true;
    LODPolicy lod{};
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    bool init(int viewport_width, int viewport_height);
    void shutdown();

    void resize(int w, int h);

    void begin_frame();
    void render_world(const World& world, const Camera& cam);
    void end_frame();

    uint32_t scene_texture_id() const { return _scene_colour_tex; }

    const RenderStats& stats() const { return _stats; }
    const RenderPipelineSettings& pipeline_settings() const { return _pipeline; }
    void set_pipeline_settings(RenderPipelineSettings settings) { _pipeline = settings; }
    void use_visibility_buffer(bool enabled) {
        _pipeline.architecture = enabled ? RenderArchitecture::VisibilityBuffer : RenderArchitecture::ForwardPlus;
    }
    void set_automatic_lod(bool enabled) { _pipeline.lod.automatic = enabled; }

    void set_sky_color(float r, float g, float b) {
        _sky_r = r;
        _sky_g = g;
        _sky_b = b;
    }

private:
    Shader _chunk_shader;
    Shader _sky_shader;
    Shader _present_shader;

    struct CKey { int cx, cz; };
    struct CKeyHash {
        size_t operator()(const CKey& k) const {
            return std::hash<int>{}(k.cx) ^ (std::hash<int>{}(k.cz) << 16);
        }
    };
    struct CKeyEq {
        bool operator()(const CKey& a, const CKey& b) const {
            return a.cx == b.cx && a.cz == b.cz;
        }
    };

    std::unordered_map<CKey, std::unique_ptr<Mesh>, CKeyHash, CKeyEq> _chunk_meshes;
    std::unordered_map<CKey, bool, CKeyHash, CKeyEq> _dirty;

    void _rebuild_dirty(const World& world);
    void _draw_chunks(const Camera& cam);
    bool _create_present_resources();
    void _destroy_present_resources();
    void _present_to_default_framebuffer();

    uint32_t _fbo = 0;
    uint32_t _scene_colour_tex = 0;
    uint32_t _depth_rbo = 0;
    uint32_t _present_vao = 0;
    uint32_t _present_vbo = 0;
    uint32_t _present_ebo = 0;
    int _vp_w = 0;
    int _vp_h = 0;
    void _create_framebuffer(int w, int h);

    float _sky_r = 0.53f;
    float _sky_g = 0.81f;
    float _sky_b = 0.92f;
    RenderStats _stats;
    RenderPipelineSettings _pipeline;
};

} // namespace Balmung


