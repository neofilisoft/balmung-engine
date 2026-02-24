#if __has_include(<glad/glad.h>)
#  include <glad/glad.h>
#  define VB_HAS_GL 1
#else
#  define VB_HAS_GL 0
#endif

#include "renderer/shader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>
#include <cmath>

namespace VoxelBlock {

// ── Math helpers ──────────────────────────────────────────────────────────────

Mat4 mat4_identity() {
    Mat4 m{};
    m.m[0]=m.m[5]=m.m[10]=m.m[15]=1.f;
    return m;
}

Mat4 mat4_multiply(const Mat4& a, const Mat4& b) {
    Mat4 r{};
    for (int col=0; col<4; ++col)
        for (int row=0; row<4; ++row)
            for (int k=0; k<4; ++k)
                r.m[col*4+row] += a.m[k*4+row] * b.m[col*4+k];
    return r;
}

Mat4 mat4_translate(Vec3 t) {
    Mat4 m = mat4_identity();
    m.m[12]=t.x; m.m[13]=t.y; m.m[14]=t.z;
    return m;
}

Mat4 perspective(float fov, float aspect, float near, float far) {
    float f = 1.f / std::tan(fov * 0.5f);
    Mat4 m{};
    m.m[0]  =  f / aspect;
    m.m[5]  =  f;
    m.m[10] =  (far + near) / (near - far);
    m.m[11] = -1.f;
    m.m[14] =  (2.f * far * near) / (near - far);
    return m;
}

Mat4 look_at(Vec3 eye, Vec3 center, Vec3 up) {
    auto sub = [](Vec3 a, Vec3 b){ return Vec3{a.x-b.x, a.y-b.y, a.z-b.z}; };
    auto norm = [](Vec3 v){
        float l = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
        return Vec3{v.x/l, v.y/l, v.z/l};
    };
    auto cross = [](Vec3 a, Vec3 b){
        return Vec3{a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
    };
    auto dot = [](Vec3 a, Vec3 b){ return a.x*b.x + a.y*b.y + a.z*b.z; };

    Vec3 f = norm(sub(center, eye));
    Vec3 s = norm(cross(f, up));
    Vec3 u = cross(s, f);

    Mat4 m = mat4_identity();
    m.m[0]=s.x; m.m[4]=s.y; m.m[8] =s.z;
    m.m[1]=u.x; m.m[5]=u.y; m.m[9] =u.z;
    m.m[2]=-f.x;m.m[6]=-f.y;m.m[10]=-f.z;
    m.m[12]=-dot(s,eye);
    m.m[13]=-dot(u,eye);
    m.m[14]= dot(f,eye);
    return m;
}

// ── Shader ────────────────────────────────────────────────────────────────────

#if VB_HAS_GL

uint32_t Shader::_compile(uint32_t type, const std::string& src) {
    uint32_t s = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(s, 1, &c, nullptr);
    glCompileShader(s);
    int ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetShaderInfoLog(s, 1024, nullptr, log);
        std::cerr << "[Shader] Compile error:\n" << log << "\n";
        glDeleteShader(s); return 0;
    }
    return s;
}

Shader::Shader(const std::string& vert, const std::string& frag) {
    uint32_t vs = _compile(GL_VERTEX_SHADER,   vert);
    uint32_t fs = _compile(GL_FRAGMENT_SHADER, frag);
    if (!vs || !fs) { glDeleteShader(vs); glDeleteShader(fs); return; }
    _program = glCreateProgram();
    glAttachShader(_program, vs);
    glAttachShader(_program, fs);
    glLinkProgram(_program);
    int ok = 0; glGetProgramiv(_program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetProgramInfoLog(_program, 1024, nullptr, log);
        std::cerr << "[Shader] Link error:\n" << log << "\n";
        glDeleteProgram(_program); _program = 0;
    }
    glDeleteShader(vs); glDeleteShader(fs);
}

Shader::~Shader() { if (_program) glDeleteProgram(_program); }

Shader::Shader(Shader&& o) noexcept : _program(o._program) { o._program=0; }
Shader& Shader::operator=(Shader&& o) noexcept {
    if (this != &o) { if(_program) glDeleteProgram(_program); _program=o._program; o._program=0; }
    return *this;
}

Shader Shader::from_files(const std::string& vp, const std::string& fp) {
    auto read = [](const std::string& path){
        std::ifstream f(path); if (!f) throw std::runtime_error("Cannot open: "+path);
        return std::string((std::istreambuf_iterator<char>(f)), {});
    };
    return Shader(read(vp), read(fp));
}

void Shader::bind()   const { if(_program) glUseProgram(_program); }
void Shader::unbind() const { glUseProgram(0); }

int Shader::_get_uniform(const std::string& name) const {
    auto it = _uniform_cache.find(name);
    if (it != _uniform_cache.end()) return it->second;
    int loc = glGetUniformLocation(_program, name.c_str());
    _uniform_cache[name] = loc;
    return loc;
}

void Shader::set_int  (const std::string& n, int v)          const { glUniform1i (_get_uniform(n), v); }
void Shader::set_float(const std::string& n, float v)        const { glUniform1f (_get_uniform(n), v); }
void Shader::set_vec2 (const std::string& n, Vec2 v)         const { glUniform2f (_get_uniform(n), v.x, v.y); }
void Shader::set_vec3 (const std::string& n, Vec3 v)         const { glUniform3f (_get_uniform(n), v.x, v.y, v.z); }
void Shader::set_vec4 (const std::string& n, Vec4 v)         const { glUniform4f (_get_uniform(n), v.x, v.y, v.z, v.w); }
void Shader::set_mat4 (const std::string& n, const Mat4& m)  const { glUniformMatrix4fv(_get_uniform(n), 1, GL_FALSE, m.m); }
void Shader::set_mat4 (const std::string& n, const float* m) const { glUniformMatrix4fv(_get_uniform(n), 1, GL_FALSE, m); }

#else // stubs
Shader::Shader(const std::string&, const std::string&) {}
Shader::~Shader() {}
Shader::Shader(Shader&& o) noexcept : _program(o._program) { o._program=0; }
Shader& Shader::operator=(Shader&& o) noexcept { _program=o._program; o._program=0; return *this; }
Shader Shader::from_files(const std::string&, const std::string&) { return Shader{}; }
void Shader::bind()   const {}
void Shader::unbind() const {}
int  Shader::_get_uniform(const std::string&) const { return -1; }
void Shader::set_int  (const std::string&, int)         const {}
void Shader::set_float(const std::string&, float)       const {}
void Shader::set_vec2 (const std::string&, Vec2)        const {}
void Shader::set_vec3 (const std::string&, Vec3)        const {}
void Shader::set_vec4 (const std::string&, Vec4)        const {}
void Shader::set_mat4 (const std::string&, const Mat4&) const {}
void Shader::set_mat4 (const std::string&, const float*) const {}
#endif

} // namespace VoxelBlock
