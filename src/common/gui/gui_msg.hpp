#pragma once

enum class GUI_MSG {
    UNKNOWN,

    CLOSE,
    CLOSE_MENU,

    MOUSE_SCROLL,

    CHILD_ADDED,
    CHILD_REMOVED,

    KEYDOWN,
    KEYUP,

    ACTIVATE,
    DEACTIVATE,

    MOVE_START,
    MOVING,
    RESIZING,
    TITLE_CHANGED,

    NOTIFY,

    NUMERIC_UPDATE,

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

    case GUI_MSG::MOUSE_SCROLL: return "MOUSE_SCROLL";

    case GUI_MSG::CHILD_ADDED: return "CHILD_ADDED";
    case GUI_MSG::CHILD_REMOVED: return "CHILD_REMOVED";

    case GUI_MSG::KEYDOWN: return "KEYDOWN";
    case GUI_MSG::KEYUP: return "KEYUP";

    case GUI_MSG::ACTIVATE: return "ACTIVATE";
    case GUI_MSG::DEACTIVATE: return "DEACTIVATE";

    case GUI_MSG::MOVE_START: return "MOVE_START";
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

    MENU_COMMAND,
    MENU_ITEM_HOVER,

    SCROLL_V,
    SCROLL_H,

    TREE_ITEM_CLICK,
    TREE_VIEW_SELECTED,

    TAB_MOUSE_ENTER,
    TAB_SWAP,
    TAB_DRAGGED_OUT,
    TAB_CLOSED,

    UNDOCKED,
    WINDOW_FRAME_ORPHANED,

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

    VIEWPORT_MOUSE_MOVE,
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