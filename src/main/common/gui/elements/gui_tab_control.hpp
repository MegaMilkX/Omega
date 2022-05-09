#pragma once

#include "common/gui/elements/gui_element.hpp"
#include "common/gui/elements/gui_window.hpp"
#include "platform/platform.hpp"


void guiCaptureMouse(GuiElement* e);

class GuiTabButton : public GuiElement {
    int id = 0;
    Font* font = 0;
    std::string caption = "MyTab";
    bool hovered = false;
    bool pressed = false;
    bool dragging = false;
public:
    GuiTabButton(Font* font)
    : font(font) {

    }

    void setCaption(const char* caption) {
        this->caption = caption;
    }
    void setId(int i) {
        this->id = i;
    }

    bool isDragged() const {
        return dragging;
    }

    void onMessage(GUI_MSG msg, uint64_t a_param, uint64_t b_param) override {
        switch (msg) {
        case GUI_MSG::MOUSE_ENTER:
            hovered = true;
            break;
        case GUI_MSG::MOUSE_LEAVE: {
            hovered = false;
        } break;
        case GUI_MSG::LBUTTON_DOWN:
            if (hovered) {
                pressed = true;
                guiCaptureMouse(this);
            }
            break;
        case GUI_MSG::LBUTTON_UP:
            if (pressed) {
                guiCaptureMouse(0);
                if (dragging) {
                    dragging = false;
                    guiPostMessage(GUI_MSG::DOCK_TAB_DRAG_STOP);
                } else if (hovered) {
                    getOwner()->onMessage(GUI_MSG::NOTIFY, (uint64_t)GUI_NOTIFICATION::TAB_CLICKED, (uint64_t)id);
                }
                
            }
            pressed = false;
            break;
        case GUI_MSG::MOUSE_MOVE:
            if (pressed && !dragging) {
                dragging = true;
                getOwner()->onMessage(GUI_MSG::NOTIFY, (uint64_t)GUI_NOTIFICATION::DRAG_TAB_START, (uint64_t)id);
            }
            break;
        }

        GuiElement::onMessage(msg, a_param, b_param);
    }

    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        this->bounding_rect = rc;
        this->client_area = bounding_rect;
    }

    void onDraw() override {
        uint32_t col = GUI_COL_BG;
        if (pressed) {
            col = GUI_COL_ACCENT;
        }
        else if (hovered) {
            col = GUI_COL_BUTTON_HOVER;
        }
        guiDrawRect(client_area, col);
        guiDrawText(client_area.min, caption.c_str(), font, .0f, GUI_COL_TEXT);
    }
};

class GuiTabControl : public GuiElement {
    Font* font = 0;

    std::vector<std::unique_ptr<GuiTabButton>> buttons;
    std::unique_ptr<GuiTabButton> dnd_fake_button;
public:
    GuiTabControl(Font* font)
    : font(font) {
    }

    void setTabCount(int n) {
        buttons.resize(n);
        for (int i = 0; i < buttons.size(); ++i) {
            if (!buttons[i]) {
                buttons[i].reset(new GuiTabButton(font));
                buttons[i]->setOwner(this);
                buttons[i]->setId(i);
            }
        }
    }

    void removeTab(int i) {
        assert(i < buttons.size());
        if (i >= buttons.size()) {
            return;
        }
        buttons.erase(buttons.begin() + i);
        for (int j = i; j < buttons.size(); ++j) {
            buttons[j]->setId(j);
        }
    }

    int getTabCount() const {
        return buttons.size();
    }

    GuiTabButton* getTabButton(int i) {
        return buttons[i].get();
    }

    GuiHitResult hitTest(int x, int y) override {
        if (!point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        if (guiIsDragDropInProgress()) {
            return GuiHitResult{ GUI_HIT::DOCK_DRAG_DROP_TARGET, this };
        }

        for (int i = 0; i < buttons.size(); ++i) {
            auto hit = buttons[i]->hitTest(x, y);
            if (hit.hit != GUI_HIT::NOWHERE) {
                return hit;
            }
        }

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }

    void onMessage(GUI_MSG msg, uint64_t a_param, uint64_t b_param) override {
        switch (msg) {
        case GUI_MSG::NOTIFY: {
            GUI_NOTIFICATION n = (GUI_NOTIFICATION)a_param;
            switch (n) {
            case GUI_NOTIFICATION::DRAG_TAB_START:
                getOwner()->onMessage(msg, a_param, b_param);
                break;
            case GUI_NOTIFICATION::TAB_CLICKED:
                getOwner()->onMessage(msg, a_param, b_param);
                break;
            }
            } break;
        case GUI_MSG::DOCK_TAB_DRAG_ENTER: {
            dnd_fake_button.reset(new GuiTabButton(font));
            dnd_fake_button->setCaption(((GuiWindow*)a_param)->getTitle());
            } break;
        case GUI_MSG::DOCK_TAB_DRAG_LEAVE:
            dnd_fake_button.reset();
            break;
        case GUI_MSG::DOCK_TAB_DRAG_HOVER:
            // TODO
            break;
        case GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD:
            dnd_fake_button.reset();
            getOwner()->onMessage(GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD, a_param, b_param);
            break;
        }
    }

    void onLayout(const gfxm::rect& rect, uint64_t flags) override {
        this->bounding_rect = rect;
        this->client_area = bounding_rect;

        const int n_buttons = buttons.size() + (dnd_fake_button ? 1 : 0);
        const float BUTTON_MARGIN = 10.0f;
        const float tab_space_width = client_area.max.x - client_area.min.x;
        const float button_width = gfxm::_min(gfxm::_max(tab_space_width / (float)n_buttons, 110.0f), 310.0f);// 100.0f + BUTTON_MARGIN;
        const float button_height = 30.0f;
        const int n_buttons_per_row = (int)(tab_space_width / button_width);
        const int n_rows = gfxm::_max((int)(n_buttons / n_buttons_per_row), 1);
        
        

        for (int i = 0; i < buttons.size(); ++i) {
            int col = i % n_buttons_per_row;
            int row = i / n_buttons_per_row;
            gfxm::vec2 min = client_area.min + gfxm::vec2(col * button_width, row * button_height);
            gfxm::rect rc(
                min, min + gfxm::vec2(button_width - BUTTON_MARGIN, button_height)
            );
            buttons[i]->onLayout(rc, 0);
        }
        if (dnd_fake_button) {
            int col = buttons.size() % n_buttons_per_row;
            int row = buttons.size() / n_buttons_per_row;
            gfxm::vec2 min = client_area.min + gfxm::vec2(col * button_width, row * button_height);
            gfxm::rect rc(
                min, min + gfxm::vec2(button_width - BUTTON_MARGIN, button_height)
            );
            dnd_fake_button->onLayout(rc, 0);
        }

        bounding_rect.max.y = bounding_rect.min.y + n_rows * button_height;
        client_area = bounding_rect;
    }

    void onDraw() override {
        int sw = 0, sh = 0;
        platformGetWindowSize(sw, sh);
        
        for (int i = 0; i < buttons.size(); ++i) {
            glScissor(
                client_area.min.x,
                sh - client_area.max.y,
                client_area.max.x - client_area.min.x,
                client_area.max.y - client_area.min.y
            );
            if (buttons[i]->isDragged()) {
                continue;
            }
            buttons[i]->onDraw();
        }
        if (dnd_fake_button) {
            dnd_fake_button->onDraw();
        }

        //guiDrawRectLine(client_area, 0xFF00FF00);
    }
};