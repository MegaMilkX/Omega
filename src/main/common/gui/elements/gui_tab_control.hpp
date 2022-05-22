#pragma once

#include "common/gui/elements/gui_element.hpp"
#include "common/gui/elements/gui_window.hpp"
#include "platform/platform.hpp"

#include "common/gui/gui_text_buffer.hpp"

void guiCaptureMouse(GuiElement* e);

class GuiTabButton : public GuiElement {
    int id = 0;
    GuiTextBuffer caption;
    bool dragging = false;
    bool is_highlighted = false;
public:
    GuiTabButton()
    : caption(guiGetDefaultFont()) {

    }

    void setCaption(const char* caption) {
        this->caption.replaceAll(caption, strlen(caption));
    }
    void setId(int i) {
        this->id = i;
    }
    void setHighlighted(bool v) {
        is_highlighted = v;
    }

    bool isDragged() const {
        return dragging;
    }

    void onMessage(GUI_MSG msg, uint64_t a_param, uint64_t b_param) override {
        switch (msg) {
        case GUI_MSG::CLICKED:
            getOwner()->onMessage(GUI_MSG::NOTIFY, (uint64_t)GUI_NOTIFICATION::TAB_CLICKED, (uint64_t)id);
            break;
        case GUI_MSG::PULL_START:
            dragging = true;
            getOwner()->onMessage(GUI_MSG::NOTIFY, (uint64_t)GUI_NOTIFICATION::DRAG_TAB_START, (uint64_t)id);
            break;
        case GUI_MSG::PULL_STOP:
            dragging = false;
            getOwner()->onMessage(GUI_MSG::NOTIFY, (uint64_t)GUI_NOTIFICATION::DRAG_TAB_END, (uint64_t)id);
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
        if(is_highlighted) {
            col = GUI_COL_ACCENT;
        } else {
            if (isPressed()) {
                col = GUI_COL_ACCENT;
            } else if (isHovered()) {
                col = GUI_COL_BUTTON_HOVER;
            }
        }
        guiDrawRect(client_area, col);

        caption.prepareDraw(guiGetCurrentFont(), false);

        gfxm::vec2 text_sz = caption.getBoundingSize();
        gfxm::vec2 text_pos = guiCalcTextPosInRect(
            gfxm::rect(gfxm::vec2(0, 0), text_sz), 
            client_area, 0, gfxm::rect(GUI_MARGIN, GUI_MARGIN, GUI_MARGIN, GUI_MARGIN), guiGetCurrentFont()->font
        );
        caption.draw(text_pos, GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};

class GuiTabControl : public GuiElement {
    std::vector<std::unique_ptr<GuiTabButton>> buttons;
    std::unique_ptr<GuiTabButton> dnd_fake_button;
public:
    GuiTabControl() {
    }

    void setTabCount(int n) {
        buttons.resize(n);
        for (int i = 0; i < buttons.size(); ++i) {
            if (!buttons[i]) {
                buttons[i].reset(new GuiTabButton());
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
            case GUI_NOTIFICATION::DRAG_TAB_END:
                getOwner()->onMessage(msg, a_param, b_param);
                break;
            case GUI_NOTIFICATION::TAB_CLICKED:
                getOwner()->onMessage(msg, a_param, b_param);
                break;
            }
            } break;
        case GUI_MSG::DOCK_TAB_DRAG_ENTER: {
            dnd_fake_button.reset(new GuiTabButton());
            dnd_fake_button->setCaption(((GuiWindow*)a_param)->getTitle().c_str());
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
            buttons[i]->layout(rc, 0);
        }
        if (dnd_fake_button) {
            int col = buttons.size() % n_buttons_per_row;
            int row = buttons.size() / n_buttons_per_row;
            gfxm::vec2 min = client_area.min + gfxm::vec2(col * button_width, row * button_height);
            gfxm::rect rc(
                min, min + gfxm::vec2(button_width - BUTTON_MARGIN, button_height)
            );
            dnd_fake_button->layout(rc, 0);
        }

        bounding_rect.max.y = bounding_rect.min.y + n_rows * button_height;
        client_area = bounding_rect;
    }

    void onDraw() override {
        int sw = 0, sh = 0;
        platformGetWindowSize(sw, sh);
        
        guiDrawPushScissorRect(client_area);
        for (int i = 0; i < buttons.size(); ++i) {
            
            if (buttons[i]->isDragged()) {
                continue;
            }
            buttons[i]->draw();
        }
        if (dnd_fake_button) {
            dnd_fake_button->draw();
        }
        guiDrawPopScissorRect();

        //guiDrawRectLine(client_area, 0xFF00FF00);
    }
};