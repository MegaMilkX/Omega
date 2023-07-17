#pragma once

#include "ifsm.hpp"


template<typename T>
class FSM_T : public IFSM {
    T* fsm_owner = 0;
public:
    FSM_T(T* owner)
        : fsm_owner(owner) {}

    T* getFsmOwner() { return fsm_owner; }
    template<typename STATE_T>
    void addState(const std::string& name) {
        auto ptr = new STATE_T;
        ptr->machine = this;
        IFSM::addState(name, ptr);
    }
};


template<typename T>
class FSMState_T : public IFSMState {
    friend FSM_T<T>;
    FSM_T<T>* machine = 0;
    T* owner = 0;
public:
    T* getFsmOwner() { return machine->getFsmOwner(); }
    FSM_T<T>* getFsm() { return machine; }
};
