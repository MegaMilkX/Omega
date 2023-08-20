#pragma once

#include <memory>
#include "input/input.hpp"
#include "viewport/viewport.hpp"
#include "player_agent.hpp"

class Actor;

class IPlayer {
    friend void playerLinkAgent(IPlayer* player, IPlayerAgent* agent);

    std::string name = "Player";
    std::unique_ptr<InputState> input_state;
    std::set<IPlayerAgent*> agents;
protected:
    void setInputState(InputState* state) {
        input_state.reset(state);
    }
public:
    virtual ~IPlayer();

    void setName(const std::string& name) { this->name = name; }
    const std::string& getName() const { return name; }

    InputState* getInputState() {
        return input_state.get();
    }
    virtual Viewport* getViewport() { return 0; }

    void attachAgent(IPlayerAgent* agent);
    void detachAgent(IPlayerAgent* agent);

    void clearAgents();
};

class LocalPlayer : public IPlayer {
    Viewport* viewport = 0;
public:
    LocalPlayer(Viewport* viewport, uint8_t input_id);

    Viewport* getViewport() override { return viewport; }
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

void        playerLinkAgent(IPlayer* player, IPlayerAgent* agent);
