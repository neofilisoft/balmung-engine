#include "asset_import_pipeline.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

namespace Balmung::Engine::Editor {
namespace {

std::string lower_copy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool contains_any(const std::string& value, const std::vector<std::string>& needles)
{
    return std::any_of(needles.begin(), needles.end(), [&](const std::string& needle) {
        return value.find(needle) != std::string::npos;
    });
}

} // namespace

ImportPlan AssetImportPipeline::plan_imports(const std::vector<std::filesystem::path>& paths,
                                             const std::filesystem::path& destination_root) const
{
    ImportPlan plan;
    for (const auto& path : paths) {
        ImportJob job;
        job.source_path = path;
        job.destination_root = destination_root;
        job.kind = classify_asset(path);
        job.texture_semantic = job.kind == AssetKind::Texture ? classify_texture_semantic(path) : TextureSemantic::Unknown;
        job.compress_gpu = job.kind == AssetKind::Texture;
        job.generate_mipmaps = job.kind == AssetKind::Texture || job.kind == AssetKind::Mesh;
        job.build_ray_tracing_proxy = job.kind == AssetKind::Mesh;
        plan.jobs.push_back(std::move(job));

        if (plan.jobs.back().kind == AssetKind::Unknown) {
            plan.warnings.push_back("Unknown asset type: " + path.generic_string());
        }
        if (plan.jobs.back().kind == AssetKind::Texture &&
            plan.jobs.back().texture_semantic == TextureSemantic::Unknown) {
            plan.warnings.push_back("Texture semantic not detected: " + path.generic_string());
        }
    }
    return plan;
}

AssetKind AssetImportPipeline::classify_asset(const std::filesystem::path& path)
{
    const auto ext = lower_copy(path.extension().string());
    if (ext == ".fbx" || ext == ".gltf" || ext == ".glb" || ext == ".obj") return AssetKind::Mesh;
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".exr" || ext == ".hdr") return AssetKind::Texture;
    if (ext == ".bmat" || ext == ".mat" || ext == ".material") return AssetKind::Material;
    if (ext == ".bmsc" || ext == ".scene" || ext == ".json") return AssetKind::Scene;
    if (ext == ".wav" || ext == ".ogg" || ext == ".flac") return AssetKind::Audio;
    if (ext == ".lua" || ext == ".cs") return AssetKind::Script;
    return AssetKind::Unknown;
}

TextureSemantic AssetImportPipeline::classify_texture_semantic(const std::filesystem::path& path)
{
    const auto stem = lower_copy(path.stem().string());
    if (contains_any(stem, {"basecolor", "base_color", "albedo", "diffuse"})) return TextureSemantic::BaseColor;
    if (contains_any(stem, {"normal", "_nrm", "_nor"})) return TextureSemantic::Normal;
    if (contains_any(stem, {"metallic", "metalness", "_metal"})) return TextureSemantic::Metallic;
    if (contains_any(stem, {"roughness", "_rough"})) return TextureSemantic::Roughness;
    if (contains_any(stem, {"ambientocclusion", "ambient_occlusion", "_ao"})) return TextureSemantic::AmbientOcclusion;
    if (contains_any(stem, {"emissive", "emission", "_emit"})) return TextureSemantic::Emissive;
    if (contains_any(stem, {"height", "displacement", "_disp"})) return TextureSemantic::Height;
    return TextureSemantic::Unknown;
}

std::string AssetImportPipeline::asset_kind_name(AssetKind kind)
{
    switch (kind) {
    case AssetKind::Mesh: return "Mesh";
    case AssetKind::Texture: return "Texture";
    case AssetKind::Material: return "Material";
    case AssetKind::Scene: return "Scene";
    case AssetKind::Audio: return "Audio";
    case AssetKind::Script: return "Script";
    case AssetKind::Unknown: return "Unknown";
    }
    return "Unknown";
}

} // namespace Balmung::Engine::Editor
