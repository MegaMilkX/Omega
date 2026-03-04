#include "actor_prefab.hpp"
#include "actor.hpp"
#include "world/common_systems/dirty_system.hpp"


static void assignInstanceProps(MetaObject* object, const std::map<property, varying>& props) {
    for (auto kv : props) {
        property prop = kv.first;
        const varying& var = kv.second;
        prop.set(object, var);
    }
}

static void instantiateNodes(ActorNode* node, const ActorPrefab::NodeBlueprint* bp) {
    assignInstanceProps(node, bp->properties);

    for (int i = 0; i < bp->children.size(); ++i) {
        ActorNode* child = node->createChild(bp->children[i].t);
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
    Actor* actor = new Actor();

    for (auto kv : components) {
        type t = kv.first;
        const ComponentBlueprint& bp = kv.second;
        ActorComponent* comp = actor->addComponent(t);
        assignInstanceProps(comp, bp.properties);

        if (auto d = dynamic_cast<IDirty*>(comp)) {
            d->resolveDirty();
        }
        // TODO:
        // finalize
        /*if (auto pl = dynamic_cast<IPostLoad*>(comp)) {
            pl->onPostLoad();
        }*/
    }

    for (auto kv : drivers) {
        type t = kv.first;
        const DriverBlueprint& bp = kv.second;
        ActorDriver* drv = actor->addDriver(t);
        assignInstanceProps(drv, bp.properties);

        if (auto d = dynamic_cast<IDirty*>(drv)) {
            d->resolveDirty();
        }
        // TODO:
        // finalize
        /*if (auto pl = dynamic_cast<IPostLoad*>(drv)) {
            pl->onPostLoad();
        }*/
    }

    actor->setRoot(root_node.t);
    instantiateNodes(actor->getRoot(), &root_node);

    return actor;
}

