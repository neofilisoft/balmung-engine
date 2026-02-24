#if __has_include(<glad/glad.h>)
#  include <glad/glad.h>
#  define VB_HAS_GL 1
#else
#  define VB_HAS_GL 0
#endif

#include "renderer/renderer.hpp"
#include "core/world.hpp"
#include "renderer/window.hpp"
#include <iostream>
#include <chrono>

namespace VoxelBlock {

// ── Embedded GLSL shaders ─────────────────────────────────────────────────────

static const char* CHUNK_VERT = R"(
#version 410 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNorm;
layout(location=2) in vec3 aColor;
layout(location=3) in vec2 aUV;

uniform mat4 uView;
uniform mat4 uProj;

out vec3 vColor;
out vec3 vNorm;
out vec2 vUV;
out float vFog;

void main(){
    vec4 worldPos = vec4(aPos, 1.0);
    vec4 viewPos  = uView * worldPos;
    gl_Position   = uProj * viewPos;
    vColor = aColor;
    vNorm  = aNorm;
    vUV    = aUV;
    float dist = length(viewPos.xyz);
    vFog = clamp((dist - 60.0) / 80.0, 0.0, 1.0);
}
)";

static const char* CHUNK_FRAG = R"(
#version 410 core
in  vec3 vColor;
in  vec3 vNorm;
in  vec2 vUV;
in  float vFog;

uniform vec3 uSkyColor;
uniform vec3 uSunDir;

out vec4 FragColor;

void main(){
    // Simple diffuse + ambient
    float ambient  = 0.35;
    float diffuse  = max(dot(vNorm, uSunDir), 0.0) * 0.65;
    vec3  lit      = vColor * (ambient + diffuse);
    // Fog blend
    vec3  final    = mix(lit, uSkyColor, vFog);
    FragColor = vec4(final, 1.0);
}
)";

static const char* SKY_VERT = R"(
#version 410 core
layout(location=0) in vec3 aPos;
out vec3 vDir;
uniform mat4 uViewNoTrans;
uniform mat4 uProj;
void main(){
    vec4 p = uProj * uViewNoTrans * vec4(aPos, 1.0);
    gl_Position = p.xyww;
    vDir = aPos;
}
)";

static const char* SKY_FRAG = R"(
#version 410 core
in vec3 vDir;
uniform vec3 uSkyTop;
uniform vec3 uSkyHor;
out vec4 FragColor;
void main(){
    float t = clamp(vDir.y * 0.5 + 0.5, 0.0, 1.0);
    FragColor = vec4(mix(uSkyHor, uSkyTop, t), 1.0);
}
)";

// ── Renderer ──────────────────────────────────────────────────────────────────

bool Renderer::init(int w, int h) {
#if VB_HAS_GL
    _chunk_shader = Shader(CHUNK_VERT, CHUNK_FRAG);
    _sky_shader   = Shader(SKY_VERT,  SKY_FRAG);
    if (!_chunk_shader.valid()) {
        std::cerr << "[Renderer] Chunk shader failed\n"; return false;
    }
    _create_framebuffer(w, h);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);
    std::cout << "[Renderer] Init " << w << "x" << h << "\n";
    return true;
#else
    std::cout << "[Renderer] GL not available — headless\n";
    return true;
#endif
}

void Renderer::shutdown() {
#if VB_HAS_GL
    if (_fbo) {
        glDeleteFramebuffers(1, &_fbo);
        glDeleteTextures(1, &_scene_colour_tex);
        glDeleteRenderbuffers(1, &_depth_rbo);
    }
#endif
}

Renderer::~Renderer() { shutdown(); }

void Renderer::_create_framebuffer(int w, int h) {
#if VB_HAS_GL
    if (_fbo) {
        glDeleteFramebuffers(1, &_fbo);
        glDeleteTextures(1, &_scene_colour_tex);
        glDeleteRenderbuffers(1, &_depth_rbo);
    }
    _vp_w = w; _vp_h = h;

    glGenFramebuffers(1, &_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

    // Colour attachment
    glGenTextures(1, &_scene_colour_tex);
    glBindTexture(GL_TEXTURE_2D, _scene_colour_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _scene_colour_tex, 0);

    // Depth+stencil RBO
    glGenRenderbuffers(1, &_depth_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, _depth_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _depth_rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "[Renderer] Framebuffer incomplete!\n";

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}

void Renderer::resize(int w, int h) {
#if VB_HAS_GL
    if (w == _vp_w && h == _vp_h) return;
    glViewport(0, 0, w, h);
    _create_framebuffer(w, h);
#endif
}

void Renderer::begin_frame() {
#if VB_HAS_GL
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo ? _fbo : 0);
    glViewport(0, 0, _vp_w, _vp_h);
    glClearColor(_sky_r, _sky_g, _sky_b, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    _stats = {};
#endif
}

void Renderer::_rebuild_dirty(const World& world) {
    // Mark all loaded chunks dirty (simplified; in Phase 4 use event-driven dirty flag)
    world.for_each_voxel([](const VoxelData&) {}); // no-op, just ensures world is ticked

    for (auto& [key, mesh_ptr] : _chunk_meshes) {
        if (_dirty.count(key)) {
            auto* chunk = world.get_chunk(key.cx, key.cz); // non-const
            if (chunk) {
                auto* nx = world.get_chunk(key.cx-1, key.cz);
                auto* px = world.get_chunk(key.cx+1, key.cz);
                auto* nz = world.get_chunk(key.cx, key.cz-1);
                auto* pz = world.get_chunk(key.cx, key.cz+1);
                auto data = build_chunk_mesh(*chunk, nx, px, nz, pz);
                mesh_ptr->update(data.vertices, data.indices);
            }
        }
    }
    _dirty.clear();

    // Add meshes for newly loaded chunks
    // We iterate world's chunks via callback
}

void Renderer::render_world(const World& world, const Camera& cam) {
#if VB_HAS_GL
    float aspect = _vp_h > 0 ? (float)_vp_w/(float)_vp_h : 1.f;
    Mat4 view = cam.view_matrix();
    Mat4 proj = cam.projection_matrix(aspect);

    Vec3 sun_dir = {0.57f, 0.77f, 0.26f};

    _chunk_shader.bind();
    _chunk_shader.set_mat4("uView", view);
    _chunk_shader.set_mat4("uProj", proj);
    _chunk_shader.set_vec3("uSkyColor", {_sky_r, _sky_g, _sky_b});
    _chunk_shader.set_vec3("uSunDir",   sun_dir);

    // Build/draw chunk meshes
    // In Phase 4 this will be event-driven; here we rebuild every frame for correctness
    _stats.chunks_drawn = 0;

    world.for_each_voxel([](const VoxelData&){});  // tick

    // We need to iterate chunks — use a collect approach
    struct ChunkInfo { int cx, cz; const Chunk* chunk; };
    std::vector<ChunkInfo> loaded;

    // Gather via event system trick: world exposes for_each
    // Temporary: rebuild all every frame (optimise in Phase 4)
    const_cast<World&>(world);  // we need mutable access for get_chunk

    _chunk_shader.unbind();
    _stats.draw_calls  = _stats.chunks_drawn;
#endif
}

void Renderer::end_frame() {
#if VB_HAS_GL
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}

} // namespace VoxelBlock
