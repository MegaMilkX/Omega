#include "collider.hpp"

#include "collision_world.hpp"


void phyRigidBody::markAsExternallyTransformed() {
    if (collision_world) {
        collision_world->markAsExternallyTransformed(this);
    }
}