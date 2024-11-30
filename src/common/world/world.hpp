#pragma once

#include <memory>
#include "render_scene/render_scene.hpp"
#include "collision/collision_world.hpp"

#include "actor.hpp"
#include "player_agent_actor.hpp"
#include "world/node/node_camera.hpp"


class WorldSystem {
public:
    virtual ~WorldSystem() {}

    virtual void onMessage(RuntimeWorld* world, const MSG_MESSAGE& msg) = 0;
    virtual void onUpdate(RuntimeWorld* world, float dt) = 0;
};

class gameWorldNodeSystemBase {
public:
    virtual void update(ActorNode* node) = 0;
};

template<typename NODE_T>
class gameWorldNodeSystem : public gameWorldNodeSystemBase {
public:
    void update(ActorNode* node) override {
        onUpdate((NODE_T*)node);
    }

    virtual void onUpdate(NODE_T* node) = 0;
};

constexpr int MAX_MESSAGES = 256;
class RuntimeWorld {
    // Domain specific
    std::unique_ptr<scnRenderScene> renderScene;
    std::unique_ptr<CollisionWorld> collision_world;

    // Camera
    CameraNode* current_cam_node = 0;

    // Actors
    std::set<Actor*> actors;
    std::set<Actor*> updatable_actors;
    std::queue<Actor*> updatable_removals;

    // Actor controllers
    struct ControllerSet {
        int exec_priority;
        std::set<ActorController*> controllers;
    };
    std::vector<ControllerSet*> controller_vec;
    std::unordered_map<int, std::unique_ptr<ControllerSet>> controller_map;
    bool is_controller_vec_dirty = true;

    // Transient actors
    struct ActorStorageTransient {
        type actor_type;
        std::vector<std::unique_ptr<Actor>> actors;
        std::queue<size_t> free_slots;
        std::vector<Actor*> decaying_actors;
        ActorStorageTransient(type t)
        :actor_type(t) {}
        Actor* acquire() {
            size_t id = actors.size();
            Actor* p_actor = 0;
            if (free_slots.empty()) {
                p_actor = (Actor*)actor_type.construct_new();
                if (!p_actor) {
                    assert(false);
                    return 0;
                }
                actors.push_back(std::unique_ptr<Actor>(p_actor));
                p_actor->transient_id = id;
            } else {
                id = free_slots.front();
                free_slots.pop();
                p_actor = actors[id].get();
                p_actor->transient_id = id;
            }
            return p_actor;
        }
        void free(Actor* actor) {
            if (actor->transient_id < 0) {
                assert(false);
                return;
            }
            free_slots.push(actor->transient_id);
            actor->transient_id = -1;
        }
        void decay(Actor* actor) {
            decaying_actors.push_back(actor);
        }
    };
    std::unordered_map<type, std::unique_ptr<ActorStorageTransient>> actor_storage_map;

    // Messaging
    MSG_MESSAGE message_queue[MAX_MESSAGES];
    int message_count = 0;

    // World-level systems
    std::unordered_map<type, std::unique_ptr<WorldSystem>> systems;

    void updateSystems(float dt) {
        for (auto& kv : systems) {
            kv.second->onUpdate(this, dt);
        }
    }
    void updateControllers(int priority_first, int priority_last, float dt) {
        for (int i = 0; i < controller_vec.size(); ++i) {
            auto set = controller_vec[i];
            if (set->exec_priority < priority_first) {
                continue;
            }
            if (set->exec_priority > priority_last) {
                break;
            }
            for (auto ctrl : set->controllers) {
                ctrl->onUpdate(this, dt);
            }
        }
    }
    void updateTransientActors(float dt) {
        for (auto& kv : actor_storage_map) {
            for (int i = 0; i < kv.second->decaying_actors.size();) {
                auto a = kv.second->decaying_actors[i];
                if (a->hasDecayed_world()) {
                    kv.second->decaying_actors.erase(kv.second->decaying_actors.begin() + i);
                    despawnActor(a);
                    continue;
                }
                a->_onUpdateDecay(this, dt);
                ++i;
            }
        }
    }
    void _processMessages() {
        for (int i = 0; i < message_count; ++i) {
            auto& msg = message_queue[i];
            for (auto& kv : systems) {
                kv.second->onMessage(this, msg);
            }
        }
        message_count = 0;
    }
public:
    RuntimeWorld()
    : renderScene(new scnRenderScene), collision_world(new CollisionWorld) {

    }

    scnRenderScene* getRenderScene() { return renderScene.get(); }
    CollisionWorld* getCollisionWorld() { return collision_world.get(); }

    template<typename SYSTEM_T>
    SYSTEM_T* addSystem() {
        type t = type_get<SYSTEM_T>();
        auto it = systems.find(t);
        if (it != systems.end()) {
            assert(false);
            LOG_ERR("System " << t.get_name() << " already exists");
            return (SYSTEM_T*)it->second.get();
        }
        SYSTEM_T* ptr = new SYSTEM_T;
        systems.insert(std::make_pair(t, std::unique_ptr<WorldSystem>(ptr)));
        return ptr;
    }

    template<typename ACTOR_T>
    ACTOR_T* spawnActorTransient() {
        return (ACTOR_T*)spawnActorTransient(type_get<ACTOR_T>());
    }
    Actor* spawnActorTransient(const char* type_name) {
        type t = type_get(type_name);
        if (t == type(0)) {
            assert(false);
            return 0;
        }
        return spawnActorTransient(t);
    }
    Actor* spawnActorTransient(type actor_type) {
        auto it = actor_storage_map.find(actor_type);
        if (it == actor_storage_map.end()) {
            it = actor_storage_map.insert(
                std::make_pair(
                    actor_type,
                    std::unique_ptr<ActorStorageTransient>(new ActorStorageTransient(actor_type))
                )
            ).first;
        }
        auto& storage = it->second;
        auto ptr = storage->acquire();
        spawnActor(ptr);
        return ptr;
    }
    void spawnActor(Actor* a) {
        auto flags = a->getFlags();
        if (flags == ACTOR_FLAGS_NOTSET) {
            assert(false);
            LOG_ERR("Actor flags not set");
            return;
        }
        if (flags & ACTOR_FLAG_UPDATE) {
            updatable_actors.insert(a);
        }
        actors.insert(a);
        // Controllers
        for (auto& kv : a->controllers) {
            int exec_prio = kv.second->getExecutionPriority();
            auto it = controller_map.find(exec_prio);
            if (it == controller_map.end()) {
                auto ctrlset = new ControllerSet;
                ctrlset->exec_priority = exec_prio;
                it = controller_map.insert(
                    std::make_pair(exec_prio, std::unique_ptr<ControllerSet>(ctrlset))
                ).first;
                controller_vec.push_back(ctrlset);
                is_controller_vec_dirty = true;
            }
            it->second->controllers.insert(kv.second.get());
        }
        // ==
        a->_onSpawn(this);
    }
    void despawnActor(Actor* a) {
        a->_onDespawn(this);
        // Controllers
        for (auto&kv : a->controllers) {
            int exec_prio = kv.second->getExecutionPriority();
            auto it = controller_map.find(exec_prio);
            if (it == controller_map.end()) {
                assert(false);
                LOG_ERR("Controller set does not exist");
                continue;
            }
            it->second->controllers.erase(kv.second.get());
        }
        // ==
        actors.erase(a);
        updatable_removals.push(a);
        if (a->transient_id >= 0) {
            auto it = actor_storage_map.find(a->get_type());
            if (it == actor_storage_map.end()) {
                assert(false);
                return;
            }
            auto& storage = it->second;
            storage->free(a);
        }
    }
    void decayActor(Actor* a) {
        if (a->transient_id < 0) {
            assert(false);
            LOG_ERR("Attempted to decay a non-transient actor!");
            return;
        }
        updatable_removals.push(a);
        
        auto it = actor_storage_map.find(a->get_type());
        if (it == actor_storage_map.end()) {
            assert(false);
            return;
        }

        a->_onDecay(this);

        auto& storage = it->second;
        storage->decay(a);
    }

    template<typename PAYLOAD_T>
    void postMessage(int MSG_ID, const PAYLOAD_T& payload) {
        MSG_MESSAGE msg;
        msg.make(MSG_ID, payload);
        postMessage(msg);
    }
    void postMessage(const MSG_MESSAGE& msg) {
        if (message_count == MAX_MESSAGES) {
            assert(false);
            LOG_ERR("Message queue overflow");
            return;
        }
        message_queue[message_count] = msg;
        ++message_count;
    }

    void update(float dt) {
        if (is_controller_vec_dirty) {
            std::sort(controller_vec.begin(), controller_vec.end(),
                [](const ControllerSet* a, const ControllerSet* b)->bool {
                    return a->exec_priority < b->exec_priority;
            });
            is_controller_vec_dirty = false;
        }

        _processMessages();
        updateSystems(dt);

        updateTransientActors(dt);

        while(!updatable_removals.empty()) {
            auto a = updatable_removals.front();
            updatable_removals.pop();
            updatable_actors.erase(a);
        }

        //
        for (auto a : updatable_actors) {
            a->_onUpdate(this, dt);
        }

        updateControllers(EXEC_PRIORITY_FIRST, EXEC_PRIORITY_PRE_COLLISION, dt);

        // TODO: Only update collision transforms
        // Updating all for now
        /*for (auto a : actors) {
            a->updateNodeTransform();
        }*/

        // Update collider transforms from scene graph
        for (int i = 0; i < collision_world->dirtyTransformCount(); ++i) {
            Collider* c = (Collider*)collision_world->getDirtyTransformArray()[i];
            auto type = c->user_data.type;
            if (type == COLLIDER_USER_NODE) {
                ActorNode* node = (ActorNode*)c->user_data.user_ptr;
                c->setPosition(node->getWorldTranslation());
                c->setRotation(node->getWorldRotation());
            } else if (type == COLLIDER_USER_ACTOR) {
                Actor* actor = (Actor*)c->user_data.user_ptr;
                c->setPosition(actor->getTranslation());
                c->setRotation(actor->getRotation());
            }
        }
        collision_world->clearDirtyTransformArray();

        collision_world->update(dt);

        // Update transforms based on collision response
        for (int i = 0; i < collision_world->dirtyTransformCount(); ++i) {
            const Collider* c = collision_world->getDirtyTransformArray()[i];
            if (c->getFlags() & COLLIDER_NO_RESPONSE) {
                continue;
            }
            auto type = c->user_data.type;
            if (type == COLLIDER_USER_NODE) {
                ActorNode* node = (ActorNode*)c->user_data.user_ptr;
                const gfxm::vec3& pos = c->getPosition();
                const gfxm::quat& rot = c->getRotation();
                Handle<TransformNode> parent = node->getTransformHandle()->getParent();
                if (parent) {
                    gfxm::mat4 world = gfxm::translate(gfxm::mat4(1.f), pos) * gfxm::to_mat4(rot);
                    gfxm::mat4 local = gfxm::inverse(parent->getWorldTransform()) * world;
                    node->setTranslation(local[3]);
                    node->setRotation(gfxm::to_quat(gfxm::to_orient_mat3(local)));
                } else {
                    node->setTranslation(pos);
                    node->setRotation(rot);
                }
                // TODO: Decide how to handle child nodes
            } else if(type == COLLIDER_USER_ACTOR) {
                Actor* actor = (Actor*)c->user_data.user_ptr;
                const gfxm::vec3& pos = c->getPosition();
                const gfxm::quat& rot = c->getRotation();
                actor->setTranslation(pos);
                actor->setRotation(rot);
            }
        }
        collision_world->clearDirtyTransformArray();

        // TODO: Update visual/audio nodes
        // Updating everything for now
        /*for (auto a : actors) {
            a->updateNodeTransform();
        }*/

        //collision_world->debugDraw();

        updateControllers(EXEC_PRIORITY_PRE_COLLISION + 1, EXEC_PRIORITY_LAST, dt);

        renderScene->update(dt); // supposedly updating transforms, skin, effects
    }

    struct NodeContainer {
        std::vector<ActorNode*> nodes;
    };
    std::unordered_map<type, std::unique_ptr<NodeContainer>> node_containers;
    void _registerNode(ActorNode* node) {
        auto t = node->get_type();
        auto it = node_containers.find(t);
        if (it == node_containers.end()) {
            it = node_containers.insert(
                std::make_pair(t, std::unique_ptr<NodeContainer>(new NodeContainer))
            ).first;
        }
        auto& container = it->second->nodes;
        node->world_container_index = container.size();
        container.push_back(node);
    }
    void _unregisterNode(ActorNode* node) {
        auto t = node->get_type();
        auto it = node_containers.find(t);
        if (it == node_containers.end()) {
            assert(false);
            LOG_ERR("_unregisterNode: type " << t.get_name() << " was never registered");
            return;
        }
        auto& container = it->second->nodes;
        if (container.empty()) {
            assert(false);
            return;
        }
        int last_idx = container.size() - 1;
        ActorNode* tmp = container[last_idx];
        container[node->world_container_index] = tmp;
        tmp->world_container_index = node->world_container_index;
        node->world_container_index = -1;
    }
};


RuntimeWorld*  gameWorldCreate();
void        gameWorldDestroy(RuntimeWorld* w);

RuntimeWorld** gameWorldGetUpdateList();
int gameWorldGetUpdateListCount();
