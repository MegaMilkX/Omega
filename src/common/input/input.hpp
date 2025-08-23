#ifndef INPUT2_HPP
#define INPUT2_HPP

#include <assert.h>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include <functional>
#include <memory>
#include "math/gfxm.hpp"


static const int   INPUT_CMD_BUFFER_LENGTH = 256;
static const int   INPUT_ACTION_EVENT_BUFFER_LENGTH = 256;
static const int   INPUT_FILTERED_ACTION_EVENT_BUFFER_LENGTH = 256;
static const float INPUT_TAP_THRESHOLD_SEC = 0.2f;


typedef uint32_t InputKey;
#define INPUT_MK_KEY(DEVICE, KEY) ( (((uint32_t)DEVICE) << 16) | ((uint32_t)KEY) )

enum class InputDeviceType {
	Null,
	Mouse,
	Keyboard,
	GamepadXbox,
	Dualshock4
};

static const struct {
    struct {
        InputKey Btn0 = INPUT_MK_KEY(InputDeviceType::Mouse, 0);
        InputKey Btn1 = INPUT_MK_KEY(InputDeviceType::Mouse, 1);
        InputKey Btn3 = INPUT_MK_KEY(InputDeviceType::Mouse, 2);
        InputKey Btn4 = INPUT_MK_KEY(InputDeviceType::Mouse, 3);
        InputKey Btn5 = INPUT_MK_KEY(InputDeviceType::Mouse, 4);
        InputKey AxisX = INPUT_MK_KEY(InputDeviceType::Mouse, 5);
        InputKey AxisY = INPUT_MK_KEY(InputDeviceType::Mouse, 6);
        InputKey Scroll = INPUT_MK_KEY(InputDeviceType::Mouse, 7);
        InputKey ScrollUp = INPUT_MK_KEY(InputDeviceType::Mouse, 8);
        InputKey ScrollDown = INPUT_MK_KEY(InputDeviceType::Mouse, 9);
        InputKey BtnLeft = INPUT_MK_KEY(InputDeviceType::Mouse, Btn0);
        InputKey BtnRight = INPUT_MK_KEY(InputDeviceType::Mouse, Btn1);
    } Mouse;
    struct {
        InputKey Unknown = -1;
        InputKey Space = INPUT_MK_KEY(InputDeviceType::Keyboard, 32);
        InputKey Left = INPUT_MK_KEY(InputDeviceType::Keyboard, 37);
        InputKey Up = INPUT_MK_KEY(InputDeviceType::Keyboard, 38);
        InputKey Right = INPUT_MK_KEY(InputDeviceType::Keyboard, 39);
        InputKey Down = INPUT_MK_KEY(InputDeviceType::Keyboard, 40);
        //InputKey Apostrophe = 39 /* ' */;
        InputKey Comma = INPUT_MK_KEY(InputDeviceType::Keyboard, 44); /* ; */
        InputKey Minus = INPUT_MK_KEY(InputDeviceType::Keyboard, 45); /* - */
        InputKey Period = INPUT_MK_KEY(InputDeviceType::Keyboard, 46); /* . */
        InputKey Slash = INPUT_MK_KEY(InputDeviceType::Keyboard, 47); /* / */
        InputKey _0 = INPUT_MK_KEY(InputDeviceType::Keyboard, 48);
        InputKey _1 = INPUT_MK_KEY(InputDeviceType::Keyboard, 49);
        InputKey _2 = INPUT_MK_KEY(InputDeviceType::Keyboard, 50);
        InputKey _3 = INPUT_MK_KEY(InputDeviceType::Keyboard, 51);
        InputKey _4 = INPUT_MK_KEY(InputDeviceType::Keyboard, 52);
        InputKey _5 = INPUT_MK_KEY(InputDeviceType::Keyboard, 53);
        InputKey _6 = INPUT_MK_KEY(InputDeviceType::Keyboard, 54);
        InputKey _7 = INPUT_MK_KEY(InputDeviceType::Keyboard, 55);
        InputKey _8 = INPUT_MK_KEY(InputDeviceType::Keyboard, 56);
        InputKey _9 = INPUT_MK_KEY(InputDeviceType::Keyboard, 57);
        InputKey Semicolon = INPUT_MK_KEY(InputDeviceType::Keyboard, 59); /* ; */
        InputKey Equal = INPUT_MK_KEY(InputDeviceType::Keyboard, 61); /* =  */
        InputKey A = INPUT_MK_KEY(InputDeviceType::Keyboard, 65);
        InputKey B = INPUT_MK_KEY(InputDeviceType::Keyboard, 66);
        InputKey C = INPUT_MK_KEY(InputDeviceType::Keyboard, 67);
        InputKey D = INPUT_MK_KEY(InputDeviceType::Keyboard, 68);
        InputKey E = INPUT_MK_KEY(InputDeviceType::Keyboard, 69);
        InputKey F = INPUT_MK_KEY(InputDeviceType::Keyboard, 70);
        InputKey G = INPUT_MK_KEY(InputDeviceType::Keyboard, 71);
        InputKey H = INPUT_MK_KEY(InputDeviceType::Keyboard, 72);
        InputKey I = INPUT_MK_KEY(InputDeviceType::Keyboard, 73);
        InputKey J = INPUT_MK_KEY(InputDeviceType::Keyboard, 74);
        InputKey K = INPUT_MK_KEY(InputDeviceType::Keyboard, 75);
        InputKey L = INPUT_MK_KEY(InputDeviceType::Keyboard, 76);
        InputKey M = INPUT_MK_KEY(InputDeviceType::Keyboard, 77);
        InputKey N = INPUT_MK_KEY(InputDeviceType::Keyboard, 78);
        InputKey O = INPUT_MK_KEY(InputDeviceType::Keyboard, 79);
        InputKey P = INPUT_MK_KEY(InputDeviceType::Keyboard, 80);
        InputKey Q = INPUT_MK_KEY(InputDeviceType::Keyboard, 81);
        InputKey R = INPUT_MK_KEY(InputDeviceType::Keyboard, 82);
        InputKey S = INPUT_MK_KEY(InputDeviceType::Keyboard, 83);
        InputKey T = INPUT_MK_KEY(InputDeviceType::Keyboard, 84);
        InputKey U = INPUT_MK_KEY(InputDeviceType::Keyboard, 85);
        InputKey V = INPUT_MK_KEY(InputDeviceType::Keyboard, 86);
        InputKey W = INPUT_MK_KEY(InputDeviceType::Keyboard, 87);
        InputKey X = INPUT_MK_KEY(InputDeviceType::Keyboard, 88);
        InputKey Y = INPUT_MK_KEY(InputDeviceType::Keyboard, 89);
        InputKey Z = INPUT_MK_KEY(InputDeviceType::Keyboard, 90);
        InputKey LeftBracket = INPUT_MK_KEY(InputDeviceType::Keyboard, 91); /* [ */
        InputKey Backslash = INPUT_MK_KEY(InputDeviceType::Keyboard, 92); /* \ */
        InputKey RightBracket = INPUT_MK_KEY(InputDeviceType::Keyboard, 93); /* ] */
        InputKey GraveAccent = INPUT_MK_KEY(InputDeviceType::Keyboard, 96); /* ` */
        InputKey World1 = INPUT_MK_KEY(InputDeviceType::Keyboard, 161); /* Non-Us #1 */
        InputKey World2 = INPUT_MK_KEY(InputDeviceType::Keyboard, 162); /* Non-Us #2 */
        InputKey Escape = INPUT_MK_KEY(InputDeviceType::Keyboard, 256);
        InputKey Enter = INPUT_MK_KEY(InputDeviceType::Keyboard, 257);
        InputKey Tab = INPUT_MK_KEY(InputDeviceType::Keyboard, 258);
        InputKey Backspace = INPUT_MK_KEY(InputDeviceType::Keyboard, 259);
        InputKey Insert = INPUT_MK_KEY(InputDeviceType::Keyboard, 260);
        InputKey Delete = INPUT_MK_KEY(InputDeviceType::Keyboard, 261);
        InputKey PageUp = INPUT_MK_KEY(InputDeviceType::Keyboard, 266);
        InputKey PageDown = INPUT_MK_KEY(InputDeviceType::Keyboard, 267);
        InputKey Home = INPUT_MK_KEY(InputDeviceType::Keyboard, 268);
        InputKey End = INPUT_MK_KEY(InputDeviceType::Keyboard, 269);
        InputKey CapsLock = INPUT_MK_KEY(InputDeviceType::Keyboard, 280);
        InputKey ScrollLock = INPUT_MK_KEY(InputDeviceType::Keyboard, 281);
        InputKey NumLock = INPUT_MK_KEY(InputDeviceType::Keyboard, 282);
        InputKey PrintScreen = INPUT_MK_KEY(InputDeviceType::Keyboard, 283);
        InputKey Pause = INPUT_MK_KEY(InputDeviceType::Keyboard, 284);
        InputKey F1 = INPUT_MK_KEY(InputDeviceType::Keyboard, 112);
        InputKey F2 = INPUT_MK_KEY(InputDeviceType::Keyboard, 113);
        InputKey F3 = INPUT_MK_KEY(InputDeviceType::Keyboard, 114);
        InputKey F4 = INPUT_MK_KEY(InputDeviceType::Keyboard, 115);
        InputKey F5 = INPUT_MK_KEY(InputDeviceType::Keyboard, 116);
        InputKey F6 = INPUT_MK_KEY(InputDeviceType::Keyboard, 117);
        InputKey F7 = INPUT_MK_KEY(InputDeviceType::Keyboard, 118);
        InputKey F8 = INPUT_MK_KEY(InputDeviceType::Keyboard, 119);
        InputKey F9 = INPUT_MK_KEY(InputDeviceType::Keyboard, 120);
        InputKey F10 = INPUT_MK_KEY(InputDeviceType::Keyboard, 121);
        InputKey F11 = INPUT_MK_KEY(InputDeviceType::Keyboard, 122);
        InputKey F12 = INPUT_MK_KEY(InputDeviceType::Keyboard, 123);
        InputKey F13 = INPUT_MK_KEY(InputDeviceType::Keyboard, 124);
        InputKey F14 = INPUT_MK_KEY(InputDeviceType::Keyboard, 125);
        InputKey F15 = INPUT_MK_KEY(InputDeviceType::Keyboard, 126);
        InputKey F16 = INPUT_MK_KEY(InputDeviceType::Keyboard, 127);
        InputKey F17 = INPUT_MK_KEY(InputDeviceType::Keyboard, 128);
        InputKey F18 = INPUT_MK_KEY(InputDeviceType::Keyboard, 129);
        InputKey F19 = INPUT_MK_KEY(InputDeviceType::Keyboard, 130);
        InputKey F20 = INPUT_MK_KEY(InputDeviceType::Keyboard, 131);
        InputKey F21 = INPUT_MK_KEY(InputDeviceType::Keyboard, 132);
        InputKey F22 = INPUT_MK_KEY(InputDeviceType::Keyboard, 133);
        InputKey F23 = INPUT_MK_KEY(InputDeviceType::Keyboard, 134);
        InputKey F24 = INPUT_MK_KEY(InputDeviceType::Keyboard, 135);
        InputKey F25 = INPUT_MK_KEY(InputDeviceType::Keyboard, 136);
        InputKey Num0 = INPUT_MK_KEY(InputDeviceType::Keyboard, 320);
        InputKey Num1 = INPUT_MK_KEY(InputDeviceType::Keyboard, 321);
        InputKey Num2 = INPUT_MK_KEY(InputDeviceType::Keyboard, 322);
        InputKey Num3 = INPUT_MK_KEY(InputDeviceType::Keyboard, 323);
        InputKey Num4 = INPUT_MK_KEY(InputDeviceType::Keyboard, 324);
        InputKey Num5 = INPUT_MK_KEY(InputDeviceType::Keyboard, 325);
        InputKey Num6 = INPUT_MK_KEY(InputDeviceType::Keyboard, 326);
        InputKey Num7 = INPUT_MK_KEY(InputDeviceType::Keyboard, 327);
        InputKey Num8 = INPUT_MK_KEY(InputDeviceType::Keyboard, 328);
        InputKey Num9 = INPUT_MK_KEY(InputDeviceType::Keyboard, 329);
        InputKey NumDecimal = INPUT_MK_KEY(InputDeviceType::Keyboard, 330);
        InputKey NumDivide = INPUT_MK_KEY(InputDeviceType::Keyboard, 331);
        InputKey NumMultiply = INPUT_MK_KEY(InputDeviceType::Keyboard, 332);
        InputKey NumSubtract = INPUT_MK_KEY(InputDeviceType::Keyboard, 333);
        InputKey NumAdd = INPUT_MK_KEY(InputDeviceType::Keyboard, 334);
        InputKey NumEnter = INPUT_MK_KEY(InputDeviceType::Keyboard, 335);
        InputKey NumEqual = INPUT_MK_KEY(InputDeviceType::Keyboard, 336);
        InputKey LeftShift = INPUT_MK_KEY(InputDeviceType::Keyboard, 340);
        InputKey LeftControl = INPUT_MK_KEY(InputDeviceType::Keyboard, 341);
        InputKey LeftAlt = INPUT_MK_KEY(InputDeviceType::Keyboard, 342);
        InputKey LeftSuper = INPUT_MK_KEY(InputDeviceType::Keyboard, 343);
        InputKey RightShift = INPUT_MK_KEY(InputDeviceType::Keyboard, 344);
        InputKey RightControl = INPUT_MK_KEY(InputDeviceType::Keyboard, 345);
        InputKey RightAlt = INPUT_MK_KEY(InputDeviceType::Keyboard, 346);
        InputKey RightSuper = INPUT_MK_KEY(InputDeviceType::Keyboard, 347);
        InputKey Menu = INPUT_MK_KEY(InputDeviceType::Keyboard, 348);
    } Keyboard;
    struct {
        InputKey DpadUp = INPUT_MK_KEY(InputDeviceType::GamepadXbox, 0);
        InputKey DpadDown = INPUT_MK_KEY(InputDeviceType::GamepadXbox, 1);
        InputKey DpadLeft = INPUT_MK_KEY(InputDeviceType::GamepadXbox, 2);
        InputKey DpadRight = INPUT_MK_KEY(InputDeviceType::GamepadXbox, 3);
        InputKey Start = INPUT_MK_KEY(InputDeviceType::GamepadXbox, 4);
        InputKey Back = INPUT_MK_KEY(InputDeviceType::GamepadXbox, 5);
        InputKey LeftThumb = INPUT_MK_KEY(InputDeviceType::GamepadXbox, 6);
        InputKey RightThumb = INPUT_MK_KEY(InputDeviceType::GamepadXbox, 7);
        InputKey LeftShoulder = INPUT_MK_KEY(InputDeviceType::GamepadXbox, 8);
        InputKey RightShoulder = INPUT_MK_KEY(InputDeviceType::GamepadXbox, 9);
        InputKey A = INPUT_MK_KEY(InputDeviceType::GamepadXbox, 10);
        InputKey B = INPUT_MK_KEY(InputDeviceType::GamepadXbox, 11);
        InputKey X = INPUT_MK_KEY(InputDeviceType::GamepadXbox, 12);
        InputKey Y = INPUT_MK_KEY(InputDeviceType::GamepadXbox, 13);
        InputKey StickLX = INPUT_MK_KEY(InputDeviceType::GamepadXbox, 14);
        InputKey StickLY = INPUT_MK_KEY(InputDeviceType::GamepadXbox, 15);
        InputKey StickRX = INPUT_MK_KEY(InputDeviceType::GamepadXbox, 16);
        InputKey StickRY = INPUT_MK_KEY(InputDeviceType::GamepadXbox, 17);
        InputKey TriggerL = INPUT_MK_KEY(InputDeviceType::GamepadXbox, 18);
        InputKey TriggerR = INPUT_MK_KEY(InputDeviceType::GamepadXbox, 19);
    } GamepadXbox;
} Key;

enum class KeyMouse {
    Btn0, Btn1, Btn3, Btn4, Btn5,
    AxisX, AxisY, Scroll,
    Left = Btn0, Right = Btn1
};
enum class KeyKeyboard {
    Unknown = -1,
    Space = 32,
    Apostrophe = 39 /* ' */,
    Comma = 44 /* , */,
    Minus = 45 /* - */,
    Period = 46 /* . */,
    Slash = 47 /* / */,
    _0 = 48,
    _1 = 49,
    _2 = 50,
    _3 = 51,
    _4 = 52,
    _5 = 53,
    _6 = 54,
    _7 = 55,
    _8 = 56,
    _9 = 57,
    Semicolon = 59 /* ; */,
    Equal = 61 /* =  */,
    A = 65,
    B = 66,
    C = 67,
    D = 68,
    E = 69,
    F = 70,
    G = 71,
    H = 72,
    I = 73,
    J = 74,
    K = 75,
    L = 76,
    M = 77,
    N = 78,
    O = 79,
    P = 80,
    Q = 81,
    R = 82,
    S = 83,
    T = 84,
    U = 85,
    V = 86,
    W = 87,
    X = 88,
    Y = 89,
    Z = 90,
    LeftBracket = 91 /* [ */,
    Backslash = 92 /* \ */,
    RightBracket = 93 /* ] */,
    GraveAccent = 96 /* ` */,
    World1 = 161 /* Non-Us #1 */,
    World2 = 162 /* Non-Us #2 */,
    Escape = 256,
    Enter = 257,
    Tab = 258,
    Backspace = 259,
    Insert = 260,
    Delete = 261,
    Right = 262,
    Left = 263,
    Down = 264,
    Up = 265,
    PageUp = 266,
    PageDown = 267,
    Home = 268,
    End = 269,
    CapsLock = 280,
    ScrollLock = 281,
    NumLock = 282,
    PrintScreen = 283,
    Pause = 284,
    F1 = 290,
    F2 = 291,
    F3 = 292,
    F4 = 293,
    F5 = 294,
    F6 = 295,
    F7 = 296,
    F8 = 297,
    F9 = 298,
    F10 = 299,
    F11 = 300,
    F12 = 301,
    F13 = 302,
    F14 = 303,
    F15 = 304,
    F16 = 305,
    F17 = 306,
    F18 = 307,
    F19 = 308,
    F20 = 309,
    F21 = 310,
    F22 = 311,
    F23 = 312,
    F24 = 313,
    F25 = 314,
    Num0 = 320,
    Num1 = 321,
    Num2 = 322,
    Num3 = 323,
    Num4 = 324,
    Num5 = 325,
    Num6 = 326,
    Num7 = 327,
    Num8 = 328,
    Num9 = 329,
    NumDecimal = 330,
    NumDivide = 331,
    NumMultiply = 332,
    NumSubtract = 333,
    NumAdd = 334,
    NumEnter = 335,
    NumEqual = 336,
    LeftShift = 340,
    LeftControl = 341,
    LeftAlt = 342,
    LeftSuper = 343,
    RightShift = 344,
    RightControl = 345,
    RightAlt = 346,
    RightSuper = 347,
    Menu = 348
};
enum class KeyGamepadXbox {
    DpadUp,
    DpadDown,
    DpadLeft,
    DpadRight,
    Start,
    Back,
    LeftThumb,
    RightThumb,
    LeftShoulder,
    RightShoulder,
    A,
    B,
    X,
    Y,
    StickLX, StickLY,
    StickRX, StickRY,
    TriggerL, TriggerR
};

enum class InputKeyType {
    Toggle, Increment, Absolute, NoKeyUp
};

struct InputCmd {
    uint8_t  user_id;     // 0 .. 255
	uint8_t  device_type; // InputDeviceType
	uint16_t key;
    float    value;
    InputKeyType value_type;
    uint64_t id;
};

enum class InputActionEventType {
    Press, Release, Tap, Hold
};

struct InputLinkDesc {
    InputKey key;
    float multiplier;
};
class InputActionDesc {
    std::vector<InputLinkDesc> links;
public:
    InputActionDesc& linkKey(InputKey key, float multiplier) {
        links.push_back(InputLinkDesc{ key, multiplier });
        return *this;
    }

    const InputLinkDesc* getLinks() { return links.data(); }
    int linkCount() const { return links.size(); }
};
class InputRangeDesc {
    std::vector<InputLinkDesc> links_x;
    std::vector<InputLinkDesc> links_y;
    std::vector<InputLinkDesc> links_z;
public:
    InputRangeDesc& linkKeyX(InputKey key, float multiplier) {
        links_x.push_back(InputLinkDesc{ key, multiplier });
        return *this;
    }
    InputRangeDesc& linkKeyY(InputKey key, float multiplier) {
        links_y.push_back(InputLinkDesc{ key, multiplier });
        return *this;
    }
    InputRangeDesc& linkKeyZ(InputKey key, float multiplier) {
        links_z.push_back(InputLinkDesc{ key, multiplier });
        return *this;
    }

    const InputLinkDesc* getLinksX() { return links_x.data(); }
    int linkXCount() const { return links_x.size(); }
    const InputLinkDesc* getLinksY() { return links_y.data(); }
    int linkYCount() const { return links_y.size(); }
    const InputLinkDesc* getLinksZ() { return links_z.data(); }
    int linkZCount() const { return links_z.size(); }
};

class InputState;
class InputAction;
struct InputLink {
    InputLink();
    ~InputLink();

    InputAction*    action;
    InputKey        key;
    InputKeyType    key_type;
    
    int             array_id = 0;
    int             priority = 0;
    float           value = .0f;
    float           prev_value = .0f;
    float           multiplier = 1.0f;
    bool            blocked = false;
};

class InputAction {
    friend InputState;

    InputState* owner_state = 0;

    std::string name;
    float       hold_time = .0f;
    float       prev_hold_time = .0f;
    bool        is_pressed = false;
    bool        is_just_pressed = false;
    bool        is_just_released = false;
    std::vector<std::unique_ptr<InputLink>>         links;

    std::vector<std::function<void(void)>> on_press_callbacks;
    std::vector<std::function<void(void)>> on_release_callbacks;
    std::vector<std::function<void(void)>> on_tap_callbacks;
    std::vector<std::pair<float, std::function<void(void)>>> on_hold_callbacks;
public:
    InputAction(const char* name = "UnnamedAction");
    ~InputAction();
    InputAction& linkKey(InputKey key, float multiplier = 1.0f);

    InputAction& bindPress(std::function<void(void)> cb);
    InputAction& bindRelease(std::function<void(void)> cb);
    InputAction& bindTap(std::function<void(void)> cb);
    InputAction& bindHold(std::function<void(void)> cb, float threshold);

    bool isPressed() const { return is_pressed; }
    bool isJustPressed() const { return is_just_pressed; }
    bool isJustReleased() const { return is_just_released; }
    const char* getName() const { return name.c_str(); }

    void registerLinks(InputState* state);
    void unregisterLinks(InputState* state);
};

enum class InputRangeType {
    Absolute,
    Relative
};

class InputRange {
    friend InputState;

    InputRangeType type = InputRangeType::Relative;
    std::string name;
    gfxm::vec3  value;
    std::vector<std::unique_ptr<InputLink>> key_links_x;
    std::vector<std::unique_ptr<InputLink>> key_links_y;
    std::vector<std::unique_ptr<InputLink>> key_links_z;
public:
    InputRange(const char* name = "UnnamedRange");
    ~InputRange();
    InputRange& linkKeyX(InputKey key, float multiplier);
    InputRange& linkKeyY(InputKey key, float multiplier);
    InputRange& linkKeyZ(InputKey key, float multiplier);
    
    const char* getName() const { return name.c_str(); }
    float getValue() const { return value.x; }
    gfxm::vec2 getVec2() const { return gfxm::vec2(value.x, value.y); }
    gfxm::vec3 getVec3() const { return value; }

    void registerLinks(InputState* state);
    void unregisterLinks(InputState* state);
};

class InputContext {
    friend void inputUpdate(float dt);
    friend InputState;

    InputState* owner_state = 0;
    std::string name;
    bool        is_enabled;
    int         stack_pos;
    std::set<std::unique_ptr<InputAction>>  actions;
    std::set<std::unique_ptr<InputRange>>   ranges;
public:
    InputContext(const char* name = "UnnamedContext");
    ~InputContext();

    void enable();
    void disable();

    void toFront();

    bool isEnabled() const;

    InputAction* createAction(const char* name);
    InputRange*  createRange(const char* name);

    const char* getName() const { return name.c_str(); }

    void registerLinks(InputState* state);
    void unregisterLinks(InputState* state);
};

struct InputActionEvent {
    char            name[64]; // For debug
    uint64_t        id;
    InputAction*    action;
    uint8_t         type;
    InputCmd        propagating_cmd;
};


class InputState {
    uint8_t user_id;

    InputCmd           input_buffer[INPUT_CMD_BUFFER_LENGTH];
    int                insert_cursor = 0;
    InputActionEvent   input_action_event_buffer[INPUT_ACTION_EVENT_BUFFER_LENGTH];
    int                action_event_insert_cursor = 0;

    uint64_t last_cmd_id = 0;
    uint64_t last_event_id = 0;

    bool                       is_context_stack_dirty = true;
    std::vector<InputContext*> context_stack;
    
    std::set<InputAction*> actions;
    std::set<InputRange*> ranges;
    std::vector<InputLink*> links;

    //std::unordered_map<std::string, InputContext*> context_map;
    std::unordered_map<std::string, InputAction*> action_map;
    std::unordered_map<std::string, InputRange*> range_map;

    int next_cmd_id = 0;
    int next_event_id = 0;
public:
    InputState(uint8_t user_id)
        : user_id(user_id) {
        memset(input_buffer, 0, sizeof(input_buffer));
        memset(input_action_event_buffer, 0, sizeof(input_action_event_buffer));
    }

    void pushContext(InputContext* ctx) {
        assert(ctx->owner_state != this);
        if (ctx->owner_state) {
            ctx->unregisterLinks(ctx->owner_state);
        }
        ctx->owner_state = this;
        ctx->stack_pos = context_stack.size();
        context_stack.push_back(ctx);
        ctx->registerLinks(this);
        setDirty();
    }
    void removeContext(InputContext* ctx) {
        assert(ctx->owner_state == this);
        ctx->unregisterLinks(this);
        for (int i = ctx->stack_pos + 1; i < context_stack.size(); ++i) {
            context_stack[i - 1] = context_stack[i];
            context_stack[i]->stack_pos = i - 1;
        }
        context_stack.resize(context_stack.size() - 1);
        setDirty();
        ctx->owner_state = 0;
    }

    void contextToFront(InputContext* ctx) {
        assert(ctx->owner_state == this);
        for (int i = ctx->stack_pos + 1; i < context_stack.size(); ++i) {
            context_stack[i - 1] = context_stack[i];
            context_stack[i]->stack_pos = i - 1;
        }
        ctx->stack_pos = context_stack.size() - 1;
        context_stack[ctx->stack_pos] = ctx;
        setDirty();
    }

    void registerAction(InputAction* action) {
        actions.insert(action);
        action->registerLinks(this);
    }
    void unregisterAction(InputAction* action) {
        action->unregisterLinks(this);
        actions.erase(action);
    }
    void registerRange(InputRange* range) {
        ranges.insert(range);
        range->registerLinks(this);
    }
    void unregisterRange(InputRange* range) {
        range->unregisterLinks(this);
        ranges.erase(range);
    }
    void registerLink(InputLink* link) {
        link->array_id = links.size();
        links.push_back(link);
        setDirty();
    }
    void unregisterLink(InputLink* link) {
        int last_id = links.size() - 1;
        if (last_id != link->array_id) {
            links[link->array_id] = links[last_id];
            links[link->array_id]->array_id = link->array_id;
        }
        links.resize(links.size() - 1);
        setDirty();
    }

    void setDirty() { is_context_stack_dirty = true; }

    uint8_t getUserId() const { return user_id; }

    void getCmdBufferSnapshot(InputCmd* dest, int count) {
        int c = gfxm::_min(count, INPUT_CMD_BUFFER_LENGTH);
        int copy_cursor = insert_cursor;
        for (int i = 0; i < c; ++i) {
            dest[i] = input_buffer[(copy_cursor + i) % INPUT_CMD_BUFFER_LENGTH];
        }
    }
    void getActionEventBufferSnapshot(InputActionEvent* dest, int count) {
        int c = gfxm::_min(count, INPUT_ACTION_EVENT_BUFFER_LENGTH);
        int copy_cursor = action_event_insert_cursor;
        for (int i = 0; i < c; ++i) {
            dest[i] = input_action_event_buffer[(copy_cursor + i) % INPUT_ACTION_EVENT_BUFFER_LENGTH];
        }
    }

    void postInput(InputDeviceType dev_type, uint16_t key, float value, InputKeyType value_type) {
        InputCmd cmd;
        cmd.device_type = (uint8_t)dev_type;
        cmd.user_id = user_id;
        cmd.key = key;
        cmd.value = value;
        cmd.value_type = value_type;
        cmd.id = ++next_cmd_id;

        input_buffer[insert_cursor] = cmd;
        insert_cursor = (++insert_cursor) % INPUT_CMD_BUFFER_LENGTH;
    }
    void postActionEvent(InputAction* action, InputActionEventType type) {
        InputCmd fake_cmd;
        fake_cmd.device_type = 0;
        fake_cmd.id = 0;
        fake_cmd.key = 0;
        fake_cmd.user_id = 0;
        fake_cmd.value = 0;
        postActionEvent(action, type, fake_cmd);
    }
    void postActionEvent(InputAction* action, InputActionEventType type, const InputCmd& propagating_cmd) {
        static int next_event_id = 1;
        InputActionEvent e = { 0 };
        e.action = action;
        e.id = next_event_id++;
        e.type = (uint8_t)type;
        strncpy(e.name, action->getName(), sizeof(e.name));
        e.propagating_cmd = propagating_cmd;

        input_action_event_buffer[action_event_insert_cursor] = e;
        action_event_insert_cursor = (++action_event_insert_cursor) % INPUT_ACTION_EVENT_BUFFER_LENGTH;
    }

    void update(float dt) {
        if (is_context_stack_dirty) {
            for (int i = context_stack.size() - 1; i >= 0; --i) {
                auto ctx = context_stack[i];
                int priority = ctx->isEnabled() ? (i * 3) : -1;
                
                for (auto& range : ctx->ranges) {
                    for (auto& link : range->key_links_x) {
                        link->priority = priority;
                    }
                    for (auto& link : range->key_links_y) {
                        link->priority = priority;
                    }
                    for (auto& link : range->key_links_z) {
                        link->priority = priority;
                    }
                }
                for (auto& action : ctx->actions) {
                    for (auto& link : action->links) {
                        link->priority = priority;
                    }
                }
            }

            std::sort(links.begin(), links.end(), [](const InputLink* a, const InputLink* b)->bool{
                if(a->key == b->key) {
                    return a->priority > b->priority;
                } else {
                    return a->key < b->key;
                }
            });
            for(int i = 0; i < links.size(); ++i) {
                links[i]->array_id = i;
            }
            for(int i = 0; i < int(links.size()) - 1; ++i) {     
                auto link = links[i];
                link->blocked = false;
                int next_i = i + 1;
                while(next_i < links.size()) {
                    auto next_link = links[next_i];
                    if(next_link->key != link->key) {
                        i = next_i - 1;
                        break;
                    }
                    next_link->blocked = true;
                    ++next_i;
                }
            }

            is_context_stack_dirty = false;
        }
        
        // Process raw button commands
        // ----------------------------
        InputCmd buf[INPUT_CMD_BUFFER_LENGTH];
        getCmdBufferSnapshot(buf, INPUT_CMD_BUFFER_LENGTH);
        int max_cmd_id = last_cmd_id;
        for(auto& cmd : buf) {
            if(cmd.id <= last_cmd_id) {
                continue;
            }
            max_cmd_id = gfxm::_max((uint64_t)max_cmd_id, cmd.id);

            for(int i = 0; i < links.size(); ++i) {
                auto link = links[i];
                auto action = link->action;
                InputKey cmdkey = INPUT_MK_KEY(cmd.device_type, cmd.key);
                if(link->priority == -1) { // Reached disabled links
                    continue;
                }
                if(link->key != cmdkey) {
                    continue;
                }
                link->key_type = cmd.value_type;
                link->prev_value = link->value;
                if (cmd.value_type == InputKeyType::Increment) {
                    link->value += cmd.value;
                } else {
                    link->value = cmd.value;
                }

                if(i < links.size() - 1 && links[i + 1]->priority == link->priority) {
                    // If next link is of same priority - continue. Links with same keys in the same context(priority) are allowed to trigger at the same time
                    // otherwise it would be undefined which link gets the commad and which doesnt
                    continue;
                } else {
                    break;
                }
            }
        }
        last_cmd_id = max_cmd_id;

        // Update action states and post appropriate events (hold cb is called immediately)
        // ----------------------------
        for (auto& action : actions) {
            action->is_just_pressed = false;
            action->is_just_released = false;
            bool is_any_link_pressed = false;
            for(auto& link : action->links) {
                if((link->value * link->multiplier <= .0f) || link->blocked) {
                    continue;
                } 
                is_any_link_pressed = true;
            }
            if(!action->is_pressed && is_any_link_pressed) {
                action->is_pressed = true;
                action->is_just_pressed = true;
                postActionEvent(action, InputActionEventType::Press);
            } else if (action->is_pressed && !is_any_link_pressed) {
                if (action->hold_time <= INPUT_TAP_THRESHOLD_SEC) {
                    postActionEvent(action, InputActionEventType::Tap);
                }
                action->hold_time = .0f;
                action->is_pressed = false;
                action->is_just_released = true;
                postActionEvent(action, InputActionEventType::Release);
            } else if (action->is_pressed) {
                for(auto& cb : action->on_hold_callbacks) {
                    if(action->hold_time >= cb.first && action->prev_hold_time < cb.first) {
                        cb.second();
                    }
                }
            }
            action->is_pressed = is_any_link_pressed;
        }

        // Apply values to ranges from their links
        // ----------------------------
        for(auto& r : ranges) {
            r->value = gfxm::vec3(.0f, .0f, .0f);
            for(auto& link : r->key_links_x) {
                if (link->blocked) {
                    continue;
                }
                float& val = r->value.x;
                if(link->key_type == InputKeyType::Toggle) {
                    val += link->value * link->multiplier;
                } else if(link->key_type == InputKeyType::Absolute) {
                    val += (link->prev_value - link->value) * link->multiplier;
                    link->prev_value = link->value;
                } else if(link->key_type == InputKeyType::Increment) {
                    val += link->value * link->multiplier;
                    link->value = .0f;
                }
            }
            for(auto& link : r->key_links_y) {
                if (link->blocked) {
                    continue;
                }
                float& val = r->value.y;
                if(link->key_type == InputKeyType::Toggle) {
                    val += link->value * link->multiplier;
                } else if(link->key_type == InputKeyType::Absolute) {
                    val += (link->prev_value - link->value) * link->multiplier;
                    link->prev_value = link->value;
                } else if(link->key_type == InputKeyType::Increment) {
                    val += link->value * link->multiplier;
                    link->value = .0f;
                }
            }
            for(auto& link : r->key_links_z) {
                if (link->blocked) {
                    continue;
                }
                float& val = r->value.z;
                if(link->key_type == InputKeyType::Toggle) {
                    val += link->value * link->multiplier;
                } else if(link->key_type == InputKeyType::Absolute) {
                    val += (link->prev_value - link->value) * link->multiplier;
                    link->prev_value = link->value;
                } else if(link->key_type == InputKeyType::Increment) {
                    val += link->value * link->multiplier;
                    link->value = .0f;
                }
            }
        }

        // Increment hold timers for all action-key links
        // ----------------------------
        for(auto& a : actions) {
            if (a->is_pressed) {
                a->prev_hold_time = a->hold_time;
                a->hold_time += dt;
            }
        }

        // Invoke callbacks
        // ----------------------------
        uint64_t max_event_id = last_event_id;
        InputActionEvent events[INPUT_ACTION_EVENT_BUFFER_LENGTH];
        getActionEventBufferSnapshot(events, INPUT_ACTION_EVENT_BUFFER_LENGTH);
        for(auto& e : events) {
            if(e.id <= last_event_id) {
                continue;
            }
            max_event_id = gfxm::_max((uint64_t)max_event_id, e.id);

            if(e.type == (uint8_t)InputActionEventType::Press) {
                for(auto& cb : e.action->on_press_callbacks) {
                    cb();
                }
            }
            if(e.type == (uint8_t)InputActionEventType::Tap) {
                for(auto& cb : e.action->on_tap_callbacks) {
                    cb();
                }
            }
            if(e.type == (uint8_t)InputActionEventType::Release) {
                for(auto& cb : e.action->on_release_callbacks) {
                    cb();
                }
            }
        }
        last_event_id = max_event_id;
    }
};


InputActionDesc&      inputCreateActionDesc(const char* name);
InputRangeDesc&       inputCreateRangeDesc(const char* name);

InputActionDesc*      inputGetActionDesc(const char* name);
InputRangeDesc*       inputGetRangeDesc(const char* name);

InputState*           inputCreateState(uint8_t user_id);

// Use inputPost to send input events from WINAPI, XInput, DirectInput, etc.
void                  inputPost(InputDeviceType dev_type, uint8_t user, uint16_t key, float value, InputKeyType value_type = InputKeyType::Toggle);
void                  inputUpdate(float dt);

const char*           inputActionEventTypeToString(InputActionEventType t);

void inputReadDevices();


void inputShowMouse(bool show);

#endif
