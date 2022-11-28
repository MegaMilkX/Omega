#include "gui/gui_system.hpp"

#include "gui/elements/gui_root.hpp"
#include "gui/elements/gui_window.hpp"

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

static std::map<std::string, std::unique_ptr<GuiIcon>> icons;
std::unique_ptr<GuiIcon> icon_error;

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

    _guiInitShaders();

    {
        icon_error.reset(new GuiIcon);
        icon_error->shapes.push_back(GuiIcon::Shape());
        auto& shape = icon_error->shapes.back();
        shape.vertices = {
            gfxm::vec3(.0f, .0f, .0f),
            gfxm::vec3(1.f, .0f, .0f),
            gfxm::vec3(1.f, 1.f, .0f),
            gfxm::vec3(.0f, 1.f, .0f)
        };
        shape.indices = {
            0, 3, 1, 1, 3, 2
        };
        shape.color = 0xFFFFFFFF;
    }
}
void guiCleanup() {
    icons.clear();
    icon_error.reset();

    _guiCleanupShaders();

    root.reset();
}

GuiRoot* guiGetRoot() {
    return root.get();
}

void guiPostMessage(GUI_MSG msg) {
    GUI_MSG_PARAMS p;
    p.setA(0);
    p.setB(0);
    guiPostMessage(msg, p);
}

void guiPostMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
    switch (msg) {
    case GUI_MSG::MOUSE_SCROLL:
        if (hovered_elem) {
            hovered_elem->sendMessage(msg, params);
        }
        break;
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
            mouse_captured_element->onMessage(msg, GUI_MSG_PARAMS());
            guiSetActiveWindow(mouse_captured_element);
            guiSetFocusedWindow(mouse_captured_element);
        } else if(hovered_elem) {
            hovered_elem->onMessage(msg, GUI_MSG_PARAMS());
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
            mouse_captured_element->sendMessage(msg, 0, 0);
            guiCaptureMouse(0);
        } else if (hovered_elem) {
            hovered_elem->sendMessage(msg, 0, 0);
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
                    pressed_elem->sendMessage<int32_t, int32_t>(GUI_MSG::DBL_CLICKED, last_mouse_pos.x, last_mouse_pos.y);
                } else {
                    last_click_data.elem = pressed_elem;
                    // If this was a pull-click, don't store new time to avoid triggering a double click on the next click
                    if (!pulled_elem) {
                        last_click_data.tp = now;
                    }
                    pressed_elem->sendMessage<int32_t, int32_t>(GUI_MSG::CLICKED, last_mouse_pos.x, last_mouse_pos.y);
                }                
            }
            pressed_elem = 0; 
        }
        if (pulled_elem) {
            pulled_elem->sendMessage(GUI_MSG::PULL_STOP, 0, 0);
            pulled_elem = 0; 
        }
        } break;
    case GUI_MSG::RBUTTON_DOWN:
    case GUI_MSG::MBUTTON_DOWN:
        if (mouse_captured_element) {
            mouse_captured_element->sendMessage(msg, 0, 0);
            guiSetActiveWindow(mouse_captured_element);
            guiSetFocusedWindow(mouse_captured_element);
        } else if (hovered_elem) {
            hovered_elem->sendMessage(msg, 0, 0);
            guiSetActiveWindow(hovered_elem);
            guiSetFocusedWindow(hovered_elem);
        }
        break;
    case GUI_MSG::RBUTTON_UP:
    case GUI_MSG::MBUTTON_UP:
        if (mouse_captured_element) {
            mouse_captured_element->sendMessage(msg, 0, 0);
            guiCaptureMouse(0);
        } else if (hovered_elem) {
            hovered_elem->sendMessage(msg, 0, 0);
        }
        break;
    case GUI_MSG::KEYDOWN:
        switch (params.getA<uint16_t>()) {
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
            focused_window->sendMessage(msg, params);
        }
        break;
    case GUI_MSG::KEYUP:
        switch (params.getA<uint16_t>()) {
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
            focused_window->sendMessage(msg, params);
        }
        break;
    case GUI_MSG::UNICHAR:
        if (focused_window) {
            focused_window->sendMessage(msg, params);
        }
        break;
    case GUI_MSG::DOCK_TAB_DRAG_START:
        SetCursor(LoadCursorA(0, IDC_HAND));
        dragging = true;
        dragdrop_source = params.getB<GuiElement*>();
        drag_drop_payload = DragDropPayload{ (uint64_t)params.getA<GuiElement*>(), (uint64_t)params.getB<GuiElement*>() };
        break;
    case GUI_MSG::DOCK_TAB_DRAG_STOP: {
        assert(dragging);
        if(dragging) {
            SetCursor(LoadCursorA(0, IDC_ARROW));
            if (hovered_elem && hovered_hit == GUI_HIT::DOCK_DRAG_DROP_TARGET) {
                hovered_elem->sendMessage<uint64_t, uint64_t>(GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD, drag_drop_payload.a, drag_drop_payload.b);
                if (dragdrop_source) {
                    dragdrop_source->sendMessage<uint64_t, uint64_t>(GUI_MSG::DOCK_TAB_DRAG_SUCCESS, drag_drop_payload.a, drag_drop_payload.b);
                }
            } else if(!moving) {
                GuiWindow* wnd = (GuiWindow*)drag_drop_payload.a;
                guiGetRoot()->addChild(wnd);
                wnd->pos = last_mouse_pos - gfxm::vec2(50.0f, 10.0f);
                if (dragdrop_source) {
                    dragdrop_source->sendMessage<uint64_t, uint64_t>(GUI_MSG::DOCK_TAB_DRAG_SUCCESS, drag_drop_payload.a, drag_drop_payload.b);
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

void _guiUpdateMouse(int x, int y) {
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
            hovered_elem->sendMessage(GUI_MSG::MOUSE_LEAVE, 0, 0);
        }
        hovered_elem = last_hovered;
        if (hovered_elem) {
            hovered_elem->sendMessage(GUI_MSG::MOUSE_ENTER, 0, 0);
        }
    }

    GuiElement* mouse_target = hovered_elem;
    if (mouse_captured_element) {
        mouse_target = mouse_captured_element;
    }

    if (mouse_target) {
        mouse_target->sendMessage<int32_t, int32_t>(GUI_MSG::MOUSE_MOVE, x, y);
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
    } else if (resizing) {
        gfxm::vec2 cur_mouse_pos(x, y);
        guiPostResizingMessage(mouse_captured_element, resizing_hit, gfxm::rect(last_mouse_pos, cur_mouse_pos));
    } else if (moving) {
        gfxm::vec2 cur_mouse_pos(x, y);
        guiPostMovingMessage(mouse_captured_element, gfxm::rect(last_mouse_pos, cur_mouse_pos));
    }
    if (dragging) {
        if (dragdrop_hovered_elem != hovered_elem) {
            if (dragdrop_hovered_elem) {
                dragdrop_hovered_elem->sendMessage(GUI_MSG::DOCK_TAB_DRAG_LEAVE, 0, 0);
            }
            if (hovered_hit == GUI_HIT::DOCK_DRAG_DROP_TARGET) {
                dragdrop_hovered_elem = hovered_elem;
                if (dragdrop_hovered_elem) {
                    dragdrop_hovered_elem->sendMessage<uint64_t, uint64_t>(GUI_MSG::DOCK_TAB_DRAG_ENTER, drag_drop_payload.a, drag_drop_payload.b);
                }
            }
            else {
                dragdrop_hovered_elem = 0;
            }
        }
        if (dragdrop_hovered_elem) {
            dragdrop_hovered_elem->sendMessage<int32_t, int32_t>(GUI_MSG::DOCK_TAB_DRAG_HOVER, x, y);
        }
    }
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
            hovered_elem->sendMessage(GUI_MSG::MOUSE_LEAVE, 0, 0);
        }
        hovered_elem = last_hovered;
        if (hovered_elem) {
            hovered_elem->sendMessage(GUI_MSG::MOUSE_ENTER, 0, 0);
        }
    }

    GuiElement* mouse_target = hovered_elem;
    if (mouse_captured_element) {
        mouse_target = mouse_captured_element;
    }

    if (mouse_target) {
        mouse_target->sendMessage<int32_t, int32_t>(GUI_MSG::MOUSE_MOVE, x, y);
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
                dragdrop_hovered_elem->sendMessage(GUI_MSG::DOCK_TAB_DRAG_LEAVE, 0, 0);
            }
            if (hovered_hit == GUI_HIT::DOCK_DRAG_DROP_TARGET) {
                dragdrop_hovered_elem = hovered_elem;
                if (dragdrop_hovered_elem) {
                    dragdrop_hovered_elem->sendMessage<uint64_t, uint64_t>(GUI_MSG::DOCK_TAB_DRAG_ENTER, drag_drop_payload.a, drag_drop_payload.b);
                }
            } else {
                dragdrop_hovered_elem = 0;
            }
        }
        if (dragdrop_hovered_elem) {
            dragdrop_hovered_elem->sendMessage<int32_t, int32_t>(GUI_MSG::DOCK_TAB_DRAG_HOVER, x, y);
        }
    }

    last_mouse_pos = gfxm::vec2(x, y);
}

void guiPostResizingMessage(GuiElement* elem, GUI_HIT border, gfxm::rect rect) {
    elem->sendMessage<GUI_HIT, gfxm::rect*>(GUI_MSG::RESIZING, border, &rect);
}

void guiPostMovingMessage(GuiElement* elem, gfxm::rect rect) {
    elem->sendMessage<int, gfxm::rect*>(GUI_MSG::MOVING, 0, &rect);
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
            active_window->sendMessage(GUI_MSG::DEACTIVATE, 0, 0);
        }
        if (new_active_window) {
            new_active_window->sendMessage(GUI_MSG::ACTIVATE, 0, 0);
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
            focused_window->sendMessage(GUI_MSG::UNFOCUS, 0, 0);
        }
        focused_window = elem;
        if (elem) {
            elem->sendMessage(GUI_MSG::FOCUS, 0, 0);
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
GuiElement* guiGetMouseCaptor() {
    return mouse_captured_element;
}


void guiLayout() {
    assert(root);
    int sw = 0, sh = 0;
    platformGetWindowSize(sw, sh);
    gfxm::rect rc(
        0, 0, sw, sh
    );

    guiPushFont(&font_global);
    root->layout(gfxm::vec2(0, 0), rc, 0);
    guiPopFont();
}

void guiDraw() {
    assert(root);

    int sw = 0, sh = 0;
    platformGetWindowSize(sw, sh);

    guiClearViewTransform();
    
    guiPushFont(&font_global);

    root->draw();    
    
    gfxm::rect dbg_rc(
        0, 0, sw, sh
    );
    dbg_rc.min.y = dbg_rc.max.y - 30.0f;
    
    guiDrawRect(dbg_rc, GUI_COL_BLACK);
    guiDrawText(
        dbg_rc.min,
        MKSTR("Hit: " << (int)hovered_hit << ", hovered_elem: " << hovered_elem << ", mouse capture: " << mouse_captured_element).c_str(), 
        guiGetCurrentFont(), .0f, 0xFFFFFFFF
    );

    if (hovered_elem) {
        // DEBUG
        //guiDrawRectLine(hovered_elem->getClientArea(), GUI_COL_GREEN);
    }
    
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
gfxm::vec2 guiGetMousePosLocal(const gfxm::rect& rc) {
    return gfxm::vec2(
        last_mouse_pos.x,
        last_mouse_pos.y
    );
}

void guiPushFont(GuiFont* font) {
    font_stack.push(font);
}
void guiPopFont() {
    font_stack.pop();
}
GuiFont* guiGetCurrentFont() {
    if (font_stack.empty()) {
        return guiGetDefaultFont();
    }
    return font_stack.top();
}
GuiFont* guiGetDefaultFont() {
    return &font_global;
}

#define NANOSVG_IMPLEMENTATION		// Expands implementation
#include "nanosvg/nanosvg.h"
#include "nanosvg/nanosvgrast.h"
#include "CDT/include/CDT.h"
#include "math/bezier.hpp"
GuiIcon* guiLoadIcon(const char* svg_path) {
    struct Edge {
        Edge(uint32_t p0, uint32_t p1)
            : p0(p0), p1(p1) {}
        uint32_t p0;
        uint32_t p1;
    };
    struct Shape {
        std::vector<gfxm::vec3> points;
        std::vector<Edge> edges;
        std::vector<CDT::V2d<double>> cdt_points;
        CDT::EdgeVec cdt_edges;
        std::vector<gfxm::vec3> vertices3;
        std::vector<uint32_t> indices;
        gfxm::rect bounding_rect;
        gfxm::vec2 super_triangle[3];
        uint32_t color;
    };
    auto pathRemoveDuplicates = [](std::vector<gfxm::vec3>& points, std::vector<Edge>& edges) {
        if (points.size() < 2) {
            return;
        }
        int key = 1;
        auto erasePoint = [&](int k) {
            points.erase(points.begin() + key);
            for (int i = 0; i < edges.size(); ++i) {
                if (edges[i].p0 > k) {
                    edges[i].p0--;
                }
                if (edges[i].p1 > k) {
                    edges[i].p1--;
                }
            }
        };
        gfxm::vec3 ref = gfxm::normalize(points[1] - points[0]);
        while (key < points.size() - 1) {
            gfxm::vec3& p0 = points[key];
            gfxm::vec3& p1 = points[key + 1];
            if (gfxm::length(p1 - p0) <= FLT_EPSILON) {
                points.erase(points.begin() + key);
                continue;
            }
            else {
                ++key;
            }
        }
    };

    std::string path = svg_path;
    auto it = icons.find(path);
    if (it != icons.end()) {
        return it->second.get();
    }

    NSVGimage* svg = 0;
    svg = nsvgParseFromFile(svg_path, "px", 72);
    if (!svg) {
        assert(false);
        LOG_WARN("Failed to parse svg file '" << path << "'");
        return icon_error.get();
    }
    float ratio = svg->width / svg->height;
    float wscl = 1.f / svg->width;
    float hscl = 1.f / svg->height;
    hscl *= 1.f / ratio;
    
    std::vector<Shape> shapes;
    for (NSVGshape* shape = svg->shapes; shape != 0; shape = shape->next) {
        if (shape->fill.type == NSVG_PAINT_NONE) {
            // TODO: Strokes
            continue;
        }
        shapes.push_back(Shape());
        auto& shape_ = shapes.back();
        shape_.color = 0xFFFFFFFF;
        for (NSVGpath* path = shape->paths; path != 0; path = path->next) {
            std::vector<gfxm::vec3> points;
            for (int i = 0; i < path->npts - 1; i += 3) {
                float* p = &path->pts[i * 2];
                bezierCubicRecursive(
                    gfxm::vec3(p[0] * wscl , p[1] * hscl, .0f),
                    gfxm::vec3(p[2] * wscl, p[3] * hscl, .0f),
                    gfxm::vec3(p[4] * wscl, p[5] * hscl, .0f),
                    gfxm::vec3(p[6] * wscl, p[7] * hscl, .0f),
                    [&points](const gfxm::vec3& pt) {
                        points.push_back(pt);
                    }
                );
            }
            simplifyPath(points);

            uint32_t base_index = shape_.points.size();
            gfxm::vec3 pt0 = gfxm::vec3(points[0].x, points[0].y, .0f);
            gfxm::vec3 pt_last = gfxm::vec3(points[points.size() - 1].x, points[points.size() - 1].y, .0f);
            shape_.points.push_back(pt0);
            int end = points.size();
            if (gfxm::length(pt0 - pt_last) <= FLT_EPSILON) {
                end = points.size() - 1;
            }

            for (int i = 1; i < end; ++i) {
                gfxm::vec3 pt1 = gfxm::vec3(points[i].x, points[i].y, .0f);

                uint32_t ip0 = shape_.points.size() - 1;
                uint32_t ip1 = shape_.points.size();
                Edge edge = Edge(ip0, ip1);
                shape_.points.push_back(pt1);
                shape_.edges.push_back(edge);
            }
            Edge edge = Edge(shape_.points.size() - 1, base_index);
            shape_.edges.push_back(edge);
        }
    }
    nsvgDelete(svg);

    for (int i = 0; i < shapes.size(); ++i) {
        pathRemoveDuplicates(shapes[i].points, shapes[i].edges);
    }


    for (int j = 0; j < shapes.size(); ++j) {
        if (shapes[j].points.empty()) {
            assert(false);
            continue;
        }
        shapes[j].cdt_points.resize(shapes[j].points.size());
        for (int i = 0; i < shapes[j].points.size(); ++i) {
            CDT::V2d<double> p;
            p.x = shapes[j].points[i].x;
            p.y = shapes[j].points[i].y;
            shapes[j].cdt_points[i] = p;
        }
        for (int i = 0; i < shapes[j].edges.size(); ++i) {
            CDT::Edge edge(shapes[j].edges[i].p0, shapes[j].edges[i].p1);
            shapes[j].cdt_edges.push_back(edge);
        }

        try {
            CDT::Triangulation<double> cdt;
            cdt.insertVertices(shapes[j].cdt_points);
            cdt.insertEdges(shapes[j].cdt_edges);
            cdt.eraseOuterTrianglesAndHoles();
            shapes[j].indices.resize(cdt.triangles.size() * 3);
            for (int i = 0; i < cdt.triangles.size(); ++i) {
                shapes[j].indices[i * 3] = cdt.triangles[i].vertices[0];
                shapes[j].indices[i * 3 + 1] = cdt.triangles[i].vertices[1];
                shapes[j].indices[i * 3 + 2] = cdt.triangles[i].vertices[2];
            }
            shapes[j].vertices3.resize(cdt.vertices.size());
            for (int i = 0; i < cdt.vertices.size(); ++i) {
                shapes[j].vertices3[i].x = cdt.vertices[i].x;
                shapes[j].vertices3[i].y = cdt.vertices[i].y;
                shapes[j].vertices3[i].z = .0f;
            }
        } catch(std::exception& ex) {
            return icon_error.get();
        }
    }

    if (shapes.empty()) {
        return icon_error.get();
    }
    GuiIcon* icon = new GuiIcon();
    for (int i = 0; i < shapes.size(); ++i) {
        icon->shapes.push_back(GuiIcon::Shape());
        GuiIcon::Shape& sh = icon->shapes.back();
        sh.vertices = shapes[i].vertices3;
        sh.indices = shapes[i].indices;
        sh.color = shapes[i].color;
    }
    icons.insert(std::make_pair(path, std::unique_ptr<GuiIcon>(icon)));
    return icon;
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
        if (getParent()) {
            guiSetActiveWindow(getParent());
        } else if(getOwner()) {
            guiSetActiveWindow(getOwner());
        } else {
            active_window = 0;
        }
    }
    if (focused_window == this) {
        if (getParent()) {
            guiSetFocusedWindow(getParent());
        } else if(getOwner()) {
            guiSetFocusedWindow(getParent());
        } else {
            focused_window = 0;
        }
    }
}

void GuiElement::addChild(GuiElement* elem) {
    assert(elem != this);
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

#include "gui/elements/gui_dock_space.hpp"
GuiDockSpace::GuiDockSpace() {
    root.reset(new DockNode(this));
    guiGetRoot()->addChild(this);
}
GuiDockSpace::~GuiDockSpace() {
    getParent()->removeChild(this);
    parent = 0;
}

#include "gui/elements/gui_window.hpp"
GuiWindow::GuiWindow(const char* title_str)
: title(guiGetDefaultFont()) {
    title.replaceAll(title_str, strlen(title_str));

    scroll_bar_v.reset(new GuiScrollBarV());
    scroll_bar_v->setOwner(this);

    guiGetRoot()->addChild(this);

    setFlags(GUI_FLAG_OVERLAPPED);

    guiSetActiveWindow(this);
}
GuiWindow::~GuiWindow() {
    getParent()->removeChild(this);
    parent = 0;
}