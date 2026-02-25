#include "ui/sprite9slice.hpp"

namespace VoxelBlock::UI {
namespace {

Rect make_uv(float tx, float ty, float tw, float th, float x, float y, float w, float h) noexcept
{
    if (tw <= 0.0f || th <= 0.0f) return {};
    return {x / tw, y / th, w / tw, h / th};
}

} // namespace

std::array<SpriteQuad, 9> Sprite9Slice::build(Rect dest, Color tint) const noexcept
{
    std::array<SpriteQuad, 9> out{};

    const auto& b = _desc.border;
    const float left = b.left;
    const float top = b.top;
    const float right = b.right;
    const float bottom = b.bottom;

    const float center_w = dest.w - left - right;
    const float center_h = dest.h - top - bottom;

    const float txw = _desc.texture_width > 0.0f ? _desc.texture_width : 1.0f;
    const float txh = _desc.texture_height > 0.0f ? _desc.texture_height : 1.0f;
    const float uv_center_w = txw - left - right;
    const float uv_center_h = txh - top - bottom;

    const Rect rects[9] = {
        {dest.x, dest.y, left, top},
        {dest.x + left, dest.y, center_w, top},
        {dest.x + dest.w - right, dest.y, right, top},
        {dest.x, dest.y + top, left, center_h},
        {dest.x + left, dest.y + top, center_w, center_h},
        {dest.x + dest.w - right, dest.y + top, right, center_h},
        {dest.x, dest.y + dest.h - bottom, left, bottom},
        {dest.x + left, dest.y + dest.h - bottom, center_w, bottom},
        {dest.x + dest.w - right, dest.y + dest.h - bottom, right, bottom},
    };

    const Rect uvs[9] = {
        make_uv(txw, txh, txw, txh, 0, 0, left, top),
        make_uv(txw, txh, txw, txh, left, 0, uv_center_w, top),
        make_uv(txw, txh, txw, txh, txw - right, 0, right, top),
        make_uv(txw, txh, txw, txh, 0, top, left, uv_center_h),
        make_uv(txw, txh, txw, txh, left, top, uv_center_w, uv_center_h),
        make_uv(txw, txh, txw, txh, txw - right, top, right, uv_center_h),
        make_uv(txw, txh, txw, txh, 0, txh - bottom, left, bottom),
        make_uv(txw, txh, txw, txh, left, txh - bottom, uv_center_w, bottom),
        make_uv(txw, txh, txw, txh, txw - right, txh - bottom, right, bottom),
    };

    for (int i = 0; i < 9; ++i) {
        out[i].dest = rects[i];
        out[i].uv = uvs[i];
        out[i].tint = tint;
    }
    return out;
}

} // namespace VoxelBlock::UI

