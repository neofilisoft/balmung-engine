#include "ui/ui_renderer.hpp"

#include <algorithm>

namespace Balmung::UI {

void UIRenderer::begin_frame()
{
    _rects.clear();
    _sprites.clear();
    _texts.clear();
    _stats = {};
}

void UIRenderer::submit_rect(UIRectCommand cmd)
{
    _rects.push_back(std::move(cmd));
}

void UIRenderer::submit_sprite(UISpriteCommand cmd)
{
    _sprites.push_back(std::move(cmd));
}

void UIRenderer::submit_text(UITextCommand cmd)
{
    _texts.push_back(std::move(cmd));
}

void UIRenderer::submit_text(Vec2 pos, std::string text, const UIFont& font, Color color, int layer)
{
    _texts.push_back({
        .position = pos,
        .color = color,
        .text = std::move(text),
        .font_asset = font.asset_id(),
        .pixel_size = font.pixel_size(),
        .layer = layer
    });
}

void UIRenderer::end_frame()
{
    auto by_layer = [](const auto& a, const auto& b) { return a.layer < b.layer; };
    std::sort(_rects.begin(), _rects.end(), by_layer);
    std::sort(_sprites.begin(), _sprites.end(), by_layer);
    std::sort(_texts.begin(), _texts.end(), by_layer);

    _stats.rect_count = _rects.size();
    _stats.sprite_count = _sprites.size();
    _stats.text_count = _texts.size();

    std::size_t sprite_draw_calls = 0;
    std::string last_tex;
    int last_layer = 0x7fffffff;
    for (const auto& s : _sprites) {
        if (sprite_draw_calls == 0 || s.layer != last_layer || s.texture_asset != last_tex) {
            ++sprite_draw_calls;
            last_layer = s.layer;
            last_tex = s.texture_asset;
        }
    }

    _stats.draw_calls = (_rects.empty() ? 0u : 1u)
                      + sprite_draw_calls
                      + (_texts.empty() ? 0u : 1u);
}

} // namespace Balmung::UI



