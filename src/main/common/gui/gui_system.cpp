#include "common/gui/gui_system.hpp"


#include <set>
#include <stack>

static std::set<GuiElement*> root_elements;
static GuiElement* hovered_elem = 0;
static GuiElement* mouse_captured_element = 0;
static GUI_HIT     hovered_hit = GUI_HIT::NOWHERE;
static GUI_HIT     resizing_hit = GUI_HIT::NOWHERE;
static bool        moving = false;
static bool        resizing = false;
static bool        dragging = false; // drag-n-drop
static GuiElement* dragdrop_source = 0;
static GuiElement* dragdrop_hovered_elem = 0;
static gfxm::vec2  last_mouse_pos = gfxm::vec2(0, 0);

struct DragDropPayload {
    uint64_t a;
    uint64_t b;
};
static DragDropPayload drag_drop_payload;

void guiPostMessage(GUI_MSG msg) {
    guiPostMessage(msg, 0, 0);
}

void guiPostMessage(GUI_MSG msg, uint64_t a, uint64_t b) {
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
            resizing_hit = hovered_hit;
            break;
        case GUI_HIT::CAPTION:
            guiCaptureMouse(hovered_elem);
            moving = true;
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
        if (moving && mouse_captured_element) {
            moving = false;
            guiCaptureMouse(0);
        }
        if (mouse_captured_element) {
            mouse_captured_element->onMessage(msg,0,0);
        } else if (hovered_elem) {
            hovered_elem->onMessage(msg,0,0);
        }
        break;
    case GUI_MSG::DOCK_TAB_DRAG_START:
        SetCursor(LoadCursorA(0, IDC_HAND));
        dragging = true;
        dragdrop_source = (GuiElement*)b;
        drag_drop_payload = DragDropPayload{ a, b };
        break;
    case GUI_MSG::DOCK_TAB_DRAG_STOP:
        SetCursor(LoadCursorA(0, IDC_ARROW));
        if (hovered_elem && hovered_hit == GUI_HIT::DOCK_DRAG_DROP_TARGET) {
            hovered_elem->onMessage(GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD, drag_drop_payload.a, drag_drop_payload.b);
            dragdrop_source->onMessage(GUI_MSG::DOCK_TAB_DRAG_SUCCESS, drag_drop_payload.a, drag_drop_payload.b);
        } else {
            dragdrop_source->onMessage(GUI_MSG::DOCK_TAB_DRAG_FAIL, drag_drop_payload.a, drag_drop_payload.b);
        }
        dragging = false;
        dragdrop_source = 0;
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
            last_hovered = hr.elem;
            hit = hr.hit;
            if (hr.hit == GUI_HIT::NOWHERE) {
                continue;
            }

            
            /*
            if (hr.hit == GUI_HIT::CLIENT) {
                for (int i = 0; i < elem->childCount(); ++i) {
                    stack.push(elem->getChild(i));
                }
            }*/
        }
    }
    hovered_hit = hit;

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
    } else if(resizing) {
        gfxm::vec2 cur_mouse_pos(x, y);
        guiPostResizingMessage(mouse_captured_element, resizing_hit, gfxm::rect(last_mouse_pos, cur_mouse_pos));    
    } else if(moving) {
        gfxm::vec2 cur_mouse_pos(x, y);
        mouse_captured_element->pos += cur_mouse_pos - last_mouse_pos;
    } else if(dragging) {
        if (dragdrop_hovered_elem != hovered_elem) {
            if (dragdrop_hovered_elem) {
                dragdrop_hovered_elem->onMessage(GUI_MSG::DOCK_TAB_DRAG_LEAVE, 0, 0);
            }
            if (hovered_hit == GUI_HIT::DOCK_DRAG_DROP_TARGET) {
                dragdrop_hovered_elem = hovered_elem;
                if (dragdrop_hovered_elem) {
                    dragdrop_hovered_elem->onMessage(GUI_MSG::DOCK_TAB_DRAG_ENTER, drag_drop_payload.a, drag_drop_payload.b);
                }
            } else {
                dragdrop_hovered_elem = 0;
            }
        }
        if (dragdrop_hovered_elem) {
            dragdrop_hovered_elem->onMessage(GUI_MSG::DOCK_TAB_DRAG_HOVER, x, y);
        }
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

    gfxm::rect dbg_rc(
        0, 0, sw, sh
    );
    dbg_rc.min.y = dbg_rc.max.y - 30.0f;
    glScissor(
        0,
        0,
        sw,
        sh
    );
    guiDrawRect(dbg_rc, GUI_COL_BLACK);
    guiDrawText(
        dbg_rc.min,
        MKSTR("Hit: " << (int)hovered_hit << ", hovered_elem: " << hovered_elem << ", mouse capture: " << mouse_captured_element).c_str(), 
        font, .0f, 0xFFFFFFFF
    );
}

bool guiIsDragDropInProgress() {
    return dragging && !moving && !resizing;
}


// ---------
GuiElement::GuiElement() {

}
GuiElement::~GuiElement() {
    if (mouse_captured_element == this) {
        mouse_captured_element = 0;
    }
    if (hovered_elem == this) {
        hovered_elem = 0;
    }
    if (dragdrop_source == this) {
        dragdrop_source = 0;
    }
    if (dragdrop_hovered_elem == this) {
        dragdrop_hovered_elem = 0;
    }
}

void GuiElement::addChild(GuiElement* elem) {
    children.push_back(elem);
    if (!elem->parent) {
        root_elements.erase(elem);
    }
    elem->parent = this;
}
void GuiElement::removeChild(GuiElement* elem) {
    int id = -1;
    for (int i = 0; i < children.size(); ++i) {
        if (children[i] == elem) {
            id = i;
            break;
        }
    }
    if (id >= 0) {
        children.erase(children.begin() + id);
    }
}
size_t GuiElement::childCount() const {
    return children.size();
}
GuiElement* GuiElement::getChild(int i) {
    return children[i];
}
int GuiElement::getChildId(GuiElement* elem) {
    int id = -1;
    for (int i = 0; i < children.size(); ++i) {
        if (children[i] == elem) {
            id = i;
            break;
        }
    }
    return id;
}

#include "common/gui/elements/gui_dock_space.hpp"
GuiDockSpace::GuiDockSpace(Font* font)
: font(font) {
    root.reset(new DockNode(font, this));
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