#pragma once

#include <map>
#include <string>
#include <memory>
#include "game_messaging/game_messaging.hpp"


template<typename CONTEXT_T>
struct ACTOR_FSM_STATE {
    void(CONTEXT_T::*pfn_on_enter)() = nullptr;
    void(CONTEXT_T::*pfn_on_update)(float) = nullptr;
    GAME_MESSAGE(CONTEXT_T::*pfn_on_message)(GAME_MESSAGE) = nullptr;
};


template<typename CONTEXT_T>
class ActorFsm {
public:
    typedef ACTOR_FSM_STATE<CONTEXT_T> state_t;

private:
    CONTEXT_T* context = 0;

    state_t* initial_state = 0;
    state_t* current_state = 0;
    state_t* next_state = 0;
    std::map<std::string, std::unique_ptr<state_t>> states;
public:
    ActorFsm(CONTEXT_T* context)
        : context(context) {}

    void addState(const char* name, ACTOR_FSM_STATE<CONTEXT_T> state) {
        std::unique_ptr<state_t> pstate(new state_t(state));
        if (states.empty()) {
            initial_state = pstate.get();
            next_state = initial_state;
        }
        states[name] = std::move(pstate);
    }

    void reset() {
        next_state = initial_state;
    }

    void setState(const char* name) {
        auto it = states.find(name);
        if (it == states.end()) {
            assert(false);
            LOG_ERR("ActorFsm: state " << name << " does not exist");
            return;
        }
        next_state = it->second.get();
    }
    GAME_MESSAGE onMessage(GAME_MESSAGE msg) {
        if (!current_state) {
            if (!next_state) {
                return GAME_MSG::NOT_HANDLED;
            }
            if (next_state->pfn_on_message == nullptr) {
                return GAME_MSG::NOT_HANDLED;
            }
            return (context->*(next_state->pfn_on_message))(msg);
        }

        if (current_state->pfn_on_message == nullptr) {
            return GAME_MSG::NOT_HANDLED;
        }
        return (context->*(current_state->pfn_on_message))(msg);
    }
    void update(float dt) {
        if (next_state) {
            current_state = next_state;
            next_state = 0;
            if (current_state->pfn_on_enter != nullptr) {
                (context->*(current_state->pfn_on_enter))();
            }
        }

        assert(current_state);
        if (current_state->pfn_on_update != nullptr) {
            (context->*(current_state->pfn_on_update))(dt);
        }
    }
};

