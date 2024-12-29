#pragma once

#include "player_agent_actor.auto.hpp"

#include "actor.hpp"
#include "player/player_agent.hpp"

#if CPPI_PARSER
[[cppi_begin, no_reflect]];
class IPlayerAgent;
[[cppi_end]];
#endif

[[cppi_class]];
class PlayerAgentActor : public Actor, public IPlayerAgent {
public:
    TYPE_ENABLE();

    void onPlayerAttach(IPlayer* player) override {
        for (auto& kv : controllers) {
            kv.second->onMessage(makeGameMessage(PAYLOAD_PLAYER_ATTACH{ player }));
        }
    }
    void onPlayerDetach(IPlayer* player) override {
        for (auto& kv : controllers) {
            kv.second->onMessage(makeGameMessage(PAYLOAD_PLAYER_DETACH{ player }));
        }
    }
};
