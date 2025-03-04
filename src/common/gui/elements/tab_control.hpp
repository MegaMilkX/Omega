#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/window.hpp"
#include "gui/elements/list_toolbar_button.hpp"
#include "platform/platform.hpp"

#include "gui/gui_text_buffer.hpp"

void guiCaptureMouse(GuiElement* e);

class GuiTabButton : public GuiElement {
    void* user_ptr = 0;
    int id = 0;
    GuiTextBuffer caption;
    bool dragging = false;
    bool is_highlighted = false;
    gfxm::rect icon_rc;
    std::unique_ptr<GuiListToolbarButton> close_btn;
    std::unique_ptr<GuiListToolbarButton> pin_btn;
public:
    bool is_front = false;

    GuiTabButton() {
        close_btn.reset(new GuiListToolbarButton(guiLoadIcon("svg/entypo/cross.svg"), GUI_MSG::TAB_CLOSE));
        pin_btn.reset(new GuiListToolbarButton(guiLoadIcon("svg/custom/pin.svg"), GUI_MSG::TAB_PIN));
        close_btn->setParent(this);
        close_btn->setOwner(this);
        pin_btn->setParent(this);
        pin_btn->setOwner(this);
    }

    void setCaption(const char* caption) {
        this->caption.replaceAll(getFont(), caption, strlen(caption));
    }
    void setUserPtr(void* ptr) {
        user_ptr = ptr;
    }
    void setId(int i) {
        this->id = i;
    }
    void setHighlighted(bool v) {
        is_highlighted = v;
    }

    void* getUserPtr() const { return user_ptr; }

    bool isDragged() const {
        return dragging;
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(rc_bounds, gfxm::vec2(x, y))) {
            return;
        }

        close_btn->hitTest(hit, x, y);
        if (hit.hasHit()) {
            return;
        }
        pin_btn->hitTest(hit, x, y);
        if (hit.hasHit()) {
            return;
        }

        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::TAB_CLOSE:
            notifyOwner(GUI_NOTIFY::TAB_CLOSED, this);
            return true;
        case GUI_MSG::MOUSE_ENTER: {
            notifyOwner(GUI_NOTIFY::TAB_MOUSE_ENTER, (int)id);
            return true;
        }
        case GUI_MSG::LCLICK:
            getOwner()->notify(GUI_NOTIFY::TAB_CLICKED, this);
            return true;
        case GUI_MSG::MCLICK:
            notifyOwner(GUI_NOTIFY::TAB_CLOSED, this);
            return true;
        case GUI_MSG::PULL_START:
            dragging = true;
            getOwner()->notify(GUI_NOTIFY::DRAG_TAB_START, (int)id);
            return true;
        case GUI_MSG::PULL_STOP:
            dragging = false;
            getOwner()->notify(GUI_NOTIFY::DRAG_TAB_END, (int)id);
            return true;
        }

        return GuiElement::onMessage(msg, params);
    }

    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        //rc_bounds.min = rc.min;
        rc_bounds.min = gfxm::vec2(0, 0);

        Font* font = getFont();

        caption.prepareDraw(font, false);
        gfxm::vec2 text_size = caption.getBoundingSize();
        rc_bounds.max.y = rc_bounds.min.y + font->getLineHeight() * 1.5f;
        rc_bounds.max.x = rc_bounds.min.x + text_size.x + GUI_MARGIN * 2.f;
        float icon_sz = client_area.max.y - client_area.min.y;
        rc_bounds.max.x += icon_sz * 2.f;
        client_area = rc_bounds;
        size.x.unit = gui_pixel;
        size.y.unit = gui_pixel;
        size.x.value = rc_bounds.max.x - rc_bounds.min.x;
        size.y.value = rc_bounds.max.y - rc_bounds.min.y;

        icon_rc = gfxm::rect(
            client_area.max - gfxm::vec2(icon_sz, icon_sz),
            client_area.max
        );
        gfxm::rect icon_rc2 = gfxm::rect(
            icon_rc.min - gfxm::vec2(icon_sz, 0),
            icon_rc.max - gfxm::vec2(icon_sz, 0)
        );
        close_btn->layout_position = icon_rc.min;
        close_btn->layout(gfxm::rect_size(icon_rc), 0);
        pin_btn->layout_position = icon_rc2.min;
        pin_btn->layout(gfxm::rect_size(icon_rc2), 0);
    }

    void onDraw() override {
        uint32_t col = GUI_COL_BG;
        uint32_t col2 = GUI_COL_BUTTON;
        if(is_highlighted) {
            col = GUI_COL_ACCENT_DIM;
            col2 = GUI_COL_ACCENT;
        } else {
            if (isPressed()) {
                col = GUI_COL_ACCENT;
                col2 = GUI_COL_ACCENT;
            } else if (isHovered()) {
                col = GUI_COL_BUTTON;
                col2 = GUI_COL_BUTTON_HOVER;
            } else if(!is_front) {
                col = GUI_COL_BG_DARK;
                col2 = GUI_COL_BG_DARK;
            }
        }
        if (is_highlighted) {
            guiDrawRectGradient(client_area, col, col2, col, col2);
        } else {
            guiDrawRectGradient(client_area, col, col2, col, col2);
        }

        {
            gfxm::rect rc = client_area;
            rc.min.x += GUI_MARGIN;
            caption.draw(getFont(), rc, GUI_LEFT | GUI_VCENTER, GUI_COL_TEXT, GUI_COL_TEXT);
        }

        // Draw close button
        close_btn->draw();
        pin_btn->draw();
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
    }
    ~GuiTabControl() {
    }

    void addTab(const char* caption, void* user_ptr) {
        auto btn = new GuiTabButton();
        btn->setCaption(caption);
        btn->setUserPtr(user_ptr);
        btn->setId(buttons.size());
        btn->setOwner(this);
        btn->setParent(this);
        buttons.push_back(std::unique_ptr<GuiTabButton>(btn));

        if (active_button) {
            active_button->is_front = false;
        }
        active_button = btn;
        active_button->is_front = true;
    }
    void removeTab(int i) {
        assert(i < buttons.size());
        if (i >= buttons.size()) {
            return;
        }
        auto btn = buttons[i].get();
        if (btn == active_button && buttons.size() > 1) {
            active_button = buttons[std::max(0, i - 1)].get();
            active_button->is_front = true;
        }
        buttons.erase(buttons.begin() + i);
        for (int j = i; j < buttons.size(); ++j) {
            buttons[j]->setId(j);
        }
    }
    void setCurrentTab(int i) {
        if (active_button) {
            active_button->is_front = false;
        }
        GuiTabButton* btn = buttons[i].get();
        btn->is_front = true;
        active_button = btn;
        notifyOwner(GUI_NOTIFY::TAB_CLICKED, btn);
    }

    int getTabCount() const {
        return buttons.size();
    }

    GuiTabButton* getTabButton(int i) {
        return buttons[i].get();
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }

        for (int i = 0; i < buttons.size(); ++i) {
            if (current_dragged_tab == i) {
                continue;
            }
            buttons[i]->hitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }

        hit.add(GUI_HIT::CLIENT, this);
        return;
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::NOTIFY: {
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::TAB_CLICKED: {
                if (active_button) {
                    active_button->is_front = false;
                }
                GuiTabButton* btn = params.getB<GuiTabButton*>();
                btn->is_front = true;
                active_button = btn;
                return false; // Pass the message further up
            }
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
        case GUI_MSG::MOUSE_MOVE: {
            gfxm::vec2 pt(params.getA<int32_t>(), params.getB<int32_t>());
            last_mouse_pos = pt;
            if (current_dragged_tab != -1) {
                gfxm::rect client_area_padded = client_area;
                float client_area_height = client_area.max.y - client_area.min.y;
                client_area_padded.min.y -= client_area_height;
                client_area_padded.max.y += client_area_height;
                if (!gfxm::point_in_rect(client_area_padded, pt)) {
                    notifyOwner(GUI_NOTIFY::TAB_DRAGGED_OUT, buttons[current_dragged_tab].get());
                    
                    current_dragged_tab = -1;
                    guiReleaseMouseCapture(this);
                } else {
                    int idx = findTabSlot(params.getA<int32_t>(), params.getB<int32_t>());
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
            return true;
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

    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        this->rc_bounds = gfxm::rect(gfxm::vec2(0, 0), extents);
        this->client_area = rc_bounds;
        rc_bounds.max.y = rc_bounds.min.y;

        gfxm::vec2 cur = gfxm::vec2(0, 0);
        float btn_max_height = .0f;
        const gfxm::rect rect(gfxm::vec2(0, 0), extents);
        for (int i = 0; i < buttons.size(); ++i) {
            gfxm::rect rc = rect;
            rc.min = cur;
            buttons[i]->layout_position = rc.min;
            buttons[i]->layout(gfxm::rect_size(rc), 0);
            float btn_width = buttons[i]->size.x.value;
            btn_max_height = gfxm::_max(btn_max_height, buttons[i]->size.y.value);
            if (rect.max.x < btn_width + cur.x) {
                cur.x = rect.min.x;
                cur.y += btn_max_height;
                rc_bounds.max.y += btn_max_height;
                btn_max_height = .0f;

                gfxm::rect rc = rect;
                rc.min = cur;
                buttons[i]->layout_position = rc.min;
                buttons[i]->layout(gfxm::rect_size(rc), 0);
                btn_max_height = gfxm::_max(btn_max_height, buttons[i]->size.y.value);
                cur.x += buttons[i]->size.x.value;
            } else {
                cur.x += buttons[i]->size.x.value;
            }
        }
        if (current_dragged_tab >= 0) {
            auto& btn = buttons[current_dragged_tab];
            auto& rc_btn = btn->getBoundingRect();
            gfxm::rect rc = rect;
            rc.min = gfxm::vec2(last_mouse_pos.x - dragged_tab_offs.x, rc_btn.min.y);
            btn->layout_position = rc.min;
            btn->layout(gfxm::rect_size(rc), 0);
        }
        rc_bounds.max.y += btn_max_height;
        client_area = rc_bounds;
    }

    void onDraw() override {
        int sw = 0, sh = 0;
        platformGetWindowSize(sw, sh);
        
        guiDrawPushScissorRect(client_area);
        for (int i = 0; i < buttons.size(); ++i) {
            if (current_dragged_tab == i) {
                continue;
            }
            buttons[i]->draw();
        }
        if (current_dragged_tab >= 0) {
            buttons[current_dragged_tab]->draw();
        }

        guiDrawPopScissorRect();

        //guiDrawRectLine(client_area, 0xFF00FF00);
    }
};