#include "notification_box.hpp"

#include "IconsForkAwesome.h"


namespace xui {


    NotificationBox::NotificationBox() {
        size = gui_vec2(gui::fill(), gui::em(4));
        //min_size = gui_vec2(gui::px(0), gui::em(4));
        style_selectors = { "notification-box" };

        icon.size = gui_vec2(gui::perc(25), gui::fill());
        icon.style_selectors = { "notification-icon" };
        icon.setText(ICON_FK_EXCLAMATION_TRIANGLE);

        caption.size = gui_vec2(gui::fill(), gui::fill());
        caption.min_size = gui_vec2(gui::px(0), gui::content());
        caption.setText("This is a notification box");
        caption.same_line = true;

        addToLayout(&icon);
        addToLayout(&caption);

        setContentTarget(nullptr);
    }


}