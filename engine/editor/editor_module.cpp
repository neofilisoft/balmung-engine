#include "editor_module.hpp"

namespace Balmung::Engine::Editor {

EditorModule::EditorModule()
{
    _viewport.set_profile(_settings.render_profile);
}

void EditorModule::apply_cinematic_3d_defaults()
{
    _settings.project_template = ProjectTemplate::Cinematic3D;
    _settings.preferred_backend = RenderBackend::Vulkan;
    _settings.render_profile = Scene3D::PbrGiScene::recommended_profile(Scene3D::RenderQualityTier::Cinematic);
    _settings.enable_live_lighting_preview = true;
    _settings.require_pbr_textures = true;
    _viewport.set_profile(_settings.render_profile);
}

} // namespace Balmung::Engine::Editor
