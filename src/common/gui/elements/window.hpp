#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/container.hpp"
#include "gui/elements/scroll_bar.hpp"
#include "gui/elements/menu_bar.hpp"
#include "gui/gui_system.hpp"

#include "window_title_bar_button.hpp"

enum GUI_WINDOW_FRAME_STYLE {
    GUI_WINDOW_FRAME_NONE,
    GUI_WINDOW_FRAME_UNSPECIFIED,
    GUI_WINDOW_FRAME_FULL
};

class GuiWindow : public GuiElement {
    void* dock_group = 0;
    uint64_t flags_cached = 0;
    const int titlebar_width = 25.f;

    std::string title;
    GuiTextBuffer title_buf;
    GuiWindowTitleBarButton close_btn = GuiWindowTitleBarButton(guiLoadIcon("svg/entypo/cross.svg"), GUI_MSG::CLOSE);;
    gfxm::rect rc_titlebar;
    gfxm::rect icon_rc;

    std::unique_ptr<GuiMenuBar> menu_bar;
    std::unique_ptr<GuiScrollBarV> scroll_v;
    std::unique_ptr<GuiScrollBarH> scroll_h;

    void calcResizeBorders(const gfxm::rect& rect, float thickness_outer, float thickness_inner, gfxm::rect* left, gfxm::rect* right, gfxm::rect* top, gfxm::rect* bottom) {
        assert(left && right && top && bottom);

        left->min = gfxm::vec2(
            rect.min.x - thickness_outer,
            rect.min.y - thickness_outer
        );
        left->max = gfxm::vec2(
            rect.min.x + thickness_inner,
            rect.max.y + thickness_outer
        );

        right->min = gfxm::vec2(
            rect.max.x - thickness_inner,
            rect.min.y - thickness_outer
        );
        right->max = gfxm::vec2(
            rect.max.x + thickness_outer,
            rect.max.y + thickness_outer
        );

        top->min = gfxm::vec2(
            rect.min.x - thickness_outer,
            rect.min.y - thickness_outer
        );
        top->max = gfxm::vec2(
            rect.max.x + thickness_outer,
            rect.min.y + thickness_inner
        );

        bottom->min = gfxm::vec2(
            rect.min.x - thickness_outer,
            rect.max.y - thickness_inner
        );
        bottom->max = gfxm::vec2(
            rect.max.x + thickness_outer,
            rect.max.y + thickness_outer
        );
    }
    void hitTestResizeBorders(GuiHitResult& hit, const gfxm::rect& rc, float border_thickness, int x, int y, char mask) {
        gfxm::vec2 pt(x, y);
        gfxm::rect rc_szleft, rc_szright, rc_sztop, rc_szbottom;
        calcResizeBorders(rc, border_thickness * .5f, border_thickness * .5f, &rc_szleft, &rc_szright, &rc_sztop, &rc_szbottom);
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
        sz_flags &= mask;
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
            hit.add(ht, this);
            return;
        }
        return;
    }
    void hitTestTitleBar(GuiHitResult& hit, const gfxm::rect& rc, int x, int y) {
        gfxm::rect rc_ = rc;
        rc_.max.y = rc_.min.y + titlebar_width;
        if (gfxm::point_in_rect(rc_, gfxm::vec2(x, y))) {
            close_btn.onHitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
            hit.add(GUI_HIT::CAPTION, this);
            return;
        }
        return;
    }
    void hitTestFrame(GuiHitResult& hit, const gfxm::rect& rc, int x, int y) {
        if ((flags_cached & GUI_LAYOUT_NO_BORDER) == 0) {
            hitTestResizeBorders(hit, rc, 10.f, x, y, 0b1111);
            if (hit.hasHit()) {
                return;
            }
        }
        if ((flags_cached & GUI_LAYOUT_NO_BORDER) == 0) {
            hitTestTitleBar(hit, rc_bounds, x, y);
            if (hit.hasHit()) {
                return;
            }
        }
        if (menu_bar) {
            menu_bar->onHitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }
        if (scroll_v && getContentHeight() > getClientHeight()) {
            scroll_v->onHitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }
        if (scroll_h && getContentWidth() > getClientWidth()) {
            scroll_h->onHitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }
        return;
    }
public:
    GuiWindow(const char* title = "MyWindow");
    ~GuiWindow();

    void setDockGroup(void* dock_group) { this->dock_group = dock_group; }
    void* getDockGroup() const { return dock_group; }

    void setTitle(const std::string& title) {
        this->title = title;
        title_buf.replaceAll(title.data(), title.size());
        GUI_MSG_PARAMS params;
        params.setA<GuiWindow*>(this);
        guiPostMessage(this, GUI_MSG::TITLE_CHANGED, params);
    }
    const std::string& getTitle() {
        return title;
    }

    void sendCloseMessage() {
        sendMessage(GUI_MSG::CLOSE, 0, 0, 0);
    }

    GuiMenuBar* createMenuBar() {
        if (menu_bar) {
            return menu_bar.get();
        } else {
            menu_bar.reset(new GuiMenuBar);
            menu_bar->setOwner(this);
            return menu_bar.get();
        }
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        gfxm::rect rc_padded(rc_bounds.min - gfxm::vec2(5.f, 5.f), rc_bounds.max + gfxm::vec2(5.f, 5.f));
        if (!gfxm::point_in_rect(rc_padded, gfxm::vec2(x, y))) {
            return;
        }
        hitTestFrame(hit, rc_bounds, x, y);
        if (hit.hasHit()) {
            return;
        }
        
        if (gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            for (auto& ch : children) {
                ch->onHitTest(hit, x, y);
                if (hit.hasHit()) {
                    return;
                }
            }
        }

        hit.add(GUI_HIT::CLIENT, this);
        return;
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::CLOSE: {
            guiDestroyWindow(this);
            return true;
        }
        case GUI_MSG::MOUSE_SCROLL:
            //pos_content.y = gfxm::_min(gfxm::_max(.0f, rc_content.max.y - (client_area.max.y - client_area.min.y)), gfxm::_max(rc_content.min.y, pos_content.y - params.getA<int32_t>()));
            pos_content.y 
                = gfxm::_max(
                    .0f, 
                    gfxm::_min(pos_content.y - params.getA<int32_t>(), (float)(getContentHeight() - getClientHeight()))
                );
            break;
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::SCROLL_V:
                pos_content.y = params.getB<float>();
                return true;
            case GUI_NOTIFY::SCROLL_H:
                pos_content.x = params.getB<float>();
                return true;
            }
            break;
        case GUI_MSG::ACTIVATE:
            //title_bar->color = GUI_COL_ACCENT;
            return true;
        case GUI_MSG::DEACTIVATE:
            //title_bar->color = GUI_COL_HEADER;
            return true;
        case GUI_MSG::MOVING: {
            gfxm::rect* prc = params.getB<gfxm::rect*>();
            pos.x.value += prc->max.x - prc->min.x;
            pos.y.value += prc->max.y - prc->min.y;
            } return true;
        case GUI_MSG::RESIZING: {
            gfxm::rect* prc = params.getB<gfxm::rect*>();
            switch (params.getA<GUI_HIT>()) {
            case GUI_HIT::LEFT:
                pos.x = prc->max.x;
                size.x.value -= prc->max.x - prc->min.x;
                size.x.unit = gui_pixel;
                break;
            case GUI_HIT::RIGHT:
                size.x.value += prc->max.x - prc->min.x;
                size.x.unit = gui_pixel;
                break;
            case GUI_HIT::TOP:
                pos.y = prc->max.y;
                size.y.value -= prc->max.y - prc->min.y;
                size.y.unit = gui_pixel;
                break;
            case GUI_HIT::BOTTOM:
                size.y.value += prc->max.y - prc->min.y;
                size.y.unit = gui_pixel;
                break;
            case GUI_HIT::TOPLEFT:
                pos.y = prc->max.y;
                size.y.value -= prc->max.y - prc->min.y;
                size.y.unit = gui_pixel;
                pos.x = prc->max.x;
                size.x.value -= prc->max.x - prc->min.x;
                size.x.unit = gui_pixel;
                break;
            case GUI_HIT::TOPRIGHT:
                pos.y = prc->max.y;
                size.y.value -= prc->max.y - prc->min.y;
                size.y.unit = gui_pixel;
                size.x.value += prc->max.x - prc->min.x;
                size.x.unit = gui_pixel;
                break;
            case GUI_HIT::BOTTOMLEFT:
                size.y.value += prc->max.y - prc->min.y;
                size.y.unit = gui_pixel;
                pos.x = prc->max.x;
                size.x.value -= prc->max.x - prc->min.x;
                size.x.unit = gui_pixel;
                break;
            case GUI_HIT::BOTTOMRIGHT:
                size.y.value += prc->max.y - prc->min.y;
                size.y.unit = gui_pixel;
                size.x.value += prc->max.x - prc->min.x;
                size.x.unit = gui_pixel;
                break;
            }
            // TODO: HANDLE DIFFERENT UNITS
            size.x.unit = gui_pixel;
            size.y.unit = gui_pixel;
            size.x.value = std::max(min_size.x.value, std::min(max_size.x.value, size.x.value));
            size.y.value = std::max(min_size.y.value, std::min(max_size.y.value, size.y.value));
        } return true;
        }

        return GuiElement::onMessage(msg, params);
    }

    void layout(const gfxm::rect& rc, uint64_t flags) {
        if (is_hidden) {
            return;
        }
        flags_cached = flags;

        if (getFont()) { guiPushFont(getFont()); }
        onLayoutFrame(rc, flags);
        onLayout(client_area, flags);
        rc_bounds = rc;
        if (getFont()) { guiPopFont(); }
    }
    void draw() {
        if (is_hidden) {
            return;
        }
        if (getFont()) { guiPushFont(getFont()); }
        onDrawFrame();
        onDraw();
        if (getFont()) { guiPopFont(); }
    }

    void onLayoutFrame(const gfxm::rect& rc, uint64_t flags) {
        rc_bounds = rc;
        client_area = rc_bounds;

        if ((flags & GUI_LAYOUT_NO_TITLE) == 0) {
            client_area.min.y += titlebar_width;
        }
        if (menu_bar) {
            menu_bar->layout(client_area, 0);
            auto menu_rc = menu_bar->getBoundingRect();
            client_area.min.y += menu_rc.max.y - menu_rc.min.y;
        }

        client_area.min += padding.min;
        client_area.max -= padding.max;
        
        if (scroll_v && (getFlags() & GUI_FLAG_SCROLLV)) {
            scroll_v->setScrollBounds(gfxm::_min(rc_content.min.y, client_area.min.y), gfxm::_max(rc_content.max.y, client_area.max.y));
            scroll_v->setScrollPageLength(getClientHeight());
            scroll_v->setScrollPosition(client_area.min.y + pos_content.y);
            scroll_v->layout(client_area, 0);
            auto scroll_rc = scroll_v->getBoundingRect();
            client_area.max.x -= scroll_rc.max.x - scroll_rc.min.x;
        }
        if (scroll_h && (getFlags() & GUI_FLAG_SCROLLH)) {
            scroll_h->setScrollBounds(0, getContentWidth());
            scroll_h->setScrollPageLength(getClientWidth());
            scroll_h->setScrollPosition(pos_content.x);
            scroll_h->layout(client_area, 0);
            auto scroll_rc = scroll_h->getBoundingRect();
            client_area.max.y -= scroll_rc.max.y - scroll_rc.min.y;
        }


        if ((flags_cached & GUI_LAYOUT_NO_TITLE) == 0) {
            rc_titlebar = rc_bounds;
            rc_titlebar.max.y = rc_titlebar.min.y + titlebar_width;

            float icon_sz = rc_titlebar.max.y - rc_titlebar.min.y;
            icon_rc = gfxm::rect(
                rc_titlebar.max - gfxm::vec2(icon_sz, icon_sz),
                rc_titlebar.max
            );
            close_btn.layout(icon_rc, 0);
        }
    }
    void onDrawFrame() {
        guiDrawRectShadow(rc_bounds);
        guiDrawRect(rc_bounds, GUI_COL_BG);
        if (guiGetActiveWindow() == this) {
            guiDrawRectLine(rc_bounds, GUI_COL_BUTTON_HOVER);
        } else {
            guiDrawRectLine(rc_bounds, GUI_COL_HEADER);
        }

        if ((flags_cached & GUI_LAYOUT_NO_TITLE) == 0) {
            if (guiGetActiveWindow() == this) {
                guiDrawRectGradient(rc_titlebar, GUI_COL_ACCENT_DIM, GUI_COL_ACCENT, GUI_COL_ACCENT_DIM, GUI_COL_ACCENT);
            } else {
                guiDrawRectGradient(rc_titlebar, GUI_COL_HEADER, GUI_COL_HEADER_LIGHT, GUI_COL_HEADER, GUI_COL_HEADER_LIGHT);
            }
            gfxm::rect rc = rc_titlebar;
            rc.min.x += GUI_MARGIN;
            title_buf.draw(rc, GUI_LEFT | GUI_VCENTER, GUI_COL_TEXT, GUI_COL_HEADER);
            
            // Draw close button
            close_btn.draw();
            
        }
        if (menu_bar) {
            menu_bar->draw();
        }
        if (scroll_v && (getFlags() & GUI_FLAG_SCROLLV)) {
            scroll_v->draw();
        }
        if (scroll_h && (getFlags() & GUI_FLAG_SCROLLH)) {
            scroll_h->draw();
        }
        //guiDrawRectLine(client_area, GUI_COL_GREEN);
        //guiDrawRectLine(rc_content, GUI_COL_RED);
    }
    /*
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        gfxm::rect rc_ = client_area;
        layoutContentTopDown(rc_);
    }*//*
    void onDraw() override {
        if (client_area.min.x >= client_area.max.x || client_area.min.y >= client_area.max.y) {
            return;
        }
        guiDrawPushScissorRect(client_area);
        drawContent();
        guiDrawPopScissorRect();
    }*/
};