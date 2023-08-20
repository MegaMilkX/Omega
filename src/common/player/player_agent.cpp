#include "player_agent.hpp"
#include "player.hpp"


// Pass nullptr for player to unlink
void playerLinkAgent(IPlayer* player, IPlayerAgent* agent) {
    assert(agent);

    if (player == nullptr) {
        if (agent->current_player) {
            agent->onPlayerDetach(agent->current_player);
            agent->current_player->agents.erase(agent);
            agent->current_player = 0;
        }
        return;
    }

    if (agent->current_player) {
        agent->onPlayerDetach(agent->current_player);
        agent->current_player->agents.erase(agent);
        agent->current_player = 0;
    }

    player->agents.insert(agent);
    agent->current_player = player;

    agent->onPlayerAttach(player);
}


IPlayerAgent::~IPlayerAgent() {
    playerLinkAgent(nullptr, this);
}

void IPlayerAgent::attachPlayer(IPlayer* player) {
    playerLinkAgent(player, this);
}
void IPlayerAgent::detachPlayer() {
    playerLinkAgent(nullptr, this);
}
