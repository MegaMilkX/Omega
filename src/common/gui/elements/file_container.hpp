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

    bool startsWithCI(std::string_view text, std::string_view prefix) {
        if (prefix.size() > text.size()) return false;
        for (size_t i = 0; i < prefix.size(); ++i)
            if (std::tolower((unsigned char)text[i]) != std::tolower((unsigned char)prefix[i]))
                return false;
        return true;
    }
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

    int itemCount() const {
        return items.size();
    }
    GuiFileListItem* getItem(int i) {
        return items[i].get();
    }

    void clearItems() {
        for (int i = 0; i < items.size(); ++i) {
            items[i]->setOwner(0);
            removeChild(items[i].get());
        }
        items.clear();
        resetScroll();
    }

    void resetScroll() {
        pos_content = gfxm::vec2(0, 0);
        target_pos_content = gfxm::vec2(0, 0);
    }

    void scrollTo(GuiFileListItem* item) {
        auto rc_cont = rc_bounds;
        auto rc_item = item->getBoundingRect();
        rc_item.min += -pos_content + item->layout_position;
        rc_item.max += -pos_content + item->layout_position;

        gfxm::vec2 delta(0, 0);

        if (rc_item.min.y < rc_bounds.min.y) {
            delta.y = rc_bounds.min.y - rc_item.min.y;
        } else if (rc_item.max.y > rc_bounds.max.y) {
            delta.y = rc_bounds.max.y - rc_item.max.y;
        }

        if (rc_item.min.x < rc_bounds.min.x) {
            delta.x = rc_bounds.min.x - rc_item.min.x;
        } else if (rc_item.max.x > rc_bounds.max.x) {
            delta.x = rc_bounds.max.x - rc_item.max.x;
        }

        if (delta.x == 0 && delta.y == 0) {
            return;
        }

        pos_content -= delta;
        target_pos_content = pos_content;
    }

    GuiFileListItem* findFirstMatch(const std::string& filter) {
        for (int i = 0; i < items.size(); ++i) {
            if (items[i]->isHidden()) {
                continue;
            }
            if (startsWithCI(items[i]->getName(), filter)) {
                return items[i].get();
            }
        }
        return nullptr;
    }
    GuiFileListItem* findNextMatch(GuiFileListItem* current, const std::string& prefix, bool wrap) {
        auto begin = items.begin();
        auto start = begin;
        if (current) {
            start = std::find_if(items.begin(), items.end(),
                [current](auto& p) { return p.get() == current; });
            if (start != items.end()) ++start;
        }
        for (auto it = start; it != items.end(); ++it)
            if (startsWithCI((*it)->getName(), prefix)) return it->get();
        if (wrap)
            for (auto it = begin; it != start && it != items.end(); ++it)
                if (startsWithCI((*it)->getName(), prefix)) return it->get();
        return nullptr;
    }
};

