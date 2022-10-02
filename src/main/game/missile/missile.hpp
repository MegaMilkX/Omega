#pragma once

#include "game/world/controller/actor_controllers.hpp"

#include "game/world/node/node_collider.hpp"
#include "game/world/node/node_sound_emitter.hpp"
#include "game/world/node/node_particle_emitter.hpp"
#include "game/world/node/node_skeletal_model.hpp"

class wMissileStateFlying : public wFsmControllerState {
    gameActorNode* root = 0;
    nodeCollider* collider = 0;
public:
    wMissileStateFlying() {}

    void onReset() override {
        root = 0;
        collider = 0;
    }
    void onActorNodeRegister(type t, gameActorNode* component, const std::string& name) override {
        if (component->isRoot()) {
            root = component;
            return;
        }
        if (t == type_get<nodeCollider>() && name == "collider") {
            collider = (nodeCollider*)component;
        }
    }
    void onUpdate(gameWorld* world, gameActor* actor, wFsmController* fsm, float dt) override {
        if (collider->collider.overlappingColliderCount() > 0) {
            fsm->setState("decay");
            world->postMessage(MSGID_EXPLOSION, MSGPLD_EXPLOSION{ root->getWorldTranslation() });
            return;
        }

        gfxm::aabb box;
        box.from = gfxm::vec3(-25.0f, 0.0f, -25.0f);
        box.to = gfxm::vec3(25.0f, 25.0f, 25.0f);
        if (!gfxm::point_in_aabb(box, root->getWorldTranslation())) {
            fsm->setState("decay");            
            world->postMessage(MSGID_EXPLOSION, MSGPLD_EXPLOSION{ root->getWorldTranslation() });
            return;
        }

        // Quake 3 rocket speed (900 units per sec)
        // 64 quake3 units is approx. 1.7 meters
        root->translate(-root->getForward() * dt * 23.90625f);
    }
};
class wMissileStateDying : public wFsmControllerState {
    nodeSoundEmitter* sound_component = 0;
    std::vector<nodeParticleEmitter*> particle_emitters;
public:
    void onReset() override {
        sound_component = 0;
        particle_emitters.clear();
    }
    void onActorNodeRegister(type t, gameActorNode* component, const std::string& name) override {
        if (t == type_get<nodeSoundEmitter>() && name == "sound") {
            sound_component = (nodeSoundEmitter*)component;
        } else if(t == type_get<nodeParticleEmitter>()) {
            particle_emitters.push_back((nodeParticleEmitter*)component);
        }
    }
    void onEnter() override {
        if (sound_component) {
            sound_component->stop();
        }
        for (int i = 0; i < particle_emitters.size(); ++i) {
            auto pe = particle_emitters[i];
            // TODO:
            pe->emitter.is_alive = false;
        }
    }
    void onUpdate(gameWorld* world, gameActor* actor, wFsmController* fsm, float dt) override {
        bool can_despawn = true;
        for (int i = 0; i < particle_emitters.size(); ++i) {
            auto pe = particle_emitters[i];
            if (pe->emitter.isAlive()) {
                can_despawn = false;
                break;
            }
        }
        if (can_despawn) {
            world->despawnActor(actor);
        }
    }
};

class actorRocketStateDefault;
class actorRocketStateDying;
class actorMissile : public gameActor {
    TYPE_ENABLE(gameActor);
public:
    actorMissile() {
        auto root = setRoot<nodeSkeletalModel>("model");
        root->setModel(resGet<mdlSkeletalModelMaster>("models/rocket/rocket.skeletal_model"));
        auto snd = root->createChild<nodeSoundEmitter>("sound");
        snd->setClip(resGet<AudioClip>("audio/sfx/rocket_loop.ogg"));
        auto ptcl = root->createChild<nodeParticleEmitter>("particles");
        ptcl->setTranslation(gfxm::vec3(0, 0, 0.3f));
        auto collider = root->createChild<nodeCollider>("collider");

        auto fsm = addController<wFsmController>();
        fsm->addState("fly", new wMissileStateFlying);
        fsm->addState("decay", new wMissileStateDying);

        setFlags(WACTOR_FLAG_UPDATE);
    }
    void onUpdate(gameWorld* world, float dt) override {}
    void onUpdateDecay(gameWorld* world, float dt) override {}
    void onDecay(gameWorld* world) override {}

    void onSpawn(gameWorld* world) override {}
    void onDespawn(gameWorld* world) override {}
};
STATIC_BLOCK{
    type_register<actorMissile>("actorMissile")
        .parent<gameActor>();
};

#include "game/particle_emitter/particle_emitter.hpp"
class actorExplosion : public gameActor {
    TYPE_ENABLE(gameActor);
    ptclEmitter emitter;
public:
    actorExplosion() {
        setFlags(WACTOR_FLAG_UPDATE);

        emitter.init();
        auto shape = emitter.setShape<ptclSphereShape>();
        shape->radius = 1.5f;
        shape->emit_mode = ptclSphereShape::EMIT_MODE::VOLUME;
        emitter.addComponent<ptclAngularVelocityComponent>();
        emitter.looping = false;
        emitter.duration = 5.5f / 60.0f;

        curve<float> emit_curve;
        emit_curve[.0f] = 512.0f;
        emitter.setParticlePerSecondCurve(emit_curve);
        curve<float> scale_curve;
        scale_curve[.0f] = 1.0f;
        scale_curve[.1f] = 1.2f;
        scale_curve[1.0f] = 0.f;
        emitter.setScaleOverLifetimeCurve(scale_curve);/*
        curve<gfxm::vec4> rgba_curve;
        rgba_curve[.0f] = gfxm::vec4(1, 0.65f, 0, 1);
        rgba_curve[1.f] = gfxm::vec4(.1f, .1f, 0.1f, .0f);
        emitter.setRGBACurve(rgba_curve);*/
    }
    void onUpdate(gameWorld* world, float dt) override {
        world->decayActor(this);
    }
    void onUpdateDecay(gameWorld* world, float dt) override {
        emitter.setWorldTransform(getWorldTransform());
        emitter.update_emit(dt);
        emitter.update(dt);
    }
    void onDecay(gameWorld* world) override {
        // ?
    }
    bool hasDecayed() const override {
        return !emitter.isAlive();
    }
    void onSpawn(gameWorld* world) override {
        emitter.reset();

        emitter.spawn(world->getRenderScene());
    }
    void onDespawn(gameWorld* world) override {
        emitter.despawn(world->getRenderScene());
    }
};
STATIC_BLOCK{
    type_register<actorExplosion>("actorExplosion")
        .parent<gameActor>();
};


class wExplosionSystem : public WorldSystem {
    RHSHARED<AudioClip> clip_explosion;
public:
    wExplosionSystem() {
        clip_explosion = resGet<AudioClip>("audio/sfx/rocklx1a.ogg");
    }

    void onMessage(gameWorld* world, const MSG_MESSAGE& msg) override {
        switch (msg.id) {
        case MSGID_EXPLOSION:
            audio().playOnce3d(
                clip_explosion->getBuffer(),
                msg.getPayload<MSGPLD_EXPLOSION>()->translation, 0.3f
            );
            world->spawnActorTransient<actorExplosion>()
                ->setTranslation(msg.getPayload<MSGPLD_EXPLOSION>()->translation);
            LOG_DBG("Explosion!");
            break;
        };
    }
    void onUpdate(gameWorld* world, float dt) override {

    }
};
class wMissileSystem : public WorldSystem {
public:
    void onMessage(gameWorld* world, const MSG_MESSAGE& msg) override {
        switch (msg.id) {
        case MSGID_MISSILE_SPAWN: {
            auto pld = msg.getPayload<MSGPLD_MISSILE_SPAWN>();
            auto missile = world->spawnActorTransient<actorMissile>();
            missile->getRoot()->setTranslation(pld->translation);
            missile->getRoot()->setRotation(pld->orientation);
            } break;
        }
    }
    void onUpdate(gameWorld* world, float dt) override {

    }
};



void missileInit(gameWorld* world, int pool_size);
void missileCleanup();

void missileSpawnOne(const gfxm::vec3& pos, const gfxm::vec3& dir);

void missileUpdate(float dt);