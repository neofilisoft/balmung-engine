#pragma once

#include "export/game_export_pipeline.hpp"
#include "gui/viewport_render_controller.hpp"
#include "import/asset_import_pipeline.hpp"
#include "settings/editor_project_settings.hpp"

namespace Balmung::Engine::Editor {

class EditorModule {
public:
    EditorModule();

    [[nodiscard]] EditorProjectSettings& settings() noexcept { return _settings; }
    [[nodiscard]] const EditorProjectSettings& settings() const noexcept { return _settings; }
    [[nodiscard]] ViewportRenderController& viewport() noexcept { return _viewport; }
    [[nodiscard]] AssetImportPipeline& importer() noexcept { return _importer; }
    [[nodiscard]] GameExportPipeline& exporter() noexcept { return _exporter; }

    void apply_cinematic_3d_defaults();

private:
    EditorProjectSettings _settings;
    ViewportRenderController _viewport;
    AssetImportPipeline _importer;
    GameExportPipeline _exporter;
};

} // namespace Balmung::Engine::Editor
