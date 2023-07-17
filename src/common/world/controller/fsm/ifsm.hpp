#pragma once

#include "ifsm_state.hpp"
#include <assert.h>
#include <map>
#include <memory>
#include "log/log.hpp"


class IFSM {
    IFSMState* initial_state = 0;
    IFSMState* current_state = 0;
    IFSMState* next_state = 0;
    std::map<std::string, std::unique_ptr<IFSMState>> states;
protected:
    void addState(const std::string& name, IFSMState* state) {
        if (states.empty()) {
            initial_state = state;
            next_state = initial_state;
        }
        states[name] = std::unique_ptr<IFSMState>(state);
    }
public:
    virtual ~IFSM() {}
    void reset() {
        next_state = initial_state;
    }
    void setState(const char* name) {
        auto it = states.find(name);
        if (it == states.end()) {
            assert(false);
            LOG_ERR("State " << name << " does not exist");
            return;
        }
        next_state = it->second.get();
    }
    void update(float dt) {
        if (next_state) {
            current_state = next_state;
            next_state = 0;
            current_state->onEnter();
        }
        assert(current_state);
        current_state->onUpdate(dt);
    }
};
