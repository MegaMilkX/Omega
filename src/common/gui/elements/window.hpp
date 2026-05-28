#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/container.hpp"
#include "gui/elements/scroll_bar.hpp"
#include "gui/elements/menu_bar.hpp"
#include "gui/gui_system.hpp"
#include "gui/gui_util.hpp"


enum GUI_WINDOW_FRAME_STYLE {
    GUI_WINDOW_FRAME_NONE,
    GUI_WINDOW_FRAME_UNSPECIFIED,
    GUI_WINDOW_FRAME_FULL
};

class GuiWindow : public GuiElement {
    void* dock_group = 0;
    uint64_t flags_cached = 0;

    std::string title;
    GuiTextBuffer title_buf;
    gfxm::rect rc_titlebar;
    gfxm::rect icon_rc;

    std::unique_ptr<GuiMenuBar> menu_bar;

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
};