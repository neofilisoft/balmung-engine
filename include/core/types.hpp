#pragma once

#include <array>
#include <cmath>
#include <cstdint>

namespace Balmung {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Vec4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;
};

struct Mat4 {
    float m[16]{};
};

struct Color3 {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
};

[[nodiscard]] Mat4 mat4_identity();
[[nodiscard]] Mat4 mat4_multiply(const Mat4& a, const Mat4& b);
[[nodiscard]] Mat4 mat4_translate(Vec3 t);
[[nodiscard]] Mat4 perspective(float fov, float aspect, float near, float far);
[[nodiscard]] Mat4 look_at(Vec3 eye, Vec3 center, Vec3 up);

} // namespace Balmung

