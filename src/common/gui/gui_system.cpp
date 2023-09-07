#include "gui/gui_system.hpp"

#include "gui/elements/root.hpp"
#include "gui/elements/window.hpp"
#include "gui/gui.hpp"
#include "gui/filesystem/gui_file_thumbnail.hpp"

#include <set>
#include <unordered_set>
#include <stack>
#include <chrono>

const long long DOUBLE_CLICK_TIME = 500;

constexpr int MSG_QUEUE_LENGTH = 512;
struct MSG_INTERNAL {
    MSG_INTERNAL()
    : target(0), msg(GUI_MSG::UNKNOWN), params(GUI_MSG_PARAMS()) {}
    MSG_INTERNAL(GuiElement* target, const GUI_MSG& msg, const GUI_MSG_PARAMS& params)
        : target(target), msg(msg), params(params) {}

    GuiElement* target;
    GUI_MSG msg;
    GUI_MSG_PARAMS params;
};
MSG_INTERNAL msg_queue[MSG_QUEUE_LENGTH];
int msgq_write_cur = 0;
int msgq_read_cur = 0;
int msgq_new_message_count = 0;

static void postMsg(GuiElement* target, GUI_MSG msg, GUI_MSG_PARAMS params) {
    assert(msgq_new_message_count < MSG_QUEUE_LENGTH - 1);
    msg_queue[msgq_write_cur] = MSG_INTERNAL(target, msg, params);
    msgq_write_cur = (msgq_write_cur + 1) % MSG_QUEUE_LENGTH;
    if (msgq_new_message_count < MSG_QUEUE_LENGTH - 1) {
        ++msgq_new_message_count;
    }
}
static bool hasMsg() {
    return msgq_new_message_count > 0;
}
static MSG_INTERNAL readMsg() {
    if (msgq_new_message_count == 0) {
        return MSG_INTERNAL(0, GUI_MSG::UNKNOWN, GUI_MSG_PARAMS());
    }
    MSG_INTERNAL msg = msg_queue[msgq_read_cur];
    msgq_read_cur = (msgq_read_cur + 1) % MSG_QUEUE_LENGTH;
    --msgq_new_message_count;
    return msg;
}

static GUI_MSG_CB_T on_message_cb;

static GUI_DRAG_PAYLOAD drag_payload;
static std::unordered_set<GuiElement*> drag_subscribers;

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
//static bool        dragging = false; // drag-n-drop
static GuiElement* dragdrop_source = 0;
static GuiElement* dragdrop_hovered_elem = 0;
static gfxm::vec2  last_mouse_pos = gfxm::vec2(0, 0);
static GUI_MSG     mouse_btn_last_event = GUI_MSG::UNKNOWN;

static int modifier_keys_state = 0;

static GuiMenuList* current_menu_root = 0;

static std::map<std::string, std::unique_ptr<GuiIcon>> icons;
std::unique_ptr<GuiIcon> icon_error;

static GuiHitResult hit_result;

constexpr int MOUSEBTN_LEFT = 0;
constexpr int MOUSEBTN_RIGHT = 1;
constexpr int MOUSEBTN_MIDDLE = 2;
constexpr int MOUSEBTN_0 = 3;
static struct {
    int btn = 0;
    GuiElement* elem;
    std::chrono::time_point<std::chrono::system_clock> tp;
} last_click_data = { 0 };
/*
struct DragDropPayload {
    uint64_t a;
    uint64_t b;
};
static DragDropPayload drag_drop_payload;
*/
void guiInit(std::shared_ptr<Font> font) {
    guiFontInit(font);
    guiFileThumbnailInit();

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

    guiFileThumbnailThreadInit();
}
void guiCleanup() {
    guiFileThumbnailThreadCleanup();

    icons.clear();
    icon_error.reset();

    _guiCleanupShaders();

    root.reset();

    guiFileThumbnailCleanup();
    guiFontCleanup();
}


static gui::style_sheet style_sheet;
gui::style_sheet& guiGetStyleSheet() {
    return style_sheet;
}

GuiElement* guiAdd(GuiElement* parent, GuiElement* owner, GuiElement* element, gui_flag_t flags) {
    if (!parent) {
        parent = guiGetRoot();
    }
    element->setOwner(owner);
    element->addFlags(flags);
    parent->addChild(element);
    return element;
}
void guiRemove(GuiElement* element) {
    // TODO:
    assert(false);
}

static std::unordered_set<GuiWindow*> managed_windows;
void guiAddManagedWindow(GuiWindow* wnd) {
    managed_windows.insert(wnd);
}
void guiDestroyWindow(GuiWindow* wnd) {
    auto it = managed_windows.find(wnd);
    if (it == managed_windows.end()) {
        return;
    }
    managed_windows.erase(wnd);
    delete wnd;
}


void guiSetMessageCallback(const GUI_MSG_CB_T& cb) {
    on_message_cb = cb;
}

GuiRoot* guiGetRoot() {
    return root.get();
}

void guiPostMessage(GuiElement* target, GUI_MSG msg, GUI_MSG_PARAMS params) {
    postMsg(target, msg, params);
}
void guiPostMessage(GUI_MSG msg) {
    GUI_MSG_PARAMS p;
    p.setA(0);
    p.setB(0);
    guiPostMessage(msg, p);
}

static int msgToMouseBtnCode(GUI_MSG msg) {
    int code = -1;
    switch (msg) {
    case GUI_MSG::LBUTTON_DOWN:
        code = MOUSEBTN_LEFT;
        break;
    case GUI_MSG::RBUTTON_DOWN:
        code = MOUSEBTN_RIGHT;
        break;
    case GUI_MSG::MBUTTON_DOWN:
        code = MOUSEBTN_MIDDLE;
        break;
    case GUI_MSG::LBUTTON_UP:
        code = MOUSEBTN_LEFT;
        break;
    case GUI_MSG::RBUTTON_UP:
        code = MOUSEBTN_RIGHT;
        break;
    case GUI_MSG::MBUTTON_UP:
        code = MOUSEBTN_MIDDLE;
        break;
    }
    return code;
}
static GUI_MSG mouseBtnCodeToClickMsg(int code) {
    switch (code) {
    case MOUSEBTN_LEFT:
        return GUI_MSG::LCLICK;
    case MOUSEBTN_RIGHT:
        return GUI_MSG::RCLICK;
    case MOUSEBTN_MIDDLE:
        return GUI_MSG::MCLICK;
    default:
        assert(false);
        return GUI_MSG::UNKNOWN;
    }
}
static GUI_MSG mouseBtnCodeToDblClickMsg(int code) {
    switch (code) {
    case MOUSEBTN_LEFT:
        return GUI_MSG::DBL_LCLICK;
    case MOUSEBTN_RIGHT:
        return GUI_MSG::DBL_RCLICK;
    case MOUSEBTN_MIDDLE:
        return GUI_MSG::DBL_MCLICK;
    default:
        assert(false);
        return GUI_MSG::UNKNOWN;
    }
}
static void handleMouseDownWindowInteractions(GuiElement* elem, GUI_HIT hit, bool is_left_btn) {
    if (hit == GUI_HIT::CLIENT) {
        pressed_elem = elem;
        return;
    }

    if (is_left_btn) {
        switch (hit)
        {
        case GUI_HIT::LEFT:
        case GUI_HIT::RIGHT:
        case GUI_HIT::TOP:
        case GUI_HIT::BOTTOM:
        case GUI_HIT::TOPLEFT:
        case GUI_HIT::BOTTOMRIGHT:
        case GUI_HIT::BOTTOMLEFT:
        case GUI_HIT::TOPRIGHT:
            guiCaptureMouse(elem);
            resizing = true;
            resizing_hit = hit;
            break;
        case GUI_HIT::CAPTION: {
            //GuiWindow* window = dynamic_cast<GuiWindow*>(elem);
            guiForceElementMoveState(elem);
            break;
        }
        }
    }
}
void guiPostMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
    postMsg(0, msg, params);
}

void guiPostMouseMove(int x, int y) {    
    GuiElement* last_hovered = 0;
    GUI_HIT hit = GUI_HIT::NOWHERE;

    hit_result.clear();
    root->onHitTest(hit_result, x, y);
    if (!hit_result.hits.empty()) {
        last_hovered = hit_result.hits.back().elem;
        hit = hit_result.hits.back().hit;
        hovered_hit = hit_result.hits.back().hit;
    } else {
        last_hovered = 0;
        hit = GUI_HIT::NOWHERE;
        hovered_hit = GUI_HIT::NOWHERE;
    }

    if (pressed_elem) {
        if (pressed_elem != pulled_elem) {
            pulled_elem = pressed_elem;            
            pulled_elem->sendMessage(GUI_MSG::PULL_START, 0, 0);
        }
        gfxm::vec2 delta = gfxm::vec2(x, y) - last_mouse_pos;
        pulled_elem->sendMessage(GUI_MSG::PULL, delta.x, delta.y);
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
        if (!guiIsDragDropInProgress()) {
            GuiWindow* wnd = dynamic_cast<GuiWindow*>(mouse_captured_element);
            if (wnd) {
                guiDragStartWindow(wnd);
            }/*
            if (wnd && wnd->isDockable()) {
                guiDragStartWindowDockable(wnd);
            } else if(wnd) {
                guiDragStartWindow(wnd);
            }*/
        }
    }
    if(guiIsDragDropInProgress()) {
        if (dragdrop_hovered_elem != hovered_elem) {
            if (dragdrop_hovered_elem) {
                dragdrop_hovered_elem->sendMessage(GUI_MSG::DOCK_TAB_DRAG_LEAVE, 0, 0);
            }
            if (hovered_hit == GUI_HIT::DOCK_DRAG_DROP_TARGET) {
                dragdrop_hovered_elem = hovered_elem;
                if (dragdrop_hovered_elem) {
                    dragdrop_hovered_elem->sendMessage<uint64_t, uint64_t>(GUI_MSG::DOCK_TAB_DRAG_ENTER, 0, 0);
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


void guiSendMessage(GuiElement* target, GUI_MSG msg, GUI_MSG_PARAMS params) {
    if (target == 0 && on_message_cb) {
        on_message_cb(msg, params);
    } else {
        target->sendMessage(msg, params);
    }
}


void guiSetActiveWindow(GuiElement* elem) {
    GuiElement* e = elem;
    if (elem != 0) {
        auto flags = elem->getFlags();
        while ((flags & GUI_FLAG_WINDOW) == 0 && e) {
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
    if (!elem) {
        if (focused_window != nullptr) {
            focused_window->sendMessage<GuiElement*, int>(GUI_MSG::UNFOCUS, 0, 0);
        }
        focused_window = 0;
    } else {
        GuiElement* new_focus = elem->sendMessage(GUI_MSG::FOCUS, 0, 0);
        if (new_focus != focused_window && focused_window != nullptr) {
            focused_window->sendMessage<GuiElement*, int>(GUI_MSG::UNFOCUS, elem, 0);
        }
        focused_window = new_focus;
    }
    /*
    if (elem != focused_window) {
        if (focused_window) {
            GuiElement* e = elem;
            while (e != focused_window && e != 0) {
                e = e->getOwner();
            }
            if (e == 0) {
                GuiElement* e = focused_window;
                while (e != elem && e != 0) {
                    e->sendMessage(GUI_MSG::UNFOCUS_MENU, 0, 0);
                    e = e->getOwner();
                }
            }
            focused_window->sendMessage<GuiElement*, int>(GUI_MSG::UNFOCUS, elem, 0);
        }
        focused_window = elem;
        if (elem) {
            elem->sendMessage(GUI_MSG::FOCUS, 0, 0);
        }
    }*/
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
    if (parent) {
        parent->bringToTop(e);
    }
}


void guiCaptureMouse(GuiElement* e) {
    mouse_captured_element = e;
    if (mouse_captured_element != hovered_elem) {
        hovered_elem->sendMessage(GUI_MSG::MOUSE_LEAVE, 0, 0);
    }
    if (e == 0) {
        platformReleaseMouse();
    } else {
        platfromCaptureMouse();
    }
}
void guiReleaseMouseCapture(GuiElement* e) {
    if (guiGetMouseCaptor() == e) {
        guiCaptureMouse(0);
    }
}
GuiElement* guiGetMouseCaptor() {
    return mouse_captured_element;
}


bool is_highlighting = false;
void guiStartHightlight(int at) {
    is_highlighting = true;
}
void guiStopHighlight() {
    is_highlighting = false;
}
bool guiIsHighlighting() {
    return is_highlighting;
}

void guiPollMessages() {
    while (hasMsg()) {
        auto msg_internal = readMsg();
        auto msg = msg_internal.msg;
        auto params = msg_internal.params;
        auto target = msg_internal.target;
        
        switch (msg) {
        default:
            if (target) {
                target->sendMessage(msg, params);
            }
            break;
        case GUI_MSG::MOUSE_SCROLL:
            if (hovered_elem) {
                hovered_elem->sendMessage(msg, params);
            }
            break;
        case GUI_MSG::LBUTTON_DOWN:
        case GUI_MSG::RBUTTON_DOWN:
        case GUI_MSG::MBUTTON_DOWN: {
            for (auto& h : hit_result.hits) {
                if (h.hit != GUI_HIT::OUTSIDE_MENU) {
                    break;
                }
                bool skip_close = h.elem->hasFlags(GUI_FLAG_MENU_SKIP_OWNER_CLICK)
                    && h.elem->getOwner() == hovered_elem;
                if (!skip_close) {
                    h.elem->sendMessage(GUI_MSG::CLOSE_MENU, GUI_MSG_PARAMS());
                }
            }
            
            int code = msgToMouseBtnCode(msg);
            handleMouseDownWindowInteractions(hovered_elem, hovered_hit, code == MOUSEBTN_LEFT);

            if (mouse_captured_element) {
                mouse_captured_element->sendMessage(msg, GUI_MSG_PARAMS());
                guiSetActiveWindow(mouse_captured_element);
                guiSetFocusedWindow(mouse_captured_element);
            } else if (hovered_elem) {
                hovered_elem->sendMessage(msg, GUI_MSG_PARAMS());
                guiSetActiveWindow(hovered_elem);
                guiSetFocusedWindow(hovered_elem);
            }
            break;
        }
        case GUI_MSG::LBUTTON_UP:
        case GUI_MSG::RBUTTON_UP:
        case GUI_MSG::MBUTTON_UP: {
            int code = msgToMouseBtnCode(msg);
            if (code == MOUSEBTN_LEFT) {
                guiDragStop();

                if (resizing && mouse_captured_element) {
                    resizing = false;
                }
                if (moving && mouse_captured_element) {
                    moving = false;
                }
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
                
                    long long time_elapsed_from_last_click
                        = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_click_data.tp).count();
                    if (pressed_elem == last_click_data.elem 
                        && time_elapsed_from_last_click <= DOUBLE_CLICK_TIME
                        && last_click_data.btn == code
                    ) {
                        // last click time to zero, to avoid triggering dbl_click again
                        last_click_data.tp = std::chrono::time_point<std::chrono::system_clock>();
                        last_click_data.btn = code;
                        mouse_btn_last_event = mouseBtnCodeToDblClickMsg(code);
                        pressed_elem->sendMessage<int32_t, int32_t>(
                            mouse_btn_last_event,
                            last_mouse_pos.x, last_mouse_pos.y
                        );
                    } else {
                        last_click_data.elem = pressed_elem;
                        // If this was a pull-click, don't store new time to avoid triggering a double click on the next click
                        if (!pulled_elem) {
                            last_click_data.tp = now;
                        }
                        last_click_data.btn = code;
                        mouse_btn_last_event = mouseBtnCodeToClickMsg(code);
                        bool skip_click = pulled_elem == pressed_elem && pulled_elem->hasFlags(GUI_FLAG_NO_PULL_CLICK);
                        if(!skip_click) {
                            pressed_elem->sendMessage<int32_t, int32_t>(
                                mouse_btn_last_event,
                                last_mouse_pos.x, last_mouse_pos.y
                            );
                        }
                    }                
                }
                pressed_elem = 0; 
            }
            if (code == MOUSEBTN_LEFT) {
                if (pulled_elem) {
                    pulled_elem->sendMessage(GUI_MSG::PULL_STOP, 0, 0);
                    pulled_elem = 0;
                }
            }
            break;
        }
        case GUI_MSG::KEYDOWN: {
            uint16_t key = params.getA<uint16_t>();
            switch (key) {
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
            } else if (active_window) {
                active_window->sendMessage(msg, params);
            }
            break;
        }
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
        case GUI_MSG::DRAG_START: {
            for (auto& e : drag_subscribers) {
                e->sendMessage(GUI_MSG::DRAG_START, 0, 0, 0);
            }
            break;
        }
        case GUI_MSG::DRAG_STOP: {
            if (hovered_elem && (hovered_elem->sys_flags & GUI_SYS_FLAG_DRAG_SUBSCRIBER)) {
                hovered_elem->sendMessage(GUI_MSG::DRAG_DROP, 0, 0, 0);
            }
            for (auto& e : drag_subscribers) {
                e->sendMessage(GUI_MSG::DRAG_STOP, 0, 0, 0);
            }
            {
                if (drag_payload.type == GUI_DRAG_FILE) {
                    auto ptr = (std::string*)drag_payload.payload_ptr;
                    assert(ptr);
                    delete ptr;
                }
                drag_payload.payload_ptr = 0;
                drag_payload.type = GUI_DRAG_NONE;
            }
            break;
        }
        case GUI_MSG::DOCK_TAB_DRAG_STOP: {/*
            assert(dragging);
            if(dragging) {
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
            }*/
            }break;
        };
    }
}

void guiLayout() {
    assert(root);
    int sw = 0, sh = 0;
    platformGetWindowSize(sw, sh);
    gfxm::rect rc(
        0, 0, sw, sh
    );
    
    //guiPushFont(guiGetDefaultFont());
    root->layout(rc, 0);
    root->update_selection_range(0);
    //guiLayoutBox(&root->box, gfxm::vec2(sw, sh));
    //guiLayoutPlaceBox(&root->box, gfxm::vec2(0, 0));
    //guiPopFont();
}

void guiDraw() {
    assert(root);

    int sw = 0, sh = 0;
    platformGetWindowSize(sw, sh);

    guiClearViewportRectStack();
    guiSetDefaultViewportRect(gfxm::rect(.0f, .0f, (float)sw, (float)sh));
    guiClearViewTransform();
    guiClearProjection();
    guiSetDefaultProjection(gfxm::ortho(.0f, (float)sw, (float)sh, .0f, .0f, 100.0f));
    
    //guiPushFont(guiGetDefaultFont());

    root->draw();
    //guiDbgDrawLayoutBox(&root->box);

    gfxm::rect dbg_rc(
        0, 0, sw, sh
    );
    dbg_rc.min.y = dbg_rc.max.y - 30.0f;
    
    guiDrawRect(dbg_rc, GUI_COL_BLACK);
    guiDrawText(
        dbg_rc.min,
        MKSTR(
            "Hit: " << guiHitTypeToString(hovered_hit) << 
            ", hovered_elem: " << hovered_elem << 
            ", mouse capture: " << mouse_captured_element <<
            ", is_moving: " << moving <<
            ", mouse_btn_last_event: " << guiMsgToString(mouse_btn_last_event)
        ).c_str(), 
        guiGetDefaultFont(), .0f, 0xFFFFFFFF
    );
    if (hovered_elem) {
        guiDrawText(
            dbg_rc.min + gfxm::vec2(0, 10),
            MKSTR("linear_begin: " << hovered_elem->linear_begin
                << " linear_end: " << hovered_elem->linear_end).c_str(),
            guiGetDefaultFont(), .0f, 0xFFFFFFFF
        );
    }

    if (hovered_elem) {
        // DEBUG
        guiDrawRectLine(hovered_elem->rc_content, GUI_COL_BLUE);
        guiDrawRectLine(hovered_elem->getBoundingRect(), GUI_COL_WHITE);
        guiDrawRectLine(hovered_elem->getClientArea(), GUI_COL_GREEN);
    }
    
    //guiPopFont();
}


static std::unordered_map<GuiElement*, std::unique_ptr<GuiElement>> menu_popups;
void guiAddContextPopup(GuiElement* owner, GuiElement* popup) {
    auto it = menu_popups.find(owner);
    if (it != menu_popups.end()) {
        //guiRemove(it->second.get());
        it->second.reset(popup);
    } else {
        menu_popups.insert(std::make_pair(owner, std::unique_ptr<GuiElement>(popup)));
    }
    owner->sys_flags |= GUI_SYS_FLAG_HAS_CONTEXT_POPUP;
    guiAdd(0, owner, popup, GUI_FLAG_MENU_POPUP);
    popup->setHidden(true);
}
void guiRemoveContextPopup(GuiElement* owner) {
    auto it = menu_popups.find(owner);
    if (it != menu_popups.end()) {
        //guiRemove(it->second.get());
        menu_popups.erase(it);
    }
    owner->sys_flags &= ~GUI_SYS_FLAG_HAS_CONTEXT_POPUP;
}
bool guiShowContextPopup(GuiElement* owner, int x, int y) {
    auto it = menu_popups.find(owner);
    if (it == menu_popups.end()) {
        return false;
    }
    it->second->setHidden(false);
    it->second->setPosition(x, y);
    return true;
}


bool guiDragStartFile(const char* path) {
    drag_payload.type = GUI_DRAG_FILE;
    drag_payload.payload_ptr = new std::string(path);
    guiPostMessage(GUI_MSG::DRAG_START);
    return true;
}
bool guiDragStartWindow(GuiWindow* window) {
    drag_payload.type = GUI_DRAG_WINDOW;
    drag_payload.payload_ptr = window;
    guiPostMessage(GUI_MSG::DRAG_START);
    return true;
}
bool guiDragStartWindowDockable(GuiWindow* window) {
    drag_payload.type = GUI_DRAG_WINDOW;
    drag_payload.payload_ptr = window;
    guiPostMessage(GUI_MSG::DRAG_START);
    return true;
}
void guiDragStop() {
    guiPostMessage(GUI_MSG::DRAG_STOP);
    // NOTE: Don't free the payload here
}
GUI_DRAG_PAYLOAD* guiDragGetPayload() {
    return &drag_payload;
}
bool guiIsDragDropInProgress() {
    return drag_payload.type != GUI_DRAG_NONE && !resizing;
}


void guiDragSubscribe(GuiElement* elem) {
    drag_subscribers.insert(elem);
    elem->sys_flags |= GUI_SYS_FLAG_DRAG_SUBSCRIBER;
}
void guiDragUnsubscribe(GuiElement* elem) {
    elem->sys_flags &= ~GUI_SYS_FLAG_DRAG_SUBSCRIBER;
    drag_subscribers.erase(elem);
}

void guiForceElementMoveState(GuiElement* wnd) {
    if (!wnd) {
        return;
    }

    guiSetActiveWindow(wnd);
    guiBringWindowToTop(wnd);
    guiCaptureMouse(wnd);
    moving = true;
}
void guiForceElementMoveState(GuiElement* wnd, int mouse_x, int mouse_y) {
    guiForceElementMoveState(wnd);
    wnd->pos = gui_vec2(
        last_mouse_pos.x - mouse_x,
        last_mouse_pos.y - mouse_y,
        gui_pixel
    );
}

int guiGetModifierKeysState() {
    return modifier_keys_state;
}
bool guiIsModifierKeyPressed(int key) {
    return (modifier_keys_state & key) != 0;
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
gfxm::vec2 guiGetMousePos() {
    return gfxm::vec2(
        last_mouse_pos.x,
        last_mouse_pos.y
    );
}
gfxm::vec2 guiGetMousePosLocal(const gfxm::rect& rc) {
    return last_mouse_pos - rc.min;
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
        gfxm::rect rc_bounds;
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


static gui_drop_file_cb_t drop_file_cb;

void guiSetDropFileCallback(gui_drop_file_cb_t cb) {
    drop_file_cb = cb;
}

void guiPostDropFile(const gfxm::vec2& xy, const std::experimental::filesystem::path& path) {
    // TODO: After multiple native windows are implemented, send an appropriate GUI_MSG
    // Using a global callback for now

    if (drop_file_cb) {
        bool result = drop_file_cb(path);
    }
}


// ---------
GuiElement::GuiElement() {

}
GuiElement::~GuiElement() {
    for (auto& ch : children) {
        ch->setParent(0);
    }
    if (getParent()) {
        getParent()->removeChild(this);
        //box.setParent(nullptr);
    }

    if (sys_flags & GUI_SYS_FLAG_DRAG_SUBSCRIBER) {
        guiDragUnsubscribe(this);
    }

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

#include "gui/elements/dock_space.hpp"
GuiDockSpace::GuiDockSpace(void* dock_group)
: dock_group(dock_group) {
    setSize(0, 0);

    root.reset(new DockNode(this));
    root->setParent(this);
    guiGetRoot()->addChild(this);
}
GuiDockSpace::~GuiDockSpace() {
    if (getParent()) {
        getParent()->removeChild(this);
    }
    parent = 0;
}

