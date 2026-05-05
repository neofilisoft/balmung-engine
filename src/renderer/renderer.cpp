#if __has_include(<glad/glad.h>)
#  include <glad/glad.h>
#  define BM_HAS_GL 1
#else
#  define BM_HAS_GL 0
#endif

#include "renderer/renderer.hpp"
#include "core/world.hpp"
#include "renderer/window.hpp"
#include <chrono>
#include <iostream>
#include <vector>

namespace Balmung {

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
    float ambient  = 0.35;
    float diffuse  = max(dot(vNorm, uSunDir), 0.0) * 0.65;
    vec3  lit      = vColor * (ambient + diffuse);
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

static const char* PRESENT_VERT = R"(
#version 410 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aUV;
out vec2 vUV;
void main(){
    vUV = aUV;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

static const char* PRESENT_FRAG = R"(
#version 410 core
in vec2 vUV;
uniform sampler2D uSceneTex;
out vec4 FragColor;
void main(){
    FragColor = texture(uSceneTex, vUV);
}
)";

bool Renderer::init(int w, int h) {
#if BM_HAS_GL
    _chunk_shader = Shader(CHUNK_VERT, CHUNK_FRAG);
    _sky_shader = Shader(SKY_VERT, SKY_FRAG);
    _present_shader = Shader(PRESENT_VERT, PRESENT_FRAG);
    if (!_chunk_shader.valid()) {
        std::cerr << "[Renderer] Chunk shader failed\n";
        return false;
    }
    if (!_present_shader.valid()) {
        std::cerr << "[Renderer] Present shader failed\n";
        return false;
    }
    _create_framebuffer(w, h);
    if (!_create_present_resources()) {
        std::cerr << "[Renderer] Present resources failed\n";
        return false;
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);
    std::cout << "[Renderer] Init " << w << "x" << h << "\n";
    return true;
#else
    std::cout << "[Renderer] GL not available - headless\n";
    return true;
#endif
}

void Renderer::shutdown() {
#if BM_HAS_GL
    _destroy_present_resources();
    if (_fbo) {
        glDeleteFramebuffers(1, &_fbo);
        glDeleteTextures(1, &_scene_colour_tex);
        glDeleteRenderbuffers(1, &_depth_rbo);
        _fbo = 0;
        _scene_colour_tex = 0;
        _depth_rbo = 0;
    }
#endif
}

Renderer::~Renderer() { shutdown(); }

bool Renderer::_create_present_resources() {
#if BM_HAS_GL
    if (_present_vao) {
        return true;
    }

    constexpr float quad_vertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f
    };
    constexpr uint32_t quad_indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &_present_vao);
    glGenBuffers(1, &_present_vbo);
    glGenBuffers(1, &_present_ebo);

    glBindVertexArray(_present_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _present_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _present_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    return true;
#else
    return true;
#endif
}

void Renderer::_destroy_present_resources() {
#if BM_HAS_GL
    if (_present_ebo) {
        glDeleteBuffers(1, &_present_ebo);
        _present_ebo = 0;
    }
    if (_present_vbo) {
        glDeleteBuffers(1, &_present_vbo);
        _present_vbo = 0;
    }
    if (_present_vao) {
        glDeleteVertexArrays(1, &_present_vao);
        _present_vao = 0;
    }
#endif
}

void Renderer::_create_framebuffer(int w, int h) {
#if BM_HAS_GL
    if (_fbo) {
        glDeleteFramebuffers(1, &_fbo);
        glDeleteTextures(1, &_scene_colour_tex);
        glDeleteRenderbuffers(1, &_depth_rbo);
    }
    _vp_w = w;
    _vp_h = h;

    glGenFramebuffers(1, &_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

    glGenTextures(1, &_scene_colour_tex);
    glBindTexture(GL_TEXTURE_2D, _scene_colour_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _scene_colour_tex, 0);

    glGenRenderbuffers(1, &_depth_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, _depth_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _depth_rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[Renderer] Framebuffer incomplete\n";
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}

void Renderer::resize(int w, int h) {
#if BM_HAS_GL
    if (w == _vp_w && h == _vp_h) {
        return;
    }
    glViewport(0, 0, w, h);
    _create_framebuffer(w, h);
#endif
}

void Renderer::begin_frame() {
#if BM_HAS_GL
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo ? _fbo : 0);
    glViewport(0, 0, _vp_w, _vp_h);
    glClearColor(_sky_r, _sky_g, _sky_b, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    _stats = {};
#endif
}

void Renderer::_rebuild_dirty(const World& world) {
    world.for_each_voxel([](const VoxelData&) {});

    for (auto& [key, mesh_ptr] : _chunk_meshes) {
        if (_dirty.count(key)) {
            auto* chunk = world.get_chunk(key.cx, key.cz);
            if (chunk) {
                auto* nx = world.get_chunk(key.cx - 1, key.cz);
                auto* px = world.get_chunk(key.cx + 1, key.cz);
                auto* nz = world.get_chunk(key.cx, key.cz - 1);
                auto* pz = world.get_chunk(key.cx, key.cz + 1);
                auto data = build_chunk_mesh(*chunk, nx, px, nz, pz);
                mesh_ptr->update(data.vertices, data.indices);
            }
        }
    }
    _dirty.clear();
}

void Renderer::render_world(const World& world, const Camera& cam) {
#if BM_HAS_GL
    float aspect = _vp_h > 0 ? static_cast<float>(_vp_w) / static_cast<float>(_vp_h) : 1.f;
    Mat4 view = cam.view_matrix();
    Mat4 proj = cam.projection_matrix(aspect);

    Vec3 sun_dir = {0.57f, 0.77f, 0.26f};

    _chunk_shader.bind();
    _chunk_shader.set_mat4("uView", view);
    _chunk_shader.set_mat4("uProj", proj);
    _chunk_shader.set_vec3("uSkyColor", {_sky_r, _sky_g, _sky_b});
    _chunk_shader.set_vec3("uSunDir", sun_dir);

    _stats.chunks_drawn = 0;
    _stats.triangles = 0;

    world.for_each_chunk([&](const Chunk& chunk) {
        const CKey key{chunk.cx(), chunk.cz()};
        auto it = _chunk_meshes.find(key);
        if (it == _chunk_meshes.end()) {
            auto mesh = std::make_unique<Mesh>();
            const auto* nx = world.get_chunk(chunk.cx() - 1, chunk.cz());
            const auto* px = world.get_chunk(chunk.cx() + 1, chunk.cz());
            const auto* nz = world.get_chunk(chunk.cx(), chunk.cz() - 1);
            const auto* pz = world.get_chunk(chunk.cx(), chunk.cz() + 1);
            auto data = build_chunk_mesh(chunk, nx, px, nz, pz);
            mesh->upload(data.vertices, data.indices);
            _stats.triangles += static_cast<int>(data.indices.size() / 3);
            it = _chunk_meshes.emplace(key, std::move(mesh)).first;
        } else {
            _stats.triangles += it->second->index_count() / 3;
        }

        if (!it->second->empty()) {
            it->second->draw();
            ++_stats.chunks_drawn;
        }
    });

    _chunk_shader.unbind();
    _stats.draw_calls = _stats.chunks_drawn;
#else
    _stats = {};
    world.for_each_chunk([&](const Chunk& chunk) {
        (void)chunk;
        ++_stats.chunks_drawn;
    });
    _stats.draw_calls = _stats.chunks_drawn;
#endif
}

void Renderer::end_frame() {
#if BM_HAS_GL
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    _present_to_default_framebuffer();
#endif
}

void Renderer::_present_to_default_framebuffer() {
#if BM_HAS_GL
    if (!_scene_colour_tex || !_present_vao || !_present_shader.valid()) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, _vp_w, _vp_h);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    _present_shader.bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _scene_colour_tex);
    _present_shader.set_int("uSceneTex", 0);
    glBindVertexArray(_present_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    _present_shader.unbind();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
#endif
}

} // namespace Balmung


