#include "ui/ui_font.hpp"

#include <algorithm>

namespace VoxelBlock::UI {

TextMeasureResult UIFont::measure_text(std::string_view text) const noexcept
{
    TextMeasureResult out{};
    out.height = _line_height;

    float line_w = 0.0f;
    float max_w = 0.0f;
    int lines = 1;
    const float advance = _pixel_size * 0.6f;

    for (char c : text) {
        if (c == '\n') {
            max_w = std::max(max_w, line_w);
            line_w = 0.0f;
            ++lines;
            continue;
        }
        if (c == '\t') {
            line_w += advance * 4.0f;
            continue;
        }
        line_w += advance;
    }

    max_w = std::max(max_w, line_w);
    out.width = max_w;
    out.line_count = lines;
    out.height = _line_height * static_cast<float>(lines);
    return out;
}

} // namespace VoxelBlock::UI

