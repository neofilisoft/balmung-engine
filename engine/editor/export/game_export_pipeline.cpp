#include "game_export_pipeline.hpp"

namespace Balmung::Engine::Editor {

const char* GameExportPipeline::platform_name(ExportPlatform platform) noexcept
{
    switch (platform) {
    case ExportPlatform::Windows: return "Windows";
    case ExportPlatform::Linux: return "Linux";
    case ExportPlatform::WebAssembly: return "WebAssembly";
    }
    return "Windows";
}

ExportReport GameExportPipeline::prepare_export(const EditorProjectSettings& settings,
                                                const ExportProfile& profile) const
{
    ExportReport report;
    const auto validation = EditorProjectSettingsStore::validate(settings);
    if (!validation.ok) {
        report.ready = false;
        report.blockers.insert(report.blockers.end(), validation.messages.begin(), validation.messages.end());
    }
    if (profile.output_dir.empty()) {
        report.ready = false;
        report.blockers.emplace_back("Export output directory is empty.");
    }
    if (profile.platform == ExportPlatform::WebAssembly &&
        settings.render_profile.gi.technique == Scene3D::GiTechnique::HardwareRayTracing) {
        report.ready = false;
        report.blockers.emplace_back("WebAssembly export cannot use hardware ray-traced GI.");
    }

    report.steps.push_back("Collect scenes from " + settings.scene_root.generic_string());
    report.steps.push_back("Collect assets from " + settings.asset_root.generic_string());
    if (profile.bake_gi) {
        report.steps.emplace_back("Bake GI probes and reflection probes.");
    }
    if (profile.build_ray_tracing_data) {
        report.steps.emplace_back("Build mesh BLAS metadata and scene TLAS manifest.");
    }
    report.steps.push_back(std::string("Package ") + platform_name(profile.platform) + " player.");
    return report;
}

} // namespace Balmung::Engine::Editor
