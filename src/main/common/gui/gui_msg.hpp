#pragma once

enum class GUI_MSG {
    PAINT,
    MOUSE_MOVE,
    MOUSE_ENTER,
    MOUSE_LEAVE,
    LBUTTON_DOWN,
    LBUTTON_UP,

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
    DRAG_DROP_TARGET_HOVERED
};
