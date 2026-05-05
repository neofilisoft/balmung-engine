#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Balmung::Engine::Scene3D {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Color {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

enum class TextureRole : std::uint8_t {
    BaseColor,
    Normal,
    Metallic,
    Roughness,
    AmbientOcclusion,
    Emissive,
    Height,
    Opacity,
};

enum class LightType : std::uint8_t {
    Directional,
    Point,
    Spot,
    Rect,
};

enum class GiTechnique : std::uint8_t {
    Disabled,
    IrradianceProbeGrid,
    ScreenSpaceGlobalIllumination,
    VoxelConeTracing,
    HardwareRayTracing,
};

enum class ReflectionTechnique : std::uint8_t {
    Disabled,
    ScreenSpace,
    ReflectionProbe,
    HybridRayTraced,
};

enum class ShadowTechnique : std::uint8_t {
    Hard,
    Pcf,
    Evsm,
    RayTraced,
};

enum class RenderQualityTier : std::uint8_t {
    Preview,
    Gameplay,
    Cinematic,
    OfflineReference,
};

struct TextureBinding {
    TextureRole role = TextureRole::BaseColor;
    std::string asset;
    std::uint32_t uv_set = 0;
    float strength = 1.0f;
};

struct PbrSurface {
    std::string id;
    Color base_color{};
    float metallic = 0.0f;
    float roughness = 0.75f;
    float ambient_occlusion = 1.0f;
    float emissive_intensity = 0.0f;
    float clearcoat = 0.0f;
    float clearcoat_roughness = 0.2f;
    bool double_sided = false;
    bool alpha_clip = false;
    std::vector<TextureBinding> textures;
};

struct ShadowSettings {
    ShadowTechnique technique = ShadowTechnique::Evsm;
    std::uint32_t atlas_size = 4096;
    std::uint32_t cascade_count = 4;
    float max_distance = 180.0f;
    float contact_shadow_length = 0.25f;
    bool transparent_shadows = false;
};

struct Light {
    std::string name;
    LightType type = LightType::Point;
    Vec3 position{};
    Vec3 direction{0.0f, -1.0f, 0.0f};
    Color color{};
    float intensity_lumens = 1000.0f;
    float range = 12.0f;
    float inner_cone_deg = 20.0f;
    float outer_cone_deg = 45.0f;
    bool casts_shadow = true;
};

struct GiSettings {
    GiTechnique technique = GiTechnique::IrradianceProbeGrid;
    std::uint32_t probe_count_x = 16;
    std::uint32_t probe_count_y = 8;
    std::uint32_t probe_count_z = 16;
    float probe_spacing = 2.5f;
    float bounce_intensity = 1.0f;
    bool temporal_accumulation = true;
    bool leak_reduction = true;
};

struct ReflectionSettings {
    ReflectionTechnique technique = ReflectionTechnique::ReflectionProbe;
    std::uint32_t probe_resolution = 256;
    float screen_space_thickness = 0.25f;
    bool rough_reflections = true;
};

struct RayTracingReadiness {
    bool build_blas = true;
    bool build_tlas = true;
    bool allow_skinned_meshes = true;
    bool allow_alpha_tested_geometry = true;
    std::uint32_t max_bounces = 2;
    std::uint32_t samples_per_pixel = 1;
};

struct RenderBudget {
    std::uint32_t max_visible_meshes = 12000;
    std::uint32_t max_shadow_casters = 4096;
    std::uint32_t max_materials = 8192;
    std::uint32_t max_lights = 1024;
    std::uint32_t max_reflection_probes = 128;
    std::uint32_t target_frame_time_us = 16667;
};

struct PbrGiSceneProfile {
    std::string name = "Balmung Cinematic PBR";
    RenderQualityTier quality = RenderQualityTier::Gameplay;
    ShadowSettings shadows{};
    GiSettings gi{};
    ReflectionSettings reflections{};
    RayTracingReadiness ray_tracing{};
    RenderBudget budget{};
    Color ambient_color{0.03f, 0.035f, 0.04f, 1.0f};
    float exposure = 1.0f;
    float bloom_threshold = 1.1f;
    float bloom_intensity = 0.08f;
    bool hdr = true;
    bool pbr = true;
    bool volumetric_fog = true;
    bool subsurface_skin = true;
};

struct ValidationIssue {
    std::string code;
    std::string message;
};

class PbrGiScene {
public:
    explicit PbrGiScene(PbrGiSceneProfile profile = {});

    [[nodiscard]] const PbrGiSceneProfile& profile() const noexcept { return _profile; }
    void set_profile(PbrGiSceneProfile profile);

    [[nodiscard]] const std::vector<PbrSurface>& materials() const noexcept { return _materials; }
    [[nodiscard]] const std::vector<Light>& lights() const noexcept { return _lights; }

    std::uint32_t add_material(PbrSurface material);
    std::uint32_t add_light(Light light);
    bool remove_material(const std::string& id);
    bool remove_light(const std::string& name);

    [[nodiscard]] const PbrSurface* find_material(const std::string& id) const noexcept;
    [[nodiscard]] std::vector<ValidationIssue> validate() const;
    [[nodiscard]] float estimated_gpu_cost() const noexcept;

    static PbrGiSceneProfile recommended_profile(RenderQualityTier tier);
    static PbrGiScene make_warm_indoor_reference_scene();

private:
    PbrGiSceneProfile _profile;
    std::vector<PbrSurface> _materials;
    std::vector<Light> _lights;
};

} // namespace Balmung::Engine::Scene3D
