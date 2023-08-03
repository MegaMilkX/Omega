#include "player.hpp"

#include <assert.h>
#include <unordered_set>


static std::unordered_set<IPlayer*> player_set;
static std::vector<IPlayer*> players;
static IPlayer* primary_player = 0;


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
}
void        playerRemove(IPlayer* player) {
    if (player_set.count(player) == 0) {
        return;
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