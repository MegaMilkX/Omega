#pragma once

#include "viewport/viewport.hpp"


class IPlayer {
public:
    virtual ~IPlayer() {}
};

class LocalPlayer : public IPlayer {
    Viewport* viewport = 0;
public:
    LocalPlayer(Viewport* viewport)
        : viewport(viewport) {}

    Viewport* getViewport() { return viewport; }
};

class NetworkPlayer : public IPlayer {
public:
};

class AiPlayer : public IPlayer {
public:
};

class ReplayPlayer : public IPlayer {
public:
};


IPlayer*    playerGetPrimary();
void        playerSetPrimary(IPlayer* player);
void        playerAdd(IPlayer* player);
void        playerRemove(IPlayer* player);
int         playerCount();
IPlayer*    playerGet(int i);
