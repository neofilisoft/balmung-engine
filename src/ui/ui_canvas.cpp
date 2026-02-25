#include "ui/ui_canvas.hpp"

namespace VoxelBlock::UI {

void UICanvas::set_viewport(float width, float height) noexcept
{
    _viewport_size.x = width > 1.0f ? width : 1.0f;
    _viewport_size.y = height > 1.0f ? height : 1.0f;
}

Vec2 UICanvas::logical_size() const noexcept
{
    if (_desc.space == CanvasSpace::ScreenSpace) {
        return {_desc.width > 0.0f ? _desc.width : _viewport_size.x,
                _desc.height > 0.0f ? _desc.height : _viewport_size.y};
    }
    return {_desc.width, _desc.height};
}

Vec2 UICanvas::screen_to_canvas(Vec2 p) const noexcept
{
    const auto ls = logical_size();
    if (_viewport_size.x <= 0.0f || _viewport_size.y <= 0.0f) return p;
    return {
        p.x * (ls.x / _viewport_size.x),
        p.y * (ls.y / _viewport_size.y)
    };
}

Vec2 UICanvas::canvas_to_screen(Vec2 p) const noexcept
{
    const auto ls = logical_size();
    if (ls.x <= 0.0f || ls.y <= 0.0f) return p;
    return {
        p.x * (_viewport_size.x / ls.x),
        p.y * (_viewport_size.y / ls.y)
    };
}

} // namespace VoxelBlock::UI

