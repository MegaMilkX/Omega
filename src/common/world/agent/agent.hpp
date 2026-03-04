#pragma once

#include <set>
#include "spawnable/spawnable.hpp"


class ISpectator {
public:
    virtual void onUpdateSpectator(float dt) = 0;
};

class IPlayerController {
public:
    virtual void onUpdateController(float dt) = 0;
};

using PlayerControllerSet = std::set<IPlayerController*>;
using SpectatorSet = std::set<ISpectator*>;

class IPlayer;
class IPlayerProxy : public ISpawnable {
    IPlayer* player = nullptr;
public:
    virtual void onAttachPlayer(IPlayer*) = 0;
    virtual void onDetachPlayer(IPlayer*) = 0;

    void setPlayer(IPlayer* p) {
        if (player) {
            onDetachPlayer(player);
        }
        player = p;
        if(player) {
            onAttachPlayer(player);
        }
    }

    IPlayer* getPlayer() { return player; }
};

