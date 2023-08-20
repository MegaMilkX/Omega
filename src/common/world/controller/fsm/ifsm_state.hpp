#pragma once

#include "game_messaging/game_messaging.hpp"


class IFSMState {
public:
    virtual ~IFSMState() {}
    virtual void onEnter() = 0;
    virtual GAME_MESSAGE onMessage(GAME_MESSAGE msg) { return GAME_MSG::NOT_HANDLED; }
    virtual void onUpdate(float dt) = 0;
};
