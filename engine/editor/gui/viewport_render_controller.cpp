#include "viewport_render_controller.hpp"

#include <sstream>
#include <utility>

namespace Balmung::Engine::Editor {

ViewportRenderController::ViewportRenderController()
    : _profile(Scene3D::PbrGiScene::recommended_profile(Scene3D::RenderQualityTier::Gameplay))
{
}

void ViewportRenderController::set_profile(Scene3D::PbrGiSceneProfile profile)
{
    Scene3D::PbrGiScene validator{std::move(profile)};
    _profile = validator.profile();
}

ViewportRenderStats ViewportRenderController::inspect_scene(const Scene3D::PbrGiScene& scene) const
{
    return ViewportRenderStats{
        .visible_meshes = static_cast<std::uint32_t>(scene.materials().size()),
        .visible_lights = static_cast<std::uint32_t>(scene.lights().size()),
        .draw_calls = static_cast<std::uint32_t>(scene.materials().size() + scene.lights().size()),
        .gpu_cost_estimate = scene.estimated_gpu_cost(),
    };
}

std::string ViewportRenderController::status_line(const Scene3D::PbrGiScene& scene) const
{
    const auto stats = inspect_scene(scene);
    std::ostringstream out;
    out << "Viewport " << (_realtime ? "Realtime" : "Paused")
        << " | profile=" << _profile.name
        << " | materials=" << stats.visible_meshes
        << " | lights=" << stats.visible_lights
        << " | draw_calls~" << stats.draw_calls
        << " | gpu_cost~" << stats.gpu_cost_estimate;
    return out.str();
}

} // namespace Balmung::Engine::Editor
