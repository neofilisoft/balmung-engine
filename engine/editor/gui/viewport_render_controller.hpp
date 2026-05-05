#pragma once

#include "../../scene/3d/pbr_gi_scene.hpp"

#include <cstdint>
#include <string>

namespace Balmung::Engine::Editor {

enum class GizmoMode : std::uint8_t {
    Select,
    Translate,
    Rotate,
    Scale,
};

struct ViewportCamera {
    float position[3]{0.0f, 2.0f, 6.0f};
    float yaw_deg = 180.0f;
    float pitch_deg = -12.0f;
    float fov_deg = 60.0f;
    float move_speed = 6.0f;
};

struct ViewportRenderStats {
    std::uint32_t visible_meshes = 0;
    std::uint32_t visible_lights = 0;
    std::uint32_t draw_calls = 0;
    float gpu_cost_estimate = 0.0f;
};

class ViewportRenderController {
public:
    ViewportRenderController();

    void set_profile(Scene3D::PbrGiSceneProfile profile);
    void set_gizmo_mode(GizmoMode mode) noexcept { _gizmo_mode = mode; }
    void set_camera(ViewportCamera camera) noexcept { _camera = camera; }
    void set_realtime(bool realtime) noexcept { _realtime = realtime; }

    [[nodiscard]] const Scene3D::PbrGiSceneProfile& profile() const noexcept { return _profile; }
    [[nodiscard]] GizmoMode gizmo_mode() const noexcept { return _gizmo_mode; }
    [[nodiscard]] const ViewportCamera& camera() const noexcept { return _camera; }
    [[nodiscard]] bool realtime() const noexcept { return _realtime; }
    [[nodiscard]] ViewportRenderStats inspect_scene(const Scene3D::PbrGiScene& scene) const;
    [[nodiscard]] std::string status_line(const Scene3D::PbrGiScene& scene) const;

private:
    Scene3D::PbrGiSceneProfile _profile;
    ViewportCamera _camera;
    GizmoMode _gizmo_mode = GizmoMode::Translate;
    bool _realtime = true;
};

} // namespace Balmung::Engine::Editor
