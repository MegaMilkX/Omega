#pragma once 

#include "gui/elements/element.hpp"
#include "gui/gui_icon.hpp"
#include "gui/gui_system.hpp"


class GuiDockDragDropSplitterTarget : public GuiElement {
    GuiIcon* icon = 0;
public:
    GUI_DOCK_SPLIT_DROP split_type;

    GuiDockDragDropSplitterTarget(GuiIcon* icon)
    : icon(icon) {
        guiDragSubscribe(this);
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!guiIsDragDropInProgress()) {
            return;
        }

        if (!point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }

        hit.add(GUI_HIT::DOCK_DRAG_DROP_TARGET, this);
        return;
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::DRAG_START:
            if (guiDragGetPayload()->type == GUI_DRAG_WINDOW) {
                getOwner()->sendMessage(GUI_MSG::DOCK_TAB_DRAG_RESET_VIEW, 0, 0);
            }
            return true;
        case GUI_MSG::DRAG_DROP:
            if (guiDragGetPayload()->type == GUI_DRAG_WINDOW) {
                GuiWindow* wnd = (GuiWindow*)guiDragGetPayload()->payload_ptr;
                int size = 0;
                if (split_type == GUI_DOCK_SPLIT_DROP::LEFT || split_type == GUI_DOCK_SPLIT_DROP::RIGHT) {
                    // TODO: FIX UNITS
                    size = wnd->size.x.value;
                } else if(split_type == GUI_DOCK_SPLIT_DROP::TOP || split_type == GUI_DOCK_SPLIT_DROP::BOTTOM) {
                    // TODO: FIX UNITS
                    size = wnd->size.y.value;
                }
                getOwner()->sendMessage(GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD_SPLIT, (uint64_t)guiDragGetPayload()->payload_ptr, split_type, size);
            }
            return true;
        case GUI_MSG::DRAG_STOP:
            if (guiDragGetPayload()->type == GUI_DRAG_WINDOW) {
                // TODO: ?
            }
            return true;
        case GUI_MSG::DOCK_TAB_DRAG_ENTER:
            if (guiDragGetPayload()->type == GUI_DRAG_WINDOW) {
                GuiWindow* wnd = (GuiWindow*)guiDragGetPayload()->payload_ptr;
                int size = 0;
                if (split_type == GUI_DOCK_SPLIT_DROP::LEFT || split_type == GUI_DOCK_SPLIT_DROP::RIGHT) {
                    // TODO: FIX UNITS
                    size = wnd->size.x.value;
                } else if(split_type == GUI_DOCK_SPLIT_DROP::TOP || split_type == GUI_DOCK_SPLIT_DROP::BOTTOM) {
                    // TODO: FIX UNITS
                    size = wnd->size.y.value;
                }
                getOwner()->notify<GUI_DOCK_SPLIT_DROP, int>(GUI_NOTIFY::DRAG_DROP_TARGET_HOVERED, split_type, size);                
            }
            return true;
        case GUI_MSG::DOCK_TAB_DRAG_LEAVE:
            getOwner()->notify<GUI_DOCK_SPLIT_DROP>(GUI_NOTIFY::DRAG_DROP_TARGET_HOVERED, GUI_DOCK_SPLIT_DROP::NONE);
            return true;
        }
        return false;
    }
    void onLayout(const gfxm::rect& rect, uint64_t flags) override {
        client_area = rect;
    }
    void onDraw() override {
        guiDrawRectRound(client_area, 10.f, GUI_COL_BUTTON);
        if (icon) {
            gfxm::rect rc = client_area;
            gfxm::expand(rc, -5.f);
            icon->draw(rc, GUI_COL_TEXT);
        }
    }
};
class GuiDockDragDropSplitter : public GuiElement {
    GuiDockDragDropSplitterTarget mid, left, right, top, bottom;
    GUI_DOCK_SPLIT_DROP hovered_target_type = GUI_DOCK_SPLIT_DROP::NONE;
    int hovered_target_size = 0;
    void* dock_group = 0;
public:

    GuiDockDragDropSplitter(void* dock_group)
    : dock_group(dock_group),
    mid(guiLoadIcon("svg/entypo/browser.svg")),
    left(guiLoadIcon("svg/entypo/align-left.svg")),
    right(guiLoadIcon("svg/entypo/align-right.svg")),
    top(guiLoadIcon("svg/entypo/align-top.svg")),
    bottom(guiLoadIcon("svg/entypo/align-bottom.svg")) {
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

    void setDockGroup(void* group) { dock_group = group; }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!isEnabled()) {
            return;
        }
        if (!guiIsDragDropInProgress()) {
            return;
        }
        auto payload = guiDragGetPayload();
        auto wnd = (GuiWindow*)payload->payload_ptr;
        if (payload->type != GUI_DRAG_WINDOW
            || wnd->getDockGroup() != dock_group) {
            return;
        }

        GuiDockDragDropSplitterTarget* targets[5];
        targets[0] = &mid;
        targets[1] = &left;
        targets[2] = &right;
        targets[3] = &top;
        targets[4] = &bottom;
        for (int i = 0; i < 5; ++i) {
            targets[i]->onHitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }

        return;
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::DOCK_TAB_DRAG_RESET_VIEW:
            hovered_target_type = GUI_DOCK_SPLIT_DROP::NONE;
            hovered_target_size = 0;
            return true;
        case GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD_SPLIT:
            forwardMessageToOwner(msg, params);
            break;
        case GUI_MSG::NOTIFY: {
            GUI_NOTIFY n = params.getA<GUI_NOTIFY>();
            switch (n) {
            case GUI_NOTIFY::DRAG_DROP_TARGET_HOVERED:
                hovered_target_type = params.getB<GUI_DOCK_SPLIT_DROP>();
                hovered_target_size = params.getC<int>();
                return true;
            }
            } break;
        }
        return false;
    }
    void onLayout(const gfxm::rect& rect, uint64_t flags) override {
        if (!isEnabled()) {
            return;
        }
        if (!guiIsDragDropInProgress()) {
            return;
        }
        auto payload = guiDragGetPayload();
        auto wnd = (GuiWindow*)payload->payload_ptr;
        if (payload->type != GUI_DRAG_WINDOW
            || wnd->getDockGroup() != dock_group) {
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
        if (!isEnabled()) {
            return;
        }
        if (!guiIsDragDropInProgress()) {
            return;
        }
        auto payload = guiDragGetPayload();
        auto wnd = (GuiWindow*)payload->payload_ptr;
        if (payload->type != GUI_DRAG_WINDOW
            || wnd->getDockGroup() != dock_group) {
            return;
        }

        uint64_t preview_box_col = GUI_COL_ACCENT;
        preview_box_col &= 0x00FFFFFF;
        preview_box_col |= 0x80000000;
        gfxm::rect rc = client_area;
        switch (hovered_target_type) {
        case GUI_DOCK_SPLIT_DROP::LEFT:            
            rc.max.x -= gfxm::_max((rc.max.x - rc.min.x) * .6f, (rc.max.x - rc.min.x) - (float)hovered_target_size);
            guiDrawRect(rc, preview_box_col);
            break;
        case GUI_DOCK_SPLIT_DROP::RIGHT:
            rc.min.x += gfxm::_max((rc.max.x - rc.min.x) * .6f, (rc.max.x - rc.min.x) - (float)hovered_target_size);
            guiDrawRect(rc, preview_box_col);
            break;
        case GUI_DOCK_SPLIT_DROP::TOP:
            rc.max.y -= gfxm::_max((rc.max.y - rc.min.y) * .6f, (rc.max.y - rc.min.y) - (float)hovered_target_size);
            guiDrawRect(rc, preview_box_col);
            break;
        case GUI_DOCK_SPLIT_DROP::BOTTOM:
            rc.min.y += gfxm::_max((rc.max.y - rc.min.y) * .6f, (rc.max.y - rc.min.y) - (float)hovered_target_size);
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