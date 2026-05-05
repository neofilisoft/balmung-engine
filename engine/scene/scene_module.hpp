#pragma once

#include "2d/scene2d_batcher.hpp"
#include "3d/pbr_gi_scene.hpp"

#include <string>
#include <vector>

namespace Balmung::Engine::Scene {

struct SceneModuleCapabilities {
    bool pbr_materials = true;
    bool dynamic_lights = true;
    bool gi_probe_grid = true;
    bool screen_space_gi = true;
    bool ray_tracing_ready = true;
    bool lit_2d_batching = true;
};

class SceneModule {
public:
    SceneModule();

    [[nodiscard]] const SceneModuleCapabilities& capabilities() const noexcept { return _capabilities; }
    [[nodiscard]] const Scene3D::PbrGiSceneProfile& active_3d_profile() const noexcept { return _profile; }
    void set_active_3d_profile(Scene3D::PbrGiSceneProfile profile);

    [[nodiscard]] std::vector<std::string> feature_labels() const;

private:
    SceneModuleCapabilities _capabilities;
    Scene3D::PbrGiSceneProfile _profile;
};

} // namespace Balmung::Engine::Scene
