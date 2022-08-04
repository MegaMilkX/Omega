#pragma once

#include "game/actor/actor.hpp"

#include "assimp_load_scene.hpp"
#include "animation/animation.hpp"
#include "collision/collision_world.hpp"
#include "gpu/gpu.hpp"
#include "gpu/render/uniform.hpp"
#include "animation/animation_sample_buffer.hpp"
#include "animation/animation_sampler.hpp"
#include "game/world/world.hpp"

#include "skeletal_model/skeletal_model.hpp"
#include "skeletal_model/skeletal_model_instance.hpp"

#include "animation/animator/animator.hpp"


class actorAnimTest : public wActor {
    HSHARED<sklmSkeletalModelInstance> model_inst;
    AnimatorEd animator;
    HSHARED<animAnimatorSequence> seq_idle;
    HSHARED<animAnimatorSequence> seq_run2;
public:
    actorAnimTest() {
        {
            expr_ e = (param_(&animator, "velocity") + 10.0f) % 9.f;
            e.dbgLog(&animator);
            value_ v = e.evaluate(&animator);
            v.dbgLog();
        }

        auto model = resGet<sklmSkeletalModelEditable>("models/chara_24/chara_24.skeletal_model");
        model_inst = model->createInstance();
        model_inst->getSkeletonInstance()->getWorldTransformsPtr()[0] = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(4, 0, 0));
        
        {
            seq_idle.reset_acquire();
            seq_run2.reset_acquire();
            seq_idle->setSkeletalAnimation(resGet<Animation>("models/chara_24/Idle.animation"));
            seq_run2->setSkeletalAnimation(resGet<Animation>("models/chara_24/Run2.animation"));

            animator.setSkeleton(model->getSkeleton());
            animator.addParam("velocity");
            animator.addParam("test");

            auto sync_def = animator.createSyncGroup("Default");
            auto smp_idle = sync_def->addSampler();
            auto smp_run2 = sync_def->addSampler();
            smp_idle->setSequence(seq_idle);
            smp_run2->setSequence(seq_run2);

            /*
            auto bt = animator.setRoot<animUnitBlendTree>();
            auto node_clip = bt->addNode<animBtNodeClip>();
            auto node_clip2 = bt->addNode<animBtNodeClip>();
            auto node_clip3 = bt->addNode<animBtNodeClip>();
            auto node_blend2 = bt->addNode<animBtNodeBlend2>();
            node_clip->setAnimation(resGet<Animation>("models/chara_24/Idle.animation"));
            node_clip2->setAnimation(resGet<Animation>("models/chara_24/Walk.animation"));
            node_clip3->setAnimation(resGet<Animation>("models/chara_24/Run2.animation"));

            node_blend2->setInputs(node_clip, node_clip3);
            node_blend2->setWeightExpression(
                abs_(1.0f - param_(&animator, "velocity") % 2.0f)
            );
            bt->setOutputNode(node_blend2);
            */
            
            auto fsm = animator.setRoot<animUnitFsm>();
            
            auto state = fsm->addState("stateA");
            auto single = state->setUnit<animUnitSingle>();
            single->setSampler(smp_idle);

            auto state2 = fsm->addState("stateB");
            auto single2 = state2->setUnit<animUnitSingle>();
            single2->setSampler(smp_run2);

            fsm->addTransition(
                "stateA", "stateB",
                state_complete_(), 0.2f
            );
            fsm->addTransition(
                "stateB", "stateA",
                state_complete_(), 0.2f
            );

            animator.compile();
        }
    }

    void onSpawn(wWorld* world) override {
        model_inst->onSpawn(world->getRenderScene());
    }
    void onDespawn(wWorld* world) override {
        model_inst->onDespawn(world->getRenderScene());
    }

    void update(float dt) {
        static float velocity = .0f;
        velocity += dt;
        animator.setParamValue(animator.getParamId("velocity"), velocity);

        animator.update(dt);
        animator.getSampleBuffer()->applySamples(model_inst->getSkeletonInstance());

        auto& rm_sample = animator.getSampleBuffer()->getRootMotionSample();
        /*
        model_inst->getSkeletonInstance()->getWorldTransformsPtr()[0]
            = gfxm::translate(
                model_inst->getSkeletonInstance()->getWorldTransformsPtr()[0],
                rm_sample.t
            );*/
        // TODO: Rotation
    }
};

#include "animation/hitbox_sequence/hitbox_seq_sample_buffer.hpp"
#include "animation/hitbox_sequence/hitbox_sequence.hpp"

#include "animation/animator/animator_instance.hpp"

class actorUltimaWeapon : public wActor {
    HSHARED<sklmSkeletalModelInstance> model_inst;
    AnimatorEd animator;
    animAnimatorInstance anim_inst;
    
    wWorld* world = 0;

    RHSHARED<hitboxSequence> hitbox_seq;
    hitboxSeqSampleBuffer hitbox_sample_buf;
    RHSHARED<animAnimatorSequence> seq_idle;
public:
    actorUltimaWeapon() {
        auto model = resGet<sklmSkeletalModelEditable>("models/ultima_weapon/ultima_weapon.skeletal_model");
        model_inst = model->createInstance();
        model_inst->getSkeletonInstance()->getWorldTransformsPtr()[0]
            = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(6, 0, -6))
            * gfxm::scale(gfxm::mat4(1.0f), gfxm::vec3(2, 2, 2));

        seq_idle.reset(HANDLE_MGR<animAnimatorSequence>::acquire());
        seq_idle->setSkeletalAnimation(resGet<Animation>("models/ultima_weapon/Idle.animation"));
        seq_idle->addGenericTimeline(hitbox_seq);
        

        animator.setSkeleton(model->getSkeleton());

        auto sync_grp = animator.createSyncGroup("Default");
        auto sampler_idle = sync_grp->addSampler();
        sampler_idle->setSequence(seq_idle);

        auto single = animator.setRoot<animUnitSingle>();
        single->setSampler(sampler_idle);
        animator.compile();

        hitbox_seq.reset(HANDLE_MGR<hitboxSequence>::acquire());
        hitbox_seq->fps = 60.0f;
        hitbox_seq->length = 55.0f;
        auto trk = hitbox_seq->addTrack();
        auto trk2 = hitbox_seq->addTrack();

        static std::unique_ptr<hitboxSeqClip> cl(new hitboxSeqClip);
        static std::unique_ptr<hitboxSeqClip> cl2(new hitboxSeqClip);
        cl->length = 40.0f;
        cl->bone_id = model->getSkeleton()->findBone("j_hand_r")->getIndex();
        cl->keep_following_bone = false;
        cl->translation[.0f] = gfxm::vec3(.1f, 0, 0 - .6f);
        cl->translation[10.0f] = gfxm::vec3(.1f, 0, .6f - .6f);
        cl->translation[20.f] = gfxm::vec3(.1f, 0, -0.6f - .6f);
        cl->translation[24.f] = gfxm::vec3(.1f, 0, -.3 - .6f);
        cl->radius[.0f] = 0.05f;
        cl->radius[15.f] = 0.4f;
        cl->radius[20.f] = 1.0f;
        cl->radius[28.f] = 1.5f;
        cl->radius[32.f] = 1.4f;

        cl2->length = 10.0f;
        cl2->bone_id = model->getSkeleton()->findBone("j_hand_l")->getIndex();
        cl2->keep_following_bone = false;
        cl2->radius[.0f] = 1.0f;

        trk->insert(15, cl.get());
        trk2->insert(0, cl2.get());

        hitbox_sample_buf.init(hitbox_seq.get());

        anim_inst.init(&animator, model_inst->getSkeletonInstance());
    }
    void onSpawn(wWorld* world) override {
        model_inst->onSpawn(world->getRenderScene());
        this->world = world;
    }
    void onDespawn(wWorld* world) override {
        model_inst->onDespawn(world->getRenderScene());
        this->world = 0;
    }
    void update(float dt) {
        animator.update(dt);
        animator.getSampleBuffer()->applySamples(model_inst->getSkeletonInstance());

        static float cur = .0f;
        static float cur_prev = .0f;
        int sample_count = hitbox_seq->sample(model_inst->getSkeletonInstance(), hitbox_sample_buf.data(), hitbox_sample_buf.count(), cur_prev, cur);
        cur_prev = cur;
        cur += hitbox_seq->fps * dt;
        if (cur > hitbox_seq->length) {
            cur -= hitbox_seq->length;
        }

        for (int i = 0; i < sample_count; ++i) {
            auto& s = hitbox_sample_buf[i];
            gfxm::mat4 tr =
                gfxm::translate(gfxm::mat4(1.0f), s.translation);
            if (s.type == HITBOX_SEQ_CLIP_EMPTY) {
                continue;
            }
            world->getCollisionWorld()->castSphere(tr, s.radius);
        }
    }
};

class Door : public wActor {
    RHSHARED<sklmSkeletalModelEditable> model;
    HSHARED<sklmSkeletalModelInstance> model_inst;

    HSHARED<Animation>  anim_open;
    animSampler    anim_sampler;
    animSampleBuffer    samples;

    CollisionSphereShape     shape_sphere;
    Collider                 collider_beacon;

    bool is_opening = false;
    float anim_cursor = .0f;
public:
    Actor ref_point_front;
    Actor ref_point_back;

    Door() {
        model = resGet<sklmSkeletalModelEditable>("models/door/door.skeletal_model");
        model_inst = model->createInstance();

        anim_open = resGet<Animation>("models/door/Open.animation");
        anim_sampler = animSampler(model->getSkeleton().get(), anim_open.get());
        samples.init(model->getSkeleton().get());

        setTranslation(gfxm::vec3(1, 0, 6.0f));

        model_inst->getSkeletonInstance()->getWorldTransformsPtr()[0]
            = getWorldTransform();

        shape_sphere.radius = 0.1f;
        collider_beacon.setShape(&shape_sphere);        
        collider_beacon.position = getTranslation();
        collider_beacon.setUserPtr(this);

        // Ref points for the character to adjust to for door opening animations
        gfxm::vec3 door_pos = getTranslation();
        door_pos.y = .0f;
        ref_point_front.setTranslation(door_pos + gfxm::vec3(0, 0, 1));
        ref_point_front.setRotation(gfxm::angle_axis(gfxm::pi, gfxm::vec3(0, 1, 0)));
        ref_point_back.setTranslation(door_pos + gfxm::vec3(0, 0, -1));
    }

    void onSpawn(wWorld* world) override {
        model_inst->onSpawn(world->getRenderScene());

        world->getCollisionWorld()->addCollider(&collider_beacon);
    }
    void onDespawn(wWorld* world) override {
        model_inst->onDespawn(world->getRenderScene());

        world->getCollisionWorld()->removeCollider(&collider_beacon);
    }

    void update(float dt) {
        collider_beacon.position = getTranslation() + gfxm::vec3(.0f, 1.0f, .0f) + getLeft() * .5f;
        if (is_opening) {            
            anim_sampler.sample(samples.data(), samples.count(), anim_cursor * anim_open->fps);
            
            for (int i = 1; i < samples.count(); ++i) {
                auto& s = samples[i];
                gfxm::mat4 m = gfxm::translate(gfxm::mat4(1.0f), s.t)
                    * gfxm::to_mat4(s.r)
                    * gfxm::scale(gfxm::mat4(1.0f), s.s);
                model_inst->getSkeletonInstance()->getLocalTransformsPtr()[i] = m;
            }
            anim_cursor += dt;
            if (anim_cursor >= anim_open->length) {
                is_opening = false;
            }
        }
    }

    wRsp onMessage(wMsg msg) override {
        if (auto m = wMsgTranslate<wMsgDoorOpen>(msg)) {
            LOG_DBG("Door: Open message received");
            is_opening = true;
            anim_cursor = .0f;
            gfxm::vec3 door_pos = getTranslation();
            gfxm::vec3 initiator_pos = m->initiator->getTranslation();
            gfxm::vec3 initiator_N = gfxm::normalize(initiator_pos - door_pos);
            gfxm::vec3 front_N = gfxm::normalize(getWorldTransform() * gfxm::vec3(0, 0, -1));
            float d = gfxm::dot(initiator_N, front_N);
            int sync_bone_id = -1;
            if (d > .0f) {
                sync_bone_id = model->getSkeleton()->findBone("SYNC_Front")->getIndex();
            } else {
                sync_bone_id = model->getSkeleton()->findBone("SYNC_Back")->getIndex();
            }
            gfxm::vec3 sync_pos = model_inst->getSkeletonInstance()->getWorldTransformsPtr()[sync_bone_id] * gfxm::vec4(0, 0, 0, 1);
            gfxm::quat sync_rot = gfxm::to_quat(gfxm::to_orient_mat3(model_inst->getSkeletonInstance()->getWorldTransformsPtr()[sync_bone_id]));
            return wRspMake(
                wRspDoorOpen{ sync_pos, sync_rot, d > .0f }
            );
        } else {
            LOG_DBG("Door: unknown message received");
        }
        return 0;
    }
};


struct ColliderData {
    Actor*          actor;
    gfxm::vec3      offset;
    CollisionShape* shape;
    Collider*       collider;
};
struct ColliderProbeData {
    Actor*          actor;
    gfxm::vec3      offset;
    CollisionShape* shape;
    ColliderProbe*  collider_probe;
};

enum class CHARACTER_STATE {
    LOCOMOTION,
    DOOR_OPEN
};


#include "import/assimp_load_skeletal_model.hpp"
class actorCharacter : public wActor {
    struct {
        HSHARED<sklmSkeletalModelInstance>  model_inst;
    };

    std::unique_ptr<scnDecal> decal;
    std::unique_ptr<scnTextBillboard> name_caption;
    scnNode caption_node;
    // TEXT STUFF, MUST BE SHARED
    Typeface typeface;
    std::unique_ptr<Font> font;

    // New Anim
    AnimatorEd animator;
    RHSHARED<animAnimatorSequence> seq_idle;
    RHSHARED<animAnimatorSequence> seq_run2;
    RHSHARED<animAnimatorSequence> seq_open_door_front;
    RHSHARED<animAnimatorSequence> seq_open_door_back;

    // Gameplay
    wActor* targeted_actor = 0;
    CHARACTER_STATE state = CHARACTER_STATE::LOCOMOTION;

    gfxm::vec3 forward_vec = gfxm::vec3(0, 0, 1);
    gfxm::vec3 loco_vec_tgt;
    gfxm::vec3 loco_vec;
    float velocity = .0f;

    // Collision
    CollisionCapsuleShape    shape_capsule;
    Collider                 collider;
    CollisionSphereShape     shape_sphere;
    ColliderProbe            collider_probe;
public:
    actorCharacter() {
        auto model = resGet<sklmSkeletalModelEditable>("models/chara_24/chara_24.skeletal_model");
        {
            model_inst = model->createInstance();
        }

        decal.reset(new scnDecal);
        decal->setTexture(resGet<gpuTexture2d>("images/character_selection_decal.png"));
        decal->setBoxSize(1.3f, 1.0f, 1.3f);
        decal->setBlending(GPU_BLEND_MODE::NORMAL);
        decal->setSkeletonNode(model_inst->getSkeletonInstance()->getScnSkeleton(), 0);

        typefaceLoad(&typeface, "OpenSans-Regular.ttf");
        font.reset(new Font(&typeface, 16, 72));
        name_caption.reset(new scnTextBillboard(font.get()));
        name_caption->setSkeletonNode(model_inst->getSkeletonInstance()->getScnSkeleton(), 16);
        caption_node.local_transform = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(.0f, 1.9f, .0f));
        caption_node.attachToSkeleton(model_inst->getSkeletonInstance()->getScnSkeleton(), 0);
        name_caption->setNode(&caption_node);

        // Animator
        {
            seq_idle.reset_acquire();
            seq_run2.reset_acquire();
            seq_open_door_front.reset_acquire();
            seq_open_door_back.reset_acquire();
            seq_idle->setSkeletalAnimation(resGet<Animation>("models/chara_24/Idle.animation"));
            seq_run2->setSkeletalAnimation(resGet<Animation>("models/chara_24/Run2.animation"));
            seq_open_door_front->setSkeletalAnimation(resGet<Animation>("models/chara_24_anim_door/Action_OpenDoor.animation"));
            seq_open_door_back->setSkeletalAnimation(resGet<Animation>("models/chara_24/Action_DoorOpenBack.animation"));

            animator.setSkeleton(model->getSkeleton());
            // Setup parameters signals and events
            animator.addParam("velocity");
            animator.addSignal("sig_door_open");
            animator.addSignal("sig_door_open_back");
            animator.addFeedbackEvent("fevt_door_open_end");

            // Setup samplers
            auto sync_def = animator.createSyncGroup("Default");
            auto sync_loco = animator.createSyncGroup("Locomotion");
            auto sync_interact = animator.createSyncGroup("Interact");
            auto smp_idle = sync_def->addSampler();
            auto smp_run2 = sync_loco->addSampler();
            auto smp_door_open_front = sync_interact->addSampler();
            auto smp_door_open_back = sync_interact->addSampler();
            smp_idle->setSequence(seq_idle);
            smp_run2->setSequence(seq_run2);
            smp_door_open_front->setSequence(seq_open_door_front);
            smp_door_open_back->setSequence(seq_open_door_back);

            // Setup the tree
            auto fsm = animator.setRoot<animUnitFsm>();
            auto st_idle = fsm->addState("Idle");
            auto st_loco = fsm->addState("Locomotion");
            auto st_door_open_front = fsm->addState("DoorOpenFront");
            auto st_door_open_back = fsm->addState("DoorOpenBack");
            st_idle->setUnit<animUnitSingle>()->setSampler(smp_idle);
            st_loco->setUnit<animUnitSingle>()->setSampler(smp_run2);
            st_door_open_front->setUnit<animUnitSingle>()->setSampler(smp_door_open_front);
            st_door_open_front->onExit(call_feedback_event_(&animator, "fevt_door_open_end"));
            st_door_open_back->setUnit<animUnitSingle>()->setSampler(smp_door_open_back);
            st_door_open_back->onExit(call_feedback_event_(&animator, "fevt_door_open_end"));
            fsm->addTransition("Idle", "Locomotion", param_(&animator, "velocity") > FLT_EPSILON, 0.15f);
            fsm->addTransition("Locomotion", "Idle", param_(&animator, "velocity") <= FLT_EPSILON, 0.15f);
            fsm->addTransitionAnySource("DoorOpenFront", signal_(&animator, "sig_door_open"), 0.15f);
            fsm->addTransitionAnySource("DoorOpenBack", signal_(&animator, "sig_door_open_back"), 0.15f);
            fsm->addTransition("DoorOpenFront", "Idle", state_complete_(), 0.15f);
            fsm->addTransition("DoorOpenBack", "Idle", state_complete_(), 0.15f);
            animator.compile();
        }
        // Collision
        shape_capsule.radius = 0.3f;
        collider.setShape(&shape_capsule);        

        shape_sphere.radius = 0.85f;
        collider_probe.setShape(&shape_sphere);        
    }

    void setDesiredLocomotionVector(const gfxm::vec3& loco) {
        float len = loco.length();
        gfxm::vec3 norm = len > 1.0f ? gfxm::normalize(loco) : loco;
        if (loco.length() > FLT_EPSILON) {
            forward_vec = gfxm::normalize(loco);
        }
        loco_vec_tgt = norm;
    }
    void actionUse() {
        if (targeted_actor) {
            wRsp rsp = targeted_actor->sendMessage(wMsgMake(wMsgDoorOpen{ this }));
            const wRspDoorOpen* trsp = wRspTranslate<wRspDoorOpen>(rsp);
            
            setTranslation(trsp->sync_pos);
            setRotation(trsp->sync_rot);

            if (trsp->is_front) {
                animator.triggerSignal(animator.getSignalId("sig_door_open"));
            } else {
                animator.triggerSignal(animator.getSignalId("sig_door_open_back"));
            }
            state = CHARACTER_STATE::DOOR_OPEN;
            velocity = .0f;
            loco_vec = gfxm::vec3(0, 0, 0);
            /*
            Door* door = dynamic_cast<Door*>(targeted_actor);
            if (door) {
                // Pick a starting point for door opening animation
                gfxm::vec3 door_pos = door->getWorldTransform() * gfxm::vec4(0, 0, 0, 1);
                gfxm::vec3 char_pos = getWorldTransform() * gfxm::vec4(0, 0, 0, 1);
                gfxm::vec3 door_char_norm = gfxm::normalize(char_pos - door_pos);
                gfxm::vec3 door_norm = gfxm::normalize(door->getWorldTransform() * gfxm::vec4(0, 0, -1, 0));
                float dot = gfxm::dot(door_norm, door_char_norm);
                if (dot > .0f) {
                    setTranslation(door->ref_point_back.getTranslation());
                    setRotation(door->ref_point_back.getRotation());
                } else {
                    setTranslation(door->ref_point_front.getTranslation());
                    setRotation(door->ref_point_front.getRotation());
                }

                animator.context.evtDoorOpen = true;
                state = CHARACTER_STATE::DOOR_OPEN;
                velocity = .0f;
                loco_vec = gfxm::vec3(0, 0, 0);
                LOG_WARN("Activated a door!");
            }*/
        }
    }

    void update_locomotion(float dt) {
        // Handle input
        velocity = loco_vec_tgt.length();// = gfxm::lerp(velocity, loco_vec.length(), 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f));
        loco_vec = gfxm::lerp(loco_vec, loco_vec_tgt, 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f));
        if (velocity > FLT_EPSILON) {
            translate(loco_vec * dt * 5.0f);

            gfxm::mat3 orient;
            orient[2] = forward_vec;
            orient[1] = gfxm::vec3(0, 1, 0);
            orient[0] = gfxm::cross(orient[1], orient[2]);
            setRotation(gfxm::slerp(getRotation(), gfxm::to_quat(orient), 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f)));
        }

        // Choose an actionable object if there are any available
        for (int i = 0; i < collider_probe.overlappingColliderCount(); ++i) {
            Collider* other = collider_probe.getOverlappingCollider(i);
            void* user_ptr = other->getUserPtr();
            if (user_ptr) {
                targeted_actor = (wActor*)user_ptr;
                break;
            }
        }
    }
    void update_doorOpen(float dt) {
        if(animator.isFeedbackEventTriggered(animator.getFeedbackEventId("fevt_door_open_end"))) {
            state = CHARACTER_STATE::LOCOMOTION;
            forward_vec = getWorldTransform() * gfxm::vec4(0, 0, 1, 0);
        }
    }
    void update(float dt) {
        // Clear stuff
        targeted_actor = 0;

        switch (state) {
        case CHARACTER_STATE::LOCOMOTION:
            update_locomotion(dt);
            break;
        case CHARACTER_STATE::DOOR_OPEN:
            update_doorOpen(dt);
            break;
        }

        // Apply animations and skinning
        animator.setParamValue(animator.getParamId("velocity"), velocity);
        animator.update(dt);
        animator.getSampleBuffer()->applySamples(model_inst->getSkeletonInstance());

        // Apply root motion
        
        gfxm::vec3 rm_t = gfxm::vec3(getWorldTransform() * gfxm::vec4(animator.getSampleBuffer()->getRootMotionSample().t, .0f));
        rm_t.y = .0f;
        translate(rm_t);
        rotate(animator.getSampleBuffer()->getRootMotionSample().r);
        

        // Update transforms
        model_inst->getSkeletonInstance()->getWorldTransformsPtr()[0] = getWorldTransform();

        collider.position = getTranslation() + gfxm::vec3(0, 1.0f, 0);
        collider.rotation = getRotation();
        collider_probe.position = getWorldTransform() * gfxm::vec4(0, 0.5f, 0.64f, 1.0f);
        collider_probe.rotation = gfxm::to_quat(gfxm::to_orient_mat3(getWorldTransform()));
    }

    void onSpawn(wWorld* world) override {
        model_inst->onSpawn(world->getRenderScene());

        world->getRenderScene()->addRenderObject(decal.get());

        world->getRenderScene()->addNode(&caption_node);
        world->getRenderScene()->addRenderObject(name_caption.get());

        world->getCollisionWorld()->addCollider(&collider);
        world->getCollisionWorld()->addCollider(&collider_probe);
    }
    void onDespawn(wWorld* world) override {
        model_inst->onDespawn(world->getRenderScene());

        world->getRenderScene()->removeRenderObject(decal.get());

        world->getRenderScene()->removeNode(&caption_node);
        world->getRenderScene()->removeRenderObject(name_caption.get());

        world->getCollisionWorld()->removeCollider(&collider);
        world->getCollisionWorld()->removeCollider(&collider_probe);
    }
};