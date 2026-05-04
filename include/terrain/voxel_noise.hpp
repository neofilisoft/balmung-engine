#pragma once

#include <cstdint>

namespace Balmung {

class NoiseSampler {
public:
    explicit NoiseSampler(std::uint32_t seed = 1337u) : seed_(seed) {}

    float value2d(float x, float z) const;
    float value3d(float x, float y, float z) const;
    float fbm2d(float x, float z, int octaves, float lacunarity, float gain) const;
    float fbm3d(float x, float y, float z, int octaves, float lacunarity, float gain) const;

private:
    std::uint32_t seed_;

    static float fade(float t);
    static float lerp(float a, float b, float t);
    float hash(int x, int y, int z) const;
};

} // namespace Balmung


