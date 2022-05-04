
#include "gui.hpp"

#include <set>
#include <stack>

static std::set<GuiElement*> root_elements;
static GuiElement* hovered_elem = 0;
static GuiElement* mouse_captured_element = 0;

void guiPostMessage(GUI_MSG msg) {
    switch (msg) {
    case GUI_MSG::LBUTTON_DOWN:
        if (mouse_captured_element) {
            mouse_captured_element->onMessage(msg,0,0);
        } else if(hovered_elem) {
            hovered_elem->onMessage(msg,0,0);
        }
        break;
    case GUI_MSG::LBUTTON_UP:
        if (mouse_captured_element) {
            mouse_captured_element->onMessage(msg,0,0);
        } else if (hovered_elem) {
            hovered_elem->onMessage(msg,0,0);
        }
        break;
    };
    /*
    for (auto& e : root_elements) {
        std::stack<GuiElement*> stack;
        stack.push(e);
        while (!stack.empty()) {
            GuiElement* elem = stack.top();
            stack.pop();

            elem->onMessage(msg);

            for (int i = 0; i < elem->childCount(); ++i) {
                stack.push(elem->getChild(i));
            }
        }
    }*/
}

void guiPostMouseMove(int x, int y) {
    GuiElement* last_hovered = 0;
    for (auto& e : root_elements) {
        std::stack<GuiElement*> stack;
        stack.push(e);
        while (!stack.empty()) {
            GuiElement* elem = stack.top();
            stack.pop();

            if (!elem->isEnabled()) {
                continue;
            }

            gfxm::rect rc_bounding = elem->getBoundingRect();
            gfxm::rect rc_client = elem->getClientArea();
            if (!gfxm::point_in_rect(rc_bounding, gfxm::vec2(x, y))) {
                continue;
            }

            if (gfxm::point_in_rect(rc_client, gfxm::vec2(x, y))) {
                last_hovered = elem;
            }

            for (int i = 0; i < elem->childCount(); ++i) {
                stack.push(elem->getChild(i));
            }
        }
        if (hovered_elem != last_hovered) {
            if (hovered_elem) {
                hovered_elem->onMessage(GUI_MSG::MOUSE_LEAVE, 0, 0);
            }
            hovered_elem = last_hovered;
            hovered_elem->onMessage(GUI_MSG::MOUSE_ENTER, 0, 0);
        }
        if (!mouse_captured_element) {
            if (hovered_elem) {
                hovered_elem->onMessage(GUI_MSG::MOUSE_MOVE, x, y);
            }
        } else {
            mouse_captured_element->onMessage(GUI_MSG::MOUSE_MOVE, x, y);
        }
    }
}

void guiCaptureMouse(GuiElement* e) {
    mouse_captured_element = e;
}


GuiElement::GuiElement() {
    root_elements.insert(this);
}
GuiElement::~GuiElement() {
    if (!getParent()) {
        root_elements.erase(this);
    }
    if(mouse_captured_element == this) {
        mouse_captured_element = 0;
    }
}

void GuiElement::addChild(GuiElement* elem) {
    children.push_back(elem);
    if (!elem->parent) {
        root_elements.erase(elem);
    }
    elem->parent = this;    
}
size_t GuiElement::childCount() const {
    return children.size();
}
GuiElement* GuiElement::getChild(int i) {
    return children[i];
}


GuiWindow::GuiWindow(Font* fnt)
: font(fnt) {
    title_bar = new GuiWindowTitleBar(fnt);
    scroll_bar_v = new GuiScrollBarV();
    addChild(title_bar);
    addChild(scroll_bar_v);
}
GuiWindow::~GuiWindow() {

}