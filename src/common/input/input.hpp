#ifndef INPUT2_HPP
#define INPUT2_HPP

#include <vector>
#include <set>
#include <functional>
#include <memory>
#include "math/gfxm.hpp"


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
        uint16_t Btn0 = 0; uint16_t Btn1 = 1; uint16_t Btn3 = 2; uint16_t Btn4 = 3; uint16_t Btn5 = 4;
        uint16_t AxisX = 5; uint16_t AxisY = 6; uint16_t Scroll = 7; uint16_t ScrollUp = 8; uint16_t ScrollDown = 9;
        uint16_t BtnLeft = Btn0; uint16_t BtnRight = Btn1;
    } Mouse;
    struct {
        uint16_t Unknown = -1;
        uint16_t Space = 32;
        uint16_t Left = 37;
        uint16_t Up = 38;
        uint16_t Right = 39;
        uint16_t Down = 40;
        //uint16_t Apostrophe = 39 /* ' */;
        uint16_t Comma = 44 /* ; */;
        uint16_t Minus = 45 /* - */;
        uint16_t Period = 46 /* . */;
        uint16_t Slash = 47 /* / */;
        uint16_t _0 = 48;
        uint16_t _1 = 49;
        uint16_t _2 = 50;
        uint16_t _3 = 51;
        uint16_t _4 = 52;
        uint16_t _5 = 53;
        uint16_t _6 = 54;
        uint16_t _7 = 55;
        uint16_t _8 = 56;
        uint16_t _9 = 57;
        uint16_t Semicolon = 59 /* ; */;
        uint16_t Equal = 61 /* =  */;
        uint16_t A = 65;
        uint16_t B = 66;
        uint16_t C = 67;
        uint16_t D = 68;
        uint16_t E = 69;
        uint16_t F = 70;
        uint16_t G = 71;
        uint16_t H = 72;
        uint16_t I = 73;
        uint16_t J = 74;
        uint16_t K = 75;
        uint16_t L = 76;
        uint16_t M = 77;
        uint16_t N = 78;
        uint16_t O = 79;
        uint16_t P = 80;
        uint16_t Q = 81;
        uint16_t R = 82;
        uint16_t S = 83;
        uint16_t T = 84;
        uint16_t U = 85;
        uint16_t V = 86;
        uint16_t W = 87;
        uint16_t X = 88;
        uint16_t Y = 89;
        uint16_t Z = 90;
        uint16_t LeftBracket = 91 /* [ */;
        uint16_t Backslash = 92 /* \ */;
        uint16_t RightBracket = 93 /* ] */;
        uint16_t GraveAccent = 96 /* ` */;
        uint16_t World1 = 161 /* Non-Us #1 */;
        uint16_t World2 = 162 /* Non-Us #2 */;
        uint16_t Escape = 256;
        uint16_t Enter = 257;
        uint16_t Tab = 258;
        uint16_t Backspace = 259;
        uint16_t Insert = 260;
        uint16_t Delete = 261;
        uint16_t PageUp = 266;
        uint16_t PageDown = 267;
        uint16_t Home = 268;
        uint16_t End = 269;
        uint16_t CapsLock = 280;
        uint16_t ScrollLock = 281;
        uint16_t NumLock = 282;
        uint16_t PrintScreen = 283;
        uint16_t Pause = 284;
        uint16_t F1 = 290;
        uint16_t F2 = 291;
        uint16_t F3 = 292;
        uint16_t F4 = 293;
        uint16_t F5 = 294;
        uint16_t F6 = 295;
        uint16_t F7 = 296;
        uint16_t F8 = 297;
        uint16_t F9 = 298;
        uint16_t F10 = 299;
        uint16_t F11 = 300;
        uint16_t F12 = 301;
        uint16_t F13 = 302;
        uint16_t F14 = 303;
        uint16_t F15 = 304;
        uint16_t F16 = 305;
        uint16_t F17 = 306;
        uint16_t F18 = 307;
        uint16_t F19 = 308;
        uint16_t F20 = 309;
        uint16_t F21 = 310;
        uint16_t F22 = 311;
        uint16_t F23 = 312;
        uint16_t F24 = 313;
        uint16_t F25 = 314;
        uint16_t Num0 = 320;
        uint16_t Num1 = 321;
        uint16_t Num2 = 322;
        uint16_t Num3 = 323;
        uint16_t Num4 = 324;
        uint16_t Num5 = 325;
        uint16_t Num6 = 326;
        uint16_t Num7 = 327;
        uint16_t Num8 = 328;
        uint16_t Num9 = 329;
        uint16_t NumDecimal = 330;
        uint16_t NumDivide = 331;
        uint16_t NumMultiply = 332;
        uint16_t NumSubtract = 333;
        uint16_t NumAdd = 334;
        uint16_t NumEnter = 335;
        uint16_t NumEqual = 336;
        uint16_t LeftShift = 340;
        uint16_t LeftControl = 341;
        uint16_t LeftAlt = 342;
        uint16_t LeftSuper = 343;
        uint16_t RightShift = 344;
        uint16_t RightControl = 345;
        uint16_t RightAlt = 346;
        uint16_t RightSuper = 347;
        uint16_t Menu = 348;
    } Keyboard;
    struct {
        uint16_t DpadUp = 0;
        uint16_t DpadDown = 1;
        uint16_t DpadLeft = 2;
        uint16_t DpadRight = 3;
        uint16_t Start = 4;
        uint16_t Back = 5;
        uint16_t LeftThumb = 6;
        uint16_t RightThumb = 7;
        uint16_t LeftShoulder = 8;
        uint16_t RightShoulder = 9;
        uint16_t A = 10;
        uint16_t B = 11;
        uint16_t X = 12;
        uint16_t Y = 13;
        uint16_t StickLX = 14; uint16_t StickLY = 15;
        uint16_t StickRX = 16; uint16_t StickRY = 17;
        uint16_t TriggerL = 18; uint16_t TriggerR = 19;
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
    friend void inputUpdate(float dt);

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
    InputAction& linkKey(InputDeviceType device_type, uint16_t key, float multiplier = 1.0f);

    InputAction& bindPress(std::function<void(void)> cb);
    InputAction& bindRelease(std::function<void(void)> cb);
    InputAction& bindTap(std::function<void(void)> cb);
    InputAction& bindHold(std::function<void(void)> cb, float threshold);

    bool isPressed() const { return is_pressed; }
    bool isJustPressed() const { return is_just_pressed; }
    bool isJustReleased() const { return is_just_released; }
    const char* getName() const { return name.c_str(); }
};

enum class InputRangeType {
    Absolute,
    Relative
};

class InputRange {
    friend void inputUpdate(float dt);

    InputRangeType type = InputRangeType::Relative;
    std::string name;
    gfxm::vec3  value;
    std::vector<std::unique_ptr<InputLink>> key_links_x;
    std::vector<std::unique_ptr<InputLink>> key_links_y;
    std::vector<std::unique_ptr<InputLink>> key_links_z;
public:
    InputRange(const char* name = "UnnamedRange");
    ~InputRange();
    InputRange& linkKeyX(InputDeviceType device, uint16_t key, float multiplier);
    InputRange& linkKeyY(InputDeviceType device, uint16_t key, float multiplier);
    InputRange& linkKeyZ(InputDeviceType device, uint16_t key, float multiplier);
    
    const char* getName() const { return name.c_str(); }
    float getValue() const { return value.x; }
    gfxm::vec2 getVec2() const { return gfxm::vec2(value.x, value.y); }
    gfxm::vec3 getVec3() const { return value; }
};

class InputContext {
    friend void inputUpdate(float dt);

    std::string name;
    bool        is_enabled;
    int         stack_pos;
    std::set<InputAction*>  actions;
    std::set<InputRange*>   ranges;
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
    const std::set<InputRange*>& getRanges() const { return ranges; }
    const std::set<InputAction*>& getActions() const { return actions; }
};

struct InputActionEvent {
    char            name[64]; // For debug
    uint64_t        id;
    InputAction*    action;
    uint8_t         type;
    InputCmd        propagating_cmd;
};


static const int   INPUT_CMD_BUFFER_LENGTH = 32;
static const int   INPUT_ACTION_EVENT_BUFFER_LENGTH = 32;
static const int   INPUT_FILTERED_ACTION_EVENT_BUFFER_LENGTH = 32;
static const float INPUT_TAP_THRESHOLD_SEC = 0.2f;

// Use inputPost to send input events from WINAPI, XInput, DirectInput, etc.
void                  inputPost(InputDeviceType dev_type, uint8_t user, uint16_t key, float value, InputKeyType value_type = InputKeyType::Toggle);

void                  inputPostActionEvent(InputAction* h, InputActionEventType type);
void                  inputPostActionEvent(InputAction* h, InputActionEventType type, const InputCmd& propagating_cmd);
void                  inputGetBufferSnapshot(InputCmd* dest, int count);
void                  inputGetActionEventBufferSnapshot(InputActionEvent* dest, int count);
int                   inputGetContextStack(InputContext** dest, int max_count);

void                  inputUpdate(float dt);

const char*           inputActionEventTypeToString(InputActionEventType t);

void inputReadDevices();


void inputShowMouse(bool show);

#endif
