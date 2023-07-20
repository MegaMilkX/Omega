#pragma once


#include "gui/elements/element.hpp"
#include "gui/elements/root.hpp"

#include "gui/gui_font.hpp"

#include "gui/gui_icon.hpp"



enum GUI_DRAG_TYPE {
    GUI_DRAG_NONE,
    GUI_DRAG_ELEMENT,
    GUI_DRAG_WINDOW,
    GUI_DRAG_FILE
};
struct GUI_DRAG_PAYLOAD {
    GUI_DRAG_TYPE type;
    void* payload_ptr;
};


void guiInit(Font* font);
void guiCleanup();

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
GuiElement* guiGetFocusedWindow();

GuiElement* guiGetHoveredElement(); 
// elem that was hovered when left mouse button was pressed
// persists until left button is released or the element is destroyed
GuiElement* guiGetPressedElement(); 
// left mouse press followed by mouse move results in an element being "pulled"
// this status by itself does not affect the element in any way
GuiElement* guiGetPulledElement();

void guiBringWindowToTop(GuiElement* e);

void guiCaptureMouse(GuiElement* e);
void guiReleaseMouseCapture(GuiElement* e);
GuiElement* guiGetMouseCaptor();

void guiPollMessages();
void guiLayout();
void guiDraw();

void guiAddContextPopup(GuiElement* owner, GuiElement* popup);
void guiRemoveContextPopup(GuiElement* owner);
bool guiShowContextPopup(GuiElement* owner, int x, int y);

class GuiWindow;
bool guiDragStartFile(const char* path);
bool guiDragStartWindow(GuiWindow* window);
bool guiDragStartWindowDockable(GuiWindow* window);
void guiDragStop();
GUI_DRAG_PAYLOAD* guiDragGetPayload();
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
gfxm::vec2 guiGetMousePosLocal(const gfxm::rect& rc);

GuiIcon* guiLoadIcon(const char* svg_path);


#include <experimental/filesystem>

typedef bool(*gui_drop_file_cb_t)(const std::experimental::filesystem::path&);
void guiSetDropFileCallback(gui_drop_file_cb_t cb);
void guiPostDropFile(const gfxm::vec2& xy, const std::experimental::filesystem::path& path);