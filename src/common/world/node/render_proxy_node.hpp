#pragma once

#include "render_proxy_node.auto.hpp"
#include "actor_node.hpp"


[[cppi_class]];
class RenderProxyNode : public ActorNode {
public:
    TYPE_ENABLE();

    void onSpawnActorNode(WorldSystemRegistry& reg) {
        
    }
    void onDespawnActorNode(WorldSystemRegistry& reg) {

    }
};

