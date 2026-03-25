#pragma once

#include <memory>
#include "world_base.hpp"
#include "render_scene/render_scene.hpp"
#include "collision/collision_world.hpp"
#include "particle_emitter/particle_simulation.hpp"

#include "actor.hpp"
#include "world_system_registry.hpp"
#include "world/node/node_camera.hpp"
#include "scene/scene.hpp"

#include "world/common_systems/dirty_system.hpp"
#include "world/common_systems/animation_system.hpp"
#include "world/common_systems/player_start_system.hpp"
#include "audio/soundscape.hpp"


class WorldController {
public:
    virtual ~WorldController() {}

    virtual void onMessage(RuntimeWorld* world, const MSG_MESSAGE& msg) = 0;
    virtual void onUpdate(RuntimeWorld* world, float dt) = 0;
};

class ActorDriverSystem {
    struct DriverSet {
        int exec_priority;
        std::set<ActorDriver*> drivers;
    };
    std::vector<DriverSet*> driver_vec;
    std::unordered_map<int, std::unique_ptr<DriverSet>> driver_map;
    bool is_driver_vec_dirty = true;
    bool is_updating_drivers = false;

public:
    ActorDriverSystem() {}
    ActorDriverSystem(const ActorDriverSystem&) = delete;
    ActorDriverSystem& operator=(const ActorDriverSystem&) = delete;

    void beginFrame() {
        if (is_driver_vec_dirty) {
            std::sort(driver_vec.begin(), driver_vec.end(),
                [](const DriverSet* a, const DriverSet* b)->bool {
                    return a->exec_priority < b->exec_priority;
            });
            is_driver_vec_dirty = false;
        }
    }

    void addActorDriver(ActorDriver* c) {
        int exec_prio = c->getExecutionPriority();
        auto it = driver_map.find(exec_prio);
        if (it == driver_map.end()) {
            auto ctrlset = new DriverSet;
            ctrlset->exec_priority = exec_prio;
            it = driver_map.insert(
                std::make_pair(exec_prio, std::unique_ptr<DriverSet>(ctrlset))
            ).first;
            driver_vec.push_back(ctrlset);
            is_driver_vec_dirty = true;
        }
        it->second->drivers.insert(c);
    }
    void removeActorDriver(ActorDriver* c) {
        if (is_updating_drivers) {
            assert(false);
            LOG_ERR("Can't remove controllers while they're being updated");
            return;
        }
        int exec_prio = c->getExecutionPriority();
        auto it = driver_map.find(exec_prio);
        if (it == driver_map.end()) {
            assert(false);
            LOG_ERR("Controller set does not exist");
            return;
        }
        it->second->drivers.erase(c);
    }

    void updateControllers(int priority_first, int priority_last, float dt) {
        is_updating_drivers = true;
        for (int i = 0; i < driver_vec.size(); ++i) {
            auto set = driver_vec[i];
            if (set->exec_priority < priority_first) {
                continue;
            }
            if (set->exec_priority > priority_last) {
                break;
            }
            for (auto ctrl : set->drivers) {
                ctrl->onUpdate(dt);
            }
        }
        is_updating_drivers = false;
    }
};

class ActorSystem {
    std::set<Actor*> actors;
    std::set<Actor*> updatable_actors;
    std::queue<Actor*> updatable_removals;
public:
    void addActor(Actor* a) {
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
    }
    void removeActor(Actor* a) {
        actors.erase(a);
        updatable_removals.push(a);
        /*
        if (a->transient_id >= 0) {
            auto it = actor_storage_map.find(a->get_type());
            if (it == actor_storage_map.end()) {
                assert(false);
                return;
            }
            auto& storage = it->second;
            storage->free(a);
        }*/
    }

    void update(float dt) {
        while(!updatable_removals.empty()) {
            auto a = updatable_removals.front();
            updatable_removals.pop();
            updatable_actors.erase(a);
        }

        //
        for (auto a : updatable_actors) {
            a->onUpdateInternal(dt);
        }
    }
};

#include "world/common_systems/scene_system.hpp"

class ITickable {
public:
    ~ITickable() {}
    virtual void onTick(float dt) = 0;
};
class TickSystem {
    std::set<ITickable*> objects;
public:
    void add(ITickable* o) { objects.insert(o); }
    void remove(ITickable* o) { objects.erase(o); }
    void update(float dt) {
        for (auto it : objects) {
            it->onTick(dt);
        }
    }
};

class IMessageEndpoint {
public:
};
class MessagingSystem {
public:
    void broadcast(int) {}
    void post(IMessageEndpoint*, int) {}
    void send(IMessageEndpoint*, int) {}
};

class Camera : public ISpawnable {
    scnRenderScene* scn = nullptr;
    scnView scn_view;
    SceneSystem* vis_sys = nullptr;

public:
    Camera() {
        scn_view.setFov(gfxm::radian(65.f));
        scn_view.setZNear(.01f);
        scn_view.setZFar(100.f);
    }

    void setCameraPosition(const gfxm::vec3& pos) { scn_view.setCameraPosition(pos); }
    void setCameraRotation(const gfxm::quat& rot) { scn_view.setCameraRotation(rot); }
    void setFov(float fov) { scn_view.setFov(fov); }
    void setZNear(float znear) { scn_view.setZNear(znear); }
    void setZFar(float zfar) { scn_view.setZFar(zfar); }

    float getFov() const { return scn_view.getFov(); }
    float getZNear() const { return scn_view.getZNear(); }
    float getZFar() const { return scn_view.getZFar(); }

    scnRenderScene* getScene() { return scn; }
    SceneSystem* getVisibilitySystem() { return vis_sys; }

    const gfxm::mat4& getViewTransform() const {
        return scn_view.getView();
    }

    void onSpawn(WorldSystemRegistry& reg) override {
        if (auto sys = reg.getSystem<scnRenderScene>()) {
            scn = sys;
            scn->addView(&scn_view);
        }
        if (auto sys = reg.getSystem<SceneSystem>()) {
            vis_sys = sys;
        }
    }
    void onDespawn(WorldSystemRegistry& reg) override {
        if (auto sys = reg.getSystem<scnRenderScene>()) {
            sys->removeView(&scn_view);
        }
        scn = nullptr;
        vis_sys = nullptr;
    }
};

#include "world/agent/agent.hpp"

constexpr int MAX_MESSAGES = 256;
class RuntimeWorld : public IWorld {
    // Baseline
    DirtySystem dirty_sys;

    // Domain specific
    SceneSystem vis_sys;
    std::unique_ptr<scnRenderScene> renderScene;
    std::unique_ptr<phyWorld> collision_world;
    std::unique_ptr<ParticleSimulation> particle_sim;
    AnimationSystem anim_sys;
    Soundscape soundscape;

    // Player related
    PlayerControllerSet player_controllers;
    SpectatorSet        spectators;

    // Actors
    ActorSystem actor_sys;
    ActorDriverSystem controller_sys;
    
    // Messaging
    MSG_MESSAGE message_queue[MAX_MESSAGES];
    int message_count = 0;

    //
    TickSystem tick_sys;

    // World-level controllers
    std::unordered_map<type, std::unique_ptr<WorldController>> world_controllers;

    // Various stuff
    PlayerStartSystem player_start_sys;

    void updateWorldControllers(float dt) {
        for (auto& kv : world_controllers) {
            kv.second->onUpdate(this, dt);
        }
    }

    void _processMessages() {
        for (int i = 0; i < message_count; ++i) {
            auto& msg = message_queue[i];
            for (auto& kv : world_controllers) {
                kv.second->onMessage(this, msg);
            }
        }
        message_count = 0;
    }
public:
    RuntimeWorld()
    : renderScene(new scnRenderScene)
    , collision_world(new phyWorld)
    , particle_sim(new ParticleSimulation(this))
    {
        registerSystem(&dirty_sys);
        registerSystem(&actor_sys);
        registerSystem(&controller_sys);
        registerSystem(&tick_sys);
        registerSystem(collision_world.get());
        registerSystem(&vis_sys);
        registerSystem(renderScene.get());
        registerSystem(particle_sim.get());
        registerSystem(&anim_sys);
        registerSystem(&soundscape);
        registerSystem(&player_controllers);
        registerSystem(&spectators);

        registerSystem(&player_start_sys);
    }

    scnRenderScene* getRenderScene() { return renderScene.get(); }
    phyWorld* getCollisionWorld() { return collision_world.get(); }
    ParticleSimulation* getParticleSim() { return particle_sim.get(); }

    template<typename CONTROLLER_T>
    CONTROLLER_T* addWorldController() {
        type t = type_get<CONTROLLER_T>();
        auto it = world_controllers.find(t);
        if (it != world_controllers.end()) {
            assert(false);
            LOG_ERR("World controller " << t.get_name() << " already exists");
            return (CONTROLLER_T*)it->second.get();
        }
        CONTROLLER_T* ptr = new CONTROLLER_T;
        world_controllers.insert(std::make_pair(t, std::unique_ptr<WorldController>(ptr)));
        return ptr;
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

    void update(float dt) override {
        IWorld::beginFrame();

        dirty_sys.update();

        anim_sys.update(dt);
        soundscape.update(dt);

        for (auto& pc : player_controllers) {
            pc->onUpdateController(dt);
        }

        controller_sys.beginFrame();

        _processMessages();
        updateWorldControllers(dt);

        actor_sys.update(dt);
        controller_sys.updateControllers(EXEC_PRIORITY_FIRST, EXEC_PRIORITY_PRE_COLLISION, dt);

        tick_sys.update(dt);

        // TODO: Only update collision transforms
        // Updating all for now
        /*for (auto a : actors) {
            a->updateNodeTransform();
        }*/

        // Update collider transforms from scene graph
        for (int i = 0; i < collision_world->dirtyTransformCount(); ++i) {
            phyRigidBody* c = (phyRigidBody*)collision_world->getDirtyTransformArray()[i];
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
        //collision_world->clearDirtyTransformArray();

        //const float COLLISION_STEP = 1.0f / 165.f;
        //const float COLLISION_STEP = .015f;
        //collision_world->update(dt, COLLISION_STEP, 1);
        collision_world->update_variableDt(dt);

        // Update transforms based on collision response
        for (int i = 0; i < collision_world->dirtyTransformCount(); ++i) {
            const phyRigidBody* c = collision_world->getDirtyTransformArray()[i];
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

        // TODO: Update visual/audio nodes
        // Updating everything for now
        /*for (auto a : actors) {
            a->updateNodeTransform();
        }*/

        //collision_world->debugDraw();

        particle_sim->update(dt);

        controller_sys.updateControllers(EXEC_PRIORITY_PRE_COLLISION + 1, EXEC_PRIORITY_LAST, dt);

        for (auto& s : spectators) {
            s->onUpdateSpectator(dt);
        }

        vis_sys.updateProxies();

        renderScene->update(dt); // supposedly updating transforms, skin, effects
    }
};

