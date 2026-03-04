#pragma once

#include "world/actor.hpp"


class IActorLink {
    Actor* actor = 0;
public:
    virtual void onAttachActor(Actor*) = 0;
    virtual void onDetachActor(Actor*) = 0;

    void linkActor(Actor* a) {
        if (actor) {
            onDetachActor(actor);
        }
        actor = a;
        if(actor) {
            onAttachActor(actor);
        }
    }
};

