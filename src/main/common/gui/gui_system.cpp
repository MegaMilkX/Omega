#include "common/gui/gui_system.hpp"

#include "common/gui/elements/gui_root.hpp"
#include "common/gui/elements/gui_window.hpp"

#include <set>
#include <stack>
#include <chrono>

const long long DOUBLE_CLICK_TIME = 500;

static GuiFont                  font_global;
static std::stack<GuiFont*>     font_stack;
static std::unique_ptr<GuiRoot> root;
static GuiElement* active_window = 0;
static GuiElement* focused_window = 0;
static GuiElement* hovered_elem = 0;
static GuiElement* pressed_elem = 0; 
static GuiElement* pulled_elem = 0;
static GuiElement* mouse_captured_element = 0;
static GUI_HIT     hovered_hit = GUI_HIT::NOWHERE;
static GUI_HIT     resizing_hit = GUI_HIT::NOWHERE;
static bool        moving = false;
static bool        resizing = false;
static bool        dragging = false; // drag-n-drop
static GuiElement* dragdrop_source = 0;
static GuiElement* dragdrop_hovered_elem = 0;
static gfxm::vec2  last_mouse_pos = gfxm::vec2(0, 0);

static int modifier_keys_state = 0;

static struct {
    GuiElement* elem;
    std::chrono::time_point<std::chrono::system_clock> tp;
} last_click_data = { 0 };

struct DragDropPayload {
    uint64_t a;
    uint64_t b;
};
static DragDropPayload drag_drop_payload;

void guiInit(Font* font) {
    guiFontCreate(font_global, font);

    root.reset(new GuiRoot());
}
void guiCleanup() {
    root.reset();
}

GuiRoot* guiGetRoot() {
    return root.get();
}

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
            if (hovered_elem->isDockable()) {
                dragging = true;
                dragdrop_source = 0; // TODO: What if dragged window is a child of another window?
                drag_drop_payload = DragDropPayload{ (uint64_t)hovered_elem, 0 };
            }
            break;
        case GUI_HIT::CLIENT:
            pressed_elem = hovered_elem;
            break;
        }

        if (mouse_captured_element) {
            mouse_captured_element->onMessage(msg,0,0);
            guiSetActiveWindow(mouse_captured_element);
            guiSetFocusedWindow(mouse_captured_element);
        } else if(hovered_elem) {
            hovered_elem->onMessage(msg,0,0);
            guiSetActiveWindow(hovered_elem);
            guiSetFocusedWindow(hovered_elem);
        }
        break;
    case GUI_MSG::LBUTTON_UP: {
        if (dragging) {
            guiPostMessage(GUI_MSG::DOCK_TAB_DRAG_STOP, 0, 0);
        }
        if (resizing && mouse_captured_element) {
            resizing = false;
        }
        if (moving && mouse_captured_element) {
            moving = false;
        }
        if (mouse_captured_element) {
            mouse_captured_element->onMessage(msg,0,0);
            guiCaptureMouse(0);
        } else if (hovered_elem) {
            hovered_elem->onMessage(msg,0,0);
        }

        if (pressed_elem) {
            if (pressed_elem == hovered_elem) {
                std::chrono::time_point<std::chrono::system_clock> now
                    = std::chrono::system_clock::now();
                
                long long time_elapsed_from_last_click = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_click_data.tp).count();
                if (pressed_elem == last_click_data.elem 
                    && time_elapsed_from_last_click <= DOUBLE_CLICK_TIME) {
                    // last click time to zero, to avoid triggering dbl_click again
                    last_click_data.tp = std::chrono::time_point<std::chrono::system_clock>();
                    pressed_elem->sendMessage(GUI_MSG::DBL_CLICKED, 0, 0);
                } else {
                    last_click_data.elem = pressed_elem;
                    // If this was a pull-click, don't store new time to avoid triggering a double click on the next click
                    if (!pulled_elem) {
                        last_click_data.tp = now;
                    }
                    pressed_elem->sendMessage(GUI_MSG::CLICKED, 0, 0);
                }                
            }
            pressed_elem = 0; 
        }
        if (pulled_elem) {
            pulled_elem->sendMessage(GUI_MSG::PULL_STOP, 0, 0);
            pulled_elem = 0; 
        }
        } break;
    case GUI_MSG::KEYDOWN:
        switch (a) {
        case VK_CONTROL:
            modifier_keys_state |= GUI_KEY_CONTROL;
            break;
        case VK_MENU:
            modifier_keys_state |= GUI_KEY_ALT;
            break;
        case VK_SHIFT:
            modifier_keys_state |= GUI_KEY_SHIFT;
            break;
        }

        if (focused_window) {
            focused_window->onMessage(msg, a, b);
        }
        break;
    case GUI_MSG::KEYUP:
        switch (a) {
        case VK_CONTROL:
            modifier_keys_state &= ~(GUI_KEY_CONTROL);
            break;
        case VK_MENU:
            modifier_keys_state &= ~(GUI_KEY_ALT);
            break;
        case VK_SHIFT:
            modifier_keys_state &= ~(GUI_KEY_SHIFT);
            break;
        }

        if (focused_window) {
            focused_window->onMessage(msg, a, b);
        }
        break;
    case GUI_MSG::UNICHAR:
        if (focused_window) {
            focused_window->onMessage(GUI_MSG::UNICHAR, a, b);
        }
        break;
    case GUI_MSG::DOCK_TAB_DRAG_START:
        SetCursor(LoadCursorA(0, IDC_HAND));
        dragging = true;
        dragdrop_source = (GuiElement*)b;
        drag_drop_payload = DragDropPayload{ a, b };
        break;
    case GUI_MSG::DOCK_TAB_DRAG_STOP: {
        assert(dragging);
        if(dragging) {
            SetCursor(LoadCursorA(0, IDC_ARROW));
            if (hovered_elem && hovered_hit == GUI_HIT::DOCK_DRAG_DROP_TARGET) {
                hovered_elem->onMessage(GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD, drag_drop_payload.a, drag_drop_payload.b);
                if (dragdrop_source) {
                    dragdrop_source->onMessage(GUI_MSG::DOCK_TAB_DRAG_SUCCESS, drag_drop_payload.a, drag_drop_payload.b);
                }
            } else if(!moving) {
                GuiWindow* wnd = (GuiWindow*)drag_drop_payload.a;
                guiGetRoot()->addChild(wnd);
                wnd->pos = last_mouse_pos - gfxm::vec2(50.0f, 10.0f);
                if (dragdrop_source) {
                    dragdrop_source->onMessage(GUI_MSG::DOCK_TAB_DRAG_SUCCESS, drag_drop_payload.a, drag_drop_payload.b);
                    // NOTE: This is not a fail state
                    //dragdrop_source->onMessage(GUI_MSG::DOCK_TAB_DRAG_FAIL, drag_drop_payload.a, drag_drop_payload.b);
                }                
            }
            dragging = false;
            dragdrop_source = 0;
        }
        }break;
    };
}

void guiPostMouseMove(int x, int y) {    
    GuiElement* last_hovered = 0;
    GUI_HIT hit = GUI_HIT::NOWHERE;

    auto ht = root->hitTest(x, y);
    last_hovered = ht.elem;
    hit = ht.hit;
    hovered_hit = ht.hit;

    if (pressed_elem) {
        if (pressed_elem != pulled_elem) {
            pulled_elem = pressed_elem;            
            pulled_elem->sendMessage(GUI_MSG::PULL_START, 0, 0);
        }
        pulled_elem->sendMessage(GUI_MSG::PULL, 0, 0);
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
    } else if(resizing) {
        gfxm::vec2 cur_mouse_pos(x, y);
        guiPostResizingMessage(mouse_captured_element, resizing_hit, gfxm::rect(last_mouse_pos, cur_mouse_pos));    
    } else if(moving) {
        gfxm::vec2 cur_mouse_pos(x, y);
        guiPostMovingMessage(mouse_captured_element, gfxm::rect(last_mouse_pos, cur_mouse_pos));
    }
    if(dragging) {
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

void guiPostMovingMessage(GuiElement* elem, gfxm::rect rect) {
    elem->onMessage(GUI_MSG::MOVING, 0, (uint64_t)&rect);
}


void guiSetActiveWindow(GuiElement* elem) {
    GuiElement* e = elem;
    if (elem != 0) {
        auto flags = elem->getFlags();
        while ((flags & GUI_FLAG_OVERLAPPED) == 0 && e) {
            e = e->getParent();
            if (e) {
                flags = e->getFlags();
            }
        }
        if (!e) {
            return; // TODO: Not sure if correct, but scrollbars don't have their windows as parents
                    // so don't switch active window if no overlapped parent found to avoid losing active state while scrolling
        }
    }
    
    GuiElement* new_active_window = 0;
    if (e && e != guiGetRoot()) {
        new_active_window = e;
    } else {
        new_active_window = 0;
    }

    if (new_active_window != active_window) {
        if (active_window) {
            active_window->onMessage(GUI_MSG::DEACTIVATE, 0, 0);
        }
        if (new_active_window) {
            new_active_window->onMessage(GUI_MSG::ACTIVATE, 0, 0);
        }
        active_window = new_active_window;
        if (active_window) {
            guiBringWindowToTop(active_window);
        }
    }
}
GuiElement* guiGetActiveWindow() {
    return active_window;
}
void guiSetFocusedWindow(GuiElement* elem) {
    if (elem != focused_window) {
        if (focused_window) {
            focused_window->onMessage(GUI_MSG::UNFOCUS, 0, 0);
        }
        focused_window = elem;
        if (elem) {
            elem->onMessage(GUI_MSG::FOCUS, 0, 0);
        }
    }
}
GuiElement* guiGetFocusedWindow() {
    return focused_window;
}
GuiElement* guiGetHoveredElement() {
    return hovered_elem;
}
GuiElement* guiGetPressedElement() {
    return pressed_elem;
}
GuiElement* guiGetPulledElement() {
    return pulled_elem;
}

void guiBringWindowToTop(GuiElement* e) {
    assert(e && e != guiGetRoot());
    if (!e || e == guiGetRoot()) {
        return;
    }
    GuiElement* parent = e->getParent();
    parent->bringToTop(e);
}


void guiCaptureMouse(GuiElement* e) {
    mouse_captured_element = e;
}


void guiLayout() {
    assert(root);
    int sw = 0, sh = 0;
    platformGetWindowSize(sw, sh);
    gfxm::rect rc(
        0, 0, sw, sh
    );

    guiPushFont(&font_global);
    root->layout(rc, 0);
    guiPopFont();
}

void guiDraw(Font* font) {
    assert(root);
    
    guiPushFont(&font_global);

    root->draw();
    
    int sw = 0, sh = 0;
    platformGetWindowSize(sw, sh);
    gfxm::rect dbg_rc(
        0, 0, sw, sh
    );
    dbg_rc.min.y = dbg_rc.max.y - 30.0f;
    glScissor(0, 0, sw, sh);
    guiDrawRect(dbg_rc, GUI_COL_BLACK);
    guiDrawText(
        dbg_rc.min,
        MKSTR("Hit: " << (int)hovered_hit << ", hovered_elem: " << hovered_elem << ", mouse capture: " << mouse_captured_element).c_str(), 
        guiGetDefaultFont(), .0f, 0xFFFFFFFF
    );
    
    guiPopFont();
}

bool guiIsDragDropInProgress() {
    return dragging && !resizing;
}


int guiGetModifierKeysState() {
    return modifier_keys_state;
}


bool guiClipboardGetString(std::string& out) {
    if (!IsClipboardFormatAvailable(CF_TEXT)) {
        return false;
    }
    if (!OpenClipboard(0)) {
        return false;
    }
    HGLOBAL hglb = { 0 };
    hglb = GetClipboardData(CF_TEXT);
    if (hglb == NULL) {
        CloseClipboard();
        return false;
    }

    LPTSTR lptstr = (LPTSTR)GlobalLock(hglb);
    if (lptstr == NULL) {
        CloseClipboard();
        return false;
    }

    out = lptstr;
    GlobalUnlock(hglb);

    CloseClipboard();
    return true;
}
bool guiClipboardSetString(std::string str) {
    if (OpenClipboard(0)) {
        HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, str.size() + 1);
        if (hglbCopy) {
            EmptyClipboard();
            LPTSTR lptstrCopy = (LPTSTR)GlobalLock(hglbCopy);
            memcpy(lptstrCopy, str.data(), str.size());
            lptstrCopy[str.size() + 1] = '\0';
            GlobalUnlock(hglbCopy);
            SetClipboardData(CF_TEXT, hglbCopy);
            CloseClipboard();
            return true;
        } else {
            CloseClipboard();
            assert(false);
        }
    }
    return false;
}

bool guiSetMousePos(int x, int y) {
    SetCursorPos(x, y);
    return true;
}

void guiPushFont(GuiFont* font) {
    font_stack.push(font);
}
void guiPopFont() {
    font_stack.pop();
}
GuiFont* guiGetCurrentFont() {
    return font_stack.top();
}
GuiFont* guiGetDefaultFont() {
    return &font_global;
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
    if (pressed_elem == this) {
        pressed_elem = 0;
    }
    if (pulled_elem == this) {
        pulled_elem = 0;
    }
    if (dragdrop_source == this) {
        dragdrop_source = 0;
    }
    if (dragdrop_hovered_elem == this) {
        dragdrop_hovered_elem = 0;
    }
    if (active_window == this) {
        active_window = 0;
    }
    if (focused_window == this) {
        focused_window = 0;
    }
}

void GuiElement::addChild(GuiElement* elem) {
    if (elem->getParent()) {
        elem->getParent()->removeChild(elem);
    }
    int new_z_order = children.size();
    children.push_back(elem);
    elem->parent = this;
    elem->z_order = new_z_order;
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
        children[id]->parent = 0;
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
GuiDockSpace::GuiDockSpace() {
    setDockPosition(GUI_DOCK::FILL);
    root.reset(new DockNode(this));
    guiGetRoot()->addChild(this);
}
GuiDockSpace::~GuiDockSpace() {
    getParent()->removeChild(this);
    parent = 0;
}

#include "common/gui/elements/gui_window.hpp"
GuiWindow::GuiWindow(const char* title_str)
: title(guiGetDefaultFont()) {
    title.replaceAll(title_str, strlen(title_str));

    scroll_bar_v.reset(new GuiScrollBarV());
    scroll_bar_v->setOwner(this);

    guiGetRoot()->addChild(this);

    setFlags(GUI_FLAG_OVERLAPPED);
}
GuiWindow::~GuiWindow() {
    getParent()->removeChild(this);
    parent = 0;
}