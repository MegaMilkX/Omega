#include "world.hpp"

#include <unordered_set>
#include <vector>

static std::unordered_set<GameWorld*> worlds;
static std::vector<GameWorld*> worlds_update_list;

GameWorld*  gameWorldCreate() {
    auto w = new GameWorld;
    worlds.insert(w);
    worlds_update_list.push_back(w);
    return w;
}

void        gameWorldDestroy(GameWorld* w) {
    worlds.erase(w);
    auto it = std::find(worlds_update_list.begin(), worlds_update_list.end(), w);
    if (it != worlds_update_list.end()) {
        worlds_update_list.erase(it);
    }

    delete w;
}


GameWorld** gameWorldGetUpdateList() {
    return &worlds_update_list[0];
}
int gameWorldGetUpdateListCount() {
    return (int)worlds_update_list.size();
}
