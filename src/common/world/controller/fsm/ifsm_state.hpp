#pragma once


class IFSMState {
public:
    virtual ~IFSMState() {}
    virtual void onEnter() = 0;
    virtual void onUpdate(float dt) = 0;
};
