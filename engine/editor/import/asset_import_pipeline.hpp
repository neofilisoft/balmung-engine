#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace Balmung::Engine::Editor {

enum class AssetKind : std::uint8_t {
    Unknown,
    Mesh,
    Texture,
    Material,
    Scene,
    Audio,
    Script,
};

enum class TextureSemantic : std::uint8_t {
    Unknown,
    BaseColor,
    Normal,
    Metallic,
    Roughness,
    AmbientOcclusion,
    Emissive,
    Height,
};

struct ImportJob {
    std::filesystem::path source_path;
    std::filesystem::path destination_root = "assets";
    AssetKind kind = AssetKind::Unknown;
    TextureSemantic texture_semantic = TextureSemantic::Unknown;
    bool generate_mipmaps = true;
    bool compress_gpu = true;
    bool build_ray_tracing_proxy = true;
    float mesh_scale = 1.0f;
};

struct ImportPlan {
    std::vector<ImportJob> jobs;
    std::vector<std::string> warnings;
};

class AssetImportPipeline {
public:
    [[nodiscard]] ImportPlan plan_imports(const std::vector<std::filesystem::path>& paths,
                                          const std::filesystem::path& destination_root) const;

    [[nodiscard]] static AssetKind classify_asset(const std::filesystem::path& path);
    [[nodiscard]] static TextureSemantic classify_texture_semantic(const std::filesystem::path& path);
    [[nodiscard]] static std::string asset_kind_name(AssetKind kind);
};

} // namespace Balmung::Engine::Editor
