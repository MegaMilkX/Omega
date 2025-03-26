#pragma once

#include "gui/elements/element.hpp"



class GuiAnimStateNode : public GuiElement {
public:
    GuiAnimStateNode() {
        setPosition(0, 0);
        setSize(250, 50);
        addFlags(GUI_FLAG_FLOATING);
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
        switch (msg) {
        case GUI_MSG::PULL_START:
        case GUI_MSG::PULL_STOP: {
            return true;
        }
        case GUI_MSG::PULL: {
            pos.x.value += params.getA<float>();
            pos.y.value += params.getB<float>();
            return true;
        }
        case GUI_MSG::LCLICK: {
            notifyOwner(GUI_NOTIFY::STATE_NODE_CLICKED, this, 0);
            return true;
        }
        case GUI_MSG::DBL_LCLICK: {
            notifyOwner(GUI_NOTIFY::STATE_NODE_DOUBLE_CLICKED, this, 0);
            return true;
        }
        case GUI_MSG::MOVING: {
            gfxm::rect* prc = params.getB<gfxm::rect*>();
            gfxm::vec2 to = gfxm::vec2(prc->max.x, prc->max.y);
            gfxm::vec2 from = gfxm::vec2(prc->min.x, prc->min.y);
            gfxm::vec2 diff = to - from;
            // TODO: FIX UNITS
            pos.x.value += diff.x;
            pos.y.value += diff.y;
            return true;
        }
        }
        return false;
    }
    void onDraw() override {
        gfxm::vec2 shadow_offset(10.0f, 10.0f);
        guiDrawRectRoundBorder(gfxm::rect(
            rc_bounds.min + shadow_offset, rc_bounds.max + shadow_offset
        ), 15.0f, 10.0f, 0x00000000, 0xAA000000);
        guiDrawRectRound(rc_bounds, 15, GUI_COL_BUTTON);
        guiDrawText(client_area, "AnimStateNode", getFont(), GUI_HCENTER | GUI_VCENTER, GUI_COL_TEXT);
    }
};
class GuiStateGraph : public GuiElement {
    struct Connection {
        GuiAnimStateNode* from;
        GuiAnimStateNode* to;
    };

    GuiAnimStateNode* connection_preview_src = 0;
    std::vector<Connection> connections;

    bool connectionExists(GuiAnimStateNode* a, GuiAnimStateNode* b) {
        for (int i = 0; i < connections.size(); ++i) {
            auto& c = connections[i];
            if (c.from == a && c.to == b) {
                return true;
            }
        }
        return false;
    }
    bool makeConnection(GuiAnimStateNode* a, GuiAnimStateNode* b) {
        if (connectionExists(a, b)) {
            return false;
        }
        connections.push_back(Connection{ a, b });
        return true;
    }
public:
    GuiStateGraph() {
        addFlags(GUI_FLAG_DRAG_CONTENT);
        setSize(0, 0);

        setStyleClasses({ "state-graph" });

        {
            auto ctx_menu = new GuiMenuList();
            ctx_menu->addItem(new GuiMenuListItem("Hello"));
            ctx_menu->addItem(new GuiMenuListItem("World!"));
            ctx_menu->addItem(new GuiMenuListItem("Foo"));
            ctx_menu->addItem(new GuiMenuListItem("Bar"));
            guiAddContextPopup(this, ctx_menu);
        }
        auto node = new GuiAnimStateNode;
        node->setPosition(100, 100);
        guiAdd(this, this, node);

        node = new GuiAnimStateNode;
        node->setPosition(150, 280);
        guiAdd(this, this, node);

        node = new GuiAnimStateNode;
        node->setPosition(250, 220);
        guiAdd(this, this, node);
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::RCLICK:
        case GUI_MSG::DBL_RCLICK: {
            if (connection_preview_src) {
                connection_preview_src = 0;
                return true;
            }
            break;
        }
        case GUI_MSG::NOTIFY: {
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::STATE_NODE_CLICKED: {
                if (connection_preview_src) {
                    auto node_b = params.getB<GuiAnimStateNode*>();
                    if (node_b == connection_preview_src) {
                        return true;
                    }
                    makeConnection(connection_preview_src, node_b);
                    connection_preview_src = 0;
                }
                return true;
            }
            case GUI_NOTIFY::STATE_NODE_DOUBLE_CLICKED: {
                connection_preview_src = params.getB<GuiAnimStateNode*>();
                return true;
            }
            }
            break;
        }
        }
        return GuiElement::onMessage(msg, params);
    }
    void onDraw() override {
        for (int i = 0; i < connections.size(); ++i) {
            auto& c = connections[i];
            gfxm::vec2 pta = (c.from->getBoundingRect().min + c.from->getBoundingRect().max) * .5f;
            gfxm::vec2 ptb = (c.to->getBoundingRect().min + c.to->getBoundingRect().max) * .5f;
            gfxm::vec2 offs = gfxm::normalize(ptb - pta);
            std::swap(offs.x, offs.y);
            offs *= 10.f;
            offs.y = -offs.y;
            guiDrawLineWithArrow(
                pta + offs,
                ptb + offs,
                5.f, GUI_COL_TEXT
            );
        }
        if (connection_preview_src) {
            gfxm::rect rc_node = connection_preview_src->getBoundingRect();
            gfxm::vec2 ptsrc = (rc_node.min + rc_node.max) * .5f;
            gfxm::vec2 mouse = guiGetMousePos();
            gfxm::vec2 offs = gfxm::normalize(mouse - ptsrc);
            std::swap(offs.x, offs.y);
            offs *= 10.f;
            offs.y = -offs.y;
            guiDrawLineWithArrow(
                ptsrc + offs,
                mouse,
                5.f, GUI_COL_TIMELINE_CURSOR
            );
        }

        GuiElement::onDraw();

        guiDrawText(client_area.min, "Double click on a node - start new connection", getFont(), 0, GUI_COL_TEXT);
        guiDrawText(client_area.min + gfxm::vec2(.0f, 20.f), MKSTR("Pos content: " << pos_content.x << " " << pos_content.y).c_str(), getFont(), 0, GUI_COL_TEXT);
    }
};
