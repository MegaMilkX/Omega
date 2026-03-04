#include "player.hpp"

#include <assert.h>
#include <unordered_set>

#include "world/actor.hpp"

#include <windows.h>



IPlayer::~IPlayer() {
    clearRoles();
}

LocalPlayer::LocalPlayer(EngineRenderView* viewport, uint8_t input_id)
    : viewport(viewport) {
    setInputState(inputCreateState(input_id));
    /*
    char buf[32];
    DWORD sz = 32;
    GetComputerNameExA(ComputerNamePhysicalDnsHostname, buf, &sz);
    setName(buf);*/
    setName("LocalPlayer");
}


static std::unordered_set<IPlayer*> player_set;
static std::vector<IPlayer*> players;
static IPlayer* primary_player = 0;
static std::set<IPlayerListener*> listeners;


IPlayer*    playerGetPrimary() {
    return primary_player;
}
void        playerSetPrimary(IPlayer* player) {
    assert(player_set.count(player));
    primary_player = player;
}
void        playerAdd(IPlayer* player) {
    if (player_set.count(player)) {
        return;
    }
    player_set.insert(player);
    players.push_back(player);

    if (primary_player == 0) {
        primary_player = player;
    }

    for (auto l : listeners) {
        l->onPlayerJoined(player);
    }
}
void        playerRemove(IPlayer* player) {
    if (player_set.count(player) == 0) {
        return;
    }

    for (auto l : listeners) {
        l->onPlayerLeft(player);
    }

    player_set.erase(player);
    players.erase(std::find(players.begin(), players.end(), player));

    if (primary_player == player) {
        primary_player = 0;
    }
}

int         playerCount() {
    return players.size();
}
IPlayer*    playerGet(int i) {
    return players[i];
}


void playerAddListener(IPlayerListener* l) {
    listeners.insert(l);
    for (int i = 0; i < players.size(); ++i) {
        l->onPlayerJoined(players[i]);
    }
}

void playerRemoveListener(IPlayerListener* l) {
    for (int i = 0; i < players.size(); ++i) {
        l->onPlayerLeft(players[i]);
    }
    listeners.erase(l);
}

