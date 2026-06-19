#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/window.hpp"
#include "platform/platform.hpp"

#include "gui/gui_text_buffer.hpp"

void guiCaptureMouse(GuiElement* e);

class GuiTabButton : public GuiElement {
    void* user_ptr = 0;
    int id = 0;
    GuiTextBuffer caption;
    gfxm::rect icon_rc;
public:
    bool dragging = false;

    void setCaption(const char* caption) {
        this->caption.replaceAll(getFont(), caption, strlen(caption));
    }
    void setUserPtr(void* ptr) {
        user_ptr = ptr;
    }
    void setId(int i) {
        this->id = i;
    }
    int getId() const {
        return this->id;
    }

    void* getUserPtr() const { return user_ptr; }

    bool isDragged() const {
        return dragging;
    }
};

class GuiTabControl : public GuiElement {
    GuiTabButton* active_button = 0;
    std::vector<std::unique_ptr<GuiTabButton>> buttons;
    int current_dragged_tab = -1;
    gfxm::vec2 dragged_tab_offs;
    int last_hovered_tab_slot = -1;
    gfxm::vec2 last_mouse_pos;

    void swapTabs(int a, int b) {
        std::iter_swap(buttons.begin() + a, buttons.begin() + b);
        buttons[a]->setId(a);
        buttons[b]->setId(b);
        notifyOwner(GUI_NOTIFY::TAB_SWAP, (int)a, (int)b);
    }

    int findTabSlot(int curx, int cury) {
        gfxm::vec2 pt(curx, cury);
        if (!gfxm::point_in_rect(getBoundingRect(), pt)) {
            return -1;
        }
        auto& btn_dragged = buttons[current_dragged_tab];
        for (int i = 0; i < buttons.size(); ++i) {
            if (i == current_dragged_tab) {
                continue;
            }
            auto& btn = buttons[i];
            auto& rc_tgt = btn->getBoundingRect();
            auto& rc_dragged = btn_dragged->getBoundingRect();
            gfxm::rect rc;
            if (current_dragged_tab > i) {
                rc = gfxm::rect(rc_tgt.min, rc_tgt.min + (rc_dragged.max - rc_dragged.min));
            } else {
                rc = gfxm::rect(rc_tgt.max - (rc_dragged.max - rc_dragged.min), rc_tgt.max);
            }
            if (gfxm::point_in_rect(rc, pt) && gfxm::point_in_rect(rc_tgt, pt)) {
                return i;
            }
        }
        return -1;
    }
public:
    GuiTabControl() {
        setSize(gui_vec2(gui::fill(), gui::content()));
        setStyleClasses({ "tab-control", "container" });
        primary_axis = GUI_PRIMARY_AXIS::X;

        subscribe<GuiEvt_MouseMove>([this](const GuiEvt_MouseMove& e) {
            gfxm::vec2 pt(e.x, e.y);
            last_mouse_pos = pt;
            if (current_dragged_tab != -1) {
                gfxm::rect client_area_padded = client_area;
                float client_area_height = client_area.max.y - client_area.min.y;
                client_area_padded.min.y -= client_area_height;
                client_area_padded.max.y += client_area_height;

                if (!gfxm::point_in_rect(client_area_padded, pt)) {
                    notifyOwner(GUI_NOTIFY::TAB_DRAGGED_OUT, buttons[current_dragged_tab].get());
                    
                    guiReleaseMouseCapture(this);
                    removeTab(current_dragged_tab);
                    current_dragged_tab = -1;
                } else {
                    int idx = findTabSlot(e.x, e.y);
                    if (last_hovered_tab_slot != idx) {
                        if (idx >= 0) {
                            swapTabs(current_dragged_tab, idx);
                            last_hovered_tab_slot = current_dragged_tab;
                            current_dragged_tab = idx;
                        } else {
                            last_hovered_tab_slot = current_dragged_tab;
                        }
                    } else {
                        last_hovered_tab_slot = idx;
                    }
                }
            }
        });
    }
    ~GuiTabControl() {
    }

    int buttonCount() const {
        return buttons.size();
    }

    void clearSelected() {
        for (int i = 0; i < buttons.size(); ++i) {
            buttons[i]->setSelected(false);
            buttons[i]->setStyleDirty();
        }
    }

    GuiTabButton* addTab(const char* caption, void* user_ptr) {
        int btn_id = buttons.size();

        auto btn = new GuiTabButton();
        btn->setStyleClasses({ "tab" });
        btn->setSize(gui_vec2(gui::content(), gui::content()));
        btn->addFlags(GUI_FLAG_SAME_LINE);
        btn->pushBack(caption);

        btn->setCaption(caption);
        btn->setUserPtr(user_ptr);
        btn->setId(buttons.size());
        btn->setOwner(this);
        btn->setParent(this);
        btn->subscribe<GuiEvt_LClick>([this, btn](const GuiEvt_LClick&) {
            active_button = btn;
        });
        btn->subscribe<GuiEvt_MClick>([this, btn](const GuiEvt_MClick&) {
            notifyOwner(GUI_NOTIFY::TAB_CLOSED, btn);
        });
        btn->subscribe<GuiEvt_MouseEnter>([this, btn](const GuiEvt_MouseEnter&) {
            // TODO: Probably can remove, doesn't do anything
            notifyOwner(GUI_NOTIFY::TAB_MOUSE_ENTER, (int)btn->getId());
        });
        btn->subscribe<GuiEvt_PullStart>([this, btn](const GuiEvt_PullStart&) {
            btn->dragging = true;
            btn->getOwner()->notify(GUI_NOTIFY::DRAG_TAB_START, (int)btn->getId());
        });
        btn->subscribe<GuiEvt_PullStop>([this, btn](const GuiEvt_PullStop&) {
            btn->dragging = false;
            btn->getOwner()->notify(GUI_NOTIFY::DRAG_TAB_END, (int)btn->getId());
        });
        buttons.push_back(std::unique_ptr<GuiTabButton>(btn));

        active_button = btn;
        return btn;
    }
    void removeTab(int i) {
        assert(i < buttons.size());
        if (i >= buttons.size()) {
            return;
        }
        auto btn = buttons[i].get();
        if (btn == active_button && buttons.size() > 1) {
            active_button = buttons[std::max(0, i - 1)].get();
        }
        buttons.erase(buttons.begin() + i);
        for (int j = i; j < buttons.size(); ++j) {
            buttons[j]->setId(j);
        }
    }
    void setCurrentTab(int i) {
        GuiTabButton* btn = buttons[i].get();
        active_button = btn;
        invoke(GuiEvt_LClick{ false, 0, 0 }); // TODO: Should not have to do this
    }

    int getTabCount() const {
        return buttons.size();
    }

    GuiTabButton* getTabButton(int i) {
        return buttons[i].get();
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::NOTIFY: {
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::DRAG_TAB_START: {
                current_dragged_tab = params.getB<int>();
                // TODO: 
                gfxm::vec2 pt = guiGetMousePosLocal(buttons[current_dragged_tab]->getGlobalPosition());
                dragged_tab_offs = pt;
                guiCaptureMouse(this);
                return true;
            }
            case GUI_NOTIFY::DRAG_TAB_END:
                current_dragged_tab = -1;
                last_hovered_tab_slot = -1;
                guiReleaseMouseCapture(this);
                return true;/*
            case GUI_NOTIFY::TAB_MOUSE_ENTER:
                if (current_dragged_tab == -1) {
                    return true;
                }
                if (current_dragged_tab == params.getB<int>()) {
                    return true;
                }
                swapTabs(current_dragged_tab, params.getB<int>());
                current_dragged_tab = params.getB<int>();
                return true;*/
            }
            break;
        }
        case GUI_MSG::DOCK_TAB_DRAG_ENTER: {
            } return true;
        case GUI_MSG::DOCK_TAB_DRAG_LEAVE:
            return true;
        case GUI_MSG::DOCK_TAB_DRAG_HOVER:
            // TODO
            return true;
        case GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD:
            getOwner()->onMessage(GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD, params);
            return true;
        }
        return false;
    }
};