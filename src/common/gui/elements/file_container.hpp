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
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);
        setStyleClasses({ "file-container" });

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
    /*
    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }
        
        scroll_bar_v->onHitTest(hit, x, y);
        if (hit.hasHit()) {
            return;
        }

        for (int i = visible_items_begin; i < visible_items_end; ++i) {
            auto ch = getChild(i);
            ch->onHitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }

        hit.add(GUI_HIT::CLIENT, this);
        return;
    }*/
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::MOUSE_SCROLL: {
            scroll_offset.y -= params.getA<int32_t>();
            scroll_bar_v->setOffset(scroll_offset.y);
            } return true;
        case GUI_MSG::SB_THUMB_TRACK:
            scroll_offset.y = params.getA<float>();
            return true;
        }
        return false;
    }/*
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        rc_bounds = rc;
        client_area = rc_bounds;

        gfxm::expand(client_area, -GUI_MARGIN);

        Font* font = getFont();

        gfxm::vec2 item_size;
        if (!children.empty()) {
            item_size = gui_to_px(children[0]->size, font, getClientSize());
        }

        gfxm::vec2 elem_size(item_size.x + GUI_MARGIN, item_size.y);
        int elems_per_row = std::floorf((client_area.max.x - client_area.min.x) / elem_size.x);
        int visible_row_count = (client_area.max.y - client_area.min.y) / elem_size.y + 2;
        int total_row_count = 0;
        if (elems_per_row > 0) {
            total_row_count = std::ceilf(childCount() / (float)(elems_per_row));
        }

        float max_scroll = gfxm::_max(.0f, (total_row_count * elem_size.y) - (client_area.max.y - client_area.min.y));
        rc_content.min = client_area.min - pos_content;
        rc_content.max = rc_content.min + gfxm::vec2(elems_per_row * elem_size.x, total_row_count * elem_size.y);
        scroll_bar_v->setScrollBounds(.0f, getContentHeight());
        scroll_bar_v->setScrollPageLength(getContentHeight());

        scroll_offset.y = gfxm::_min(max_scroll, gfxm::_max(.0f, scroll_offset.y));
        smooth_scroll = gfxm::vec2(
            gfxm::lerp(smooth_scroll.x, scroll_offset.x, .4f),
            gfxm::lerp(smooth_scroll.y, scroll_offset.y, .4f)
        );

        visible_items_begin = gfxm::_min((int)childCount(), (int)(smooth_scroll.y / elem_size.y) * elems_per_row);
        visible_items_end = gfxm::_min((int)childCount(), visible_items_begin + visible_row_count * elems_per_row);
        int small_offset = smooth_scroll.y - (int)(smooth_scroll.y / elem_size.y) * elem_size.y;

        gfxm::vec2 cur = client_area.min - gfxm::vec2(.0f, small_offset);
        for (int i = visible_items_begin; i < visible_items_end; ++i) {
            auto ch = getChild(i);
            if ((client_area.max.x - cur.x) < elem_size.x) {
                cur.x = client_area.min.x;
                cur.y += elem_size.y;
            }
            gfxm::rect rc = client_area;
            rc.min = cur;
            ch->layout(rc, flags);
            cur.x += elem_size.x;
        }

        if (getContentHeight() > getClientHeight()) {
            scroll_bar_v->setEnabled(true);
        } else {
            scroll_bar_v->setEnabled(false);
        }
        
        scroll_bar_v->layout(client_area, 0);
    }*//*
    void onDraw() override {
        guiDrawRect(rc_bounds, GUI_COL_HEADER);
        guiDrawPushScissorRect(rc_bounds);
        for (int i = visible_items_begin; i < visible_items_end; ++i) {
            auto ch = getChild(i);
            ch->draw();
        }
        scroll_bar_v->draw();
        guiDrawPopScissorRect();
        //guiDrawRectLine(rc_content, GUI_COL_RED);
    }*/
};
