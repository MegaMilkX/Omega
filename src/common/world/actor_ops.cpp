#include "actor_ops.hpp"

#include "world/actor.hpp"


ActorNode* ActorOps::replaceNode(ActorNode* node, rtti::type type) {
    if (!has_flags(node->flags, FActorNode::TreeOwned)) {
        return nullptr;
    }

    Actor* actor = node->actor;
    assert(actor);

    if (actor->isSpawned()) {
        // Have to do this here so that all nested nodes get despawned too
        // so they don't try to despawn on rebuild with stale link-derived data
        node->onDespawnNodeInternal(*actor->getRegistry());
    }

    if (node->isRoot()) {
        rtti::PropSnapshot snap;
        node->makeSnapshot(snap);
        auto children = std::move(node->children);
        ActorNode* new_node = actor->setRoot(type);
        new_node->children = std::move(children);
        for (int i = 0; i < new_node->children.size(); ++i) {
            // TODO: Not all nodes might want attachment in the future
            // maybe it should be moved to the build step or handled by an internal virtual
            transformNodeAttach(new_node->getTransformHandle(), new_node->children[i]->getTransformHandle());
        }
        new_node->applySnapshot(snap);

        actor->requestRebuild();
        return new_node;
    } else {
        ActorNode* parent = node->getParent();

        rtti::PropSnapshot snap;
        node->makeSnapshot(snap);
        auto children = std::move(node->children);
        node->removeThis();

        ActorNode* new_node = parent->createChild(type);
        new_node->children = std::move(children);
        for (int i = 0; i < new_node->children.size(); ++i) {
            // TODO: Not all nodes might want attachment in the future
            // maybe it should be moved to the build step or handled by an internal virtual
            transformNodeAttach(new_node->getTransformHandle(), new_node->children[i]->getTransformHandle());
        }
        new_node->applySnapshot(snap);

        new_node->requestRebuild();
        return new_node;
    }
}

bool ActorOps::reparentNode(ActorNode* new_parent, ActorNode* child) {
    return new_parent->reparentChild(child);
}

