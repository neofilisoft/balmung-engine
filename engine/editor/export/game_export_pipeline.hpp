#pragma once

#include "../settings/editor_project_settings.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Balmung::Engine::Editor {

enum class ExportPlatform : std::uint8_t {
    Windows,
    Linux,
    WebAssembly,
};

struct ExportProfile {
    ExportPlatform platform = ExportPlatform::Windows;
    std::filesystem::path output_dir = "release";
    bool include_editor_symbols = false;
    bool strip_debug_names = true;
    bool bake_gi = true;
    bool build_ray_tracing_data = true;
};

struct ExportReport {
    bool ready = true;
    std::vector<std::string> steps;
    std::vector<std::string> blockers;
};

class GameExportPipeline {
public:
    [[nodiscard]] ExportReport prepare_export(const EditorProjectSettings& settings,
                                              const ExportProfile& profile) const;
    [[nodiscard]] static const char* platform_name(ExportPlatform platform) noexcept;
};

} // namespace Balmung::Engine::Editor
