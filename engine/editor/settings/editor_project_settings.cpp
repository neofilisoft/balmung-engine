#include "editor_project_settings.hpp"

#include <sstream>
#include <string_view>

namespace Balmung::Engine::Editor {
namespace {

std::string bool_text(bool value)
{
    return value ? "true" : "false";
}

bool parse_bool(std::string_view value, bool fallback) noexcept
{
    if (value == "true" || value == "1" || value == "yes") return true;
    if (value == "false" || value == "0" || value == "no") return false;
    return fallback;
}

Scene3D::RenderQualityTier quality_from_text(std::string_view value) noexcept
{
    if (value == "preview") return Scene3D::RenderQualityTier::Preview;
    if (value == "cinematic") return Scene3D::RenderQualityTier::Cinematic;
    if (value == "offline") return Scene3D::RenderQualityTier::OfflineReference;
    return Scene3D::RenderQualityTier::Gameplay;
}

std::string quality_text(Scene3D::RenderQualityTier tier)
{
    switch (tier) {
    case Scene3D::RenderQualityTier::Preview: return "preview";
    case Scene3D::RenderQualityTier::Gameplay: return "gameplay";
    case Scene3D::RenderQualityTier::Cinematic: return "cinematic";
    case Scene3D::RenderQualityTier::OfflineReference: return "offline";
    }
    return "gameplay";
}

} // namespace

const char* EditorProjectSettingsStore::backend_name(RenderBackend backend) noexcept
{
    switch (backend) {
    case RenderBackend::OpenGL: return "OpenGL";
    case RenderBackend::Vulkan: return "Vulkan";
    case RenderBackend::Direct3D12: return "Direct3D12";
    case RenderBackend::Metal: return "Metal";
    }
    return "Vulkan";
}

const char* EditorProjectSettingsStore::template_name(ProjectTemplate project_template) noexcept
{
    switch (project_template) {
    case ProjectTemplate::Game2D: return "2D";
    case ProjectTemplate::HD2D: return "HD-2D";
    case ProjectTemplate::Game3D: return "3D";
    case ProjectTemplate::Cinematic3D: return "Cinematic3D";
    }
    return "3D";
}

std::string EditorProjectSettingsStore::to_text(const EditorProjectSettings& settings)
{
    std::ostringstream out;
    out << "project_name=" << settings.project_name << "\n";
    out << "template=" << template_name(settings.project_template) << "\n";
    out << "backend=" << backend_name(settings.preferred_backend) << "\n";
    out << "quality=" << quality_text(settings.render_profile.quality) << "\n";
    out << "asset_root=" << settings.asset_root.generic_string() << "\n";
    out << "scene_root=" << settings.scene_root.generic_string() << "\n";
    out << "hot_reload=" << bool_text(settings.enable_hot_reload) << "\n";
    out << "live_lighting_preview=" << bool_text(settings.enable_live_lighting_preview) << "\n";
    out << "require_pbr_textures=" << bool_text(settings.require_pbr_textures) << "\n";
    return out.str();
}

EditorProjectSettings EditorProjectSettingsStore::from_text(const std::string& text)
{
    EditorProjectSettings settings{};
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        const auto eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        const auto key = std::string_view(line.data(), eq);
        const auto value = std::string_view(line.data() + eq + 1, line.size() - eq - 1);

        if (key == "project_name") settings.project_name = std::string(value);
        else if (key == "template" && value == "2D") settings.project_template = ProjectTemplate::Game2D;
        else if (key == "template" && value == "HD-2D") settings.project_template = ProjectTemplate::HD2D;
        else if (key == "template" && value == "Cinematic3D") settings.project_template = ProjectTemplate::Cinematic3D;
        else if (key == "template") settings.project_template = ProjectTemplate::Game3D;
        else if (key == "backend" && value == "OpenGL") settings.preferred_backend = RenderBackend::OpenGL;
        else if (key == "backend" && value == "Direct3D12") settings.preferred_backend = RenderBackend::Direct3D12;
        else if (key == "backend" && value == "Metal") settings.preferred_backend = RenderBackend::Metal;
        else if (key == "backend") settings.preferred_backend = RenderBackend::Vulkan;
        else if (key == "quality") settings.render_profile = Scene3D::PbrGiScene::recommended_profile(quality_from_text(value));
        else if (key == "asset_root") settings.asset_root = std::string(value);
        else if (key == "scene_root") settings.scene_root = std::string(value);
        else if (key == "hot_reload") settings.enable_hot_reload = parse_bool(value, settings.enable_hot_reload);
        else if (key == "live_lighting_preview") settings.enable_live_lighting_preview = parse_bool(value, settings.enable_live_lighting_preview);
        else if (key == "require_pbr_textures") settings.require_pbr_textures = parse_bool(value, settings.require_pbr_textures);
    }
    return settings;
}

SettingsValidationResult EditorProjectSettingsStore::validate(const EditorProjectSettings& settings)
{
    SettingsValidationResult result{};
    if (settings.project_name.empty()) {
        result.ok = false;
        result.messages.emplace_back("Project name is empty.");
    }
    if (settings.asset_root.empty()) {
        result.ok = false;
        result.messages.emplace_back("Asset root is empty.");
    }
    if (settings.scene_root.empty()) {
        result.ok = false;
        result.messages.emplace_back("Scene root is empty.");
    }
    Scene3D::PbrGiScene probe{settings.render_profile};
    for (const auto& issue : probe.validate()) {
        result.ok = false;
        result.messages.push_back(issue.code + ": " + issue.message);
    }
    return result;
}

} // namespace Balmung::Engine::Editor
