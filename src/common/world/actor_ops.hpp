#pragma once

#include "world/node/actor_node.hpp"


struct ActorOps {
    static ActorNode* replaceNode(ActorNode*, rtti::type);
    static bool reparentNode(ActorNode* new_parent, ActorNode* child);
};