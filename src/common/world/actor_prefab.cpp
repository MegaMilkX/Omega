#include "actor_prefab.hpp"
#include "actor.hpp"
#include "world/common_systems/dirty_system.hpp"


static void assignInstanceProps(rtti::MetaObject* object, const std::map<rtti::property, rtti::varying>& props) {
    for (auto kv : props) {
        rtti::property prop = kv.first;
        const rtti::varying& var = kv.second;
        prop.set(object, var);
    }
}

static void instantiateNodes(ActorNode* node, const ActorPrefab::NodeBlueprint* bp) {
    node->applySnapshot(bp->snap);

    for (int i = 0; i < bp->children.size(); ++i) {
        auto t = bp->children[i].snap.type_;
        ActorNode* child = node->createChild(t);
        if (!child) {
            LOG_ERR("Failed to create node of type '" << t.get_name() << "'");
            assert(false);
            continue;
        }
        instantiateNodes(child, &bp->children[i]);
    }

    if (auto d = dynamic_cast<IDirty*>(node)) {
        d->resolveDirty();
    }
    // TODO:
    // finalize
    /*if (auto pl = dynamic_cast<IPostLoad*>(node)) {
        pl->onPostLoad();
    }*/
}

Actor* ActorPrefab::instantiate() const {
    Actor* actor = rtti::new_from_snapshot<Actor>(snapshot);
    if(!actor) {
        assert(false);
        actor = new Actor();
        actor->applySnapshot(snapshot);
    }

    for (auto& snap : drivers) {
        rtti::type t = snap.type_;
        ActorDriver* drv = actor->addDriver(t);
        drv->applySnapshot(snap);

        if (auto d = dynamic_cast<IDirty*>(drv)) {
            d->resolveDirty();
        }
    }

    ActorNode* root = actor->setRoot(root_node.snap.type_);
    if (!root) {
        LOG_ERR("Prefab root of unresolved type '" << root_node.snap.type_.get_name() << "'");
        assert(false);
        return actor;
    }
    instantiateNodes(actor->getRoot(), &root_node);

    return actor;
}

