#pragma once

#include "common/gui/elements/gui_element.hpp"
#include "platform/platform.hpp"


void guiCaptureMouse(GuiElement* e);

class GuiTabButton : public GuiElement {
    Font* font = 0;
    std::string caption = "MyTab";
    bool hovered = false;
    bool pressed = false;
public:
    GuiTabButton(Font* font)
    : font(font) {

    }

    void setCaption(const char* caption) {
        this->caption = caption;
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
                if (hovered) {
                    // TODO: Button clicked
                    LOG_WARN("Button clicked!");
                }
            }
            pressed = false;
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
public:
    GuiTabControl(Font* font)
    : font(font) {
    }

    void setTabCount(int n) {
        buttons.resize(n);
        for (int i = 0; i < buttons.size(); ++i) {
            if (!buttons[i]) {
                buttons[i].reset(new GuiTabButton(font));
            }
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
        case GUI_MSG::PAINT: {
        } break;
        }
    }

    void onLayout(const gfxm::rect& rect, uint64_t flags) override {
        this->bounding_rect = rect;
        this->client_area = bounding_rect;

        const float BUTTON_MARGIN = 10.0f;
        const float tab_space_width = client_area.max.x - client_area.min.x;
        const float button_width = gfxm::_min(gfxm::_max(tab_space_width / (float)buttons.size(), 110.0f), 310.0f);// 100.0f + BUTTON_MARGIN;
        const float button_height = 20.0f;
        const int n_buttons_per_row = (int)(tab_space_width / button_width);
        const int n_rows = gfxm::_max((int)(buttons.size() / n_buttons_per_row), 1);
        
        

        for (int i = 0; i < buttons.size(); ++i) {
            int col = i % n_buttons_per_row;
            int row = i / n_buttons_per_row;
            gfxm::vec2 min = client_area.min + gfxm::vec2(col * button_width, row * button_height);
            gfxm::rect rc(
                min, min + gfxm::vec2(button_width - BUTTON_MARGIN, button_height)
            );
            buttons[i]->onLayout(rc, 0);
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
            buttons[i]->onDraw();
        }

        //guiDrawRectLine(bounding_rect);
    }
};