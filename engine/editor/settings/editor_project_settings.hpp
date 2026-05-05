#pragma once

#include "../../scene/3d/pbr_gi_scene.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Balmung::Engine::Editor {

enum class RenderBackend : std::uint8_t {
    OpenGL,
    Vulkan,
    Direct3D12,
    Metal,
};

enum class ProjectTemplate : std::uint8_t {
    Game2D,
    HD2D,
    Game3D,
    Cinematic3D,
};

struct EditorProjectSettings {
    std::string project_name = "Balmung Game";
    ProjectTemplate project_template = ProjectTemplate::Game3D;
    RenderBackend preferred_backend = RenderBackend::Vulkan;
    Scene3D::PbrGiSceneProfile render_profile = Scene3D::PbrGiScene::recommended_profile(Scene3D::RenderQualityTier::Gameplay);
    std::filesystem::path asset_root = "assets";
    std::filesystem::path scene_root = "scenes";
    bool enable_hot_reload = true;
    bool enable_live_lighting_preview = true;
    bool require_pbr_textures = true;
};

struct SettingsValidationResult {
    bool ok = true;
    std::vector<std::string> messages;
};

class EditorProjectSettingsStore {
public:
    static std::string to_text(const EditorProjectSettings& settings);
    static EditorProjectSettings from_text(const std::string& text);
    static SettingsValidationResult validate(const EditorProjectSettings& settings);

    static const char* backend_name(RenderBackend backend) noexcept;
    static const char* template_name(ProjectTemplate project_template) noexcept;
};

} // namespace Balmung::Engine::Editor
