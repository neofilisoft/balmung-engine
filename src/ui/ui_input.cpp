#include "ui/ui_input.hpp"

namespace VoxelBlock::UI {
namespace {
constexpr std::uint8_t bit_for(PointerButton b) noexcept
{
    return static_cast<std::uint8_t>(1u << static_cast<unsigned>(b));
}
} // namespace

void UIInputState::begin_frame()
{
    _events.clear();
}

void UIInputState::push_pointer_move(float x, float y)
{
    _pointer = {x, y};
    _events.push_back({.type = UIInputEventType::PointerMove, .pointer = _pointer});
}

void UIInputState::push_pointer_button(PointerButton button, bool down, float x, float y)
{
    _pointer = {x, y};
    if (down) _button_mask |= bit_for(button);
    else _button_mask &= static_cast<std::uint8_t>(~bit_for(button));

    _events.push_back({
        .type = down ? UIInputEventType::PointerDown : UIInputEventType::PointerUp,
        .pointer = _pointer,
        .button = button
    });
}

void UIInputState::push_wheel(float delta, float x, float y)
{
    _pointer = {x, y};
    _events.push_back({
        .type = UIInputEventType::MouseWheel,
        .pointer = _pointer,
        .wheel_delta = delta
    });
}

void UIInputState::push_key(std::uint32_t key_code, bool down)
{
    _events.push_back({
        .type = down ? UIInputEventType::KeyDown : UIInputEventType::KeyUp,
        .key_code = key_code
    });
}

void UIInputState::push_text(std::string utf8_text)
{
    _events.push_back({
        .type = UIInputEventType::TextInput,
        .text_utf8 = std::move(utf8_text)
    });
}

bool UIInputState::is_button_down(PointerButton button) const noexcept
{
    return (_button_mask & bit_for(button)) != 0;
}

} // namespace VoxelBlock::UI

