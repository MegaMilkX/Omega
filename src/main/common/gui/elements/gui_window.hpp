#pragma once

#include "common/gui/elements/gui_element.hpp"
#include "common/gui/elements/gui_scroll_bar.hpp"
#include "common/gui/gui_system.hpp"


class GuiWindow : public GuiElement {    
    std::string title = "MyWindow";

    uint64_t layout_flags = 0;

    gfxm::rect rc_window;
    gfxm::rect rc_body;
    gfxm::rect rc_content;

    gfxm::rect rc_nonclient;
    gfxm::rect rc_client;
    gfxm::rect rc_header;
    gfxm::rect rc_scroll_v;
    gfxm::rect rc_scroll_h;

    gfxm::rect rc_szleft, rc_szright, rc_sztop, rc_szbottom;

    std::unique_ptr<GuiScrollBarV> scroll_bar_v;

    gfxm::vec2 content_offset;

    gfxm::vec2 mouse_pos;

    const float resize_frame_thickness = 10.0f;

    GUI_DOCK dock_position = GUI_DOCK::NONE;
    bool is_dockable = true;

    gfxm::vec2 updateContentLayout() {
        gfxm::rect current_content_rc = gfxm::rect(
            rc_content.min - content_offset,
            rc_content.max - content_offset
        );
        gfxm::expand(current_content_rc, -GUI_PADDING);
        float total_content_height = .0f;
        float total_content_width = .0f;
        for (int i = 0; i < children.size(); ++i) {
            children[i]->layout(current_content_rc, 0);
            auto& r = children[i]->getBoundingRect();
            current_content_rc.min.y = r.max.y + GUI_PADDING;

            total_content_width = gfxm::_max(total_content_width, r.max.x - r.min.x);
            total_content_height += r.max.y - r.min.y;
        }
        gfxm::vec2 content_size(
            total_content_width,
            total_content_height
        );
        return content_size;
    }

    char getResizeBorderMask() const {
        char sz_mask = 0b1111;
        if (getDockPosition() == GUI_DOCK::FILL) {
            sz_mask = 0b0000;
        } else if(getDockPosition() == GUI_DOCK::LEFT) {
            sz_mask = 0b0010;
        } else if(getDockPosition() == GUI_DOCK::RIGHT) {
            sz_mask = 0b0001;
        } else if(getDockPosition() == GUI_DOCK::TOP) {
            sz_mask = 0b1000;
        } else if(getDockPosition() == GUI_DOCK::BOTTOM) {
            sz_mask = 0b0100;
        }
        return sz_mask;
    }
public:
    GuiWindow(const char* title = "MyWindow");
    ~GuiWindow();

    const char* getTitle() const {
        return title.c_str();
    }

    GuiHitResult hitTest(int x, int y) override {
        const gfxm::vec2 pt(x, y);

        if (!gfxm::point_in_rect(bounding_rect, pt)) {
            return GuiHitResult{ GUI_HIT::NOWHERE, this };
        }

        char sz_mask = getResizeBorderMask();
        char sz_flags = 0b0000;
        if (gfxm::point_in_rect(rc_szleft, pt)) {
            sz_flags |= 0b0001;
        } else if (gfxm::point_in_rect(rc_szright, pt)) {
            sz_flags |= 0b0010;
        }
        if (gfxm::point_in_rect(rc_sztop, pt)) {
            sz_flags |= 0b0100;
        } else if (gfxm::point_in_rect(rc_szbottom, pt)) {
            sz_flags |= 0b1000;
        }
        sz_flags &= sz_mask;
        
        GUI_HIT ht = GUI_HIT::ERR;
        switch (sz_flags) {
        case 0b0001: ht = GUI_HIT::LEFT; break;
        case 0b0010: ht = GUI_HIT::RIGHT; break;
        case 0b0100: ht = GUI_HIT::TOP; break;
        case 0b1000: ht = GUI_HIT::BOTTOM; break;
        case 0b0101: ht = GUI_HIT::TOPLEFT; break;
        case 0b1001: ht = GUI_HIT::BOTTOMLEFT; break;
        case 0b0110: ht = GUI_HIT::TOPRIGHT; break;
        case 0b1010: ht = GUI_HIT::BOTTOMRIGHT; break;
        };
        if (ht != GUI_HIT::ERR) {
            return GuiHitResult{ ht, this };
        }

        if (!gfxm::point_in_rect(client_area, pt)) {
            GuiHitResult{ GUI_HIT::NOWHERE, this };
        }

        if (gfxm::point_in_rect(rc_header, pt)) {
            return GuiHitResult{ GUI_HIT::CAPTION, this };
        }

        if (gfxm::point_in_rect(rc_scroll_v, pt)) {
            return GuiHitResult{ GUI_HIT::VSCROLL, scroll_bar_v.get() };
        }
        if (gfxm::point_in_rect(rc_scroll_h, pt)) {
            return GuiHitResult{ GUI_HIT::HSCROLL, 0 /* TODO */ };
        }

        for (int i = 0; i < children.size(); ++i) {
            auto hit = children[i]->hitTest(x, y);
            if (hit.hit != GUI_HIT::NOWHERE && hit.hit != GUI_HIT::ERR) {
                return hit;
            }
        }

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }

    void onMessage(GUI_MSG msg, uint64_t a_param, uint64_t b_param) override {
        switch (msg) {
        case GUI_MSG::MOUSE_MOVE:
            mouse_pos = gfxm::vec2(a_param, b_param);

            break;
        case GUI_MSG::SB_THUMB_TRACK:
            content_offset.y = a_param;
            break;
        case GUI_MSG::MOVING: {
            gfxm::rect* prc = (gfxm::rect*)b_param;
            pos += prc->max - prc->min;
            } break;
        case GUI_MSG::RESIZING: {
            gfxm::rect* prc = (gfxm::rect*)b_param;
            switch ((GUI_HIT)a_param) {
            case GUI_HIT::LEFT:
                pos.x = prc->max.x;
                size.x -= prc->max.x - prc->min.x;
                break;
            case GUI_HIT::RIGHT:
                size.x += prc->max.x - prc->min.x;
                break;
            case GUI_HIT::TOP:
                pos.y = prc->max.y;
                size.y -= prc->max.y - prc->min.y;
                break;
            case GUI_HIT::BOTTOM:
                size.y += prc->max.y - prc->min.y;
                break;
            case GUI_HIT::TOPLEFT:
                pos.y = prc->max.y;
                size.y -= prc->max.y - prc->min.y;
                pos.x = prc->max.x;
                size.x -= prc->max.x - prc->min.x;
                break;
            case GUI_HIT::TOPRIGHT:
                pos.y = prc->max.y;
                size.y -= prc->max.y - prc->min.y;
                size.x += prc->max.x - prc->min.x;
                break;
            case GUI_HIT::BOTTOMLEFT:
                size.y += prc->max.y - prc->min.y;
                pos.x = prc->max.x;
                size.x -= prc->max.x - prc->min.x;
                break;
            case GUI_HIT::BOTTOMRIGHT:
                size.y += prc->max.y - prc->min.y;
                size.x += prc->max.x - prc->min.x;
                break;
            }
        } break;
        }

        GuiElement::onMessage(msg, a_param, b_param);
    }

    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        layout_flags = flags;

        pos = rc.min;
        size = rc.max - rc.min;
        this->rc_nonclient = rc;
        this->bounding_rect = gfxm::rect(
            rc_nonclient.min - gfxm::vec2(resize_frame_thickness, resize_frame_thickness),
            rc_nonclient.max + gfxm::vec2(resize_frame_thickness, resize_frame_thickness)
        );

        guiCalcResizeBorders(rc_nonclient, 5.0f, 5.0f, &rc_szleft, &rc_szright, &rc_sztop, &rc_szbottom);
        
        rc_window = gfxm::rect(rc.min, rc.max);
        rc_header = gfxm::rect(rc_window.min, gfxm::vec2(rc_window.max.x, rc_window.min.y + 30.0f));
        if (flags & GUI_LAYOUT_NO_TITLE) {
            rc_header.max.y = rc_header.min.y;
        }
        rc_body = gfxm::rect(gfxm::vec2(rc.min.x, rc_header.max.y), rc_window.max);
        rc_client = gfxm::rect(rc_body.min + gfxm::vec2(GUI_PADDING, GUI_PADDING), rc_body.max - gfxm::vec2(GUI_PADDING, GUI_PADDING));
        rc_content = rc_client;

        gfxm::vec2 content_size = updateContentLayout();
        if (content_size.y > rc_content.max.y - rc_content.min.y) {
            scroll_bar_v->setEnabled(true);
            rc_content.max.x -= 10.0f;
            rc_client.max.x -= 10.0f;
            content_size = updateContentLayout();
            scroll_bar_v->setScrollData((rc_content.max.y - rc_content.min.y), content_size.y);
        } else {
            scroll_bar_v->setEnabled(false);
        }

        rc_scroll_v = gfxm::rect(
            gfxm::vec2(rc_client.max.x, rc_client.min.y),
            gfxm::vec2(rc_client.max.x + 10.0f, rc_client.max.y)
        );
        rc_scroll_h = gfxm::rect(
            gfxm::vec2(rc_client.min.x, rc_client.max.y),
            gfxm::vec2(rc_client.max.x, rc_client.max.y + 10.0f)
        );

        scroll_bar_v->layout(rc_scroll_v, 0);

        this->client_area = rc_client;
    }

    void onDraw() override {
        guiDrawPushScissorRect(rc_nonclient);
        guiDrawRect(rc_nonclient, GUI_COL_BG);
        if (guiGetActiveWindow() == this) {
            guiDrawRectLine(rc_nonclient, GUI_COL_BUTTON_HOVER);
        }
        if ((layout_flags & GUI_LAYOUT_NO_TITLE) == 0) {
            guiDrawTitleBar(this, guiGetCurrentFont()->font, title.c_str(), rc_header);
        }
        scroll_bar_v->draw();
        guiDrawPopScissorRect();
        
        guiDrawPushScissorRect(rc_content);
        for (int i = 0; i < children.size(); ++i) {
            children[i]->draw();
        }
        guiDrawPopScissorRect();
        
        /*
        glScissor(0, 0, sw, sh);
        guiDrawRectLine(rc_nonclient, 0xFFFF00FF);
        guiDrawRectLine(rc_client, 0xFF00FF00);

        char sz_mask = getResizeBorderMask();
        if (sz_mask & 0b0001) guiDrawRectLine(rc_szleft, 0xFFcccccc);
        if (sz_mask & 0b0010) guiDrawRectLine(rc_szright, 0xFFcccccc);
        if (sz_mask & 0b0100) guiDrawRectLine(rc_sztop, 0xFFcccccc);
        if (sz_mask & 0b1000) guiDrawRectLine(rc_szbottom, 0xFFcccccc);*/
    }

    GuiElement* getScrollBarV() override { 
        return scroll_bar_v.get(); 
    }
    GuiElement* getScrollBarH() override { 
        return 0; 
    }

    GUI_DOCK getDockPosition() const override {
        return dock_position;
    }
    void setDockPosition(GUI_DOCK dock) override {
        dock_position = dock;
    }

    bool isDockable() const {
        return is_dockable;
    }
    virtual void setDockable(bool is_dockable) {
        this->is_dockable = is_dockable;
    }
};