#include "popup_layer.hpp"


GuiPopupLayer::GuiPopupLayer() {
    addFlags(GUI_FLAG_NO_HIT);
}
void GuiPopupLayer::onHitTest(GuiHitResult& hit, int x, int y) {
    GuiElement::onHitTest(hit, x, y);
}

bool GuiPopupLayer::onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
    return GuiElement::onMessage(msg, params);
}

void GuiPopupLayer::onLayoutOld(const gui_layout_context& ctx) {
    if (ctx.flags & GUI_LAYOUT_WIDTH_PASS) {
        rc_bounds.min.x = 0;
        rc_bounds.max.x = ctx.width.value_or(0);
        client_area = rc_bounds;
    }

    if (ctx.flags & GUI_LAYOUT_HEIGHT_PASS) {
        rc_bounds.min.y = 0;
        rc_bounds.max.y = ctx.height.value_or(0);
        client_area = rc_bounds;
    }

    if (ctx.flags & GUI_LAYOUT_POSITION_PASS) {
        for (int i = 0; i < children.size(); ++i) {
            auto ch = children[i];
            if (ch->isHidden()) {
                continue;
            }

            gui_float width = gui_float_convert(ch->size.x, ch->getFont(), rc_bounds.max.x - rc_bounds.min.x);
            gui_float height = gui_float_convert(ch->size.y, ch->getFont(), rc_bounds.max.y - rc_bounds.min.y);
            if (width.unit == gui_fill) {
                width = gui::content();
            }
            if (height.unit == gui_fill) {
                height = gui::content();
            }
            gui_layout_context ctx;
            ctx.width = std::nullopt;
            ctx.height = std::nullopt;
            
            ctx.flags = GUI_LAYOUT_WIDTH_PASS;
            if (width.unit != gui_content) {
                ctx.width = width.value;
            } else {
                ctx.flags |= GUI_LAYOUT_FIT_CONTENT;
            }
            ch->layout_2(ctx);
            if (ctx.flags & GUI_LAYOUT_FIT_CONTENT) {
                ctx.width = ch->getBoundingRect().max.x - ch->getBoundingRect().min.x;
            }

            ctx.flags = GUI_LAYOUT_HEIGHT_PASS;
            if (height.unit != gui_content) {
                ctx.height = height.value;
            } else {
                ctx.flags |= GUI_LAYOUT_FIT_CONTENT;
            }
            ch->layout_2(ctx);
            if (ctx.flags & GUI_LAYOUT_FIT_CONTENT) {
                ctx.height = ch->getBoundingRect().max.y - ch->getBoundingRect().min.y;
            }

            ctx.flags = GUI_LAYOUT_POSITION_PASS;
            ch->layout_2(ctx);

            ch->layout_position.x = ch->pos.x.value;
            ch->layout_position.y = ch->pos.y.value;
        }
    }
}

void GuiPopupLayer::onDraw() {
    GuiElement::onDraw();
}

