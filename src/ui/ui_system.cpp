#include "ui/ui_system.hpp"

#include <algorithm>

namespace Balmung::UI {

UISystem::UISystem()
{
    _root = create_widget("Root");
    if (auto* root = try_get(_root)) {
        root->layout = WidgetLayout::Absolute;
        root->draw_background = false;
    }
}

WidgetId UISystem::create_widget(std::string name)
{
    const WidgetId id = _next_id++;
    auto [it, _] = _widgets.emplace(id, UIWidget{});
    auto& w = it->second;
    w.id = id;
    w.name = std::move(name);
    w.rect = {0, 0, 100, 24};
    w.computed_rect = w.rect;
    if (_root != 0 && id != _root) {
        set_parent(id, _root);
    }
    return id;
}

bool UISystem::destroy_widget(WidgetId id)
{
    if (id == 0 || id == _root) return false;
    auto it = _widgets.find(id);
    if (it == _widgets.end()) return false;

    auto children = it->second.children;
    for (auto child : children) {
        destroy_widget(child);
    }

    if (it->second.parent != 0) {
        if (auto* p = try_get(it->second.parent)) {
            p->children.erase(std::remove(p->children.begin(), p->children.end(), id), p->children.end());
        }
    }

    if (_hovered_widget == id) _hovered_widget = 0;
    if (_focused_widget == id) _focused_widget = 0;
    _widgets.erase(it);
    return true;
}

bool UISystem::set_parent(WidgetId child, WidgetId parent)
{
    if (child == 0 || parent == 0 || child == parent) return false;
    auto* c = try_get(child);
    auto* p = try_get(parent);
    if (!c || !p) return false;

    if (c->parent == parent) return true;

    if (c->parent != 0) {
        if (auto* old = try_get(c->parent)) {
            old->children.erase(std::remove(old->children.begin(), old->children.end(), child), old->children.end());
        }
    }

    c->parent = parent;
    p->children.push_back(child);
    return true;
}

UIWidget* UISystem::try_get(WidgetId id)
{
    auto it = _widgets.find(id);
    return it == _widgets.end() ? nullptr : &it->second;
}

const UIWidget* UISystem::try_get(WidgetId id) const
{
    auto it = _widgets.find(id);
    return it == _widgets.end() ? nullptr : &it->second;
}

void UISystem::update_layout()
{
    auto* root = try_get(_root);
    if (!root) return;
    root->computed_rect = _canvas.canvas_rect();
    layout_children(*root);
}

void UISystem::layout_children(UIWidget& widget)
{
    if (widget.children.empty()) return;

    const Rect content = inset(widget.computed_rect, widget.padding);
    float cursor_x = content.x;
    float cursor_y = content.y;

    for (auto child_id : widget.children) {
        auto* child = try_get(child_id);
        if (!child || !child->visible) continue;

        if (widget.layout == WidgetLayout::VerticalStack) {
            child->computed_rect = {content.x + child->rect.x, cursor_y + child->rect.y, child->rect.w, child->rect.h};
            cursor_y += child->rect.h;
        } else if (widget.layout == WidgetLayout::HorizontalStack) {
            child->computed_rect = {cursor_x + child->rect.x, content.y + child->rect.y, child->rect.w, child->rect.h};
            cursor_x += child->rect.w;
        } else {
            child->computed_rect = {
                widget.computed_rect.x + child->rect.x,
                widget.computed_rect.y + child->rect.y,
                child->rect.w,
                child->rect.h
            };
        }

        layout_children(*child);
    }
}

void UISystem::update(float, const UIInputState& input)
{
    update_hover_and_focus(input);
}

void UISystem::update_hover_and_focus(const UIInputState& input)
{
    const auto pt = _canvas.screen_to_canvas(input.pointer_position());
    _hovered_widget = hit_test(pt);

    for (const auto& ev : input.events()) {
        if (ev.type == UIInputEventType::PointerDown && ev.button == PointerButton::Left) {
            _focused_widget = hit_test(_canvas.screen_to_canvas(ev.pointer));
        }
    }
}

WidgetId UISystem::hit_test(Vec2 canvas_point) const
{
    WidgetId hit = 0;
    for (const auto& [id, widget] : _widgets) {
        if (id == _root) continue;
        if (!widget.visible || !widget.enabled) continue;
        if (widget.computed_rect.contains(canvas_point)) {
            hit = id;
        }
    }
    return hit;
}

void UISystem::build_draw_commands(UIRenderer& renderer, const UIFont& default_font) const
{
    for (const auto& [id, widget] : _widgets) {
        if (id == _root) continue;
        if (!widget.visible) continue;
        emit_widget_draw(widget, renderer, default_font);
    }
}

void UISystem::emit_widget_draw(const UIWidget& widget, UIRenderer& renderer, const UIFont& default_font) const
{
    const int base_layer = 100;

    if (widget.draw_background) {
        if (widget.panel_9slice) {
            Sprite9Slice slice(*widget.panel_9slice);
            const auto quads = slice.build(widget.computed_rect, widget.background);
            for (const auto& q : quads) {
                renderer.submit_sprite({
                    .rect = q.dest,
                    .uv = q.uv,
                    .tint = q.tint,
                    .texture_asset = widget.panel_9slice->texture_asset,
                    .layer = base_layer
                });
            }
        } else {
            renderer.submit_rect({
                .rect = widget.computed_rect,
                .color = widget.background,
                .layer = base_layer
            });
        }
    }

    if (widget.text && !widget.text->empty()) {
        const auto text_pos = Vec2{widget.computed_rect.x + 6.0f, widget.computed_rect.y + 4.0f};
        renderer.submit_text(text_pos, *widget.text, default_font, widget.text_color, base_layer + 1);
    }
}

} // namespace Balmung::UI



