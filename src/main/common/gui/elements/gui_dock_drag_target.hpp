#pragma once 

#include "common/gui/elements/gui_element.hpp"
#include "common/gui/gui_system.hpp"



class GuiDockDragDropSplitterTarget : public GuiElement {
public:
    GUI_DOCK_SPLIT_DROP split_type;

    GuiHitResult hitTest(int x, int y) override {
        if (!guiIsDragDropInProgress()) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        if (!point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }

        return GuiHitResult{ GUI_HIT::DOCK_DRAG_DROP_TARGET, this };
    }
    void onMessage(GUI_MSG msg, uint64_t a_param, uint64_t b_param) override {
        switch (msg) {
        case GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD:
            getOwner()->onMessage(GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD_SPLIT, a_param, (uint64_t)split_type);
            //getOwner()->onMessage(GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD, a_param, b_param);
            break;
        }
    }
    void onLayout(const gfxm::rect& rect, uint64_t flags) override {
        client_area = rect;
    }
    void onDraw() override {
        guiDrawRect(client_area, GUI_COL_BUTTON);
    }
};
class GuiDockDragDropSplitter : public GuiElement {
    GuiDockDragDropSplitterTarget mid, left, right, top, bottom;
public:
    GuiDockDragDropSplitter() {
        mid.setOwner(this);
        left.setOwner(this);
        right.setOwner(this);
        top.setOwner(this);
        bottom.setOwner(this);
        mid.split_type = GUI_DOCK_SPLIT_DROP::MID;
        left.split_type = GUI_DOCK_SPLIT_DROP::LEFT;
        right.split_type = GUI_DOCK_SPLIT_DROP::RIGHT;
        top.split_type = GUI_DOCK_SPLIT_DROP::TOP;
        bottom.split_type = GUI_DOCK_SPLIT_DROP::BOTTOM;
    }
    GuiHitResult hitTest(int x, int y) override {
        if (!guiIsDragDropInProgress()) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }/*
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }*/

        GuiDockDragDropSplitterTarget* targets[5];
        targets[0] = &mid;
        targets[1] = &left;
        targets[2] = &right;
        targets[3] = &top;
        targets[4] = &bottom;
        for (int i = 0; i < 5; ++i) {
            GuiHitResult h = targets[i]->hitTest(x, y);
            if (h.hit != GUI_HIT::NOWHERE) {
                return h;
            }
        }

        return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
    }
    void onMessage(GUI_MSG msg, uint64_t a_param, uint64_t b_param) override {
        switch (msg) {
        case GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD_SPLIT:
            getOwner()->onMessage(GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD_SPLIT, a_param, b_param);
            break;
        }
    }
    void onLayout(const gfxm::rect& rect, uint64_t flags) override {
        gfxm::vec2 cmid = gfxm::lerp(rect.min, rect.max, 0.5f);
        gfxm::vec2 cleft = cmid - gfxm::vec2(50.0f, .0f);
        gfxm::vec2 cright = cmid + gfxm::vec2(50.0f, .0f);
        gfxm::vec2 ctop = cmid - gfxm::vec2(.0f, 50.0f);
        gfxm::vec2 cbottom = cmid + gfxm::vec2(.0f, 50.0f);
        mid.onLayout(gfxm::rect(
            cmid - gfxm::vec2(20.0f, 20.0f),
            cmid + gfxm::vec2(20.0f, 20.0f)
        ), 0);
        left.onLayout(gfxm::rect(
            cleft - gfxm::vec2(20.0f, 20.0f),
            cleft + gfxm::vec2(20.0f, 20.0f)
        ), 0);
        right.onLayout(gfxm::rect(
            cright - gfxm::vec2(20.0f, 20.0f),
            cright + gfxm::vec2(20.0f, 20.0f)
        ), 0);
        top.onLayout(gfxm::rect(
            ctop - gfxm::vec2(20.0f, 20.0f),
            ctop + gfxm::vec2(20.0f, 20.0f)
        ), 0);
        bottom.onLayout(gfxm::rect(
            cbottom - gfxm::vec2(20.0f, 20.0f),
            cbottom + gfxm::vec2(20.0f, 20.0f)
        ), 0);
    }
    void onDraw() override {
        mid.onDraw();
        left.onDraw();
        right.onDraw();
        top.onDraw();
        bottom.onDraw();
    }

};