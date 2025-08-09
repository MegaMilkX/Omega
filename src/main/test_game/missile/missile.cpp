#include "missile.hpp"

#include <vector>


STATIC_BLOCK {
    type_register<MissileActor>("MissileActor")
        .parent<Actor>();
};

STATIC_BLOCK {
    type_register<actorExplosion>("actorExplosion")
        .parent<Actor>();
};

std::vector<HSHARED<MissileActor>> rocketActors;


void missileInit(RuntimeWorld* world, int pool_size) {

}
void missileCleanup() {

}

void missileSpawnOne(const gfxm::vec3& pos, const gfxm::vec3& dir) {

}

void missileUpdate(float dt) {

}