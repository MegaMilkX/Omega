#include "common/gui/gui_system.hpp"


#include <set>
#include <stack>

static std::set<GuiElement*> root_elements;
static GuiElement* hovered_elem = 0;
static GuiElement* mouse_captured_element = 0;
static GUI_HIT     hovered_hit = GUI_HIT::NOWHERE;
static bool        dragging = false;
static bool        resizing = false;
static gfxm::vec2  last_mouse_pos = gfxm::vec2(0, 0);

void guiPostMessage(GUI_MSG msg) {
    switch (msg) {
    case GUI_MSG::LBUTTON_DOWN:
        switch (hovered_hit)
        {
        case GUI_HIT::LEFT:
        case GUI_HIT::RIGHT:
        case GUI_HIT::TOP:
        case GUI_HIT::BOTTOM:
        case GUI_HIT::TOPLEFT:
        case GUI_HIT::BOTTOMRIGHT:
        case GUI_HIT::BOTTOMLEFT:
        case GUI_HIT::TOPRIGHT:
            guiCaptureMouse(hovered_elem);
            resizing = true;
            break;
        case GUI_HIT::CAPTION:
            guiCaptureMouse(hovered_elem);
            dragging = true;
            break;
        }

        if (mouse_captured_element) {
            mouse_captured_element->onMessage(msg,0,0);
        } else if(hovered_elem) {
            hovered_elem->onMessage(msg,0,0);
        }
        break;
    case GUI_MSG::LBUTTON_UP:
        if (resizing && mouse_captured_element) {
            resizing = false;
            guiCaptureMouse(0);
        }
        if (dragging && mouse_captured_element) {
            dragging = false;
            guiCaptureMouse(0);
        }
        if (mouse_captured_element) {
            mouse_captured_element->onMessage(msg,0,0);
        } else if (hovered_elem) {
            hovered_elem->onMessage(msg,0,0);
        }
        break;
    };
}

void guiPostMouseMove(int x, int y) {
    GuiElement* last_hovered = 0;
    GUI_HIT hit = GUI_HIT::NOWHERE;
    for (auto& e : root_elements) {
        std::stack<GuiElement*> stack;
        stack.push(e);
        while (!stack.empty()) {
            GuiElement* elem = stack.top();
            stack.pop();

            if (!elem->isEnabled()) {
                continue;
            }

            GuiHitResult hr = elem->hitTest(x, y);
            if (hr.hit == GUI_HIT::NOWHERE) {
                continue;
            }

            last_hovered = hr.elem;
            hit = hr.hit;
            
            if (hr.hit == GUI_HIT::CLIENT) {
                for (int i = 0; i < elem->childCount(); ++i) {
                    stack.push(elem->getChild(i));
                }
            }
        }
    }

    if (hovered_elem != last_hovered) {
        if (hovered_elem) {
            hovered_elem->onMessage(GUI_MSG::MOUSE_LEAVE, 0, 0);
        }
        hovered_elem = last_hovered;
        if (hovered_elem) {
            hovered_elem->onMessage(GUI_MSG::MOUSE_ENTER, 0, 0);
        }
    }

    GuiElement* mouse_target = hovered_elem;
    if (mouse_captured_element) {
        mouse_target = mouse_captured_element;
    }

    if (mouse_target) {
        mouse_target->onMessage(GUI_MSG::MOUSE_MOVE, x, y);
    }

    if (!mouse_captured_element) {
        switch (hit) {
        case GUI_HIT::LEFT:
        case GUI_HIT::RIGHT:
            SetCursor(LoadCursorA(0, IDC_SIZEWE));
            break;
        case GUI_HIT::TOP:
        case GUI_HIT::BOTTOM:
            SetCursor(LoadCursorA(0, IDC_SIZENS));
            break;
        case GUI_HIT::TOPLEFT:
        case GUI_HIT::BOTTOMRIGHT:
            SetCursor(LoadCursorA(0, IDC_SIZENWSE));
            break;
        case GUI_HIT::BOTTOMLEFT:
        case GUI_HIT::TOPRIGHT:
            SetCursor(LoadCursorA(0, IDC_SIZENESW));
            break;
        default:
            SetCursor(LoadCursorA(0, IDC_ARROW));
        }

        hovered_hit = hit;
    } else if(resizing) {
        gfxm::vec2 cur_mouse_pos(x, y);
        guiPostResizingMessage(mouse_captured_element, hovered_hit, gfxm::rect(last_mouse_pos, cur_mouse_pos));    
    } else if(dragging) {
        gfxm::vec2 cur_mouse_pos(x, y);
        mouse_captured_element->pos += cur_mouse_pos - last_mouse_pos;
    }

    last_mouse_pos = gfxm::vec2(x, y);
}

void guiPostResizingMessage(GuiElement* elem, GUI_HIT border, gfxm::rect rect) {
    elem->onMessage(GUI_MSG::RESIZING, (uint64_t)border, (uint64_t)&rect);
}


void guiCaptureMouse(GuiElement* e) {
    mouse_captured_element = e;
}


void guiLayout() {
    int sw = 0, sh = 0;
    platformGetWindowSize(sw, sh);
    gfxm::rect rc(
        0, 0, sw, sh
    );
    for (auto& e : root_elements) {
        e->onLayout(rc, 0);
    }
}

void guiDraw(Font* font) {
    int sw = 0, sh = 0;
    platformGetWindowSize(sw, sh);

    for (auto& e : root_elements) {
        glScissor(
            0,
            0,
            sw,
            sh
        );
        e->onDraw();
    }

    glScissor(
        0,
        0,
        sw,
        sh
    );/*
    guiDrawText(
        gfxm::vec2(0, 0), 
        MKSTR("Hit: " << (int)hovered_hit << ", hovered_elem: " << hovered_elem << ", mouse capture: " << mouse_captured_element).c_str(), 
        font, .0f, 0xFFFFFFFF
    );*/
}


// ---------
GuiElement::GuiElement() {

}
GuiElement::~GuiElement() {
    if (mouse_captured_element == this) {
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

#include "common/gui/elements/gui_dock_space.hpp"
GuiDockSpace::GuiDockSpace(Font* font)
: root(font) {
    root_elements.insert(this);
}
GuiDockSpace::~GuiDockSpace() {
    if (!getParent()) {
        root_elements.erase(this);
    }
}

#include "common/gui/elements/gui_window.hpp"
GuiWindow::GuiWindow(Font* fnt, const char* title)
: font(fnt), title(title) {
    scroll_bar_v.reset(new GuiScrollBarV());
    scroll_bar_v->setOwner(this);

    root_elements.insert(this);
}
GuiWindow::~GuiWindow() {
    if (!getParent()) {
        root_elements.erase(this);
    }
}