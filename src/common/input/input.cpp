#include "input.hpp"

#include <algorithm>
#include <map>
#include <unordered_map>
#include <set>
#include <time.h>
#include <assert.h>

#include <Windows.h>


static std::vector<std::unique_ptr<InputState>> input_states;

typedef std::function<void(void)> action_callback_t;

struct InputActionEventKey {
    union {
        struct {
            uint32_t action_hdl;
            uint32_t event_type;
        };
        uint64_t value;
    };
};

InputLink::InputLink()
: key(0) {}
InputLink::~InputLink() {}

InputAction::InputAction(const char* name)
: name(name) {}
InputAction::~InputAction()
{}
InputAction& InputAction::linkKey(InputKey key, float multiplier) {
    InputLink* l = new InputLink();
    l->action = this;
    l->key = key;
    l->multiplier = multiplier;
    links.push_back(std::unique_ptr<InputLink>(l));
    return *this;
}
InputAction& InputAction::bindPress(std::function<void(void)> cb) {
    on_press_callbacks.push_back(cb);
    return *this;
}
InputAction& InputAction::bindRelease(std::function<void(void)> cb) {
    on_release_callbacks.push_back(cb);
    return *this;
}
InputAction& InputAction::bindTap(std::function<void(void)> cb) {
    on_tap_callbacks.push_back(cb);
    return *this;
}
InputAction& InputAction::bindHold(std::function<void(void)> cb, float threshold) {
    on_hold_callbacks.push_back(std::make_pair(threshold, cb));
    return *this;
}
void InputAction::registerLinks(InputState* state) {
    for (auto& l : links) {
        state->registerLink(l.get());
    }
}
void InputAction::unregisterLinks(InputState* state) {
    for (auto& l : links) {
        state->unregisterLink(l.get());
    }
}


InputRange::InputRange(const char* name)
: name(name) {}
InputRange::~InputRange()
{}
InputRange& InputRange::linkKeyX(InputKey key, float multiplier) {
    InputLink* l = new InputLink();
    l->action = 0;
    l->key = key;
    l->value = .0f;
    l->multiplier = multiplier;
    key_links_x.push_back(std::unique_ptr<InputLink>(l));
    return *this;
}
InputRange& InputRange::linkKeyY(InputKey key, float multiplier) {
    InputLink* l = new InputLink();
    l->action = 0;
    l->key = key;
    l->value = .0f;
    l->multiplier = multiplier;
    key_links_y.push_back(std::unique_ptr<InputLink>(l));
    return *this;
}
InputRange& InputRange::linkKeyZ(InputKey key, float multiplier) {
    InputLink* l = new InputLink();
    l->action = 0;
    l->key = key;
    l->value = .0f;
    l->multiplier = multiplier;
    key_links_z.push_back(std::unique_ptr<InputLink>(l));
    return *this;
}
void InputRange::registerLinks(InputState* state) {
    for (auto& l : key_links_x) {
        state->registerLink(l.get());
    }
    for (auto& l : key_links_y) {
        state->registerLink(l.get());
    }
    for (auto& l : key_links_z) {
        state->registerLink(l.get());
    }
}
void InputRange::unregisterLinks(InputState* state) {
    for (auto& l : key_links_x) {
        state->unregisterLink(l.get());
    }
    for (auto& l : key_links_y) {
        state->unregisterLink(l.get());
    }
    for (auto& l : key_links_z) {
        state->unregisterLink(l.get());
    }
}



InputContext::InputContext(const char* name)
: owner_state(0), name(name), is_enabled(true) {
}
InputContext::~InputContext() {}
void InputContext::enable() {
    is_enabled = true;
    owner_state->setDirty();  
}
void InputContext::disable() {
    is_enabled = false;
    owner_state->setDirty();
}
void InputContext::toFront() {
    owner_state->contextToFront(this);
}
bool InputContext::isEnabled() const {
    return is_enabled;
}
InputAction* InputContext::createAction(const char* name) {
    auto action_desc = inputGetActionDesc(name);
    auto action = new InputAction(name);
    assert(action_desc);
    if (action_desc) {
        for (int i = 0; i < action_desc->linkCount(); ++i) {
            action->linkKey(action_desc->getLinks()[i].key, action_desc->getLinks()[i].multiplier);
        }
    }
    actions.insert(std::unique_ptr<InputAction>(action));

    if (owner_state) {
        owner_state->registerAction(action);
        owner_state->setDirty();
    }
    return action;
}
InputRange* InputContext::createRange(const char* name) {
    auto range_desc = inputGetRangeDesc(name);
    auto range = new InputRange(name);
    assert(range_desc);
    if (range_desc) {
        for (int i = 0; i < range_desc->linkXCount(); ++i) {
            range->linkKeyX(range_desc->getLinksX()[i].key, range_desc->getLinksX()[i].multiplier);
        }
        for (int i = 0; i < range_desc->linkYCount(); ++i) {
            range->linkKeyY(range_desc->getLinksY()[i].key, range_desc->getLinksY()[i].multiplier);
        }
        for (int i = 0; i < range_desc->linkZCount(); ++i) {
            range->linkKeyZ(range_desc->getLinksZ()[i].key, range_desc->getLinksZ()[i].multiplier);
        }
    }
    ranges.insert(std::unique_ptr<InputRange>(range));

    if (owner_state) {
        owner_state->registerRange(range);
        owner_state->setDirty();
    }
    return range;
}
void InputContext::registerLinks(InputState* state) {
    for (auto& a : actions) {
        state->registerAction(a.get());
    }
    for (auto& r : ranges) {
        state->registerRange(r.get());
    }
}
void InputContext::unregisterLinks(InputState* state) {
    for (auto& a : actions) {
        state->unregisterAction(a.get());
    }
    for (auto& r : ranges) {
        state->unregisterRange(r.get());
    }
}


static std::unordered_map<std::string, std::unique_ptr<InputActionDesc>> action_descs;
static std::unordered_map<std::string, std::unique_ptr<InputRangeDesc>> range_descs;
InputActionDesc&      inputCreateActionDesc(const char* name) {
    auto ptr = new InputActionDesc;
    action_descs.insert(std::make_pair( std::string(name), std::unique_ptr<InputActionDesc>(ptr) ));
    return *ptr;
}
InputRangeDesc&       inputCreateRangeDesc(const char* name) {
    auto ptr = new InputRangeDesc;
    range_descs.insert(std::make_pair( std::string(name), std::unique_ptr<InputRangeDesc>(ptr) ));
    return *ptr;
}

InputActionDesc*      inputGetActionDesc(const char* name) {
    auto it = action_descs.find(name);
    if (it == action_descs.end()) {
        return 0;
    }
    return it->second.get();
}
InputRangeDesc*       inputGetRangeDesc(const char* name) {
    auto it = range_descs.find(name);
    if (it == range_descs.end()) {
        return 0;
    }
    return it->second.get();
}


InputState* inputCreateState(uint8_t user_id) {
    auto ptr = new InputState(user_id);
    input_states.push_back(std::unique_ptr<InputState>(ptr));
    return ptr;
}

void inputPost(InputDeviceType dev_type, uint8_t user, uint16_t key, float value, InputKeyType value_type) {
    for (auto& state : input_states) {
        if (state->getUserId() != user) {
            continue;
        }
        state->postInput(dev_type, key, value, value_type);
    }
}


void inputUpdate(float dt) {
    for (auto& state : input_states) {
        state->update(dt);
    }
}


const char* inputActionEventTypeToString(InputActionEventType t) {
    const char* names[] = {
        "Press",
        "Release",
        "Tap",
        "Hold"
    };
    return names[(uint8_t)t];
}


#include <Windows.h>
#include <Xinput.h>
#pragma comment(lib, "xinput.lib")
XINPUT_STATE prev_states[XUSER_MAX_COUNT] = { {0}, {0}, {0}, {0} };
static gfxm::vec2 stick_l_prev[XUSER_MAX_COUNT];
static gfxm::vec2 stick_r_prev[XUSER_MAX_COUNT];
void inputReadDevices() {
    for(int i = 0; i < XUSER_MAX_COUNT; ++i) {
        XINPUT_STATE xstate = { 0 };
        DWORD dwResult;
        dwResult = XInputGetState(i, &xstate);
        if(dwResult != ERROR_SUCCESS) {
            continue;
        }
        WORD diff = xstate.Gamepad.wButtons ^ prev_states[i].Gamepad.wButtons;
        if (diff & XINPUT_GAMEPAD_DPAD_UP) {
            inputPost(InputDeviceType::GamepadXbox, i, (uint16_t)KeyGamepadXbox::DpadUp, (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) ? 1.0f : .0f);
        }
        if (diff & XINPUT_GAMEPAD_DPAD_DOWN) {
            inputPost(InputDeviceType::GamepadXbox, i, (uint16_t)KeyGamepadXbox::DpadDown, (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) ? 1.0f : .0f);
        }
        if (diff & XINPUT_GAMEPAD_DPAD_LEFT) {
            inputPost(InputDeviceType::GamepadXbox, i, (uint16_t)KeyGamepadXbox::DpadLeft, (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) ? 1.0f : .0f);
        }
        if (diff & XINPUT_GAMEPAD_DPAD_RIGHT) {
            inputPost(InputDeviceType::GamepadXbox, i, (uint16_t)KeyGamepadXbox::DpadRight, (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) ? 1.0f : .0f);
        }
        if (diff & XINPUT_GAMEPAD_START) {
            inputPost(InputDeviceType::GamepadXbox, i, (uint16_t)KeyGamepadXbox::Start, (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_START) ? 1.0f : .0f);
        }
        if (diff & XINPUT_GAMEPAD_BACK) {
            inputPost(InputDeviceType::GamepadXbox, i, (uint16_t)KeyGamepadXbox::Back, (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) ? 1.0f : .0f);
        }
        if (diff & XINPUT_GAMEPAD_LEFT_THUMB) {
            inputPost(InputDeviceType::GamepadXbox, i, (uint16_t)KeyGamepadXbox::LeftThumb, (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) ? 1.0f : .0f);
        }
        if (diff & XINPUT_GAMEPAD_RIGHT_THUMB) {
            inputPost(InputDeviceType::GamepadXbox, i, (uint16_t)KeyGamepadXbox::RightThumb, (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) ? 1.0f : .0f);
        }
        if (diff & XINPUT_GAMEPAD_LEFT_SHOULDER) {
            inputPost(InputDeviceType::GamepadXbox, i, (uint16_t)KeyGamepadXbox::LeftShoulder, (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) ? 1.0f : .0f);
        }
        if (diff & XINPUT_GAMEPAD_RIGHT_SHOULDER) {
            inputPost(InputDeviceType::GamepadXbox, i, (uint16_t)KeyGamepadXbox::RightShoulder, (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) ? 1.0f : .0f);
        }
        if (diff & XINPUT_GAMEPAD_A) {
            inputPost(InputDeviceType::GamepadXbox, i, (uint16_t)KeyGamepadXbox::A, (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_A) ? 1.0f : .0f);
        }
        if (diff & XINPUT_GAMEPAD_B) {
            inputPost(InputDeviceType::GamepadXbox, i, (uint16_t)KeyGamepadXbox::B, (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_B) ? 1.0f : .0f);
        }
        if (diff & XINPUT_GAMEPAD_X) {
            inputPost(InputDeviceType::GamepadXbox, i, (uint16_t)KeyGamepadXbox::X, (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_X) ? 1.0f : .0f);
        }
        if (diff & XINPUT_GAMEPAD_Y) {
            inputPost(InputDeviceType::GamepadXbox, i, (uint16_t)KeyGamepadXbox::Y, (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_Y) ? 1.0f : .0f);
        }

        if(xstate.Gamepad.bLeftTrigger != prev_states[i].Gamepad.bLeftTrigger) {
            inputPost(InputDeviceType::GamepadXbox, i, Key.GamepadXbox.TriggerL, xstate.Gamepad.bLeftTrigger / 255.0f);
        }
        if(xstate.Gamepad.bRightTrigger != prev_states[i].Gamepad.bRightTrigger) {
            inputPost(InputDeviceType::GamepadXbox, i, Key.GamepadXbox.TriggerR, xstate.Gamepad.bRightTrigger / 255.0f);
        }

        gfxm::vec2 stick_l(xstate.Gamepad.sThumbLX / 32767.0f, xstate.Gamepad.sThumbLY / 32767.0f);
        gfxm::vec2 stick_r(xstate.Gamepad.sThumbRX / 32767.0f, xstate.Gamepad.sThumbRY / 32767.0f);
        if(gfxm::length(stick_l) < 0.1f) {
            stick_l = gfxm::vec2(0,0);
        }
        if(gfxm::length(stick_r) < 0.1f) {
            stick_r = gfxm::vec2(0,0);
        }
        if(stick_l.x != stick_l_prev[i].x) {
            inputPost(InputDeviceType::GamepadXbox, i, Key.GamepadXbox.StickLX, stick_l.x);
        }
        if(stick_l.y != stick_l_prev[i].y) {
            inputPost(InputDeviceType::GamepadXbox, i, Key.GamepadXbox.StickLY, stick_l.y);
        }
        if(stick_r.x != stick_r_prev[i].x) {
            inputPost(InputDeviceType::GamepadXbox, i, Key.GamepadXbox.StickRX, stick_r.x);
        }
        if(stick_r.y != stick_r_prev[i].y) {
            inputPost(InputDeviceType::GamepadXbox, i, Key.GamepadXbox.StickRY, stick_r.y);
        }

        prev_states[i] = xstate;
        stick_l_prev[i] = stick_l;
        stick_r_prev[i] = stick_r;
    }
}


#include "platform/platform.hpp"

void inputShowMouse(bool show) {
    assert(false);
    // TODO:
    //GLFWwindow* window = (GLFWwindow*)platformGetGlfwWindow();
    //glfwSetInputMode(window, GLFW_CURSOR, show ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}
