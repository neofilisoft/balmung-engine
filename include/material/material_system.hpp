#pragma once

#include "material/material_types.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

namespace Balmung::Material {

using MaterialHandle = std::uint32_t;
using MaterialAsset = std::variant<PbrMaterial, LitSpriteMaterial>;

struct MaterialRecord {
    MaterialHandle handle = 0;
    ShadingModel model = ShadingModel::Unlit;
    MaterialAsset asset{};
};

class MaterialSystem {
public:
    MaterialSystem() = default;

    MaterialHandle register_pbr(PbrMaterial material);
    MaterialHandle register_lit_sprite(LitSpriteMaterial material);

    [[nodiscard]] const MaterialRecord* find(MaterialHandle handle) const noexcept;
    [[nodiscard]] const MaterialRecord* find_by_id(const std::string& id) const noexcept;

    bool erase(MaterialHandle handle);
    void clear();

    [[nodiscard]] std::size_t size() const noexcept { return _records.size(); }

private:
    MaterialHandle _next_handle = 1;
    std::unordered_map<MaterialHandle, MaterialRecord> _records;
    std::unordered_map<std::string, MaterialHandle> _ids;

    MaterialHandle register_asset(ShadingModel model, MaterialAsset asset, const std::string& id);
};

} // namespace Balmung::Material



