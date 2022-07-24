#pragma once

#include <memory>
#include "render_scene/render_scene.hpp"
#include "common/collision/collision_world.hpp"

typedef uint64_t actor_flags_t;

const actor_flags_t WACTOR_FLAG_UPDATE = 0x00000001;

class wWorld;

class cActorComponent {
public:
    virtual void onSpawn(wWorld* world) {}
    virtual void onDespawn(wWorld* world) {}
};

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

struct wMsgDoorOpen {};

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

class wActor {
protected:
    actor_flags_t flags = 0;

    gfxm::mat4 world_transform;
    gfxm::vec3 translation;
    gfxm::quat rotation;

    void setFlags(actor_flags_t flags) { this->flags = flags; }
public:
    virtual ~wActor() {}

    actor_flags_t getFlags() const { return flags; }

    virtual void onSpawn(wWorld* world) = 0;
    virtual void onDespawn(wWorld* world) = 0;
    virtual void onUpdate(float dt) {}

    virtual wRsp onMessage(wMsg msg) { return 0; }

    wRsp sendMessage(wMsg msg) { return onMessage(msg); }

    void setTranslation(const gfxm::vec3& t) { translation = t; }
    void setRotation(const gfxm::quat& q) { rotation = q; }

    void translate(const gfxm::vec3& t) { translation += t; }

    const gfxm::vec3& getTranslation() const { return translation; }
    const gfxm::quat& getRotation() const { return rotation; }

    gfxm::vec3 getForward() { return getWorldTransform() * gfxm::vec4(0, 0, 1, 0); }

    const gfxm::mat4& getWorldTransform() {
        return world_transform 
            = gfxm::translate(gfxm::mat4(1.0f), translation)
            * gfxm::to_mat4(rotation);
    }
};

class wWorld {
    std::unique_ptr<scnRenderScene> renderScene;
    std::unique_ptr<CollisionWorld> collision_world;

    std::vector<wActor*> actors;

public:
    wWorld()
    : renderScene(new scnRenderScene), collision_world(new CollisionWorld) {

    }

    scnRenderScene* getRenderScene() { return renderScene.get(); }
    CollisionWorld* getCollisionWorld() { return collision_world.get(); }

    void addActor(wActor* a) {
        actors.emplace_back(a);
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
    }

    void update(float dt) {
        collision_world->update();
        collision_world->debugDraw();
        
        for (auto a : actors) {
            a->onUpdate(dt);
        }

        renderScene->update(); // supposedly updating transforms, skin, effects
    }
};