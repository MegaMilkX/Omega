#pragma once

#include "gui/elements/element.hpp"

void guiForceElementMoveState(GuiElement* wnd);
void guiForceElementMoveState(GuiElement* wnd, int mouse_x, int mouse_y);
bool guiDragStartWindowDockable(GuiElement* window);


class GuiWindowDecorator : public GuiElement {
    GuiElement* wrapped = nullptr;
public:
    GuiWindowDecorator(GuiElement* wrapped) {
        setStyleClasses({ "window-frame" });
        addFlags(GUI_FLAG_WINDOW);

        auto title = new GuiElement();
        title->setSize(gui::fill(), gui::px(30));
        title->pushBack("WindowDecorator");
        title->setStyleClasses({ "title-bar" });
        title->addFlags(GUI_FLAG_CAPTION);
        _addChild(title);
        auto content = new GuiElement();
        content->setSize(gui::fill(), gui::fill());
        _addChild(content);
        this->content = content;

        setSize(wrapped->size);

        pushBack(wrapped);
        this->wrapped = wrapped;
        wrapped->removeFlags(GUI_FLAG_FLOATING);
        //wrapped->setSize(gui::fill(), gui::fill());
    }

    GuiElement* getWrapped() { return wrapped; }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::CHILD_REMOVED: {
            if(params.getA<GuiElement*>() == wrapped) {
                getParent()->notify<GuiElement*>(GUI_NOTIFY::WINDOW_FRAME_ORPHANED, this);
                return true;
            }
            break;
        }
        case GUI_MSG::MOVE_START: {
            guiDragStartWindowDockable(wrapped);
            return true;
        }
        case GUI_MSG::MOVING: {
            auto rc = params.getB<gfxm::rect*>();
            gfxm::vec2 offs = rc->max - rc->min;
            pos.x.value += offs.x;
            pos.y.value += offs.y;
            return true;
        }
        }
        return GuiElement::onMessage(msg, params);
    }
};


class GuiWindowLayer2 : public GuiElement {
    std::set<GuiWindowDecorator*> windows;
    gfxm::vec2 next_window_pos = gfxm::vec2(50, 50);
public:
    GuiWindowLayer2() {
        addFlags(GUI_FLAG_NO_HIT);
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::NOTIFY:
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::WINDOW_FRAME_ORPHANED:
                _removeChild(params.getB<GuiElement*>());
                return true;
            }
            break;
        }
        return GuiElement::onMessage(msg, params);
    }
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        rc_bounds = gfxm::rect(gfxm::vec2(0, 0), extents);
        client_area = rc_bounds;
        for (int i = 0; i < children.size(); ++i) {
            auto& e = children[i];

            if (e->isHidden()) {
                continue;
            }

            gfxm::vec2 wnd_size = gui_to_px(e->size, getFont(), extents);
            //gfxm::vec2 wnd_size(800, 600);
            e->layout_position = gfxm::vec2(e->pos.x.value, e->pos.y.value);

            e->layout(wnd_size, flags/* | GUI_LAYOUT_NO_BORDER | GUI_LAYOUT_NO_TITLE*/);
        }
    }
    void onDraw() override {
        for (int i = 0; i < children.size(); ++i) {
            auto& e = children[i];
            if (e->isHidden()) {
                continue;
            }
            children[i]->draw(e->layout_position.x, e->layout_position.y);
        }
    }
    
    void pushBackInDragState(GuiElement* e) {
        GuiElement::pushBack(e);
        guiForceElementMoveState(children.back(), 55, 15);
        guiDragStartWindowDockable(e);
    }
    void addChild(GuiElement* elem) override {
        auto frame = new GuiWindowDecorator(elem);
        _addChild(frame);
        windows.insert(frame);

        frame->setPosition(gui::px(next_window_pos.x), gui::px(next_window_pos.y));
        next_window_pos += gfxm::vec2(20, 20);
    }
    void removeChild(GuiElement* elem) override {
        GuiWindowDecorator* window = nullptr;
        for (auto f : windows) {
            if (f->getWrapped() == elem) {
                window = f;
                break;
            }
        }
        if (!window) {
            return;
        }
        _removeChild(window);
        windows.erase(window);
    }
};

