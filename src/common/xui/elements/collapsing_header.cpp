#include "collapsing_header.hpp"

#include "IconsForkAwesome.h"


namespace xui {


    CollapsingHeader::CollapsingHeader()
    : icon_arrow(ICON_FK_CARET_DOWN), caption("CollapsingHeader"), icon_close(ICON_FK_TIMES) {
        size = gui_vec2(gui::fill(), gui::content());

        icon_arrow.style_selectors = { "icon" };
        icon_arrow.size = gui_vec2(gui::em(1), gui::fill());
        icon_arrow.hit_type = HIT::NOWHERE;

        caption.style_selectors = { "xui-label" };
        caption.same_line = true;
        caption.size = gui_vec2(gui::fill(), gui::fill());
        caption.hit_type = HIT::NOWHERE;
        
        icon_close.same_line = true;
        icon_close.size = gui_vec2(gui::em(1), gui::fill());
        icon_close.style_selectors = { "icon" };

        header.addToLayout(&icon_arrow);
        header.addToLayout(&caption);
        header.addToLayout(&icon_close);

        addToLayout(&header);
        addToLayout(&content);
        setContentTarget(&content);

        header.size = gui_vec2(gui::fill(), gui::em(2.f));
        content.size = gui_vec2(gui::fill(), gui::content());
        content.draw_flags |= DRAW_FLAG_CLIP_CONTENT;

        header.style_selectors = { "collapsing-header-header" };
        content.style_selectors = { "collapsing-header-content" };

        header.subscribe<EvtClick>([this](const EvtClick& e) {
            content.setHidden(!content.isHidden());
            if (content.isHidden()) {
                icon_arrow.setText(ICON_FK_CARET_RIGHT);
            } else {
                icon_arrow.setText(ICON_FK_CARET_DOWN);
            }
        });
    }


}