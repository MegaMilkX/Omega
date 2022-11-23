#pragma once

enum class GUI_MSG {
    PAINT,
    MOUSE_MOVE,
    MOUSE_ENTER,
    MOUSE_LEAVE,
    LBUTTON_DOWN,
    LBUTTON_UP,
    RBUTTON_DOWN,
    RBUTTON_UP,
    MBUTTON_DOWN,
    MBUTTON_UP,
    MOUSE_SCROLL,

    KEYDOWN,
    KEYUP,

    UNICHAR,

    ACTIVATE,
    DEACTIVATE,

    FOCUS,
    UNFOCUS,

    CLICKED,    // left mouse buttton pressed and released while hovering the same element
    DBL_CLICKED,
    PULL_START, // user has pressed down the left mouse button and moved the mouse
    PULL,       // for any mouse move while pulling is in action
    PULL_STOP,  // 

    // SCROLL BAR
    SB_THUMB_TRACK,

    MOVING,
    RESIZING,

    NOTIFY,

    DOCK_TAB_DRAG_START,
    DOCK_TAB_DRAG_STOP,
    DOCK_TAB_DRAG_ENTER,
    DOCK_TAB_DRAG_LEAVE,
    DOCK_TAB_DRAG_HOVER,
    DOCK_TAB_DRAG_DROP_PAYLOAD,
    DOCK_TAB_DRAG_DROP_PAYLOAD_SPLIT,
    DOCK_TAB_DRAG_SUCCESS,  // Received by the sender when drag-drop operation finished successfully
    DOCK_TAB_DRAG_FAIL      // Received by the sender when drag-drop operation failed or was cancelled
};

enum class GUI_NOTIFICATION {
    NONE,

    TAB_CLICKED,

    DRAG_TAB_START,
    DRAG_TAB_END,
    DRAG_DROP_TARGET_HOVERED,

    NODE_CLICKED,
    NODE_INPUT_CLICKED,
    NODE_OUTPUT_CLICKED,
    NODE_INPUT_BREAK,
    NODE_OUTPUT_BREAK,

    TIMELINE_JUMP,
    TIMELINE_ZOOM,
    TIMELINE_PAN_X,
    TIMELINE_PAN_Y
};

struct GUI_MSG_PARAMS {
    uint64_t a;
    uint64_t b;
    template<typename T>
    const T& getA() {
        return *(T*)(&a);
    }
    template<typename T>
    const T& getB() {
        return *(T*)(&b);
    }
    template<typename T>
    void setA(const T& param) {
        if (sizeof(T) > sizeof(a)) {
            assert(false);
            return;
        }
        memcpy(&a, &param, std::min(sizeof(a), sizeof(param)));
    }
    template<typename T>
    void setB(const T& param) {
        if (sizeof(T) > sizeof(b)) {
            assert(false);
            return;
        }
        memcpy(&b, &param, std::min(sizeof(b), sizeof(param)));
    }
};