#include "terrain/voxel_noise.hpp"
#include <cmath>

namespace Balmung {

float NoiseSampler::fade(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float NoiseSampler::lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

float NoiseSampler::hash(int x, int y, int z) const {
    std::uint32_t h = seed_;
    h ^= 0x9e3779b9u + static_cast<std::uint32_t>(x) + (h << 6) + (h >> 2);
    h ^= 0x85ebca6bu + static_cast<std::uint32_t>(y) + (h << 6) + (h >> 2);
    h ^= 0xc2b2ae35u + static_cast<std::uint32_t>(z) + (h << 6) + (h >> 2);
    h ^= (h >> 16);
    h *= 0x7feb352du;
    h ^= (h >> 15);
    h *= 0x846ca68bu;
    h ^= (h >> 16);
    return static_cast<float>(h & 0x00ffffffu) / static_cast<float>(0x00ffffffu) * 2.0f - 1.0f;
}

float NoiseSampler::value2d(float x, float z) const {
    return value3d(x, 0.0f, z);
}

float NoiseSampler::value3d(float x, float y, float z) const {
    const auto x0 = static_cast<int>(std::floor(x));
    const auto y0 = static_cast<int>(std::floor(y));
    const auto z0 = static_cast<int>(std::floor(z));
    const auto x1 = x0 + 1;
    const auto y1 = y0 + 1;
    const auto z1 = z0 + 1;

    const float tx = fade(x - static_cast<float>(x0));
    const float ty = fade(y - static_cast<float>(y0));
    const float tz = fade(z - static_cast<float>(z0));

    const float c000 = hash(x0, y0, z0);
    const float c100 = hash(x1, y0, z0);
    const float c010 = hash(x0, y1, z0);
    const float c110 = hash(x1, y1, z0);
    const float c001 = hash(x0, y0, z1);
    const float c101 = hash(x1, y0, z1);
    const float c011 = hash(x0, y1, z1);
    const float c111 = hash(x1, y1, z1);

    const float x00 = lerp(c000, c100, tx);
    const float x10 = lerp(c010, c110, tx);
    const float x01 = lerp(c001, c101, tx);
    const float x11 = lerp(c011, c111, tx);
    const float y0v = lerp(x00, x10, ty);
    const float y1v = lerp(x01, x11, ty);
    return lerp(y0v, y1v, tz);
}

float NoiseSampler::fbm2d(float x, float z, int octaves, float lacunarity, float gain) const {
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float total = 0.0f;
    float norm = 0.0f;
    for (int octave = 0; octave < octaves; ++octave) {
        total += value2d(x * frequency, z * frequency) * amplitude;
        norm += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }
    return norm > 0.0f ? total / norm : 0.0f;
}

float NoiseSampler::fbm3d(float x, float y, float z, int octaves, float lacunarity, float gain) const {
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float total = 0.0f;
    float norm = 0.0f;
    for (int octave = 0; octave < octaves; ++octave) {
        total += value3d(x * frequency, y * frequency, z * frequency) * amplitude;
        norm += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }
    return norm > 0.0f ? total / norm : 0.0f;
}

} // namespace Balmung


