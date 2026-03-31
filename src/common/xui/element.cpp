#include "element.hpp"
#include "xui/host.hpp"


namespace xui {


    void Element::registerChild(Element* e) {
        if (e->layout_parent) {
            e->layout_parent->unregisterChild(e);
        }
        e->layout_parent = this;
        layout_children.push_back(e);
        e->is_style_dirty = true;
    }

    void Element::unregisterChild(Element* e) {
        auto it = std::find(layout_children.begin(), layout_children.end(), e);
        if (it != layout_children.end()) {
            layout_children.erase(it);
        }
        e->layout_parent = nullptr;
        e->is_style_dirty = true;
    }


    Element::~Element() {
        if (layout_parent) {
            layout_parent->unregisterChild(this);
        }
        for (auto* ch : layout_children) {
            ch->layout_parent = nullptr;
        }
    }

    void Element::applyStyle(Host* host) {
        if (is_style_dirty) {
            auto& sheet = host->getStyleSheet();
            GUI_STYLE_FLAGS flags = 0;
            flags |= host->isHovered(this) ? GUI_STYLE_FLAG_HOVERED : 0;
            //flags |= isPressed() ? GUI_STYLE_FLAG_PRESSED : 0;
            //flags |= isSelected() ? GUI_STYLE_FLAG_SELECTED : 0;
            //flags |= isFocused() ? GUI_STYLE_FLAG_FOCUSED : 0;
            //flags |= isActive() ? GUI_STYLE_FLAG_ACTIVE : 0;
            // TODO: Add more flags
            // Disabled, ReadOnly
            getStyle()->clear();
            sheet.select_styles(getStyle(), style_selectors, flags);
            if (layout_parent) {
                getStyle()->inherit(*layout_parent->getStyle());
            }
            getStyle()->finalize();
        }

        for (int i = 0; i < layout_children.size(); ++i) {
            Element* ch = layout_children[i];
            // TODO
            /*if (ch->is_hidden) {
            continue;
            }*/
            ch->applyStyle(host);
        }

        is_style_dirty = false;
    }

    void Element::hitTest(HitResult& hit, int x, int y) {
        onHitTest(hit, x - px_pos.x, y - px_pos.y);
    }

    void Element::draw(IRenderer* r) {
        r->pushOffset(gfxm::vec2(px_pos.x, px_pos.y));
        onDraw(r);
        r->popOffset();
    }


    gfxm::rect Element::getGlobalRect() const {
        gfxm::vec2 offs;
        const Element* e = this;
        while (e) {
            offs += gfxm::vec2(e->px_pos.x, e->px_pos.y);
            e = e->layout_parent;
        }
        return gfxm::rect(offs.x, offs.y, offs.x + px_size.x, offs.y + px_size.y);
    }
    gui::style* Element::getStyle() {
        if (!style) {
            style.reset(new gui::style);
        }
        return style.get();
    }
}

