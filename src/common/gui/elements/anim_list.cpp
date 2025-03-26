#include "anim_list.hpp"
#include "gui/elements/list_toolbar_button.hpp"
#include "gui/build.hpp"



GuiAnimSyncListToolbar::GuiAnimSyncListToolbar() {
    setSize(gui::fill(), gui::content());
    setStyleClasses({ "list-toolbar" });
    guiAdd(this, this, new GuiListToolbarButton(guiLoadIcon("svg/entypo/add-to-list.svg"), GUI_MSG::UNKNOWN), GUI_FLAG_SAME_LINE);
    guiAdd(this, this, new GuiListToolbarButton(guiLoadIcon("svg/entypo/plus.svg"), GUI_MSG::UNKNOWN), GUI_FLAG_SAME_LINE);
    guiAdd(this, this, new GuiListToolbarButton(guiLoadIcon("svg/entypo/minus.svg"), GUI_MSG::UNKNOWN), GUI_FLAG_SAME_LINE);
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