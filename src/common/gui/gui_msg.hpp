#pragma once

enum class GUI_MSG {
    UNKNOWN,

    CLOSE,
    CLOSE_MENU,

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

    CHILD_ADDED,
    CHILD_REMOVED,

    KEYDOWN,
    KEYUP,

    UNICHAR,

    ACTIVATE,
    DEACTIVATE,

    FOCUS,
    UNFOCUS,
    UNFOCUS_MENU,

    LCLICK,
    RCLICK,
    MCLICK,
    DBL_LCLICK,
    DBL_RCLICK,
    DBL_MCLICK,
    PULL_START, // user has pressed down the left mouse button and moved the mouse
    PULL,       // for any mouse move while pulling is in action
    PULL_STOP,  // 

    // SCROLL BAR
    SB_THUMB_TRACK,

    TEXT_HIGHTLIGHT_UPDATE,

    MOVING,
    RESIZING,
    TITLE_CHANGED,

    NOTIFY,

    NUMERIC_UPDATE,

    COLLAPSING_HEADER_REMOVE,

    TAB_CLOSE,
    TAB_PIN,

    DRAG_START,
    DRAG_DROP,
    DRAG_STOP,

    DOCK_TAB_DRAG_START,
    DOCK_TAB_DRAG_STOP,
    DOCK_TAB_DRAG_ENTER,
    DOCK_TAB_DRAG_LEAVE,
    DOCK_TAB_DRAG_HOVER,
    DOCK_TAB_DRAG_DROP_PAYLOAD,
    DOCK_TAB_DRAG_DROP_PAYLOAD_SPLIT,
    DOCK_TAB_DRAG_SUCCESS,  // Received by the sender when drag-drop operation finished successfully
    DOCK_TAB_DRAG_FAIL,      // Received by the sender when drag-drop operation failed or was cancelled
    DOCK_TAB_DRAG_RESET_VIEW,

    FILE_EXPL_OPEN_FILE,
    
    LIST_ADD,
    LIST_REMOVE,
    LIST_ADD_GROUP,
    LIST_REMOVE_GROUP
};

// Look for: (\S*),
// Replace with: case GUI_MSG::$1: return "$1";
inline const char* guiMsgToString(GUI_MSG msg) {
    switch (msg) {
    case GUI_MSG::UNKNOWN: return "UNKNOWN";

    case GUI_MSG::CLOSE: return "CLOSE";
    case GUI_MSG::CLOSE_MENU: return "CLOSE_MENU";

    case GUI_MSG::PAINT: return "PAINT";
    case GUI_MSG::MOUSE_MOVE: return "MOUSE_MOVE";
    case GUI_MSG::MOUSE_ENTER: return "MOUSE_ENTER";
    case GUI_MSG::MOUSE_LEAVE: return "MOUSE_LEAVE";
    case GUI_MSG::LBUTTON_DOWN: return "LBUTTON_DOWN";
    case GUI_MSG::LBUTTON_UP: return "LBUTTON_UP";
    case GUI_MSG::RBUTTON_DOWN: return "RBUTTON_DOWN";
    case GUI_MSG::RBUTTON_UP: return "RBUTTON_UP";
    case GUI_MSG::MBUTTON_DOWN: return "MBUTTON_DOWN";
    case GUI_MSG::MBUTTON_UP: return "MBUTTON_UP";
    case GUI_MSG::MOUSE_SCROLL: return "MOUSE_SCROLL";

    case GUI_MSG::CHILD_ADDED: return "CHILD_ADDED";
    case GUI_MSG::CHILD_REMOVED: return "CHILD_REMOVED";

    case GUI_MSG::KEYDOWN: return "KEYDOWN";
    case GUI_MSG::KEYUP: return "KEYUP";

    case GUI_MSG::UNICHAR: return "UNICHAR";

    case GUI_MSG::ACTIVATE: return "ACTIVATE";
    case GUI_MSG::DEACTIVATE: return "DEACTIVATE";

    case GUI_MSG::FOCUS: return "FOCUS";
    case GUI_MSG::UNFOCUS: return "UNFOCUS";
    case GUI_MSG::UNFOCUS_MENU: return "UNFOCUS_MENU";

    case GUI_MSG::LCLICK: return "LCLICK";
    case GUI_MSG::RCLICK: return "RCLICK";
    case GUI_MSG::MCLICK: return "MCLICK";
    case GUI_MSG::DBL_LCLICK: return "DBL_LCLICK";
    case GUI_MSG::DBL_RCLICK: return "DBL_RCLICK";
    case GUI_MSG::DBL_MCLICK: return "DBL_MCLICK";
    case GUI_MSG::PULL_START: return "PULL_START";
    case GUI_MSG::PULL: return "PULL";
    case GUI_MSG::PULL_STOP: return "PULL_STOP";

    case GUI_MSG::SB_THUMB_TRACK: return "SB_THUMB_TRACK";

    case GUI_MSG::TEXT_HIGHTLIGHT_UPDATE: return "TEXT_HIGHTLIGHT_UPDATE";

    case GUI_MSG::MOVING: return "MOVING";
    case GUI_MSG::RESIZING: return "RESIZING";
    case GUI_MSG::TITLE_CHANGED: return "TITLE_CHANGED";

    case GUI_MSG::NOTIFY: return "NOTIFY";

    case GUI_MSG::NUMERIC_UPDATE: return "NUMERIC_UPDATE";

    case GUI_MSG::TAB_CLOSE: return "TAB_CLOSE";
    case GUI_MSG::TAB_PIN: return "TAB_PIN";

    case GUI_MSG::DRAG_START: return "DRAG_START";
    case GUI_MSG::DRAG_DROP: return "DRAG_DROP";
    case GUI_MSG::DRAG_STOP: return "DRAG_STOP";

    case GUI_MSG::DOCK_TAB_DRAG_START: return "DOCK_TAB_DRAG_START";
    case GUI_MSG::DOCK_TAB_DRAG_STOP: return "DOCK_TAB_DRAG_STOP";
    case GUI_MSG::DOCK_TAB_DRAG_ENTER: return "DOCK_TAB_DRAG_ENTER";
    case GUI_MSG::DOCK_TAB_DRAG_LEAVE: return "DOCK_TAB_DRAG_LEAVE";
    case GUI_MSG::DOCK_TAB_DRAG_HOVER: return "DOCK_TAB_DRAG_HOVER";
    case GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD: return "DOCK_TAB_DRAG_DROP_PAYLOAD";
    case GUI_MSG::DOCK_TAB_DRAG_DROP_PAYLOAD_SPLIT: return "DOCK_TAB_DRAG_DROP_PAYLOAD_SPLIT";
    case GUI_MSG::DOCK_TAB_DRAG_SUCCESS: return "DOCK_TAB_DRAG_SUCCESS";
    case GUI_MSG::DOCK_TAB_DRAG_FAIL: return "DOCK_TAB_DRAG_FAIL";
    case GUI_MSG::DOCK_TAB_DRAG_RESET_VIEW: return "DOCK_TAB_DRAG_RESET_VIEW";
    case GUI_MSG::FILE_EXPL_OPEN_FILE: return "FILE_EXPL_OPEN_FILE";
    case GUI_MSG::LIST_ADD: return "LIST_ADD";
    case GUI_MSG::LIST_REMOVE: return "LIST_REMOVE";
    case GUI_MSG::LIST_ADD_GROUP: return "LIST_ADD_GROUP";
    case GUI_MSG::LIST_REMOVE_GROUP: return "LIST_REMOVE_GROUP";
    default:
        return "<UNKNOWN_GUI_MESSAGE>";
    }
}

enum class GUI_NOTIFY {
    NONE,

    BUTTON_CLICKED,

    MENU_COMMAND,
    MENU_ITEM_CLICKED,
    MENU_ITEM_HOVER,

    SCROLL_V,
    SCROLL_H,

    TREE_ITEM_CLICK,
    TREE_VIEW_SELECTED,

    COLLAPSING_HEADER_TOGGLE,

    FILE_ITEM_CLICK,
    FILE_ITEM_DOUBLE_CLICK,

    TAB_MOUSE_ENTER,
    TAB_CLICKED,
    TAB_SWAP,
    TAB_DRAGGED_OUT,
    TAB_CLOSED,

    LIST_ITEM_SELECTED,

    DRAG_TAB_START,
    DRAG_TAB_END,
    DRAG_DROP_TARGET_HOVERED,

    NODE_CLICKED,
    NODE_INPUT_CLICKED,
    NODE_OUTPUT_CLICKED,
    NODE_INPUT_BREAK,
    NODE_OUTPUT_BREAK,

    STATE_NODE_CLICKED,
    STATE_NODE_DOUBLE_CLICKED,

    TIMELINE_JUMP,
    TIMELINE_ZOOM,
    TIMELINE_PAN_X,
    TIMELINE_PAN_Y,
    TIMELINE_DRAG_BLOCK,
    TIMELINE_DRAG_BLOCK_CROSS_TRACK,
    TIMELINE_DRAG_EVENT,
    TIMELINE_DRAG_EVENT_CROSS_TRACK,
    TIMELINE_ERASE_BLOCK,
    TIMELINE_ERASE_EVENT,
    TIMELINE_RESIZE_BLOCK_LEFT,
    TIMELINE_RESIZE_BLOCK_RIGHT,

    TIMELINE_KEYFRAME_TRACK_ADDED,
    TIMELINE_KEYFRAME_ADDED,
    TIMELINE_KEYFRAME_REMOVED,
    TIMELINE_KEYFRAME_MOVED,

    TIMELINE_EVENT_TRACK_ADDED,
    TIMELINE_EVENT_ADDED,
    TIMELINE_EVENT_REMOVED,
    TIMELINE_EVENT_MOVED,

    TIMELINE_BLOCK_TRACK_ADDED,
    TIMELINE_BLOCK_ADDED,
    TIMELINE_BLOCK_REMOVED,
    TIMELINE_BLOCK_MOVED_RESIZED,

    TIMELINE_KEYFRAME_SELECTED,
    TIMELINE_EVENT_SELECTED,
    TIMELINE_BLOCK_SELECTED,

    VIEWPORT_MOUSE_MOVE,
    VIEWPORT_LCLICK,
    VIEWPORT_RCLICK,
    VIEWPORT_TOOL_DONE,
    VIEWPORT_DRAG_DROP_HOVER,
    VIEWPORT_DRAG_DROP,

    TRANSFORM_UPDATE,
    TRANSLATION_UPDATE,
    ROTATION_UPDATE,
    TRANSFORM_UPDATE_STOPPED,

    CSG_SHAPE_SELECTED,
    CSG_SHAPE_DESELECTED,
    CSG_SHAPE_CREATED,
    CSG_SHAPE_DELETE,
    CSG_SHAPE_CHANGED,
    CSG_REBUILD
};

struct GUI_MSG_PARAMS {
    uint64_t a;
    uint64_t b;
    uint64_t c;
    template<typename T>
    const T& getA() {
        return *(T*)(&a);
    }
    template<typename T>
    const T& getB() {
        return *(T*)(&b);
    }
    template<typename T>
    const T& getC() {
        return *(T*)(&c);
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
    template<typename T>
    void setC(const T& param) {
        if (sizeof(T) > sizeof(c)) {
            assert(false);
            return;
        }
        memcpy(&c, &param, std::min(sizeof(c), sizeof(param)));
    }
};