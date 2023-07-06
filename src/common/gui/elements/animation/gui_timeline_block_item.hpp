#pragma once

#include "gui/elements/gui_element.hpp"


template<bool IS_RIGHT>
class GuiTimelineBlockResizer : public GuiElement {
public:
    GuiHitResult onHitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        if (IS_RIGHT) {
            return GuiHitResult{ GUI_HIT::RIGHT, this };
        } else {
            return GuiHitResult{ GUI_HIT::LEFT, this };            
        }
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::RESIZING: {
            if (getOwner()) {
                getOwner()->sendMessage(msg, params);
            }
            return true;
        }
        }
        return false;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        rc_bounds = rc;
        client_area = rc;
    }
    void onDraw() override {
        if (IS_RIGHT) {
            guiDrawRectRound(client_area, 5.f, GUI_COL_TEXT, GUI_DRAW_CORNER_RIGHT);
        } else {
            guiDrawRectRound(client_area, 5.f, GUI_COL_TEXT, GUI_DRAW_CORNER_LEFT);
        }
    }
};
class GuiTimelineBlockItem : public GuiElement {
    bool is_dragging = false;
    gfxm::rect rc_resize_left;
    gfxm::rect rc_resize_right;
    std::unique_ptr<GuiTimelineBlockResizer<true>> resizer_right;
    std::unique_ptr<GuiTimelineBlockResizer<false>> resizer_left;
public:
    int type = 0;
    void* user_ptr = 0;

    uint32_t color = 0xFFCCCCCC;

    bool highlight_override = false;
    int frame;
    int length;
    gfxm::vec2 grab_point;
    GuiTimelineBlockItem(int frame, int len)
        : frame(frame), length(len) {
        resizer_right.reset(new GuiTimelineBlockResizer<true>());
        resizer_right->setOwner(this);
        addChild(resizer_right.get());
        resizer_left.reset(new GuiTimelineBlockResizer<false>());
        resizer_left->setOwner(this);
        addChild(resizer_left.get());
    }
    GuiHitResult onHitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        GuiHitResult hit;
        hit = resizer_left->onHitTest(x, y);
        if (hit.hasHit()) {
            return hit;
        }
        hit = resizer_right->onHitTest(x, y);
        if (hit.hasHit()) {
            return hit;
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::LCLICK:
        case GUI_MSG::DBL_LCLICK:
            notifyOwner<int, GuiTimelineBlockItem*>(GUI_NOTIFY::TIMELINE_BLOCK_SELECTED, 0, this);
            return true;
        case GUI_MSG::MOUSE_MOVE:
            if (is_dragging) {
                if (getOwner()) {
                    getOwner()->notify(GUI_NOTIFY::TIMELINE_DRAG_BLOCK, this);
                }
            } else {
                grab_point = gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>()) - client_area.min;
            }
            return true;
        case GUI_MSG::LBUTTON_DOWN:
            guiCaptureMouse(this);
            is_dragging = true;
            return true;
        case GUI_MSG::LBUTTON_UP:
            guiCaptureMouse(0);
            is_dragging = false;
            return true;
        case GUI_MSG::RBUTTON_DOWN:
            notifyOwner(GUI_NOTIFY::TIMELINE_ERASE_BLOCK, this);
            return true;
        case GUI_MSG::RESIZING: {
            GUI_HIT hit = params.getA<GUI_HIT>();
            gfxm::rect* prc = params.getB<gfxm::rect*>();
            if (hit == GUI_HIT::LEFT) {
                notifyOwner(GUI_NOTIFY::TIMELINE_RESIZE_BLOCK_LEFT, this, prc->min.x);
            } else if(hit == GUI_HIT::RIGHT) {
                notifyOwner(GUI_NOTIFY::TIMELINE_RESIZE_BLOCK_RIGHT, this, prc->max.x);
            }
            return true;
        }
        }
        return false;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        rc_bounds = rc;
        client_area = rc;
        rc_resize_left = rc;
        rc_resize_left.max.x = rc_resize_left.min.x + 5.f;
        rc_resize_right = rc;
        rc_resize_right.min.x = rc_resize_right.max.x - 5.f;

        resizer_right->layout(rc_resize_right, flags);
        resizer_left->layout(rc_resize_left, flags);
    }
    void onDraw() override {
        uint32_t col = color;
        bool draw_resizers = false;
        if (highlight_override 
            || isHovered() 
            || resizer_right->isHovered() || resizer_left->isHovered() 
            || resizer_right->hasMouseCapture() || resizer_left->hasMouseCapture() 
            || is_dragging
        ) {
            col = GUI_COL_TIMELINE_CURSOR;
            draw_resizers = true;
        }
        guiDrawRectRound(client_area, 5.f, col);
        guiDrawRectRoundBorder(client_area, 5.f, 2.f, GUI_COL_BG, GUI_COL_BG);

        if (draw_resizers) {
            resizer_right->draw();
            resizer_left->draw();
        }
    }
};

