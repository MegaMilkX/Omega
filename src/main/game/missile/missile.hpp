#pragma once

#include "world/controller/actor_controllers.hpp"

#include "world/node/node_collider.hpp"
#include "world/node/node_sound_emitter.hpp"
#include "world/node/node_particle_emitter.hpp"
#include "world/node/node_skeletal_model.hpp"


class fsmCharacterStateLocomotion : public ctrlFsmState {
    InputRange* rangeTranslation = 0;
    InputAction* actionInteract = 0;

    AnimatorComponent* anim_component = 0;

    const float TURN_LERP_SPEED = 0.995f;
    float velocity = .0f;
    gfxm::vec3 desired_dir = gfxm::vec3(0, 0, 1);

    bool is_grounded = true;
    gfxm::vec3 grav_velo;
public:
    void onReset() override {
        rangeTranslation = inputGetRange("CharacterLocomotion");
        actionInteract = inputGetAction("CharacterInteract");
    }
    void onActorNodeRegister(type t, gameActorNode* node, const std::string& name) override {}
    bool onSpawn(gameActor* actor) override { 
        anim_component = actor->getComponent<AnimatorComponent>();
        if (!anim_component) {
            return false;
        }
        return true;
    }
    void onDespawn(gameActor* actor) override {}
    void onEnter() override {
        // TODO
    }
    void onUpdate(gameWorld* world, gameActor* actor, ctrlFsm* fsm, float dt) override {
        auto cam_node = world->getCurrentCameraNode();
        auto root = actor->getRoot();

        bool has_dir_input = rangeTranslation->getVec3().length() > FLT_EPSILON;
        gfxm::vec3 input_dir = gfxm::normalize(rangeTranslation->getVec3());

        // Ground raytest
        if (!is_grounded) {
            root->translate(grav_velo * dt);
        }
        {
            RayCastResult r = world->getCollisionWorld()->rayTest(
                root->getTranslation() + gfxm::vec3(.0f, .3f, .0f),
                root->getTranslation() - gfxm::vec3(.0f, .35f, .0f),
                COLLISION_LAYER_DEFAULT
            );
            if (r.hasHit) {
                gfxm::vec3 pos = root->getTranslation();
                float y_offset = r.position.y - pos.y;
                root->translate(gfxm::vec3(.0f, y_offset, .0f));
                is_grounded = true;
                grav_velo = gfxm::vec3(0, 0, 0);
            } else {
                is_grounded = false;
                grav_velo -= gfxm::vec3(.0f, 9.8f * dt, .0f);
                // 53m/s is the maximum approximate terminal velocity for a human body
                grav_velo.y = gfxm::_min(53.f, grav_velo.y);
            }
        }
        
        if(is_grounded) {
            if (has_dir_input) {
                desired_dir = input_dir;
            }

            if (input_dir.length() > velocity) {
                velocity = gfxm::lerp(velocity, input_dir.length(), 1 - pow(1.f - .995f, dt));
            } else if(!has_dir_input) {
                velocity = .0f;
            }
        } else {
            velocity += -velocity * dt;
        }

        if (velocity > FLT_EPSILON) {
            gfxm::mat4 trs(1.0f);
            if (cam_node) {
                trs = cam_node->getWorldTransform();
            }
            gfxm::mat3 orient;
            gfxm::vec3 fwd = trs * gfxm::vec4(0, 0, 1, 0);
            fwd.y = .0f;
            fwd = gfxm::normalize(fwd);
            orient[2] = fwd;
            orient[1] = gfxm::vec3(0, 1, 0);
            orient[0] = gfxm::cross(orient[1], orient[2]);
            gfxm::vec3 loco_vec = orient * desired_dir;

            orient[2] = loco_vec;
            orient[1] = gfxm::vec3(0, 1, 0);
            orient[0] = gfxm::cross(orient[1], orient[2]);
            gfxm::quat tgt_rot = gfxm::to_quat(orient);

            float dot = fabsf(gfxm::dot(root->getRotation(), tgt_rot));
            float angle = 2.f * acosf(gfxm::_min(dot, 1.f));
            float slerp_fix = (1.f - angle / gfxm::pi) * (1.0f - TURN_LERP_SPEED);

            gfxm::quat cur_rot = gfxm::slerp(root->getRotation(), tgt_rot, 1 - pow(slerp_fix, dt));
            root->setRotation(cur_rot);
            root->translate((gfxm::to_mat4(cur_rot) * gfxm::vec3(0,0,1)) * dt * 5.f * velocity);
        }

        if (anim_component) {
            auto anim_inst = anim_component->getAnimatorInstance();
            auto anim_master = anim_component->getAnimatorMaster();
            anim_inst->setParamValue(anim_master->getParamId("velocity"), input_dir.length());
            anim_inst->setParamValue(anim_master->getParamId("is_falling"), is_grounded ? .0f : 1.f);
        }
        if (actionInteract->isJustPressed()) {
            if (anim_component) {
                auto anim_inst = anim_component->getAnimatorInstance();
                auto anim_master = anim_component->getAnimatorMaster();
                anim_inst->triggerSignal(anim_master->getSignalId("sig_door_open"));
                fsm->setState("interacting");
            }
        }
    }
};
class fsmCharacterStateInteracting : public ctrlFsmState {
    AnimatorComponent* anim_component = 0;
public:
    void onReset() override {}
    void onActorNodeRegister(type t, gameActorNode* node, const std::string& name) override {}
    void onEnter() override {
        // TODO
        // velocity = .0f;
        // loco_vec = gfxm::vec3(.0f, .0f, .0f);
    }
    bool onSpawn(gameActor* actor) override {
        anim_component = actor->getComponent<AnimatorComponent>();
        if (!anim_component) {
            return false;
        }
        return true;
    }
    void onUpdate(gameWorld* world, gameActor* actor, ctrlFsm* fsm, float dt) override {
        if (anim_component) {
            auto anim_inst = anim_component->getAnimatorInstance();
            auto anim_master = anim_component->getAnimatorMaster();
            if (anim_inst->isFeedbackEventTriggered(anim_master->getFeedbackEventId("fevt_door_open_end"))) {
                fsm->setState("locomotion");
            }
        }
    }
};

class wMissileStateFlying : public ctrlFsmState {
    gameActorNode* root = 0;
    nodeCollider* collider = 0;

    gfxm::vec3 target;
    gfxm::vec3 velocity_dir;
public:
    wMissileStateFlying() {
    }

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
    void onEnter() override {
        target.x = rand() % 50 - 25;
        target.y = (rand() % 20 + 10);
        target.z = rand() % 50 - 25;

        velocity_dir = -root->getWorldForward();
    }
    void onUpdate(gameWorld* world, gameActor* actor, ctrlFsm* fsm, float dt) override {
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

        gfxm::vec3 N_to_target = gfxm::normalize(target - root->getWorldTranslation());
        velocity_dir += N_to_target * dt * 5.0f;
        velocity_dir = (velocity_dir);

        // Quake 3 rocket speed (900 units per sec)
        // 64 quake3 units is approx. 1.7 meters
        root->translate(velocity_dir * dt * 23.90625f);
        root->lookAtDir(-velocity_dir);
    }
};
class wMissileStateDying : public ctrlFsmState {
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
    void onUpdate(gameWorld* world, gameActor* actor, ctrlFsm* fsm, float dt) override {
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

        curve<float> emit_curve;
        emit_curve[.0f] = 128.0f;
        ptcl->emitter.setParticlePerSecondCurve(emit_curve);

        auto collider = root->createChild<nodeCollider>("collider");

        auto fsm = addController<ctrlFsm>();
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

#include "particle_emitter/particle_emitter.hpp"
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
            //LOG_DBG("Explosion!");
            break;
        };
    }
    void onUpdate(gameWorld* world, float dt) override {

    }
};
class wMissileSystem : public WorldSystem {
    RHSHARED<AudioClip> clip_launch;
public:
    wMissileSystem() {
        clip_launch = resGet<AudioClip>("audio/sfx/rocket_launch.ogg");
    }
    void onMessage(gameWorld* world, const MSG_MESSAGE& msg) override {
        switch (msg.id) {
        case MSGID_MISSILE_SPAWN: {
            auto pld = msg.getPayload<MSGPLD_MISSILE_SPAWN>();
            auto missile = world->spawnActorTransient<actorMissile>();
            missile->getRoot()->setTranslation(pld->translation);
            missile->getRoot()->setRotation(pld->orientation);
            audio().playOnce3d(clip_launch->getBuffer(), pld->translation, .3f);
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