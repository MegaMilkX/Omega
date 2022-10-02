#include "input.hpp"

#include <algorithm>
#include <map>
#include <unordered_map>
#include <set>
#include <time.h>
#include <assert.h>

#include <Windows.h>


static InputCmd           input_buffer[INPUT_CMD_BUFFER_LENGTH];
static int                insert_cursor = 0;
static InputActionEvent   input_action_event_buffer[INPUT_ACTION_EVENT_BUFFER_LENGTH];
static int                action_event_insert_cursor = 0;

// TODO: Synchronize
static std::set<InputAction*> actions; // All actions automatically store themselves here
static std::set<InputRange*> ranges;
static std::vector<InputLink*> links;

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

static bool                       is_context_stack_dirty = false;

InputLink::InputLink()
: key(0) {
    array_id = links.size();
    links.push_back(this);
    is_context_stack_dirty = true;
}
InputLink::~InputLink() {
    int last_id = links.size() - 1;
    if(last_id != array_id) {
        links[array_id] = links[last_id];
        links[array_id]->array_id = array_id;
    }
    links.resize(links.size() - 1);
    is_context_stack_dirty = true;    
}

InputAction::InputAction(const char* name)
: name(name) {
    actions.insert(this);
}
InputAction::~InputAction() {
    actions.erase(this);
}
InputAction& InputAction::linkKey(InputKey key, float multiplier) {
    InputLink* l = new InputLink();
    l->action = this;
    l->key = key;
    l->multiplier = multiplier;
    links.push_back(std::unique_ptr<InputLink>(l));
    is_context_stack_dirty = true;
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


InputRange::InputRange(const char* name)
: name(name) {
    ranges.insert(this);
}
InputRange::~InputRange() {
    ranges.erase(this);
}
InputRange& InputRange::linkKeyX(InputKey key, float multiplier) {
    InputLink* l = new InputLink();
    l->key = key;
    l->value = .0f;
    l->multiplier = multiplier;
    key_links_x.push_back(std::unique_ptr<InputLink>(l));
    is_context_stack_dirty = true;
    return *this;
}
InputRange& InputRange::linkKeyY(InputKey key, float multiplier) {
    InputLink* l = new InputLink();
    l->key = key;
    l->value = .0f;
    l->multiplier = multiplier;
    key_links_y.push_back(std::unique_ptr<InputLink>(l));
    is_context_stack_dirty = true;
    return *this;
}
InputRange& InputRange::linkKeyZ(InputKey key, float multiplier) {
    InputLink* l = new InputLink();
    l->key = key;
    l->value = .0f;
    l->multiplier = multiplier;
    key_links_z.push_back(std::unique_ptr<InputLink>(l));
    is_context_stack_dirty = true;
    return *this;
}


static std::vector<InputContext*> context_stack;

InputContext::InputContext(const char* name)
: name(name), is_enabled(true) {
    stack_pos = context_stack.size();
    context_stack.push_back(this);
    is_context_stack_dirty = true;
}
InputContext::~InputContext() {
    for(int i = stack_pos + 1; i < context_stack.size(); ++i) {
        context_stack[i - 1] = context_stack[i];
        context_stack[i]->stack_pos = i - 1;
    }
    context_stack.resize(context_stack.size() - 1);
    is_context_stack_dirty = true;
}
void InputContext::enable() {
    is_enabled = true;
    is_context_stack_dirty = true;    
}
void InputContext::disable() {
    is_enabled = false;
    is_context_stack_dirty = false;
}
void InputContext::toFront() {
    for(int i = stack_pos + 1; i < context_stack.size(); ++i) {
        context_stack[i - 1] = context_stack[i];
        context_stack[i]->stack_pos = i - 1;
    }
    stack_pos = context_stack.size() - 1;
    context_stack[stack_pos] = this;
    is_context_stack_dirty = true;
}
bool InputContext::isEnabled() const {
    return is_enabled;
}
InputAction* InputContext::createAction(const char* name) {
    auto p = new InputAction(name);
    ::actions.insert(p);
    actions.insert(p);
    return p;
}
InputRange*  InputContext::createRange(const char* name) {
    auto p = new InputRange(name);
    ::ranges.insert(p);
    ranges.insert(p);
    return p;
}


void inputPost(InputDeviceType dev_type, uint8_t user, uint16_t key, float value, InputKeyType value_type) {
    static int next_cmd_id = 0;
    InputCmd cmd;
    cmd.device_type = (uint8_t)dev_type;
    cmd.user_id = user;
    cmd.key = key;
    cmd.value = value;
    cmd.value_type = value_type;
    cmd.id = ++next_cmd_id;
    input_buffer[insert_cursor] = cmd;
    insert_cursor = (++insert_cursor) % INPUT_CMD_BUFFER_LENGTH;
}

void inputPostActionEvent(InputAction* h, InputActionEventType type) {
    InputCmd fake_cmd;
    fake_cmd.device_type = 0;
    fake_cmd.id = 0;
    fake_cmd.key = 0;
    fake_cmd.user_id = 0;
    fake_cmd.value = 0;
    inputPostActionEvent(h, type, fake_cmd);
}
void inputPostActionEvent(InputAction* h, InputActionEventType type, const InputCmd& propagating_cmd) {
    static int next_event_id = 1;
    InputActionEvent e = { 0 };
    e.action = h;
    e.id = next_event_id++;
    e.type = (uint8_t)type;
    strncpy(e.name, h->getName(), sizeof(e.name));
    e.propagating_cmd = propagating_cmd;

    input_action_event_buffer[action_event_insert_cursor] = e;
    action_event_insert_cursor = (++action_event_insert_cursor) % INPUT_ACTION_EVENT_BUFFER_LENGTH;
}

void inputGetBufferSnapshot(InputCmd* dest, int count) {
    int c = gfxm::_min(count, INPUT_CMD_BUFFER_LENGTH);
    int copy_cursor = insert_cursor;
    for(int i = 0; i < c; ++i) {
        dest[i] = input_buffer[(copy_cursor + i) % INPUT_CMD_BUFFER_LENGTH];
    }
}

void inputGetActionEventBufferSnapshot(InputActionEvent* dest, int count) {
    int c = gfxm::_min(count, INPUT_ACTION_EVENT_BUFFER_LENGTH);
    int copy_cursor = action_event_insert_cursor;
    for(int i = 0; i < c; ++i) {
        dest[i] = input_action_event_buffer[(copy_cursor + i) % INPUT_ACTION_EVENT_BUFFER_LENGTH];
    }
}

int inputGetContextStack(InputContext** dest, int max_count) {
    int c = gfxm::_min(max_count, (int)context_stack.size());
    memcpy(dest, context_stack.data(), c * sizeof(InputContext*));
    return c;
}

uint64_t last_cmd_id = 0;
uint64_t last_event_id = 0;
void inputUpdate(float dt) {
    // Mark obscured input actions and ranges
    // ---------------------------
    if(is_context_stack_dirty) {
        for(int i = context_stack.size() - 1; i >= 0; --i) {
            auto ctx = context_stack[i];
            int priority = ctx->isEnabled() ? (i * 3) : -1;

            for(auto& range : ctx->ranges) {
                for(auto& link : range->key_links_x) {
                    link->priority = priority;
                }
                for(auto& link : range->key_links_y) {
                    link->priority = priority;
                }
                for(auto& link : range->key_links_z) {
                    link->priority = priority;
                }
            }
            for(auto& action : ctx->actions) {
                for(auto& link : action->links) {
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
        for(int i = 0; i < links.size() - 1; ++i) {     
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
    inputGetBufferSnapshot(buf, INPUT_CMD_BUFFER_LENGTH);
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
            inputPostActionEvent(action, InputActionEventType::Press);
        } else if (action->is_pressed && !is_any_link_pressed) {
            if (action->hold_time <= INPUT_TAP_THRESHOLD_SEC) {
                inputPostActionEvent(action, InputActionEventType::Tap);
            }
            action->hold_time = .0f;
            action->is_pressed = false;
            action->is_just_released = true;
            inputPostActionEvent(action, InputActionEventType::Release);
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
    inputGetActionEventBufferSnapshot(events, INPUT_ACTION_EVENT_BUFFER_LENGTH);
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
