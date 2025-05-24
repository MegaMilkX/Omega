#pragma once

#include "missile.auto.hpp"

#include "world/controller/actor_controllers.hpp"

#include "world/node/node_collider.hpp"
#include "world/node/node_sound_emitter.hpp"
#include "world/node/node_particle_emitter.hpp"
#include "world/node/node_skeletal_model.hpp"
#include "particle_emitter/particle_impl.hpp"


class wMissileStateFlying : public ctrlFsmState {
    ActorNode* root = 0;
    ColliderNode* collider = 0;

    gfxm::vec3 target;
    gfxm::vec3 velocity_dir;
public:
    wMissileStateFlying() {
    }

    void onReset() override {
        root = 0;
        collider = 0;
        
    }
    void onActorNodeRegister(type t, ActorNode* component, const std::string& name) override {
        if (component->isRoot()) {
            root = component;
            return;
        }
        if (t == type_get<ColliderNode>() && name == "collider") {
            collider = (ColliderNode*)component;
        }
    }
    void onEnter() override {
        target.x = rand() % 50 - 25;
        target.y = (rand() % 20 + 10);
        target.z = rand() % 50 - 25;

        velocity_dir = -root->getWorldForward();
    }
    void onUpdate(RuntimeWorld* world, Actor* actor, FsmController* fsm, float dt) override {
        if (collider->collider.overlappingColliderCount() > 0) {
            fsm->setState("decay");
            world->postMessage(MSGID_EXPLOSION, MSGPLD_EXPLOSION{ root->getWorldTranslation() });
            return;
        }

        gfxm::aabb box;
        box.from = gfxm::vec3(-50.0f, 0.0f, -50.0f);
        box.to = gfxm::vec3(50.0f, 50.0f, 50.0f);
        if (!gfxm::point_in_aabb(box, root->getWorldTranslation())) {
            fsm->setState("decay");            
            world->postMessage(MSGID_EXPLOSION, MSGPLD_EXPLOSION{ root->getWorldTranslation() });
            return;
        }

        //gfxm::vec3 N_to_target = gfxm::normalize(target - root->getWorldTranslation());
        //velocity_dir += N_to_target * dt * 5.0f;
        //velocity_dir = (velocity_dir);

        // Quake 3 rocket speed (900 units per sec)
        // 64 quake3 units is approx. 1.7 meters
        root->translate(velocity_dir * dt * 23.90625f);
        //root->lookAtDir(-velocity_dir);
    }
};
class wMissileStateDying : public ctrlFsmState {
    SoundEmitterNode* sound_component = 0;
    std::vector<ParticleEmitterNode*> particle_emitters;
public:
    void onReset() override {
        sound_component = 0;
        particle_emitters.clear();
    }
    void onActorNodeRegister(type t, ActorNode* component, const std::string& name) override {
        if (t == type_get<SoundEmitterNode>() && name == "sound") {
            sound_component = (SoundEmitterNode*)component;
        } else if(t == type_get<ParticleEmitterNode>()) {
            particle_emitters.push_back((ParticleEmitterNode*)component);
        }
    }
    void onEnter() override {
        if (sound_component) {
            sound_component->stop();
        }
        for (int i = 0; i < particle_emitters.size(); ++i) {
            auto pe = particle_emitters[i];
            // TODO:
            pe->setAlive(false);
        }
    }
    void onUpdate(RuntimeWorld* world, Actor* actor, FsmController* fsm, float dt) override {
        bool can_despawn = true;
        for (int i = 0; i < particle_emitters.size(); ++i) {
            auto pe = particle_emitters[i];
            if (pe->isAlive()) {
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
[[cppi_class]];
class MissileActor : public Actor {
public:
    TYPE_ENABLE();
    MissileActor() {
        auto root = setRoot<SkeletalModelNode>("model");
        root->setModel(getSkeletalModel("models/rocket/rocket.skeletal_model"));
        auto snd = root->createChild<SoundEmitterNode>("sound");
        snd->setClip(getAudioClip("audio/sfx/rocket_loop.ogg"));
        auto ptcl = root->createChild<ParticleEmitterNode>("particles");
        ptcl->setTranslation(gfxm::vec3(0, 0, 0.3f));
        ptcl->setEmitter(resGet<ParticleEmitterMaster>("particle_emitters/rocket_trail.pte"));
        /*
        curve<float> emit_curve;
        emit_curve[.0f] = 128.0f;
        ptcl->emitter.setParticlePerSecondCurve(emit_curve);
        */
        auto collider = root->createChild<ColliderNode>("collider");
        collider->collider.collision_group
            = COLLISION_LAYER_PROJECTILE;
        collider->collider.collision_mask
            = COLLISION_LAYER_DEFAULT;

        auto fsm = addController<FsmController>();
        fsm->addState("fly", new wMissileStateFlying);
        fsm->addState("decay", new wMissileStateDying);

        setFlags(ACTOR_FLAG_UPDATE);
    }
    void onUpdate(RuntimeWorld* world, float dt) override {}
    void onUpdateDecay(RuntimeWorld* world, float dt) override {}
    void onDecay(RuntimeWorld* world) override {}

    void onSpawn(RuntimeWorld* world) override {}
    void onDespawn(RuntimeWorld* world) override {}
};

#include "particle_emitter/particle_emitter.hpp"
class actorExplosion : public Actor {
    RHSHARED<ParticleEmitterMaster> emitter;
    ParticleEmitterInstance* emitter_inst = 0;
public:
    TYPE_ENABLE();
    actorExplosion() {
        setFlags(ACTOR_FLAG_UPDATE);

        emitter = resGet<ParticleEmitterMaster>("particle_emitters/explosion.pte");/*
        auto shape = emitter.setShape<SphereParticleEmitterShape>();
        shape->radius = 1.5f;
        shape->emit_mode = EMIT_MODE::VOLUME;
        //emitter.addComponent<ptclAngularVelocityComponent>();
        emitter.looping = false;
        emitter.duration = 5.5f / 60.0f;

        curve<float> emit_curve;
        emit_curve[.0f] = 512.0f;
        emitter.setParticlePerSecondCurve(emit_curve);
        curve<float> scale_curve;
        scale_curve[.0f] = 1.0f;
        scale_curve[.1f] = 1.2f;
        scale_curve[1.0f] = 0.f;
        emitter.setScaleOverLifetimeCurve(scale_curve);*//*
        curve<gfxm::vec4> rgba_curve;
        rgba_curve[.0f] = gfxm::vec4(1, 0.65f, 0, 1);
        rgba_curve[1.f] = gfxm::vec4(.1f, .1f, 0.1f, .0f);
        emitter.setRGBACurve(rgba_curve);*/

        emitter_inst = emitter->createInstance();
    }
    void onUpdate(RuntimeWorld* world, float dt) override {
        world->decayActor(this);
    }
    void onUpdateDecay(RuntimeWorld* world, float dt) override {
        emitter_inst->setWorldTransform(getWorldTransform());
        ptclUpdateEmit(dt, emitter_inst);
        ptclUpdate(dt, emitter_inst);
    }
    void onDecay(RuntimeWorld* world) override {
        // ?
    }
    bool hasDecayed() const override {
        return !emitter_inst->isAlive();
    }
    void onSpawn(RuntimeWorld* world) override {
        emitter_inst->reset();

        emitter_inst->spawn(world->getRenderScene());
    }
    void onDespawn(RuntimeWorld* world) override {
        emitter_inst->despawn(world->getRenderScene());
    }
};


class wExplosionSystem : public WorldSystem {
    RHSHARED<AudioClip> clip_explosion;
public:
    wExplosionSystem() {
        clip_explosion = resGet<AudioClip>("audio/sfx/rocklx1a.ogg");
    }

    void onMessage(RuntimeWorld* world, const MSG_MESSAGE& msg) override {
        switch (msg.id) {
        case MSGID_EXPLOSION:
            audioPlayOnce3d(
                clip_explosion->getBuffer(),
                msg.getPayload<MSGPLD_EXPLOSION>()->translation, 0.3f
            );
            world->spawnActorTransient<actorExplosion>()
                ->setTranslation(msg.getPayload<MSGPLD_EXPLOSION>()->translation);
            //LOG_DBG("Explosion!");
            break;
        };
    }
    void onUpdate(RuntimeWorld* world, float dt) override {

    }
};
class wMissileSystem : public WorldSystem {
    RHSHARED<AudioClip> clip_launch;
public:
    wMissileSystem() {
        clip_launch = resGet<AudioClip>("audio/sfx/rocket_launch.ogg");
    }
    void onMessage(RuntimeWorld* world, const MSG_MESSAGE& msg) override {
        switch (msg.id) {
        case MSGID_MISSILE_SPAWN: {
            auto pld = msg.getPayload<MSGPLD_MISSILE_SPAWN>();
            auto missile = world->spawnActorTransient<MissileActor>();
            missile->getRoot()->setTranslation(pld->translation);
            missile->getRoot()->setRotation(pld->orientation);
            audioPlayOnce3d(clip_launch->getBuffer(), pld->translation, .3f);
            } break;
        }
    }
    void onUpdate(RuntimeWorld* world, float dt) override {

    }
};



void missileInit(RuntimeWorld* world, int pool_size);
void missileCleanup();

void missileSpawnOne(const gfxm::vec3& pos, const gfxm::vec3& dir);

void missileUpdate(float dt);