#pragma once


#include "gui/elements/element.hpp"
#include "gui/elements/root.hpp"

#include "gui/gui_font.hpp"

#include "gui/gui_icon.hpp"

#include "gui/style/style_component.hpp"

#include "reflection/varying.hpp"


struct GuiDDPayload {
    virtual ~GuiDDPayload() {}
};

struct GuiStringDDPayload : public GuiDDPayload {
    std::string string;
};
struct GuiElementDDPayload : public GuiDDPayload {
    GuiElement* elem = nullptr;
    GuiElementDDPayload() {}
    GuiElementDDPayload(GuiElement* elem) : elem(elem) {}
};
struct GuiWindowDDPayload : public GuiDDPayload {
    GuiElement* elem = nullptr;
};


void guiInit(std::shared_ptr<Font> font);
void guiCleanup();

bool guiIsMouseCaptured();

gui::style_sheet& guiGetStyleSheet();
void guiMakeDefaultStyleSheet(gui::style_sheet& sheet);

GuiElement* guiAdd(GuiElement* parent, GuiElement* owner, GuiElement* element, gui_flag_t flags = 0);
void guiRemove(GuiElement* element);

class GuiWindow;
void guiAddManagedWindow(GuiWindow* wnd);
void guiDestroyWindow(GuiWindow* wnd);
template<typename T>
GuiWindow* guiCreateWindow() {
    GuiWindow* wnd = new T();
    guiAddManagedWindow(wnd);
    return wnd;
}

typedef std::function<bool(GUI_MSG, GUI_MSG_PARAMS)> GUI_MSG_CB_T;
void guiSetMessageCallback(const GUI_MSG_CB_T& cb);

GuiRoot* guiGetRoot();

template<typename T, typename... ARGS>
T* guiCreate(ARGS... args) {
    void guiAddManaged(GuiElement* e);

    static_assert(std::is_base_of_v<GuiElement, T>, "T must derive from GuiElement");
    T* e = new T(std::forward<ARGS>(args)...);
    guiAddManaged(e);
    return e;
}

void guiPostMouseButton(GUI_MOUSE_BUTTON btn, GUI_KEY_STATE state);
void guiPostMouseScroll(int value);
bool guiPostKeyDown(uint16_t vkey);
bool guiPostKeyUp(uint16_t vkey);
void guiPostUnichar(uint32_t ch);

void guiPostMessage(GuiElement* target, GUI_MSG msg, GUI_MSG_PARAMS params = GUI_MSG_PARAMS());
void guiPostMessage(GUI_MSG msg);
void guiPostMessage(GUI_MSG msg, GUI_MSG_PARAMS params);
template<typename TYPE_A, typename TYPE_B>
void guiPostMessage(GUI_MSG msg, const TYPE_A& a, const TYPE_B& b) {
    GUI_MSG_PARAMS p;
    p.setA(a);
    p.setB(b);
    guiPostMessage(msg, p);
}
void guiPostMouseMove(int x, int y);
void guiPostResizingMessage(GuiElement* elem, GUI_HIT border, gfxm::rect rect);
void guiPostMovingMessage(GuiElement* elem, gfxm::rect rect);

void guiSendMessage(GuiElement* target, GUI_MSG msg, GUI_MSG_PARAMS params);
template<typename TA, typename TB, typename TC>
void guiSendMessage(GuiElement* target, GUI_MSG msg, const TA& pa, const TB& pb, const TC& pc) {
    GUI_MSG_PARAMS p;
    p.setA(pa);
    p.setB(pb);
    p.setC(pc);
    guiSendMessage(target, msg, p);
}

void        guiSetActiveWindow(GuiElement* elem);
GuiElement* guiGetActiveWindow();
void        guiSetFocusedWindow(GuiElement* elem);
void        guiUnfocusWindow(GuiElement* elem);
void        guiUnfocus();
GuiElement* guiGetFocusedWindow();

GuiElement* guiGetHoveredElement(); 
// elem that was hovered when left mouse button was pressed
// persists until left button is released or the element is destroyed
GuiElement* guiGetPressedElement(); 
// left mouse press followed by mouse move results in an element being "pulled"
// this status by itself does not affect the element in any way
GuiElement* guiGetPulledElement();

enum GUI_TRANSIENT_SCOPE_MODE {
    GUI_TRANSIENT_SCOPE_NOTIFY,
    GUI_TRANSIENT_SCOPE_POP
};
void guiAddTransientScope(GuiElement* root, GuiElement* popup, GUI_TRANSIENT_SCOPE_MODE mode = GUI_TRANSIENT_SCOPE_POP);
void guiRemoveTransientScope(GuiElement* root);
void guiPokeTransientScopes(GuiElement* clicked);

void guiAddTransientPopup(GuiElement* scope_owner, GuiElement* popup, const gfxm::vec2& glob_pos = gfxm::vec2());
void guiRemoveTransientPopup(GuiElement* popup);
bool guiIsTransientScopeRoot(GuiElement* root);

void guiBringWindowToTop(GuiElement* e);

void guiCaptureMouse(GuiElement* e);
void guiReleaseMouseCapture(GuiElement* e);
GuiElement* guiGetMouseCaptor();
bool guiHasMouseCapture(GuiElement* e);

void guiStartHightlight(int begin);
void guiUpdateHightlight(int end);
void guiStopHighlight();
void guiSetHighlight(int begin, int end);
bool guiIsHighlighting();
int guiGetHighlightBegin();
int guiGetHighlightEnd();
void guiSetTextCursor(int at, bool highlight = false);
int guiGetTextCursor();
void guiResetTextCursor();
uint32_t guiGetTextCursorTime();
void guiAdvanceTextCursor(int, bool highlight = false);

void guiScheduleTick(GuiElement* e, float delay, GUI_TICK_ID tick_id = GUI_TICK_GENERIC);
void guiCancelTick(GuiElement* e); // Cancels all ticks for this element
void guiCancelTick(GuiElement* e, GUI_TICK_ID tick_id); // Only specific tick_id

void guiForceHitTest();

void guiCollectGarbage();
void guiPollMessages();
void guiUpdate(float dt);
void guiLayout();
void guiDraw();


class GuiWindow;
bool guiDragStart(GuiDDPayload*);
bool guiDragStartFile(const char* path, GuiElement* elem = 0);
bool guiDragStartWindow(GuiElement* window);
bool guiDragStartWindowDockable(GuiElement* window);
void guiDragStop();
GuiDDPayload* guiDragGetPayload();
template<typename T>
T* guiDragGetPayload() {
    return dynamic_cast<T*>(guiDragGetPayload());
}
bool guiIsDragDropInProgress();
void guiDragSubscribe(GuiElement* elem);
void guiDragUnsubscribe(GuiElement* elem);

void guiForceElementMoveState(GuiElement* wnd);
void guiForceElementMoveState(GuiElement* wnd, int mouse_x, int mouse_y);

int guiGetModifierKeysState();
bool guiIsModifierKeyPressed(int key);

bool guiClipboardGetString(std::string& out);
bool guiClipboardSetString(std::string str);

bool guiSetMousePos(int x, int y);
gfxm::vec2 guiGetMousePos();
gfxm::vec2 guiGetMousePosLocal(const gfxm::vec2& origin);

GuiIcon* guiLoadIcon(const char* svg_path);

gfxm::vec2 guiConvertToGlobal(GuiElement* e, const gfxm::vec2& v);
gfxm::vec2 guiConvertToLocal(GuiElement* e, const gfxm::vec2& v);
gfxm::vec2 guiConvertPosition(GuiElement* from, GuiElement* to, const gfxm::vec2& pos);

#include <filesystem>

typedef bool(*gui_drop_file_cb_t)(const std::filesystem::path&);
void guiSetDropFileCallback(gui_drop_file_cb_t cb);
void guiPostDropFile(const gfxm::vec2& xy, const std::filesystem::path& path);