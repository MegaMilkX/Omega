#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/container.hpp"
#include "gui/elements/scroll_bar.hpp"
#include "gui/elements/menu_bar.hpp"
#include "gui/gui_system.hpp"
#include "gui/gui_util.hpp"

#include "list_toolbar_button.hpp"

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
    std::unique_ptr<GuiListToolbarButton> close_btn;
    gfxm::rect rc_titlebar;
    gfxm::rect icon_rc;

    std::unique_ptr<GuiMenuBar> menu_bar;

    void hitTestTitleBar(GuiHitResult& hit, const gfxm::rect& rc, int x, int y) {
        gfxm::rect rc_ = rc;
        rc_.max.y = rc_.min.y + titlebar_width;
        if (gfxm::point_in_rect(rc_, gfxm::vec2(x, y))) {
            close_btn->hitTest(hit, x, y);
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
            guiHitTestResizeBorders(hit, this, rc, 10.f, x, y, 0b1010);
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
            menu_bar->hitTest(hit, x, y);
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
        title_buf.replaceAll(getFont(), title.data(), title.size());
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
            for (auto ch : children) {
                if (ch->isHidden()) {
                    continue;
                }
                ch->hitTest(hit, x, y);
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
                size.x.value = getGlobalBoundingRect().max.x - prc->max.x;
                pos.x = prc->max.x;
                size.x.unit = gui_pixel;
                break;
            case GUI_HIT::RIGHT:
                size.x.value = prc->max.x - getGlobalBoundingRect().min.x;
                size.x.unit = gui_pixel;
                break;
            case GUI_HIT::TOP:
                size.y.value = getGlobalBoundingRect().max.y - prc->max.y;
                pos.y = prc->max.y;
                size.y.unit = gui_pixel;
                break;
            case GUI_HIT::BOTTOM:
                size.y.value = prc->max.y - getGlobalBoundingRect().min.y;
                size.y.unit = gui_pixel;
                break;
            case GUI_HIT::TOPLEFT:
                size.y.value = getGlobalBoundingRect().max.y - prc->max.y;
                pos.y = prc->max.y;
                size.y.unit = gui_pixel;
                size.x.value = getGlobalBoundingRect().max.x - prc->max.x;
                pos.x = prc->max.x;
                size.x.unit = gui_pixel;
                break;
            case GUI_HIT::TOPRIGHT:
                size.y.value = getGlobalBoundingRect().max.y - prc->max.y;
                pos.y = prc->max.y;
                size.y.unit = gui_pixel;
                size.x.value = prc->max.x - getGlobalBoundingRect().min.x;
                size.x.unit = gui_pixel;
                break;
            case GUI_HIT::BOTTOMLEFT:
                size.y.value = prc->max.y - getGlobalBoundingRect().min.y;
                size.y.unit = gui_pixel;
                size.x.value = getGlobalBoundingRect().max.x - prc->max.x;
                pos.x = prc->max.x;
                size.x.unit = gui_pixel;
                break;
            case GUI_HIT::BOTTOMRIGHT:
                size.y.value = prc->max.y - getGlobalBoundingRect().min.y;
                size.y.unit = gui_pixel;
                size.x.value = prc->max.x - getGlobalBoundingRect().min.x;
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

    void layout(const gfxm::vec2& extents, uint64_t flags) {
        if (is_hidden) {
            return;
        }
        flags_cached = flags;

        onLayoutFrame(extents, flags);
        onLayout(extents, flags);
        //rc_bounds = gfxm::rect(gfxm::vec2(0, 0), extents);
    }

    using GuiElement::draw;
    void draw(int x, int y) override {
        if (is_hidden) {
            return;
        }
        //if (getFont()) { guiPushFont(getFont()); }
        guiPushOffset(gfxm::vec2(x, y));
        onDrawFrame();
        onDraw();
        guiPopOffset();
        //if (getFont()) { guiPopFont(); }
    }

    void onLayoutFrame(const gfxm::vec2& extents, uint64_t flags) {
        if (flags & GUI_LAYOUT_WIDTH_PASS) {
            rc_bounds.min.x = 0;
            rc_bounds.max.x = extents.x;
            
            client_area.min.x = rc_bounds.min.x;
            client_area.max.x = rc_bounds.max.x;

            tmp_padding.min.x = 0;
            tmp_padding.max.x = 0;

            if ((flags_cached & GUI_LAYOUT_NO_TITLE) == 0) {
                rc_titlebar = rc_bounds;
                rc_titlebar.max.y = rc_titlebar.min.y + titlebar_width;

                float icon_sz = rc_titlebar.max.y - rc_titlebar.min.y;
                icon_rc = gfxm::rect(
                    rc_titlebar.max - gfxm::vec2(icon_sz, icon_sz),
                    rc_titlebar.max
                );
                close_btn->layout_position = icon_rc.min;
                close_btn->layout(gfxm::rect_size(icon_rc), GUI_LAYOUT_WIDTH_PASS);
            }
        }

        if (flags & GUI_LAYOUT_HEIGHT_PASS) {
            rc_bounds.min.y = 0;
            rc_bounds.max.y = extents.y;

            client_area.min.y = rc_bounds.min.y;
            client_area.max.y = rc_bounds.max.y;

            tmp_padding.min.y = 0;
            tmp_padding.max.y = 0;

            if ((flags & GUI_LAYOUT_NO_TITLE) == 0) {
                tmp_padding.min.y += titlebar_width;
            }

            if (menu_bar) {
                menu_bar->layout_position = client_area.min;
                menu_bar->layout(gfxm::rect_size(client_area), 0);
                auto menu_rc = menu_bar->getBoundingRect();
                //client_area.min.y += menu_rc.max.y - menu_rc.min.y;
                tmp_padding.min.y += menu_rc.max.y - menu_rc.min.y;
            }

            if ((flags_cached & GUI_LAYOUT_NO_TITLE) == 0) {
                rc_titlebar = rc_bounds;
                rc_titlebar.max.y = rc_titlebar.min.y + titlebar_width;

                float icon_sz = rc_titlebar.max.y - rc_titlebar.min.y;
                icon_rc = gfxm::rect(
                    rc_titlebar.max - gfxm::vec2(icon_sz, icon_sz),
                    rc_titlebar.max
                );
                close_btn->layout_position = icon_rc.min;
                close_btn->layout(gfxm::rect_size(icon_rc), GUI_LAYOUT_HEIGHT_PASS);
            }
        }

        if (flags & GUI_LAYOUT_POSITION_PASS) {
            rc_titlebar = rc_bounds;
            rc_titlebar.max.y = rc_titlebar.min.y + titlebar_width;

            if ((flags_cached & GUI_LAYOUT_NO_TITLE) == 0) {
                float icon_sz = rc_titlebar.max.y - rc_titlebar.min.y;
                icon_rc = gfxm::rect(
                    rc_titlebar.max - gfxm::vec2(icon_sz, icon_sz),
                    rc_titlebar.max
                );
                close_btn->layout_position = icon_rc.min;
                close_btn->layout(gfxm::rect_size(icon_rc), GUI_LAYOUT_POSITION_PASS);
            }
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
            title_buf.draw(getFont(), rc, GUI_LEFT | GUI_VCENTER, GUI_COL_TEXT, GUI_COL_HEADER);
            
            // Draw close button
            close_btn->draw();
            
        }
        if (menu_bar) {
            menu_bar->draw();
        }
        //guiDrawRectLine(client_area, GUI_COL_GREEN);
        //guiDrawRectLine(rc_content, GUI_COL_RED);
    }
};