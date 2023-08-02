#include "anim_list.hpp"
#include "gui/elements/window_title_bar_button.hpp"
#include "gui/build.hpp"



GuiAnimSyncListToolbar::GuiAnimSyncListToolbar() {
    setSize(0, gui::em(2));
    padding = gfxm::rect(GUI_PADDING, 0, GUI_PADDING, 0);
    margin = gfxm::rect(0, 0, 0, 0);
    overflow = GUI_OVERFLOW_FIT;
    guiAdd(this, this, new GuiWindowTitleBarButton(guiLoadIcon("svg/entypo/add-to-list.svg"), GUI_MSG::UNKNOWN), GUI_FLAG_SAME_LINE);
    guiAdd(this, this, new GuiWindowTitleBarButton(guiLoadIcon("svg/entypo/plus.svg"), GUI_MSG::UNKNOWN), GUI_FLAG_SAME_LINE);
    guiAdd(this, this, new GuiWindowTitleBarButton(guiLoadIcon("svg/entypo/minus.svg"), GUI_MSG::UNKNOWN), GUI_FLAG_SAME_LINE);
}

GuiAnimationSyncList::GuiAnimationSyncList() {
    setSize(0, 350);

    guiAdd(this, this, new GuiAnimSyncListToolbar(), GUI_FLAG_FRAME);
    auto group = guiAdd(this, this, new GuiAnimSyncListGroup());
    guiAdd(group, group, new GuiAnimSyncListItem());
    guiAdd(group, group, new GuiAnimSyncListItem());
    guiAdd(group, group, new GuiAnimSyncListItem());
    group = guiAdd(this, this, new GuiAnimSyncListGroup());
    guiAdd(group, group, new GuiAnimSyncListItem());
    guiAdd(this, this, new GuiAnimSyncListGroup());

    {
        using namespace gui::build;


    }
}