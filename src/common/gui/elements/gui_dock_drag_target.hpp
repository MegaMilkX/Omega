#pragma once 

#include "gui/elements/gui_element.hpp"
#include "gui/gui_system.hpp"


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
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::DOCK_TAB_DRAG_ENTER:
            getOwner()->notify<GUI_DOCK_SPLIT_DROP>(GUI_NOTIFY::DRAG_DROP_TARGET_HOVERED, split_type);
            break;
        case GUI_MSG::DOCK_TAB_DRAG_LEAVE:
            getOwner()->notify<GUI_DOCK_SPLIT_DROP>(GUI_NOTIFY::DRAG_DROP_TARGET_HOVERED, GUI_DOCK_SPLIT_DROP::NONE);
            break;
        case GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD:
            getOwner()->sendMessage(GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD_SPLIT, params.getA<uint64_t>(), split_type);
            //getOwner()->onMessage(GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD, a_param, b_param);
            break;
        }
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rect, uint64_t flags) override {
        client_area = rect;
    }
    void onDraw() override {
        guiDrawRect(client_area, GUI_COL_BUTTON);
    }
};
class GuiDockDragDropSplitter : public GuiElement {
    GuiDockDragDropSplitterTarget mid, left, right, top, bottom;
    GUI_DOCK_SPLIT_DROP hovered_target_type = GUI_DOCK_SPLIT_DROP::NONE;
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

        guiGetRoot()->addChild(this);
    }
    ~GuiDockDragDropSplitter() {
        guiGetRoot()->removeChild(this);
    }
    GuiHitResult hitTest(int x, int y) override {
        if (!isEnabled()) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
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
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD_SPLIT:
            getOwner()->sendMessage(GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD_SPLIT, params);
            break;
        case GUI_MSG::NOTIFY: {
            GUI_NOTIFY n = params.getA<GUI_NOTIFY>();
            switch (n) {
            case GUI_NOTIFY::DRAG_DROP_TARGET_HOVERED:
                hovered_target_type = params.getB<GUI_DOCK_SPLIT_DROP>();
                break;
            }
            } break;
        }
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rect, uint64_t flags) override {
        if (!isEnabled()) {
            return;
        }
        if (!guiIsDragDropInProgress()) {
            return;
        }
        gfxm::rect rc = rect;
        if (getOwner()) {
            rc = getOwner()->getClientArea(); // TODO: Think of a better solution
        }
        client_area = rc;
        gfxm::vec2 cmid = gfxm::lerp(rc.min, rc.max, 0.5f);
        gfxm::vec2 cleft = cmid - gfxm::vec2(50.0f, .0f);
        gfxm::vec2 cright = cmid + gfxm::vec2(50.0f, .0f);
        gfxm::vec2 ctop = cmid - gfxm::vec2(.0f, 50.0f);
        gfxm::vec2 cbottom = cmid + gfxm::vec2(.0f, 50.0f);
        mid.onLayout(cursor, gfxm::rect(
            cmid - gfxm::vec2(20.0f, 20.0f),
            cmid + gfxm::vec2(20.0f, 20.0f)
        ), 0);
        left.onLayout(cursor, gfxm::rect(
            cleft - gfxm::vec2(20.0f, 20.0f),
            cleft + gfxm::vec2(20.0f, 20.0f)
        ), 0);
        right.onLayout(cursor, gfxm::rect(
            cright - gfxm::vec2(20.0f, 20.0f),
            cright + gfxm::vec2(20.0f, 20.0f)
        ), 0);
        top.onLayout(cursor, gfxm::rect(
            ctop - gfxm::vec2(20.0f, 20.0f),
            ctop + gfxm::vec2(20.0f, 20.0f)
        ), 0);
        bottom.onLayout(cursor, gfxm::rect(
            cbottom - gfxm::vec2(20.0f, 20.0f),
            cbottom + gfxm::vec2(20.0f, 20.0f)
        ), 0);
    }
    void onDraw() override {
        if (!isEnabled()) {
            return;
        }
        if (!guiIsDragDropInProgress()) {
            return;
        }

        uint64_t preview_box_col = GUI_COL_ACCENT;
        preview_box_col &= 0x00FFFFFF;
        preview_box_col |= 0x80000000;
        gfxm::rect rc = client_area;
        switch (hovered_target_type) {
        case GUI_DOCK_SPLIT_DROP::LEFT:            
            rc.max.x -= (rc.max.x - rc.min.x) * .6f;
            guiDrawRect(rc, preview_box_col);
            break;
        case GUI_DOCK_SPLIT_DROP::RIGHT:
            rc.min.x += (rc.max.x - rc.min.x) * .6f;
            guiDrawRect(rc, preview_box_col);
            break;
        case GUI_DOCK_SPLIT_DROP::TOP:
            rc.max.y -= (rc.max.y - rc.min.y) * .6f;
            guiDrawRect(rc, preview_box_col);
            break;
        case GUI_DOCK_SPLIT_DROP::BOTTOM:
            rc.min.y += (rc.max.y - rc.min.y) * .6f;
            guiDrawRect(rc, preview_box_col);
            break;
        case GUI_DOCK_SPLIT_DROP::MID:
            guiDrawRect(client_area, preview_box_col);
            break;
        }

        mid.onDraw();
        left.onDraw();
        right.onDraw();
        top.onDraw();
        bottom.onDraw();
    }

};