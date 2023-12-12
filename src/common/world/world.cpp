#include "world.hpp"

#include <unordered_set>
#include <vector>

static std::unordered_set<RuntimeWorld*> worlds;
static std::vector<RuntimeWorld*> worlds_update_list;

RuntimeWorld*  gameWorldCreate() {
    auto w = new RuntimeWorld;
    worlds.insert(w);
    worlds_update_list.push_back(w);
    return w;
}

void        gameWorldDestroy(RuntimeWorld* w) {
    worlds.erase(w);
    auto it = std::find(worlds_update_list.begin(), worlds_update_list.end(), w);
    if (it != worlds_update_list.end()) {
        worlds_update_list.erase(it);
    }

    delete w;
}


RuntimeWorld** gameWorldGetUpdateList() {
    return &worlds_update_list[0];
}
int gameWorldGetUpdateListCount() {
    return (int)worlds_update_list.size();
}
