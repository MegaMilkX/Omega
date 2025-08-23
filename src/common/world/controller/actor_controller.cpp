#include "actor_controller.hpp"

#include "world/node/actor_node.hpp"



void ActorController::onActorNodeRegister(type t, ActorNode* component, const std::string& name) {
    for (int i = 0; i < node_views.size(); ++i) {
        auto nv = node_views[i].get();
        if (nv->check(t, name, component->isRoot())) {
            nv->assign(component);
        }
    }
}

void ActorController::onActorNodeUnregister(type t, ActorNode* component, const std::string& name) {
    for (int i = 0; i < node_views.size(); ++i) {
        auto nv = node_views[i].get();
        if (nv->check(t, name, component->isRoot())) {
            nv->clear();
        }
    }
}

