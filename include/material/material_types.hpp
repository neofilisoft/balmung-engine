#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace Balmung::Material {

enum class ShadingModel : std::uint8_t {
    Unlit = 0,
    Lit2D = 1,
    PbrMetalRough = 2,
};

struct ColorRGBA {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

struct TextureSet {
    std::string base_color;
    std::string normal;
    std::string metallic;
    std::string roughness;
    std::string ambient_occlusion;
    std::string emissive;
};

struct PbrMaterial {
    std::string id;
    std::string display_name;
    TextureSet textures{};

    ColorRGBA base_color_tint{};
    float metallic_factor = 0.0f;
    float roughness_factor = 1.0f;
    float ao_factor = 1.0f;
    float emissive_intensity = 0.0f;
    bool double_sided = false;
    bool alpha_clip = false;
    float alpha_cutoff = 0.5f;
};

struct LitSpriteMaterial {
    std::string id;
    std::string display_name;
    std::string sprite_texture;
    std::string normal_map;
    std::string emissive_map;

    ColorRGBA tint{};
    float emissive_intensity = 0.0f;
    bool pixel_snap = true;
    bool cast_2d_shadow = false;
};

} // namespace Balmung::Material



