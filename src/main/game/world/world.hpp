#pragma once

#include <memory>
#include "render_scene/render_scene.hpp"
#include "collision/collision_world.hpp"

typedef uint64_t actor_flags_t;

const actor_flags_t WACTOR_FLAGS_NOTSET     = 0x00000000;
const actor_flags_t WACTOR_FLAG_DEFAULT     = 0x00000001;
const actor_flags_t WACTOR_FLAG_UPDATE      = 0x00000010;

class wWorld;

struct wMsg {
    type t;
    uint64_t payload[4];
};
struct wRsp {
    type t;
    uint64_t payload[4];

    wRsp() {}
    wRsp(int i) {}
};

class wActor;
struct wMsgInteract {
    wActor* sender;
};
struct wmsgMissileSpawn {
    gfxm::vec3 pos;
    gfxm::vec3 dir;
};
struct wRspInteractDoorOpen {
    gfxm::vec3 sync_pos;
    gfxm::quat sync_rot;
    bool is_front;
};
struct wRspInteractJukebox {};

template<typename T>
wMsg wMsgMake(const T& msg) {
    static_assert(sizeof(T) <= sizeof(wMsg::payload), "");
    wMsg w_msg;
    memcpy(w_msg.payload, &msg, gfxm::_min(sizeof(w_msg.payload), sizeof(msg)));
    w_msg.t = type_get<T>();
    return w_msg;
}
template<typename T>
const T* wMsgTranslate(const wMsg& msg) {
    if (type_get<T>() != msg.t) {
        return 0;
    }
    return (const T*)&msg.payload[0];
}

template<typename T>
wRsp wRspMake(const T& rsp) {
    static_assert(sizeof(T) <= sizeof(wRsp::payload), "");
    wRsp w_rsp;
    memcpy(w_rsp.payload, &rsp, gfxm::_min(sizeof(w_rsp.payload), sizeof(rsp)));
    w_rsp.t = type_get<T>();
    return w_rsp;
}
template<typename T>
const T* wRspTranslate(const wRsp& rsp) {
    if (type_get<T>() != rsp.t) {
        return 0;
    }
    return (const T*)&rsp.payload[0];
}

class wWorld;
class wActor {
    friend wWorld;

    type current_state_type = 0;
    size_t current_state_array_index = 0;
public:
    TYPE_ENABLE_BASE();
protected:
    actor_flags_t flags = WACTOR_FLAGS_NOTSET;

    gfxm::mat4 world_transform;
    gfxm::vec3 translation;
    gfxm::quat rotation;

    void setFlags(actor_flags_t flags) { this->flags = flags; }
    void setFlagsDefault() { flags = WACTOR_FLAG_DEFAULT; }
public:
    virtual ~wActor() {}

    actor_flags_t getFlags() const { return flags; }

    virtual void onSpawn(wWorld* world) = 0;
    virtual void onDespawn(wWorld* world) = 0;
    virtual void onUpdate(wWorld* world, float dt) {}

    virtual wRsp onMessage(wMsg msg) { return 0; }

    wRsp sendMessage(wMsg msg) { return onMessage(msg); }

    void setTranslation(const gfxm::vec3& t) { translation = t; }
    void setRotation(const gfxm::quat& q) { rotation = q; }

    void translate(const gfxm::vec3& t) { translation += t; }
    void rotate(const gfxm::quat& q) { rotation = q * rotation; }

    const gfxm::vec3& getTranslation() const { return translation; }
    const gfxm::quat& getRotation() const { return rotation; }

    gfxm::vec3 getForward() { return gfxm::normalize(getWorldTransform()[2]); }
    gfxm::vec3 getLeft() { return gfxm::normalize(-getWorldTransform()[0]); }

    const gfxm::mat4& getWorldTransform() {
        return world_transform 
            = gfxm::translate(gfxm::mat4(1.0f), translation)
            * gfxm::to_mat4(rotation);
    }
};

class wActorStateBase {
public:
    virtual ~wActorStateBase() {}
    virtual void enter(wWorld* world, wActor** actors, size_t count) = 0;
    virtual void update(wWorld* world, float dt, wActor** actors, size_t count) = 0;
};
template<typename T>
class wActorStateT : public wActorStateBase {
    void enter(wWorld* world, wActor** actors, size_t count) override {
        onEnter(world, (T**)actors, count);
    }
    void update(wWorld* world, float dt, wActor** actors, size_t count) override {
        onUpdate(world, dt, (T**)actors, count);
    }
public:
    virtual void onEnter(wWorld* world, T** actor, size_t count) = 0;
    virtual void onUpdate(wWorld* world, float dt, T** actor, size_t count) = 0;
};

class wWorld {
    std::unique_ptr<scnRenderScene> renderScene;
    std::unique_ptr<CollisionWorld> collision_world;

    std::vector<wActor*> actors;
    std::set<wActor*> updatable_actors;

    struct StateBlock {
        std::unique_ptr<wActorStateBase> state;
        std::vector<wActor*> arrived_actors;
        std::vector<wActor*> actors;
        std::vector<wActor*> leaving_actors;
    };
    std::unordered_map<type, StateBlock> state_blocks;
    struct StateChange {
        wActor* actor;
        type from;
        type to;
    };
    std::queue<StateChange> state_changes;

    void updateStates(float dt) {
        // Handle state changes
        while (!state_changes.empty()) {
            StateChange& change = state_changes.front();
            
            auto it = state_blocks.find(change.to);
            if (it == state_blocks.end()) {
                state_changes.pop();
                continue;
            }
            if (change.actor->current_state_type != type(0)) {
                auto it = state_blocks.find(change.actor->current_state_type);
                assert(it != state_blocks.end());
                it->second.leaving_actors.push_back(change.actor);
                it->second.actors.erase(it->second.actors.begin() + change.actor->current_state_array_index);
                for (int i = 0; i < it->second.actors.size(); ++i) {
                    it->second.actors[i]->current_state_array_index = i;
                }
            }
            
            change.actor->current_state_array_index = -1;
            change.actor->current_state_type = change.to;
            it->second.arrived_actors.push_back(change.actor);

            state_changes.pop();
        }

        // Handle departures
        for (auto& s : state_blocks) {
            if (s.second.leaving_actors.empty()) {
                continue;
            }
            for (int i = 0; i < s.second.leaving_actors.size(); ++i) {
                LOG_WARN("ActorLeaving: " << s.second.leaving_actors[i]);
            }
            s.second.leaving_actors.clear();
        }

        // Run on_enter
        for (auto& s : state_blocks) {
            if (s.second.arrived_actors.empty()) {
                continue;
            }
            for (int i = 0; i < s.second.arrived_actors.size(); ++i) {
                LOG_WARN("ActorEntering: " << s.second.arrived_actors[i]);
            }
            s.second.state->enter(this, s.second.arrived_actors.data(), s.second.arrived_actors.size());
            for (int i = 0; i < s.second.arrived_actors.size(); ++i) {
                s.second.arrived_actors[i]->current_state_array_index = i + s.second.actors.size();
            }
            s.second.actors.insert(s.second.actors.end(), s.second.arrived_actors.begin(), s.second.arrived_actors.end());
            s.second.arrived_actors.clear();
        }

        // Run update
        for (auto& s : state_blocks) {
            for (int i = 0; i < s.second.actors.size(); ++i) {
                //LOG_WARN("ActorUpdating: " << s.second.actors[i]);
            }
            s.second.state->update(this, dt, s.second.actors.data(), s.second.actors.size());
        }
    }

public:
    wWorld()
    : renderScene(new scnRenderScene), collision_world(new CollisionWorld) {

    }

    scnRenderScene* getRenderScene() { return renderScene.get(); }
    CollisionWorld* getCollisionWorld() { return collision_world.get(); }

    template<typename T>
    void registerState() {
        auto it = state_blocks.insert(std::make_pair(type_get<T>(), StateBlock())).first;
        it->second.state.reset(new T());
    }
    template<typename T>
    void setActorState(wActor* actor) {
        if (actor->current_state_type == type_get<T>()) {
            assert(false);
            return;
        }
        state_changes.push(StateChange{ actor, actor->current_state_type, type_get<T>() });
    }

    template<typename T>
    void registerSystem() {
        // TODO
    }

    void addActor(wActor* a) {
        auto flags = a->getFlags();
        if (flags == WACTOR_FLAGS_NOTSET) {
            assert(false);
            LOG_ERR("Actor flags not set");
            return;
        }
        actors.emplace_back(a);
        if (flags & WACTOR_FLAG_UPDATE) {
            updatable_actors.insert(a);
        }
        a->onSpawn(this);
    }
    void removeActor(wActor* a) {
        a->onDespawn(this);
        for (int i = 0; i < actors.size(); ++i) {
            if (actors[i] == a) {
                actors.erase(actors.begin() + i);
                break;
            }
        }
        updatable_actors.erase(a);
    }

    void postMessage() {
        // TODO
    }

    void update(float dt) {
        collision_world->update();
        collision_world->debugDraw();
        
        updateStates(dt);

        for (auto a : updatable_actors) {
            a->onUpdate(this, dt);
        }

        renderScene->update(); // supposedly updating transforms, skin, effects
    }
};