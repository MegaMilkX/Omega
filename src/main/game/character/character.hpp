#pragma once

#include "game/actor/actor.hpp"

#include "assimp_load_scene.hpp"
#include "animation/animation.hpp"
#include "collision/collision_world.hpp"
#include "gpu/gpu.hpp"
#include "gpu/render/uniform.hpp"
#include "animation/animation_sample_buffer.hpp"
#include "animation/animation_sampler.hpp"
#include "world/world.hpp"

#include "skeletal_model/skeletal_model.hpp"
#include "skeletal_model/skeletal_model_instance.hpp"

#include "animation/animator/animator.hpp"

#include "util/static_block.hpp"

class actorAnimatedSkeletalModel : public gameActor {
    TYPE_ENABLE(gameActor);

    RHSHARED<mdlSkeletalModelInstance> model_inst;
    HSHARED<animAnimatorInstance> animator_inst;
public:
    // TODO
};
STATIC_BLOCK{
    type_register<actorAnimatedSkeletalModel>("actorAnimatedSkeletalModel")
        .parent<gameActor>();
};

class actorJukebox : public gameActor {
    TYPE_ENABLE(gameActor);

    RHSHARED<mdlSkeletalModelInstance> model_inst;
    CollisionSphereShape     shape_sphere;
    Collider                 collider_beacon;
    RHSHARED<AudioClip>      audio_clip_click;
    RHSHARED<AudioClip>      audio_clip;
    Handle<AudioChannel>     audio_ch;
public:
    actorJukebox() {
        setFlagsDefault();

        auto model = resGet<mdlSkeletalModelMaster>("models/jukebox_low/jukebox_low.skeletal_model");
        model_inst = model->createInstance();
        model_inst->getSkeletonInstance()->getWorldTransformsPtr()[0]
            = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(-2, 0, 4))
            * gfxm::scale(gfxm::mat4(1.0f), gfxm::vec3(1, 1, 1) * 0.35f);

        shape_sphere.radius = 0.5f;
        collider_beacon.setShape(&shape_sphere);
        collider_beacon.setPosition(gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(-2, 0, 4) + gfxm::vec3(0, 1, 0)) * gfxm::vec4(0,0,0,1));
        collider_beacon.user_data.type = COLLIDER_USER_ACTOR;
        collider_beacon.user_data.user_ptr = this;
        collider_beacon.collision_group = COLLISION_LAYER_BEACON;
        collider_beacon.collision_mask = COLLISION_LAYER_PROBE;

        audio_clip_click = resGet<AudioClip>("audio/sfx/switch_click.ogg");
        audio_clip = resGet<AudioClip>("audio/track02.ogg");
        audio_ch = audio().createChannel();
        audio().setBuffer(audio_ch, audio_clip->getBuffer());
        audio().setPosition(audio_ch, collider_beacon.getPosition());
        audio().setLooping(audio_ch, true);
    }
    ~actorJukebox() {
        audio().freeChannel(audio_ch);
    }
    void onSpawn(gameWorld* world) override {
        model_inst->spawn(world->getRenderScene());

        world->getCollisionWorld()->addCollider(&collider_beacon);
    }
    void onDespawn(gameWorld* world) override {
        model_inst->despawn(world->getRenderScene());

        world->getCollisionWorld()->removeCollider(&collider_beacon);
    }
    wRsp onMessage(wMsg msg) override {
        if (auto m = wMsgTranslate<wMsgInteract>(msg)) {
            audio().playOnce(audio_clip_click->getBuffer(), 1.0f);
            if (audio().isPlaying(audio_ch)) {
                audio().stop(audio_ch);
            } else {
                audio().play(audio_ch);
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
        .parent<gameActor>();
};

class actorAnimTest : public gameActor {
    TYPE_ENABLE(gameActor);

    HSHARED<mdlSkeletalModelInstance> model_inst;
    RHSHARED<AnimatorMaster> animator;
    HSHARED<animAnimatorInstance> anim_inst;
    HSHARED<animSequence> seq_idle;
    HSHARED<animSequence> seq_run2;
public:
    actorAnimTest() {
        setFlags(WACTOR_FLAG_UPDATE);

        {/*
            expr_ e = (param_(&animator, "velocity") + 10.0f) % 9.f;
            e.dbgLog(&animator);
            value_ v = e.evaluate(&animator);
            v.dbgLog();*/
        }

        auto model = resGet<mdlSkeletalModelMaster>("models/chara_24/chara_24.skeletal_model");
        model_inst = model->createInstance();
        model_inst->getSkeletonInstance()->getWorldTransformsPtr()[0] = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(4, 0, 0));
        
        {
            seq_idle.reset_acquire();
            seq_run2.reset_acquire();
            seq_idle->setSkeletalAnimation(resGet<Animation>("models/chara_24/Idle.animation"));
            seq_run2->setSkeletalAnimation(resGet<Animation>("models/chara_24/Run.animation"));

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

            anim_inst = animator->createInstance();
        }
    }

    void onSpawn(gameWorld* world) override {
        model_inst->spawn(world->getRenderScene());
    }
    void onDespawn(gameWorld* world) override {
        model_inst->despawn(world->getRenderScene());
    }

    void onUpdate(gameWorld* world, float dt) override {
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
        .parent<gameActor>();
};

#include "animation/hitbox_sequence/hitbox_seq_sample_buffer.hpp"
#include "animation/hitbox_sequence/hitbox_sequence.hpp"

#include "animation/audio_sequence/audio_sequence.hpp"

#include "animation/animator/animator_instance.hpp"

#include "animation/model_sequence/model_sequence.hpp"
class actorVfxTest : public gameActor {
    TYPE_ENABLE(gameActor);

    RHSHARED<mdlSkeletalModelMaster> model;
    RHSHARED<AnimatorMaster> animator;

    HSHARED<mdlSkeletalModelInstance> model_inst;
    HSHARED<animAnimatorInstance> anim_inst;

    RHSHARED<animSequence> seq_test;
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

            anim_inst = animator->createInstance();
        }
    }
    void onSpawn(gameWorld* world) override {
        model_inst->spawn(world->getRenderScene());
    }
    void onDespawn(gameWorld* world) override {
        model_inst->despawn(world->getRenderScene());
    }
    void onUpdate(gameWorld* world, float dt) override {
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

        // NOTE: hack to get it out of the way for now
        model_inst->getSkeletonInstance()->getWorldTransformsPtr()[0] =
            gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(3, 0, -5));
    }
};
STATIC_BLOCK{
    type_register<actorVfxTest>("actorVfxTest")
        .parent<gameActor>();
};

class actorUltimaWeapon : public gameActor {
    TYPE_ENABLE(gameActor);

    HSHARED<mdlSkeletalModelInstance> model_inst;
    RHSHARED<AnimatorMaster> animator;
    HSHARED<animAnimatorInstance> anim_inst;
    
    gameWorld* world = 0;

    RHSHARED<hitboxCmdSequence> hitbox_seq;
    hitboxCmdBuffer hitbox_cmd_buf;

    RHSHARED<animSequence> seq_idle;
public:
    actorUltimaWeapon() {
        setFlags(WACTOR_FLAG_UPDATE);

        auto model = resGet<mdlSkeletalModelMaster>("models/ultima_weapon/ultima_weapon.skeletal_model");
        
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
        seq_idle.reset(HANDLE_MGR<animSequence>::acquire());
        seq_idle->setSkeletalAnimation(resGet<Animation>("models/ultima_weapon/Idle.animation"));
        seq_idle->setHitboxSequence(hitbox_seq);

        // Animator
        animator.reset_acquire();
        animator->setSkeleton(model->getSkeleton());
        animator->addSampler("idle", "Default", seq_idle);

        auto single = animator->setRoot<animUnitSingle>();
        single->setSampler("idle");
        animator->compile();

        anim_inst = animator->createInstance();
    }
    void onSpawn(gameWorld* world) override {
        model_inst->spawn(world->getRenderScene());
        this->world = world;
    }
    void onDespawn(gameWorld* world) override {
        model_inst->despawn(world->getRenderScene());
        this->world = 0;
    }
    void onUpdate(gameWorld* world, float dt) override {
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
            world->getCollisionWorld()->sphereTest(m, s.radius);
        }
        */
    }
};
STATIC_BLOCK{
    type_register<actorUltimaWeapon>("actorUltimaWeapon")
        .parent<gameActor>();
};

class Door : public gameActor {
    TYPE_ENABLE(gameActor);

    RHSHARED<mdlSkeletalModelMaster> model;
    HSHARED<mdlSkeletalModelInstance> model_inst;

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

        model = resGet<mdlSkeletalModelMaster>("models/door/door.skeletal_model");
        model_inst = model->createInstance();

        anim_open = resGet<Animation>("models/door/Open.animation");
        anim_sampler = animSampler(model->getSkeleton().get(), anim_open.get());
        samples.init(model->getSkeleton().get());

        setTranslation(gfxm::vec3(1, 0, 6.0f));

        model_inst->getSkeletonInstance()->getWorldTransformsPtr()[0]
            = getWorldTransform();

        shape_sphere.radius = 0.1f;
        collider_beacon.setShape(&shape_sphere);        
        collider_beacon.setPosition(getTranslation());
        collider_beacon.user_data.type = COLLIDER_USER_ACTOR;
        collider_beacon.user_data.user_ptr = this;
        collider_beacon.collision_group = COLLISION_LAYER_BEACON;
        collider_beacon.collision_mask = COLLISION_LAYER_PROBE;

        // Ref points for the character to adjust to for door opening animations
        gfxm::vec3 door_pos = getTranslation();
        door_pos.y = .0f;
        ref_point_front.setTranslation(door_pos + gfxm::vec3(0, 0, 1));
        ref_point_front.setRotation(gfxm::angle_axis(gfxm::pi, gfxm::vec3(0, 1, 0)));
        ref_point_back.setTranslation(door_pos + gfxm::vec3(0, 0, -1));
    }

    void onSpawn(gameWorld* world) override {
        model_inst->spawn(world->getRenderScene());

        world->getCollisionWorld()->addCollider(&collider_beacon);
    }
    void onDespawn(gameWorld* world) override {
        model_inst->despawn(world->getRenderScene());

        world->getCollisionWorld()->removeCollider(&collider_beacon);
    }

    void onUpdate(gameWorld* world, float dt) override {
        collider_beacon.setPosition(getTranslation() + gfxm::vec3(.0f, 1.0f, .0f) + getLeft() * .5f);
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
        .parent<gameActor>();
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

#include "world/node/node_character_capsule.hpp"
#include "world/node/node_skeletal_model.hpp"
#include "world/node/node_decal.hpp"
class actorCharacter2 : public gameActor {
    TYPE_ENABLE(gameActor);
public:
    actorCharacter2() {
        auto root = setRoot<nodeCharacterCapsule>("capsule");
        auto model = root->createChild<nodeSkeletalModel>("model");
        auto decal = root->createChild<nodeDecal>("decal");
        model->setModel(resGet<mdlSkeletalModelMaster>("models/chara_24/chara_24.skeletal_model"));


    }
};

class actorCharacter : public gameActor {
    TYPE_ENABLE(gameActor);

    struct {
        HSHARED<mdlSkeletalModelInstance>  model_inst;
    };

    std::unique_ptr<scnDecal> decal;
    std::unique_ptr<scnTextBillboard> name_caption;
    scnNode caption_node;
    Font* font = 0;

    // New Anim
    RHSHARED<AnimatorMaster> animator;
    HSHARED<animAnimatorInstance> anim_inst;
    RHSHARED<animSequence> seq_idle;
    RHSHARED<animSequence> seq_run2;
    RHSHARED<animSequence> seq_open_door_front;
    RHSHARED<animSequence> seq_open_door_back;

    RHSHARED<audioSequence> audio_seq;

    // Gameplay
    gameActor* targeted_actor = 0;
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

        auto model = resGet<mdlSkeletalModelMaster>("models/chara_24/chara_24.skeletal_model");
        {
            model_inst = model->createInstance();
        }
        
        // Audio
        audio_seq.reset_acquire();
        audio_seq->length = 40.0f;
        audio_seq->fps = 60.0f;
        audio_seq->insert(0, resGet<AudioClip>("audio/sfx/footsteps/asphalt03.ogg"));
        audio_seq->insert(20, resGet<AudioClip>("audio/sfx/footsteps/asphalt04.ogg"));
        
        decal.reset(new scnDecal);
        decal->setTexture(resGet<gpuTexture2d>("images/character_selection_decal.png"));
        decal->setBoxSize(1.3f, 1.0f, 1.3f);
        decal->setBlending(GPU_BLEND_MODE::NORMAL);
        decal->setSkeletonNode(model_inst->getSkeletonInstance()->getScnSkeleton(), 0);
        
        font = fontGet("fonts/OpenSans-Regular.ttf", 16, 72);
        name_caption.reset(new scnTextBillboard(font));
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
            seq_run2->setSkeletalAnimation(resGet<Animation>("models/chara_24/Run.animation"));
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
            // Add samplers
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
            
            anim_inst = animator->createInstance();
            if (!anim_inst.isValid()) {
                assert(false);
            }
        }
        // Collision
        shape_capsule.radius = 0.3f;
        collider.setShape(&shape_capsule);
        collider.setCenterOffset(gfxm::vec3(.0f, 1.f, .0f));
        collider.setPosition(getTranslation());
        collider.setRotation(getRotation());
        collider.collision_group = COLLISION_LAYER_CHARACTER;
        collider.collision_mask
            = COLLISION_LAYER_DEFAULT
            | COLLISION_LAYER_PROBE
            | COLLISION_LAYER_CHARACTER;

        shape_sphere.radius = 0.85f;
        collider_probe.setShape(&shape_sphere);
        collider_probe.collision_group = COLLISION_LAYER_PROBE;
        collider_probe.collision_mask
            = COLLISION_LAYER_BEACON;
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
                collider.setPosition(trsp->sync_pos);
                //setTranslation(trsp->sync_pos);
                setRotation(trsp->sync_rot);

                if (trsp->is_front) {
                    anim_inst->triggerSignal(animator->getSignalId("sig_door_open"));
                } else {
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
            //translate(loco_vec * dt * 5.0f);
            collider.translate(loco_vec * dt * 5.0f);

            gfxm::mat3 orient;
            orient[2] = forward_vec;
            orient[1] = gfxm::vec3(0, 1, 0);
            orient[0] = gfxm::cross(orient[1], orient[2]);
            setRotation(gfxm::slerp(getRotation(), gfxm::to_quat(orient), 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f)));
        }

        // Choose an actionable object if there are any available
        for (int i = 0; i < collider_probe.overlappingColliderCount(); ++i) {
            Collider* other = collider_probe.getOverlappingCollider(i);
            void* user_ptr = other->user_data.user_ptr;
            if (user_ptr && other->user_data.type == COLLIDER_USER_ACTOR) {
                targeted_actor = (gameActor*)user_ptr;
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
    void onUpdate(gameWorld* world, float dt) override {
        // Clear stuff
        targeted_actor = 0;

        setTranslation(collider.getPosition());

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
        collider.translate(rm_t);
        rotate(anim_inst->getSampleBuffer()->getRootMotionSample().r);
        
        // Ground raytest
        {
            RayCastResult r = world->getCollisionWorld()->rayTest(
                getTranslation() + gfxm::vec3(.0f, .3f, .0f),
                getTranslation() - gfxm::vec3(.0f, .35f, .0f),
                COLLISION_LAYER_DEFAULT
            );
            if (r.hasHit) {
                gfxm::vec3 pos = collider.getPosition();
                float y_offset = r.position.y - pos.y;
                collider.translate(gfxm::vec3(.0f, y_offset, .0f));
            }
        }

        // Update transforms
        model_inst->getSkeletonInstance()->getWorldTransformsPtr()[0] = getWorldTransform();

        //collider.position = getTranslation() + gfxm::vec3(0, 1.0f, 0);
        collider.setRotation(getRotation());
        collider_probe.setPosition(getWorldTransform() * gfxm::vec4(0, 0.5f, 0.64f, 1.0f));
        collider_probe.setRotation(gfxm::to_quat(gfxm::to_orient_mat3(getWorldTransform())));
    }

    void onSpawn(gameWorld* world) override {
        model_inst->spawn(world->getRenderScene());

        world->getRenderScene()->addRenderObject(decal.get());

        world->getRenderScene()->addNode(&caption_node);
        world->getRenderScene()->addRenderObject(name_caption.get());

        world->getCollisionWorld()->addCollider(&collider);
        world->getCollisionWorld()->addCollider(&collider_probe);
    }
    void onDespawn(gameWorld* world) override {
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
        .parent<gameActor>();
};


struct cameraState {
    gfxm::mat4 projection;
    gfxm::mat4 transform;
    gfxm::mat4 view;

    const gfxm::mat4& getProjection() { return projection; }
    const gfxm::mat4& getTransform() { return transform; }
    const gfxm::mat4& getView() { return view; }
};


#include "game/missile/missile.hpp"
#include "input/input.hpp"
class playerControllerFps {
    InputRange* inputRotation = 0;
    InputRange* inputLoco = 0;
    InputAction* inputSprint = 0;
    InputAction* inputShoot = 0;
    InputAction* inputInteract = 0;

    float total_distance_walked = .0f;
    gfxm::vec3 translation;
    gfxm::quat qcam;
    gfxm::quat qarms;
    float rotation_y = .0f;
    float rotation_x = .0f;// gfxm::pi * .5f;
    float cam_height = 1.6f;
    float sway_weight = .0f;

    float reload_time = .0f;
    float recoil_offset = .0f;

    RHSHARED<mdlSkeletalModelInstance> mdl_inst;

    RHSHARED<AudioClip> clip_rocket_launch;
public:
    playerControllerFps() {
        inputGetContext("Player")->toFront();
        inputRotation = inputCreateRange("CameraRotation");
        inputLoco = inputCreateRange("CharacterLocomotion");
        inputSprint = inputCreateAction("Sprint");
        inputShoot = inputCreateAction("Shoot");
        inputInteract = inputCreateAction("CharacterInteract");

        mdl_inst = resGet<mdlSkeletalModelMaster>("models/fps_q3_rocket_launcher/fps_q3_rocket_launcher.skeletal_model")->createInstance();
    
        clip_rocket_launch = resGet<AudioClip>("audio/sfx/rocket_launch.ogg");
    }

    void init(cameraState* camState, gameWorld* world) {
        platformLockMouse(true);
        platformHideMouse(true);

        camState->projection = gfxm::perspective(gfxm::radian(65), 16.0f / 9.0f, 0.01f, 1000.0f);
        
        mdl_inst->spawn(world->getRenderScene());
    }
    void update(gameWorld* world, float dt, cameraState* camState) {
        const float base_speed = 10.0f;
        float speed = base_speed;
        if (inputSprint->isPressed()) {
            speed *= 2.0f;
        }
        if (speed > .0f) {
            sway_weight = gfxm::lerp(sway_weight, 1.0f, 1 - pow(1 - 0.1f * 3.0f, dt * 3.0f));
        } else {
            sway_weight = gfxm::lerp(sway_weight, 0.0f, 1 - pow(1 - 0.1f * 3.0f, dt * 3.0f));
        }

        rotation_y += gfxm::radian(inputRotation->getVec3().y) *.1f;// *100.f * dt;
        rotation_x += gfxm::radian(inputRotation->getVec3().x) *.1f;// *100.f * dt;
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
        qcam = qy * qx;// gfxm::slerp(qcam, qy * qx, 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f)/* 0.1f*/);
        qarms = gfxm::slerp(qarms, qcam, 1 - pow(1 - 0.1f * 3.0f, dt * 120.0f)/* 0.1f*/);
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

        if (inputInteract->isJustPressed()) {
            gfxm::vec3 from = cam_translation * gfxm::vec4(0, 0, 0, 1);
            gfxm::vec3 to = gfxm::vec3(-gfxm::to_mat4(qcam)[2] * 1.2f) + from;
            RayCastResult r = world->getCollisionWorld()->rayTest(from, to);
            if (r.collider) {
                void* user_ptr = r.collider->user_data.user_ptr;
                if (user_ptr && r.collider->user_data.type == COLLIDER_USER_ACTOR) {
                    gameActor* actor = (gameActor*)user_ptr;
                    wRsp rsp = actor->sendMessage(wMsgMake(wMsgInteract{ 0 }));
                }
            }
        }
        if (inputShoot->isPressed() && reload_time <= .0f) {
            auto missile = world->spawnActorTransient<actorMissile>();
            missile->getRoot()->setTranslation(camState->transform * gfxm::vec4(0, 0, 0, 1));
            missile->getRoot()->setRotation(qcam);
            missile->getRoot()->translate(-missile->getRoot()->getWorldForward() * 1.f);

            audio().playOnce(clip_rocket_launch->getBuffer(), 1.0f);

            reload_time = 0.8f;
            recoil_offset = .2f;
        }
    }
};
