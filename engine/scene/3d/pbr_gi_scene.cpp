#include "pbr_gi_scene.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Balmung::Engine::Scene3D {
namespace {

float clamp01(float value) noexcept
{
    return std::clamp(value, 0.0f, 1.0f);
}

std::uint32_t clamp_power_of_two(std::uint32_t value, std::uint32_t min_value, std::uint32_t max_value) noexcept
{
    value = std::clamp(value, min_value, max_value);
    std::uint32_t out = min_value;
    while (out < value && out < max_value) {
        out <<= 1U;
    }
    return out;
}

void sanitize_surface(PbrSurface& material)
{
    material.base_color.r = clamp01(material.base_color.r);
    material.base_color.g = clamp01(material.base_color.g);
    material.base_color.b = clamp01(material.base_color.b);
    material.base_color.a = clamp01(material.base_color.a);
    material.metallic = clamp01(material.metallic);
    material.roughness = std::clamp(material.roughness, 0.025f, 1.0f);
    material.ambient_occlusion = clamp01(material.ambient_occlusion);
    material.emissive_intensity = std::max(0.0f, material.emissive_intensity);
    material.clearcoat = clamp01(material.clearcoat);
    material.clearcoat_roughness = clamp01(material.clearcoat_roughness);
}

void sanitize_profile(PbrGiSceneProfile& profile)
{
    profile.shadows.atlas_size = clamp_power_of_two(profile.shadows.atlas_size, 1024, 16384);
    profile.shadows.cascade_count = std::clamp(profile.shadows.cascade_count, 1U, 8U);
    profile.shadows.max_distance = std::max(profile.shadows.max_distance, 1.0f);
    profile.shadows.contact_shadow_length = std::max(profile.shadows.contact_shadow_length, 0.0f);

    profile.gi.probe_count_x = std::clamp(profile.gi.probe_count_x, 1U, 128U);
    profile.gi.probe_count_y = std::clamp(profile.gi.probe_count_y, 1U, 64U);
    profile.gi.probe_count_z = std::clamp(profile.gi.probe_count_z, 1U, 128U);
    profile.gi.probe_spacing = std::max(profile.gi.probe_spacing, 0.1f);
    profile.gi.bounce_intensity = std::max(profile.gi.bounce_intensity, 0.0f);

    profile.reflections.probe_resolution = clamp_power_of_two(profile.reflections.probe_resolution, 64, 2048);
    profile.reflections.screen_space_thickness = std::max(profile.reflections.screen_space_thickness, 0.01f);

    profile.ray_tracing.max_bounces = std::clamp(profile.ray_tracing.max_bounces, 1U, 16U);
    profile.ray_tracing.samples_per_pixel = std::clamp(profile.ray_tracing.samples_per_pixel, 1U, 256U);
    profile.exposure = std::max(profile.exposure, 0.01f);
    profile.bloom_threshold = std::max(profile.bloom_threshold, 0.0f);
    profile.bloom_intensity = std::max(profile.bloom_intensity, 0.0f);
}

} // namespace

PbrGiScene::PbrGiScene(PbrGiSceneProfile profile)
{
    set_profile(std::move(profile));
}

void PbrGiScene::set_profile(PbrGiSceneProfile profile)
{
    sanitize_profile(profile);
    _profile = std::move(profile);
}

std::uint32_t PbrGiScene::add_material(PbrSurface material)
{
    sanitize_surface(material);
    if (material.id.empty()) {
        material.id = "material_" + std::to_string(_materials.size() + 1);
    }

    if (auto* existing = const_cast<PbrSurface*>(find_material(material.id))) {
        *existing = std::move(material);
        return static_cast<std::uint32_t>((existing - _materials.data()) + 1);
    }

    _materials.push_back(std::move(material));
    return static_cast<std::uint32_t>(_materials.size());
}

std::uint32_t PbrGiScene::add_light(Light light)
{
    if (light.name.empty()) {
        light.name = "light_" + std::to_string(_lights.size() + 1);
    }
    light.intensity_lumens = std::max(light.intensity_lumens, 0.0f);
    light.range = std::max(light.range, 0.01f);
    light.inner_cone_deg = std::clamp(light.inner_cone_deg, 0.0f, 89.0f);
    light.outer_cone_deg = std::clamp(light.outer_cone_deg, light.inner_cone_deg, 179.0f);

    auto it = std::find_if(_lights.begin(), _lights.end(), [&](const Light& current) {
        return current.name == light.name;
    });
    if (it != _lights.end()) {
        *it = std::move(light);
        return static_cast<std::uint32_t>((it - _lights.begin()) + 1);
    }

    _lights.push_back(std::move(light));
    return static_cast<std::uint32_t>(_lights.size());
}

bool PbrGiScene::remove_material(const std::string& id)
{
    auto it = std::remove_if(_materials.begin(), _materials.end(), [&](const PbrSurface& material) {
        return material.id == id;
    });
    if (it == _materials.end()) {
        return false;
    }
    _materials.erase(it, _materials.end());
    return true;
}

bool PbrGiScene::remove_light(const std::string& name)
{
    auto it = std::remove_if(_lights.begin(), _lights.end(), [&](const Light& light) {
        return light.name == name;
    });
    if (it == _lights.end()) {
        return false;
    }
    _lights.erase(it, _lights.end());
    return true;
}

const PbrSurface* PbrGiScene::find_material(const std::string& id) const noexcept
{
    auto it = std::find_if(_materials.begin(), _materials.end(), [&](const PbrSurface& material) {
        return material.id == id;
    });
    return it == _materials.end() ? nullptr : &*it;
}

std::vector<ValidationIssue> PbrGiScene::validate() const
{
    std::vector<ValidationIssue> issues;
    if (!_profile.pbr) {
        issues.push_back({"PBR_DISABLED", "PBR is disabled for a 3D profile that is expected to ship modern materials."});
    }
    if (_profile.gi.technique == GiTechnique::HardwareRayTracing &&
        (!_profile.ray_tracing.build_blas || !_profile.ray_tracing.build_tlas)) {
        issues.push_back({"RT_ACCELERATION_DISABLED", "Hardware GI requires BLAS and TLAS build data."});
    }
    if (_lights.size() > _profile.budget.max_lights) {
        issues.push_back({"LIGHT_BUDGET_EXCEEDED", "Scene light count exceeds the configured renderer light budget."});
    }
    if (_materials.size() > _profile.budget.max_materials) {
        issues.push_back({"MATERIAL_BUDGET_EXCEEDED", "Scene material count exceeds the configured renderer material budget."});
    }
    if (_profile.shadows.technique == ShadowTechnique::RayTraced &&
        (!_profile.ray_tracing.build_blas || !_profile.ray_tracing.build_tlas)) {
        issues.push_back({"RT_SHADOW_ACCELERATION_DISABLED", "Ray-traced shadows require acceleration structures."});
    }
    return issues;
}

float PbrGiScene::estimated_gpu_cost() const noexcept
{
    const float light_cost = static_cast<float>(_lights.size()) * 0.055f;
    const float shadow_cost = static_cast<float>(std::count_if(_lights.begin(), _lights.end(), [](const Light& light) {
        return light.casts_shadow;
    })) * 0.18f;
    const float material_cost = static_cast<float>(_materials.size()) * 0.012f;
    const float probe_count = static_cast<float>(_profile.gi.probe_count_x * _profile.gi.probe_count_y * _profile.gi.probe_count_z);
    const float gi_cost = _profile.gi.technique == GiTechnique::Disabled ? 0.0f : std::log2(probe_count + 1.0f) * 0.22f;
    const float rt_cost = _profile.gi.technique == GiTechnique::HardwareRayTracing ||
                                  _profile.reflections.technique == ReflectionTechnique::HybridRayTraced ||
                                  _profile.shadows.technique == ShadowTechnique::RayTraced
                              ? static_cast<float>(_profile.ray_tracing.samples_per_pixel * _profile.ray_tracing.max_bounces) * 0.75f
                              : 0.0f;
    return light_cost + shadow_cost + material_cost + gi_cost + rt_cost;
}

PbrGiSceneProfile PbrGiScene::recommended_profile(RenderQualityTier tier)
{
    PbrGiSceneProfile profile{};
    profile.quality = tier;

    switch (tier) {
    case RenderQualityTier::Preview:
        profile.name = "Balmung Preview PBR";
        profile.gi.technique = GiTechnique::ScreenSpaceGlobalIllumination;
        profile.gi.probe_count_x = 4;
        profile.gi.probe_count_y = 2;
        profile.gi.probe_count_z = 4;
        profile.reflections.technique = ReflectionTechnique::ScreenSpace;
        profile.shadows.atlas_size = 2048;
        profile.shadows.cascade_count = 2;
        profile.budget.target_frame_time_us = 8333;
        profile.volumetric_fog = false;
        break;
    case RenderQualityTier::Gameplay:
        profile.name = "Balmung Gameplay PBR GI";
        break;
    case RenderQualityTier::Cinematic:
        profile.name = "Balmung Cinematic PBR GI";
        profile.gi.technique = GiTechnique::VoxelConeTracing;
        profile.gi.probe_count_x = 32;
        profile.gi.probe_count_y = 12;
        profile.gi.probe_count_z = 32;
        profile.reflections.technique = ReflectionTechnique::HybridRayTraced;
        profile.shadows.atlas_size = 8192;
        profile.shadows.cascade_count = 6;
        profile.bloom_intensity = 0.16f;
        profile.budget.target_frame_time_us = 33333;
        break;
    case RenderQualityTier::OfflineReference:
        profile.name = "Balmung Offline Reference RT";
        profile.gi.technique = GiTechnique::HardwareRayTracing;
        profile.reflections.technique = ReflectionTechnique::HybridRayTraced;
        profile.shadows.technique = ShadowTechnique::RayTraced;
        profile.ray_tracing.max_bounces = 8;
        profile.ray_tracing.samples_per_pixel = 64;
        profile.shadows.atlas_size = 16384;
        profile.budget.target_frame_time_us = 1000000;
        break;
    }

    sanitize_profile(profile);
    return profile;
}

PbrGiScene PbrGiScene::make_warm_indoor_reference_scene()
{
    PbrGiScene scene{recommended_profile(RenderQualityTier::Cinematic)};
    scene.add_material(PbrSurface{
        .id = "aged_wood_oiled",
        .base_color = {0.55f, 0.29f, 0.12f, 1.0f},
        .metallic = 0.0f,
        .roughness = 0.48f,
        .ambient_occlusion = 0.9f,
        .textures = {
            {TextureRole::BaseColor, "textures/wood/basecolor", 0, 1.0f},
            {TextureRole::Normal, "textures/wood/normal", 0, 1.0f},
            {TextureRole::Roughness, "textures/wood/roughness", 0, 1.0f},
            {TextureRole::AmbientOcclusion, "textures/wood/ao", 0, 1.0f},
        },
    });
    scene.add_material(PbrSurface{
        .id = "skin_subsurface_preview",
        .base_color = {0.92f, 0.66f, 0.54f, 1.0f},
        .metallic = 0.0f,
        .roughness = 0.62f,
        .ambient_occlusion = 1.0f,
        .clearcoat = 0.08f,
        .clearcoat_roughness = 0.35f,
    });
    scene.add_light(Light{
        .name = "warm_table_lamp",
        .type = LightType::Point,
        .position = {2.0f, 1.25f, -1.0f},
        .color = {1.0f, 0.72f, 0.42f, 1.0f},
        .intensity_lumens = 1800.0f,
        .range = 7.0f,
        .casts_shadow = true,
    });
    scene.add_light(Light{
        .name = "cool_window_fill",
        .type = LightType::Rect,
        .position = {-3.0f, 2.0f, 1.0f},
        .direction = {1.0f, -0.25f, -0.2f},
        .color = {0.5f, 0.65f, 1.0f, 1.0f},
        .intensity_lumens = 650.0f,
        .range = 12.0f,
        .casts_shadow = false,
    });
    return scene;
}

} // namespace Balmung::Engine::Scene3D
