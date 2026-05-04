#pragma once

#include "ecs/components.hpp"
#include "ecs/registry.hpp"
#include "scene/scene_document.hpp"

#include <filesystem>
#include <string>

namespace Balmung::Scene {

class SceneSerializer {
public:
    static SceneDocument capture_registry(const ECS::Registry& registry, std::string scene_name = "scene");
    static std::string to_json(const SceneDocument& doc);
    static std::string to_bmsc_text(const SceneDocument& doc);
    static bool save_json_file(const SceneDocument& doc, const std::filesystem::path& path, std::string* error = nullptr);
    static bool save_bmsc_file(const SceneDocument& doc, const std::filesystem::path& path, std::string* error = nullptr);
};

} // namespace Balmung::Scene



