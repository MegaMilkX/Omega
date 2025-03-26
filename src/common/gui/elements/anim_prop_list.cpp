#include "anim_prop_list.hpp"
#include "gui/elements/list_toolbar_button.hpp"
#include "gui/build.hpp"


GuiAnimPropListToolbar::GuiAnimPropListToolbar() {
    setSize(gui::fill(), gui::content());
    setStyleClasses({ "anim-prop-list-toolbar" });
    guiAdd(this, this, new GuiListToolbarButton(guiLoadIcon("svg/entypo/plus.svg"), GUI_MSG::UNKNOWN), GUI_FLAG_SAME_LINE);
    guiAdd(this, this, new GuiListToolbarButton(guiLoadIcon("svg/entypo/minus.svg"), GUI_MSG::UNKNOWN), GUI_FLAG_SAME_LINE);
}


GuiAnimationPropList::GuiAnimationPropList() {
    setSize(0, 350);

    guiAdd(this, this, new GuiAnimPropListToolbar(), GUI_FLAG_FRAME | GUI_FLAG_PERSISTENT);
    guiAdd(this, this, new GuiAnimPropListItem());
    guiAdd(this, this, new GuiAnimPropListItem());
    guiAdd(this, this, new GuiAnimPropListItem());
}