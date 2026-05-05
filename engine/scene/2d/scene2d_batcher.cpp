#include "scene2d_batcher.hpp"

#include <algorithm>
#include <utility>

namespace Balmung::Engine::Scene2D {

bool SpriteKey::operator==(const SpriteKey& other) const noexcept
{
    return texture_asset == other.texture_asset &&
           material_asset == other.material_asset &&
           sort_layer == other.sort_layer &&
           normal_mapped == other.normal_mapped;
}

void Scene2DBatcher::clear()
{
    _sprites.clear();
    _batches.clear();
}

void Scene2DBatcher::add(LitSprite sprite)
{
    _sprites.push_back(std::move(sprite));
}

void Scene2DBatcher::build()
{
    _batches.clear();
    std::stable_sort(_sprites.begin(), _sprites.end(), [](const LitSprite& a, const LitSprite& b) {
        if (a.key.sort_layer != b.key.sort_layer) return a.key.sort_layer < b.key.sort_layer;
        if (a.z != b.z) return a.z < b.z;
        if (a.key.material_asset != b.key.material_asset) return a.key.material_asset < b.key.material_asset;
        return a.key.texture_asset < b.key.texture_asset;
    });

    for (std::uint32_t i = 0; i < _sprites.size();) {
        const auto first = i;
        const auto key = _sprites[i].key;
        while (i < _sprites.size() && _sprites[i].key == key) {
            ++i;
        }
        _batches.push_back(SpriteBatch{
            .key = key,
            .first = first,
            .count = i - first,
        });
    }
}

} // namespace Balmung::Engine::Scene2D
