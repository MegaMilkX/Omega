#pragma once

#include <memory>
#include "game/world/render_scene/render_scene.hpp"

typedef uint64_t actor_flags_t;

const actor_flags_t WACTOR_FLAG_UPDATE = 0x00000001;

class wWorld;

class cActorComponent {
public:
    virtual void onSpawn(wWorld* world) {}
    virtual void onDespawn(wWorld* world) {}
};

class wActor {
protected:
    actor_flags_t flags = 0;

    void setFlags(actor_flags_t flags) { this->flags = flags; }
public:
    virtual ~wActor() {}

    actor_flags_t getFlags() const { return flags; }

    virtual void onSpawn(wWorld* world) = 0;
    virtual void onDespawn(wWorld* world) = 0;
    virtual void onUpdate(float dt) {}

    virtual int onMessage(int msg) { return 0; }
};

class wWorld {
    std::unique_ptr<scnRenderScene> renderScene;

    std::vector<wActor*> actors;

public:
    wWorld()
    : renderScene(new scnRenderScene) {

    }

    scnRenderScene* getRenderScene() { return renderScene.get(); }

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
        for (auto a : actors) {
            a->onUpdate(dt);
        }

        renderScene->update(); // supposedly updating transforms, skin, effects
    }
};