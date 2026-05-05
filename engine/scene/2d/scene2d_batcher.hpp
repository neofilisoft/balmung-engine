#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Balmung::Engine::Scene2D {

struct SpriteKey {
    std::string texture_asset;
    std::string material_asset;
    std::uint32_t sort_layer = 0;
    bool normal_mapped = false;

    [[nodiscard]] bool operator==(const SpriteKey& other) const noexcept;
};

struct LitSprite {
    SpriteKey key;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float width = 16.0f;
    float height = 16.0f;
    float rotation_deg = 0.0f;
    float color[4]{1.0f, 1.0f, 1.0f, 1.0f};
    float emissive = 0.0f;
};

struct SpriteBatch {
    SpriteKey key;
    std::uint32_t first = 0;
    std::uint32_t count = 0;
};

class Scene2DBatcher {
public:
    void clear();
    void add(LitSprite sprite);
    void build();

    [[nodiscard]] const std::vector<LitSprite>& sprites() const noexcept { return _sprites; }
    [[nodiscard]] const std::vector<SpriteBatch>& batches() const noexcept { return _batches; }
    [[nodiscard]] std::uint32_t draw_call_count() const noexcept { return static_cast<std::uint32_t>(_batches.size()); }

private:
    std::vector<LitSprite> _sprites;
    std::vector<SpriteBatch> _batches;
};

} // namespace Balmung::Engine::Scene2D
