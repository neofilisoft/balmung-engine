#include "scene_module.hpp"

#include <utility>

namespace Balmung::Engine::Scene {

SceneModule::SceneModule()
    : _profile(Scene3D::PbrGiScene::recommended_profile(Scene3D::RenderQualityTier::Gameplay))
{
}

void SceneModule::set_active_3d_profile(Scene3D::PbrGiSceneProfile profile)
{
    _profile = std::move(profile);
}

std::vector<std::string> SceneModule::feature_labels() const
{
    std::vector<std::string> labels;
    if (_capabilities.pbr_materials) labels.emplace_back("PBR metal/roughness materials");
    if (_capabilities.dynamic_lights) labels.emplace_back("Directional, point, spot, and rect lights");
    if (_capabilities.gi_probe_grid) labels.emplace_back("Irradiance probe-grid GI");
    if (_capabilities.screen_space_gi) labels.emplace_back("Screen-space GI path");
    if (_capabilities.ray_tracing_ready) labels.emplace_back("BLAS/TLAS ray-tracing scene metadata");
    if (_capabilities.lit_2d_batching) labels.emplace_back("Normal-mapped lit 2D sprite batching");
    return labels;
}

} // namespace Balmung::Engine::Scene
