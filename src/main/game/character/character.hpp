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

#include "util/static_block.hpp"

class actorAnimatedSkeletalModel : public wActor {
    TYPE_ENABLE(wActor);

    RHSHARED<sklmSkeletalModelInstance> model_inst;
    HSHARED<animAnimatorInstance> animator_inst;
public:
    // TODO
};
STATIC_BLOCK{
    type_register<actorAnimatedSkeletalModel>("actorAnimatedSkeletalModel")
        .parent<wActor>();
};

class actorJukebox : public wActor {
    TYPE_ENABLE(wActor);

    RHSHARED<sklmSkeletalModelInstance> model_inst;
    CollisionSphereShape     shape_sphere;
    Collider                 collider_beacon;
    RHSHARED<AudioClip>      audio_clip_click;
    RHSHARED<AudioClip>      audio_clip;
    Handle<AudioChannel>     audio_ch;
public:
    actorJukebox() {
        setFlagsDefault();

        auto model = resGet<sklmSkeletalModelEditable>("models/jukebox_low/jukebox_low.skeletal_model");
        model_inst = model->createInstance();
        model_inst->getSkeletonInstance()->getWorldTransformsPtr()[0]
            = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(-2, 0, 4))
            * gfxm::scale(gfxm::mat4(1.0f), gfxm::vec3(1, 1, 1) * 0.35f);

        shape_sphere.radius = 0.1f;
        collider_beacon.setShape(&shape_sphere);
        collider_beacon.position = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(-2, 0, 4) + gfxm::vec3(0, 1, 0)) * gfxm::vec4(0,0,0,1);
        collider_beacon.setUserPtr(this);

        audio_clip_click = resGet<AudioClip>("audio/sfx/switch_click.ogg");
        audio_clip = resGet<AudioClip>("audio/Cepheid - Goddess (feat. Nonon).ogg");
        audio_ch = audio().createChannel();
        audio().setBuffer(audio_ch, audio_clip->getBuffer());
        audio().setPosition(audio_ch, collider_beacon.position);
        audio().setLooping(audio_ch, true);
    }
    ~actorJukebox() {
        audio().freeChannel(audio_ch);
    }
    void onSpawn(wWorld* world) override {
        model_inst->spawn(world->getRenderScene());

        world->getCollisionWorld()->addCollider(&collider_beacon);
    }
    void onDespawn(wWorld* world) override {
        model_inst->despawn(world->getRenderScene());

        world->getCollisionWorld()->removeCollider(&collider_beacon);
    }
    wRsp onMessage(wMsg msg) override {
        if (auto m = wMsgTranslate<wMsgInteract>(msg)) {
            audio().playOnce(audio_clip_click->getBuffer(), 0.2f);
            if (audio().isPlaying(audio_ch)) {
                audio().stop(audio_ch);
            } else {
                audio().play3d(audio_ch);
            }

            return wRspMake(
                wRspInteractJukebox{}
            );
        } else {
            LOG_DBG("Door: unknown message received");
        }
        return 0;
    }
};
STATIC_BLOCK{
    type_register<actorJukebox>("actorJukebox")
        .parent<wActor>();
};

class actorAnimTest : public wActor {
    TYPE_ENABLE(wActor);

    HSHARED<sklmSkeletalModelInstance> model_inst;
    RHSHARED<AnimatorEd> animator;
    HSHARED<animAnimatorInstance> anim_inst;
    HSHARED<animAnimatorSequence> seq_idle;
    HSHARED<animAnimatorSequence> seq_run2;
public:
    actorAnimTest() {
        setFlags(WACTOR_FLAG_UPDATE);

        {/*
            expr_ e = (param_(&animator, "velocity") + 10.0f) % 9.f;
            e.dbgLog(&animator);
            value_ v = e.evaluate(&animator);
            v.dbgLog();*/
        }

        auto model = resGet<sklmSkeletalModelEditable>("models/chara_24/chara_24.skeletal_model");
        model_inst = model->createInstance();
        model_inst->getSkeletonInstance()->getWorldTransformsPtr()[0] = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(4, 0, 0));
        
        {
            seq_idle.reset_acquire();
            seq_run2.reset_acquire();
            seq_idle->setSkeletalAnimation(resGet<Animation>("models/chara_24/Idle.animation"));
            seq_run2->setSkeletalAnimation(resGet<Animation>("models/chara_24/Run2.animation"));

            animator.reset_acquire();
            animator->setSkeleton(model->getSkeleton());
            animator->addParam("velocity");
            animator->addParam("test");

            animator->addSampler("idle", "Default", seq_idle);
            animator->addSampler("run2", "Default", seq_run2);

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
            
            auto fsm = animator->setRoot<animUnitFsm>();
            
            auto state = fsm->addState("stateA");
            auto single = state->setUnit<animUnitSingle>();
            single->setSampler("idle");

            auto state2 = fsm->addState("stateB");
            auto single2 = state2->setUnit<animUnitSingle>();
            single2->setSampler("run2");

            fsm->addTransition(
                "stateA", "stateB",
                state_complete_(), 0.2f
            );
            fsm->addTransition(
                "stateB", "stateA",
                state_complete_(), 0.2f
            );

            animator->compile();

            anim_inst = animator->createInstance(model_inst->getSkeletonInstance());
        }
    }

    void onSpawn(wWorld* world) override {
        model_inst->spawn(world->getRenderScene());
    }
    void onDespawn(wWorld* world) override {
        model_inst->despawn(world->getRenderScene());
    }

    void onUpdate(wWorld* world, float dt) override {
        static float velocity = .0f;
        velocity += dt;
        anim_inst->setParamValue(animator->getParamId("velocity"), velocity);

        anim_inst->update(dt);
        anim_inst->getSampleBuffer()->applySamples(model_inst->getSkeletonInstance());

        auto& rm_sample = anim_inst->getSampleBuffer()->getRootMotionSample();
        /*
        model_inst->getSkeletonInstance()->getWorldTransformsPtr()[0]
            = gfxm::translate(
                model_inst->getSkeletonInstance()->getWorldTransformsPtr()[0],
                rm_sample.t
            );*/
        // TODO: Rotation
    }
};
STATIC_BLOCK{
    type_register<actorAnimTest>("actorAnimTest")
        .parent<wActor>();
};

#include "animation/hitbox_sequence/hitbox_seq_sample_buffer.hpp"
#include "animation/hitbox_sequence/hitbox_sequence.hpp"

#include "animation/audio_sequence/audio_sequence.hpp"

#include "animation/animator/animator_instance.hpp"

#include "animation/model_sequence/model_sequence.hpp"
class actorVfxTest : public wActor {
    TYPE_ENABLE(wActor);

    RHSHARED<sklmSkeletalModelEditable> model;
    RHSHARED<AnimatorEd> animator;

    HSHARED<sklmSkeletalModelInstance> model_inst;
    HSHARED<animAnimatorInstance> anim_inst;

    RHSHARED<animAnimatorSequence> seq_test;
    RHSHARED<Animation> anim_skl;
    RHSHARED<animModelSequence> anim_mdl;
    animModelSampleBuffer sample_buf;
    animModelAnimMapping mapping;
public:
    actorVfxTest() {
        setFlags(WACTOR_FLAG_UPDATE);

        model.reset_acquire();
        
        model->getSkeleton()->getRoot()->createChild("Root_B");
        
        auto decal = model->addComponent<sklmDecalComponent>("decal");
        decal->bone_name = "Root_B";
        decal->texture = resGet<gpuTexture2d>("textures/decals/golden_glow.png");

        model_inst = model->createInstance();
        
        {
            anim_skl.reset_acquire();
            anim_skl->length = 80;
            anim_skl->fps = 60;
            auto& node = anim_skl->createNode("Root_B");
            node.s[.0f] = gfxm::vec3(0,0,0);
            node.s[10] = gfxm::vec3(5.0, 5.0, 5.0);
            //node.s[15] = gfxm::vec3(4.2, 4.2, 4.2);
            //node.s[80] = gfxm::vec3(4, 4, 4);
        }
        {
            model->initSampleBuffer(sample_buf);

            anim_mdl.reset_acquire();
            anim_mdl->length = 80;
            anim_mdl->fps = 60;
            auto node = anim_mdl->createNode<animDecalNode>("decal");
            node->rgba[0] = gfxm::vec4(1, 1, 1, 1);
            node->rgba[10] = gfxm::vec4(2, 2, 2, 1);
            node->rgba[20] = gfxm::vec4(1, 1, 1, 1);
            node->rgba[80] = gfxm::vec4(1, 1, 1, 0);

            animMakeModelAnimMapping(&mapping, model.get(), anim_mdl.get());
        }

        {
            seq_test.reset_acquire();
            seq_test->setSkeletalAnimation(anim_skl);
        }

        {
            animator.reset_acquire();
            animator->setSkeleton(model->getSkeleton());
            animator->addSampler("my_loop", "default", seq_test);
            auto fsm = animator->setRoot<animUnitFsm>();
            auto state_default = fsm->addState("default");
            auto single = state_default->setUnit<animUnitSingle>();
            single->setSampler("my_loop");

            animator->compile();

            anim_inst = animator->createInstance(model_inst->getSkeletonInstance());
        }
    }
    void onSpawn(wWorld* world) override {
        model_inst->spawn(world->getRenderScene());
    }
    void onDespawn(wWorld* world) override {
        model_inst->despawn(world->getRenderScene());
    }
    void onUpdate(wWorld* world, float dt) override {
        anim_inst->update(dt);
        anim_inst->getSampleBuffer()->applySamples(model_inst->getSkeletonInstance());
    
        static float cur = .0f;
        if (cur > anim_mdl->length) {
            cur -= anim_mdl->length;
        }
        anim_mdl->sample_remapped(
            &sample_buf,
            cur, mapping
        );
        cur += dt * anim_mdl->fps;

        model_inst->applySampleBuffer(sample_buf);
    }
};
STATIC_BLOCK{
    type_register<actorVfxTest>("actorVfxTest")
        .parent<wActor>();
};

class actorUltimaWeapon : public wActor {
    TYPE_ENABLE(wActor);

    HSHARED<sklmSkeletalModelInstance> model_inst;
    RHSHARED<AnimatorEd> animator;
    HSHARED<animAnimatorInstance> anim_inst;
    
    wWorld* world = 0;

    RHSHARED<hitboxCmdSequence> hitbox_seq;
    hitboxCmdBuffer hitbox_cmd_buf;

    RHSHARED<animAnimatorSequence> seq_idle;
public:
    actorUltimaWeapon() {
        setFlags(WACTOR_FLAG_UPDATE);

        auto model = resGet<sklmSkeletalModelEditable>("models/ultima_weapon/ultima_weapon.skeletal_model");
        
        model_inst = model->createInstance();
        model_inst->getSkeletonInstance()->getWorldTransformsPtr()[0]
            = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(6, 0, -6))
            * gfxm::scale(gfxm::mat4(1.0f), gfxm::vec3(2, 2, 2));

        // Hitbox
        hitbox_seq.reset(HANDLE_MGR<hitboxCmdSequence>::acquire());
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

        hitbox_cmd_buf.resize(8);

        // Sequence
        seq_idle.reset(HANDLE_MGR<animAnimatorSequence>::acquire());
        seq_idle->setSkeletalAnimation(resGet<Animation>("models/ultima_weapon/Idle.animation"));
        seq_idle->setHitboxSequence(hitbox_seq);

        // Animator
        animator.reset_acquire();
        animator->setSkeleton(model->getSkeleton());
        animator->addSampler("idle", "Default", seq_idle);

        auto single = animator->setRoot<animUnitSingle>();
        single->setSampler("idle");
        animator->compile();

        anim_inst = animator->createInstance(model_inst->getSkeletonInstance());
    }
    void onSpawn(wWorld* world) override {
        model_inst->spawn(world->getRenderScene());
        this->world = world;
    }
    void onDespawn(wWorld* world) override {
        model_inst->despawn(world->getRenderScene());
        this->world = 0;
    }
    void onUpdate(wWorld* world, float dt) override {
        anim_inst->update(dt);
        anim_inst->getSampleBuffer()->applySamples(model_inst->getSkeletonInstance());
        anim_inst->getHitboxCmdBuffer()->execute(model_inst->getSkeletonInstance(), world->getCollisionWorld());
        //anim_inst->getAudioCmdBuffer()->execute();
        /*
        static float cur = .0f;
        static float cur_prev = .0f;
        int sample_count = hitbox_seq->sample(hitbox_cmd_buf.data(), hitbox_cmd_buf.count(), cur_prev, cur);
        cur_prev = cur;
        cur += hitbox_seq->fps * dt;
        if (cur > hitbox_seq->length) {
            cur -= hitbox_seq->length;
        }

        for (int i = 0; i < sample_count; ++i) {
            auto& s = hitbox_cmd_buf[i];
            if (s.type == HITBOX_SEQ_CLIP_EMPTY) {
                continue;
            }
            gfxm::vec3 translation 
                = model_inst->getSkeletonInstance()->getWorldTransformsPtr()[s.bone_id]
                * gfxm::vec4(s.translation, 1.0f);
            gfxm::mat4 m = gfxm::translate(gfxm::mat4(1.0f), translation);
            world->getCollisionWorld()->castSphere(m, s.radius);
        }
        */
    }
};
STATIC_BLOCK{
    type_register<actorUltimaWeapon>("actorUltimaWeapon")
        .parent<wActor>();
};

class Door : public wActor {
    TYPE_ENABLE(wActor);

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
        setFlags(WACTOR_FLAG_UPDATE);

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
        model_inst->spawn(world->getRenderScene());

        world->getCollisionWorld()->addCollider(&collider_beacon);
    }
    void onDespawn(wWorld* world) override {
        model_inst->despawn(world->getRenderScene());

        world->getCollisionWorld()->removeCollider(&collider_beacon);
    }

    void onUpdate(wWorld* world, float dt) override {
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
        if (auto m = wMsgTranslate<wMsgInteract>(msg)) {
            LOG_DBG("Door: Open message received");
            is_opening = true;
            anim_cursor = .0f;
            gfxm::vec3 door_pos = getTranslation();
            gfxm::vec3 initiator_pos = m->sender->getTranslation();
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
                wRspInteractDoorOpen{ sync_pos, sync_rot, d > .0f }
            );
        } else {
            LOG_DBG("Door: unknown message received");
        }
        return 0;
    }
};
STATIC_BLOCK{
    type_register<Door>("Door")
        .parent<wActor>();
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
    TYPE_ENABLE(wActor);

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
    RHSHARED<AnimatorEd> animator;
    HSHARED<animAnimatorInstance> anim_inst;
    RHSHARED<animAnimatorSequence> seq_idle;
    RHSHARED<animAnimatorSequence> seq_run2;
    RHSHARED<animAnimatorSequence> seq_open_door_front;
    RHSHARED<animAnimatorSequence> seq_open_door_back;

    RHSHARED<audioSequence> audio_seq;

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
        setFlags(WACTOR_FLAG_UPDATE);

        auto model = resGet<sklmSkeletalModelEditable>("models/chara_24/chara_24.skeletal_model");
        {
            model_inst = model->createInstance();
        }
        
        // Audio
        audio_seq.reset_acquire();
        audio_seq->length = 40.0f;
        audio_seq->fps = 60.0f;
        audio_seq->insert(0, resGet<AudioClip>("audio/sfx/gravel1.ogg"));
        audio_seq->insert(20, resGet<AudioClip>("audio/sfx/gravel2.ogg"));
        
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
            seq_run2->setAudioSequence(audio_seq);
            seq_open_door_front->setSkeletalAnimation(resGet<Animation>("models/chara_24_anim_door/Action_OpenDoor.animation"));
            seq_open_door_back->setSkeletalAnimation(resGet<Animation>("models/chara_24/Action_DoorOpenBack.animation"));
            
            animator.reset_acquire();
            animator->setSkeleton(model->getSkeleton());
            // Setup parameters signals and events
            animator->addParam("velocity");
            animator->addSignal("sig_door_open");
            animator->addSignal("sig_door_open_back");
            animator->addFeedbackEvent("fevt_door_open_end");
            animator
                ->addSampler("idle", "Default", seq_idle)
                .addSampler("run", "Locomotion", seq_run2)
                .addSampler("open_door_front", "Interact", seq_open_door_front)
                .addSampler("open_door_back", "Interact", seq_open_door_back);
            
            // Setup the tree
            auto fsm = animator->setRoot<animUnitFsm>();
            auto st_idle = fsm->addState("Idle");
            auto st_loco = fsm->addState("Locomotion");
            auto st_door_open_front = fsm->addState("DoorOpenFront");
            auto st_door_open_back = fsm->addState("DoorOpenBack");
            st_idle->setUnit<animUnitSingle>()->setSampler("idle");
            st_loco->setUnit<animUnitSingle>()->setSampler("run");
            st_door_open_front->setUnit<animUnitSingle>()->setSampler("open_door_front");
            st_door_open_front->onExit(call_feedback_event_(animator.get(), "fevt_door_open_end"));
            st_door_open_back->setUnit<animUnitSingle>()->setSampler("open_door_back");
            st_door_open_back->onExit(call_feedback_event_(animator.get(), "fevt_door_open_end"));
            fsm->addTransition("Idle", "Locomotion", param_(animator.get(), "velocity") > FLT_EPSILON, 0.15f);
            fsm->addTransition("Locomotion", "Idle", param_(animator.get(), "velocity") <= FLT_EPSILON, 0.15f);
            fsm->addTransitionAnySource("DoorOpenFront", signal_(animator.get(), "sig_door_open"), 0.15f);
            fsm->addTransitionAnySource("DoorOpenBack", signal_(animator.get(), "sig_door_open_back"), 0.15f);
            fsm->addTransition("DoorOpenFront", "Idle", state_complete_(), 0.15f);
            fsm->addTransition("DoorOpenBack", "Idle", state_complete_(), 0.15f);
            animator->compile();
            
            anim_inst = animator->createInstance(model_inst->getSkeletonInstance());
            if (!anim_inst.isValid()) {
                assert(false);
            }
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
            wRsp rsp = targeted_actor->sendMessage(wMsgMake(wMsgInteract{ this }));
            if (rsp.t == type_get<wRspInteractDoorOpen>()) {
                const wRspInteractDoorOpen* trsp = wRspTranslate<wRspInteractDoorOpen>(rsp);
                setTranslation(trsp->sync_pos);
                setRotation(trsp->sync_rot);

                if (trsp->is_front) {
                    anim_inst->triggerSignal(animator->getSignalId("sig_door_open"));
                }
                else {
                    anim_inst->triggerSignal(animator->getSignalId("sig_door_open_back"));
                }
                state = CHARACTER_STATE::DOOR_OPEN;
                velocity = .0f;
                loco_vec = gfxm::vec3(0, 0, 0);
            } else if(rsp.t == type_get<wRspInteractJukebox>()) {
                // Nothing?
            }
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
        if(anim_inst->isFeedbackEventTriggered(animator->getFeedbackEventId("fevt_door_open_end"))) {
            state = CHARACTER_STATE::LOCOMOTION;
            forward_vec = getWorldTransform() * gfxm::vec4(0, 0, 1, 0);
        }
    }
    void onUpdate(wWorld* world, float dt) override {
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
        anim_inst->setParamValue(animator->getParamId("velocity"), velocity);
        anim_inst->update(dt);
        anim_inst->getSampleBuffer()->applySamples(model_inst->getSkeletonInstance());
        anim_inst->getAudioCmdBuffer()->execute(model_inst->getSkeletonInstance());

        // Apply root motion
        
        gfxm::vec3 rm_t = gfxm::vec3(getWorldTransform() * gfxm::vec4(anim_inst->getSampleBuffer()->getRootMotionSample().t, .0f));
        rm_t.y = .0f;
        translate(rm_t);
        rotate(anim_inst->getSampleBuffer()->getRootMotionSample().r);
        

        // Update transforms
        model_inst->getSkeletonInstance()->getWorldTransformsPtr()[0] = getWorldTransform();

        collider.position = getTranslation() + gfxm::vec3(0, 1.0f, 0);
        collider.rotation = getRotation();
        collider_probe.position = getWorldTransform() * gfxm::vec4(0, 0.5f, 0.64f, 1.0f);
        collider_probe.rotation = gfxm::to_quat(gfxm::to_orient_mat3(getWorldTransform()));
    }

    void onSpawn(wWorld* world) override {
        model_inst->spawn(world->getRenderScene());

        world->getRenderScene()->addRenderObject(decal.get());

        world->getRenderScene()->addNode(&caption_node);
        world->getRenderScene()->addRenderObject(name_caption.get());

        world->getCollisionWorld()->addCollider(&collider);
        world->getCollisionWorld()->addCollider(&collider_probe);
    }
    void onDespawn(wWorld* world) override {
        model_inst->despawn(world->getRenderScene());

        world->getRenderScene()->removeRenderObject(decal.get());

        world->getRenderScene()->removeNode(&caption_node);
        world->getRenderScene()->removeRenderObject(name_caption.get());

        world->getCollisionWorld()->removeCollider(&collider);
        world->getCollisionWorld()->removeCollider(&collider_probe);
    }
};
STATIC_BLOCK{
    type_register<actorCharacter>("actorCharacter")
        .parent<wActor>();
};


struct cameraState {
    gfxm::mat4 projection;
    gfxm::mat4 transform;
    gfxm::mat4 view;

    const gfxm::mat4& getProjection() { return projection; }
    const gfxm::mat4& getTransform() { return transform; }
    const gfxm::mat4& getView() { return view; }
};

#include "game/particle_emitter/particle_emitter.hpp"
class actorExplosion : public wActor {
    TYPE_ENABLE(wActor);
    ptclEmitter emitter;
public:
    actorExplosion() {
        setFlags(WACTOR_FLAG_UPDATE);

        emitter.init();
        auto shape = emitter.setShape<ptclSphereShape>();
        shape->radius = 1.f;
        shape->emit_mode = ptclSphereShape::EMIT_MODE::VOLUME;
        emitter.addComponent<ptclAngularVelocityComponent>();

        curve<float> emit_curve;
        emit_curve[.0f] = 256.0f;
        emitter.setParticlePerSecondCurve(emit_curve);
    }
    void onUpdate(wWorld* world, float dt) override {
        emitter.setWorldTransform(getWorldTransform());
        emitter.update_emit(dt);

        emitter.update(dt);
    }
    void onSpawn(wWorld* world) override {
        emitter.spawn(world->getRenderScene());
    }
    void onDespawn(wWorld* world) override {
        emitter.despawn(world->getRenderScene());
    }
};
STATIC_BLOCK{
    type_register<actorExplosion>("actorExplosion")
        .parent<wActor>();
};
class actorRocketStateDefault;
class actorRocketStateDying;
class actorMissile : public wActor {
    TYPE_ENABLE(wActor);

    RHSHARED<AudioClip> clip_rocket_loop;
    RHSHARED<sklmSkeletalModelInstance> mdl_inst;

    Handle<AudioChannel> chan;

    ptclEmitter emitter;
public:
    actorMissile() {
        setFlags(WACTOR_FLAG_UPDATE);
        clip_rocket_loop = resGet<AudioClip>("audio/sfx/rocket_loop.ogg");
        mdl_inst = resGet<sklmSkeletalModelEditable>("models/rocket/rocket.skeletal_model")->createInstance();
    
        {
            emitter.init();
            auto shape = emitter.setShape<ptclSphereShape>();
            shape->radius = 0.05f;
            shape->emit_mode = ptclSphereShape::EMIT_MODE::VOLUME;
            emitter.addComponent<ptclAngularVelocityComponent>();
        }
    }
    void onUpdate(wWorld* world, float dt) override {
        mdl_inst->getSkeletonInstance()->getWorldTransformsPtr()[0] = getWorldTransform();
        audio().setPosition(chan, getTranslation());

        emitter.setWorldTransform(getWorldTransform());
        emitter.update_emit(dt);

        emitter.update(dt);
    }

    void onSpawn(wWorld* world) override {
        world->setActorState<actorRocketStateDefault>(this);

        mdl_inst->spawn(world->getRenderScene());

        chan = audio().createChannel();
        audio().setLooping(chan, true);
        audio().setBuffer(chan, clip_rocket_loop->getBuffer());
        audio().play3d(chan);

        emitter.spawn(world->getRenderScene());
    }
    void onDespawn(wWorld* world) override {
        emitter.despawn(world->getRenderScene());

        audio().freeChannel(chan);

        mdl_inst->despawn(world->getRenderScene());
    }
};
STATIC_BLOCK{
    type_register<actorMissile>("actorMissile")
        .parent<wActor>();
};


#include "input/input.hpp"
class playerControllerFps {
    InputContext inputCtx = InputContext("FirstPersonCtrl");
    InputRange* inputRotation = 0;
    InputRange* inputLoco = 0;
    InputAction* inputSprint = 0;
    InputAction* inputShoot = 0;

    float total_distance_walked = .0f;
    gfxm::vec3 translation;
    gfxm::quat qcam;
    gfxm::quat qarms;
    float rotation_y = .0f;
    float rotation_x = .0f;// gfxm::pi * .5f;
    float cam_height = 1.6f;

    float reload_time = .0f;
    float recoil_offset = .0f;

    RHSHARED<sklmSkeletalModelInstance> mdl_inst;
    std::vector<HSHARED<actorMissile>> rocketActors;

    RHSHARED<AudioClip> clip_rocket_launch;
public:
    playerControllerFps() {
        inputRotation = inputCtx.createRange("Rotation");
        inputLoco = inputCtx.createRange("Locomotion");
        inputSprint = inputCtx.createAction("Sprint");
        inputShoot = inputCtx.createAction("Shoot");

        inputRotation
            ->linkKeyY(InputDeviceType::Mouse, Key.Mouse.AxisX, 1.0f)
            .linkKeyX(InputDeviceType::Mouse, Key.Mouse.AxisY, 1.0f);
        inputLoco
            ->linkKeyZ(InputDeviceType::Keyboard, Key.Keyboard.W, -1.0f)
            .linkKeyZ(InputDeviceType::Keyboard, Key.Keyboard.S, 1.0f)
            .linkKeyX(InputDeviceType::Keyboard, Key.Keyboard.A, -1.0f)
            .linkKeyX(InputDeviceType::Keyboard, Key.Keyboard.D, 1.0f);
        inputSprint
            ->linkKey(InputDeviceType::Keyboard, Key.Keyboard.LeftShift, 1.0f);
        inputShoot
            ->linkKey(InputDeviceType::Mouse, Key.Mouse.BtnLeft, 1.0f);

        mdl_inst = resGet<sklmSkeletalModelEditable>("models/fps_q3_rocket_launcher/fps_q3_rocket_launcher.skeletal_model")->createInstance();
    
        clip_rocket_launch = resGet<AudioClip>("audio/sfx/rocket_launch.ogg");
    }

    void init(cameraState* camState, wWorld* world) {
        platformLockMouse(true);
        platformHideMouse(true);

        camState->projection = gfxm::perspective(gfxm::radian(65), 16.0f / 9.0f, 0.01f, 1000.0f);
        
        mdl_inst->spawn(world->getRenderScene());
    }
    void update(wWorld* world, float dt, cameraState* camState) {
        const float base_speed = 10.0f;
        float speed = base_speed;
        if (inputSprint->isPressed()) {
            speed *= 2.0f;
        }

        rotation_y += gfxm::radian(inputRotation->getVec3().y) * .8f;// *(1.0f / 60.f);
        rotation_x += gfxm::radian(inputRotation->getVec3().x) * .8f;// *(1.0f / 60.f);
        rotation_x = gfxm::clamp(rotation_x, -gfxm::pi * 0.5f, gfxm::pi * 0.5f);
        
        gfxm::quat qy = gfxm::angle_axis(rotation_y, gfxm::vec3(0, 1, 0));

        gfxm::vec3 translation_delta = inputLoco->getVec3();
        translation_delta = gfxm::to_mat4(qy) * gfxm::vec4(gfxm::normalize(translation_delta), .0f);
        if (inputLoco->getVec3().length() > .0f) {
            translation.x += translation_delta.x * dt * speed;
            translation.z += translation_delta.z * dt * speed;
            total_distance_walked += translation_delta.length() * dt * 5.0f;
        }

        float cam_height_final = cam_height + cosf(total_distance_walked * 3.0f) * 0.05f;
        float cam_sway = cosf(total_distance_walked * 1.5f) * .1f * (speed / base_speed);
        gfxm::vec3 sway = gfxm::to_mat4(qcam) * gfxm::vec4(cam_sway, .0f, .0f, .0f);
        gfxm::quat qx = gfxm::angle_axis(rotation_x, gfxm::vec3(1, 0, 0));
        qcam = gfxm::slerp(qcam, qy * qx, 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f)/* 0.1f*/);
        qarms = gfxm::slerp(qarms, qcam, 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f)/* 0.1f*/);
        //gfxm::mat4 cam_translation = gfxm::translate(gfxm::mat4(1.0f), translation + gfxm::vec3(0, cam_height_final, 0) + sway);
        gfxm::mat4 cam_translation = gfxm::translate(gfxm::mat4(1.0f), translation + gfxm::vec3(0, cam_height, 0));
        camState->transform 
            = cam_translation
            * gfxm::to_mat4(qcam);
        camState->view = gfxm::inverse(camState->transform);

        gfxm::vec3 recoil_v3 = camState->transform * gfxm::vec4(.0f, .0f, recoil_offset, .0f);
        mdl_inst->getSkeletonInstance()->getWorldTransformsPtr()[0] 
            = gfxm::translate(gfxm::mat4(1.0f), recoil_v3 + sway * .2f + gfxm::vec3(0, cosf(total_distance_walked * 3.0f) * 0.05f,0) * .2f)
            * cam_translation
            * gfxm::to_mat4(qarms);

        if (reload_time) {
            reload_time -= dt;
        }
        if (recoil_offset) {
            recoil_offset = gfxm::lerp(recoil_offset, .0f, 1 - pow(1 - 0.1f * 3.0f, dt * 10.0f));
        }

        if (inputShoot->isJustPressed() && reload_time <= .0f) {
            HSHARED<actorMissile> rocket;
            rocket.reset_acquire();
            rocket->setTranslation(camState->transform * gfxm::vec4(0, 0, 0, 1));
            rocket->setRotation(qcam);
            rocket->translate(-rocket->getForward() * 1.f);
            rocketActors.push_back(rocket);
            world->addActor(rocket.get());
            audio().playOnce(clip_rocket_launch->getBuffer(), 1.0f);

            reload_time = .6f;
            recoil_offset = .2f;
        }
    }
};

/*
struct ActorStateDef {
    type state_type;
    type actor_type;
    (void)(*on_enter_fn)(wActor** actors, size_t count);
    (void)(*on_update_fn)(wActor** actors, size_t count);
    (void)(*on_leave_fn)(wActor** actors, size_t count);
};*/


class actorRocketStateDefault : public wActorStateT<actorMissile> {
    RHSHARED<AudioClip> clip_explosion;
public:
    actorRocketStateDefault() {
        clip_explosion = resGet<AudioClip>("audio/sfx/rocklx1a.ogg");
    }
    void onEnter(wWorld* world, actorMissile** rockets, size_t count) override {

    }
    void onUpdate(wWorld* world, float dt, actorMissile** rockets, size_t count) override {
        // TODO: fly forward

        gfxm::aabb box;
        box.from = gfxm::vec3(-25.0f, 0.0f, -25.0f);
        box.to = gfxm::vec3(25.0f, 25.0f, 25.0f);
        
        for (int i = 0; i < count; ++i) {
            auto a = rockets[i];
            if (!gfxm::point_in_aabb(box, a->getTranslation())) {
                world->setActorState<actorRocketStateDying>(a);
                //audio().playOnce3d(clip_explosion->getBuffer(), a->getTranslation());
                audio().playOnce(clip_explosion->getBuffer(), .5f);
                // TODO: POOL
                auto ptr = new actorExplosion();
                ptr->setTranslation(a->getTranslation());
                world->addActor(ptr);
                continue;
            }

            // Quake 3 rocket speed (900 units per sec)
            // 64 quake3 units is approx. 1.7 meters
            a->translate(-a->getForward() * dt * 23.90625f);
        }
    }
};
class actorRocketStateDying : public wActorStateT<actorMissile> {
public:
    void onEnter(wWorld* world, actorMissile** rockets, size_t count) override {
        // TODO: Explosion?
    }
    void onUpdate(wWorld* world, float dt, actorMissile** rockets, size_t count) override {
        // TODO: Wait for particles to die, then remove yourself
    }
};
