#pragma once


#include "gui/elements/file_list_item.hpp"

#include "gui/elements/tree_item.hpp"
#include "gui/elements/tree_handler.hpp"
#include "gui/elements/scroll_bar.hpp"

class GuiFileContainer : public GuiElement {
    int visible_items_begin = 0;
    int visible_items_end = 0;
    gfxm::vec2 smooth_scroll = gfxm::vec2(.0f, .0f);

    std::unique_ptr<GuiScrollBarV> scroll_bar_v;
    gfxm::rect rc_scroll_v;
    gfxm::rect rc_scroll_h;

    std::vector<std::unique_ptr<GuiFileListItem>> items;
public:
    gfxm::vec2 scroll_offset = gfxm::vec2(.0f, .0f);

    GuiFileContainer() {
        setSize(gui::fill(), gui::fill());
        setStyleClasses({ "file-container" });
        addFlags(GUI_FLAG_ENABLE_WRAPPING);
        primary_axis = GUI_PRIMARY_AXIS::X;

        scroll_bar_v.reset(new GuiScrollBarV());
        scroll_bar_v->setOwner(this);
    }

    GuiFileListItem* addItem(const char* name, bool is_dir, const guiFileThumbnail* thumb = 0) {
        auto ptr = new GuiFileListItem(name, thumb);
        ptr->is_directory = is_dir;
        items.emplace_back(std::unique_ptr<GuiFileListItem>(ptr));
        addChild(ptr);
        ptr->setOwner(this);
        return ptr;
    }
    void removeItem(GuiFileListItem* item) {
        // TODO:
    }
    void clearItems() {
        for (int i = 0; i < items.size(); ++i) {
            items[i]->setOwner(0);
            removeChild(items[i].get());
        }
        items.clear();
    }
};

