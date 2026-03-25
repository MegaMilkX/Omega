#pragma once

#include "assimp_load_scene.hpp"
#include "animation/animation.hpp"
#include "collision/phy.hpp"
#include "gpu/gpu.hpp"
#include "gpu/render/uniform.hpp"
#include "animation/animation_sample_buffer.hpp"
#include "animation/animation_sampler.hpp"
#include "world/world.hpp"

#include "skeletal_model/skeletal_model.hpp"
#include "skeletal_model/skeletal_model_instance.hpp"

#include "animation/animator/animator.hpp"

#include "util/static_block.hpp"

class actorAnimatedSkeletalModel : public Actor {

    RHSHARED<SkeletalModelInstance> model_inst;
    HSHARED<AnimatorInstance> animator_inst;
public:
    TYPE_ENABLE();
    // TODO
};

class actorJukebox : public Actor {

    RHSHARED<SkeletalModelInstance> model_inst;
    phySphereShape     shape_sphere;
    phyRigidBody                 collider_beacon;
    ResourceRef<AudioClip>      audio_clip_click;
    ResourceRef<AudioClip>      audio_clip;
    Handle<AudioChannel>     audio_ch;
public:
    TYPE_ENABLE();
    actorJukebox() {
        setFlagsDefault();

        auto model = resGet<SkeletalModel>("models/jukebox_low/jukebox_low.skeletal_model");
        model_inst = model->createInstance();
        model_inst->setExternalRootTransform(getRoot()->getTransformHandle());

        translate(gfxm::vec3(-2, 0, 4));
        setScale(gfxm::vec3(1, 1, 1) * 0.35f);

        shape_sphere.radius = 0.5f;
        collider_beacon.setShape(&shape_sphere);
        collider_beacon.setPosition(gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(-2, 0, 4) + gfxm::vec3(0, 1, 0)) * gfxm::vec4(0,0,0,1));
        collider_beacon.user_data.type = COLLIDER_USER_ACTOR;
        collider_beacon.user_data.user_ptr = this;
        collider_beacon.collision_group = COLLISION_LAYER_BEACON;
        collider_beacon.collision_mask = COLLISION_LAYER_PROBE;

        audio_clip_click = loadResource<AudioClip>("audio/sfx/switch_click");
        audio_clip = loadResource<AudioClip>(
            //"audio/track02"
            "audio/SeaShanty2"
            //"audio/subways"
            //"audio/sfx/cow"
        );
        audio_ch = audioCreateChannel();
        audioSetBuffer(audio_ch, audio_clip->getBuffer());
        audioSetPosition(audio_ch, collider_beacon.getPosition());
        audioSetLooping(audio_ch, true);
        audioSetAttenuationRadius(audio_ch, 3.f);
        audioSetGain(audio_ch, .5f);
    }
    ~actorJukebox() {
        audioFreeChannel(audio_ch);
    }
    void onSpawn(WorldSystemRegistry& reg) override {
        auto old_scene_sys = reg.getSystem<scnRenderScene>();
        auto scene_sys = reg.getSystem<SceneSystem>();
        if(old_scene_sys || scene_sys) {
            model_inst->spawnModel(scene_sys, old_scene_sys);
            model_inst->enableTechnique("Outline", false);
        }

        if(auto sys = reg.getSystem<phyWorld>()) {
            sys->addCollider(&collider_beacon);
        }

        Actor::onSpawn(reg);
    }
    void onDespawn(WorldSystemRegistry& reg) override {
        auto old_scene_sys = reg.getSystem<scnRenderScene>();
        auto scene_sys = reg.getSystem<SceneSystem>();
        if(old_scene_sys || scene_sys) {
            model_inst->despawnModel(scene_sys, old_scene_sys);
        }

        if(auto sys = reg.getSystem<phyWorld>()) {
            sys->removeCollider(&collider_beacon);
        }

        Actor::onDespawn(reg);
    }

    GAME_MESSAGE onMessage(GAME_MESSAGE msg) { 
        switch (msg.msg) {
        case GAME_MSG::INTERACT: {
            auto m = msg.getPayload<GAME_MSG::INTERACT>();
            audioPlayOnce3d(audio_clip_click->getBuffer(), getTranslation(), .5f);
            if (audioIsPlaying(audio_ch)) {
                audioStop(audio_ch);
            } else {
                audioPlay3d(audio_ch);
            }
            return GAME_MSG::HANDLED;
        }
        case GAME_MSG::HIGHLIGHT_ON: {
            model_inst->enableTechnique("Outline", true);
            return GAME_MSG::HANDLED;
        }
        case GAME_MSG::HIGHLIGHT_OFF: {
            model_inst->enableTechnique("Outline", false);
            return GAME_MSG::HANDLED;
        }
        }
        return Actor::onMessage(msg);
    }
};

class actorAnimTest : public Actor {

    HSHARED<SkeletalModelInstance> model_inst;
    RHSHARED<AnimatorMaster> animator;
    HSHARED<AnimatorInstance> anim_inst;
    ResourceRef<Animation> anm_idle;
    ResourceRef<Animation> anm_run2;
public:
    TYPE_ENABLE();
    actorAnimTest() {
        setFlags(ACTOR_FLAG_UPDATE);

        {/*
            expr_ e = (param_(&animator, "velocity") + 10.0f) % 9.f;
            e.dbgLog(&animator);
            value_ v = e.evaluate(&animator);
            v.dbgLog();*/
        }

        auto model = resGet<SkeletalModel>("models/chara_24/chara_24.skeletal_model");
        model_inst = model->createInstance();
        model_inst->setExternalRootTransform(getRoot()->getTransformHandle());
        translate(gfxm::vec3(4, 0, 0));
        
        {
            anm_idle = loadResource<Animation>("models/chara_24/Idle");
            anm_run2 = loadResource<Animation>("models/chara_24/Run");

            animator.reset_acquire();
            animator->setSkeleton(model->getSkeleton());
            animator->addParam("velocity");
            animator->addParam("is_falling");
            animator->addParam("test");

            animator->addSampler("idle", "Default", anm_idle);
            animator->addSampler("run2", "Default", anm_run2);

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
            
            animUnitFsm* fsm = new animUnitFsm;
            animator->setRoot(fsm);
            
            auto state = fsm->addState("stateA");
            auto single = new animUnitSingle;
            single->setSampler("idle");
            state->setUnit(single);

            auto state2 = fsm->addState("stateB");
            auto single2 = new animUnitSingle;
            single2->setSampler("run2");
            state2->setUnit(single2);

            fsm->addTransition(
                "stateA", "stateB",
                "state_complete", 0.2f
            );
            fsm->addTransition(
                "stateB", "stateA",
                "state_complete", 0.2f
            );

            animator->compile();

            anim_inst = animator->createInstance();
        }
    }

    void onSpawn(WorldSystemRegistry& reg) override {
        auto old_scene_sys = reg.getSystem<scnRenderScene>();
        auto scene_sys = reg.getSystem<SceneSystem>();
        if(old_scene_sys || scene_sys) {
            model_inst->spawnModel(scene_sys, old_scene_sys);
        }
        Actor::onSpawn(reg);
    }
    void onDespawn(WorldSystemRegistry& reg) override {
        auto old_scene_sys = reg.getSystem<scnRenderScene>();
        auto scene_sys = reg.getSystem<SceneSystem>();
        if(old_scene_sys || scene_sys) {
            model_inst->despawnModel(scene_sys, old_scene_sys);
        }
        Actor::onDespawn(reg);
    }

    void onUpdate(float dt) override {
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

#include "animation/hitbox_sequence/hitbox_seq_sample_buffer.hpp"
#include "animation/hitbox_sequence/hitbox_sequence.hpp"

#include "animation/audio_sequence/audio_sequence.hpp"

#include "animation/animator/animator_instance.hpp"

#include "animation/model_sequence/model_sequence.hpp"
class actorVfxTest : public Actor {

    RHSHARED<SkeletalModel> model;
    RHSHARED<AnimatorMaster> animator;

    HSHARED<SkeletalModelInstance> model_inst;
    HSHARED<AnimatorInstance> anim_inst;

    ResourceRef<Animation> anm_test;
    ResourceRef<Animation> anim_skl;
    RHSHARED<animModelSequence> anim_mdl;
    animModelSampleBuffer sample_buf;
    animModelAnimMapping mapping;
public:
    TYPE_ENABLE();
    actorVfxTest() {
        setFlags(ACTOR_FLAG_UPDATE);

        model.reset_acquire();
        
        model->getSkeleton()->getRoot()->createChild("Root_B");
        
        auto decal = model->addComponent<sklmDecalComponent>("decal");
        decal->bone_name = "Root_B";
        decal->material = resGet<gpuMaterial>("materials/decals/glow.mat");

        model_inst = model->createInstance();
        model_inst->setExternalRootTransform(getRoot()->getTransformHandle());
        translate(gfxm::vec3(3, 0, -5));
        
        {
            anim_skl = ResourceManager::get()->create<Animation>("");
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
            anm_test = anim_skl;
        }

        {
            animator.reset_acquire();
            animator->setSkeleton(model->getSkeleton());
            animator->addSampler("my_loop", "default", anm_test);
            auto fsm = new animUnitFsm;
            animator->setRoot(fsm);
            auto state_default = fsm->addState("default");
            auto single = new animUnitSingle;
            state_default->setUnit(single);
            single->setSampler("my_loop");

            animator->compile();

            anim_inst = animator->createInstance();
        }
    }
    void onSpawn(WorldSystemRegistry& reg) override {
        auto old_scene_sys = reg.getSystem<scnRenderScene>();
        auto scene_sys = reg.getSystem<SceneSystem>();
        if(old_scene_sys || scene_sys) {
            model_inst->spawnModel(scene_sys, old_scene_sys);
        }
        Actor::onSpawn(reg);
    }
    void onDespawn(WorldSystemRegistry& reg) override {
        auto old_scene_sys = reg.getSystem<scnRenderScene>();
        auto scene_sys = reg.getSystem<SceneSystem>();
        if(old_scene_sys || scene_sys) {
            model_inst->despawnModel(scene_sys, old_scene_sys);
        }
        Actor::onDespawn(reg);
    }
    void onUpdate(float dt) override {
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

class actorUltimaWeapon : public Actor {
    phyWorld* collision_world = nullptr;
    HSHARED<SkeletalModelInstance> model_inst;
    RHSHARED<AnimatorMaster> animator;
    HSHARED<AnimatorInstance> anim_inst;

    RHSHARED<hitboxCmdSequence> hitbox_seq;
    hitboxCmdBuffer hitbox_cmd_buf;

    ResourceRef<Animation> anm_idle;
public:
    TYPE_ENABLE();
    actorUltimaWeapon() {
        setFlags(ACTOR_FLAG_UPDATE);

        auto model = resGet<SkeletalModel>("models/ultima_weapon/ultima_weapon.skeletal_model");
        
        model_inst = model->createInstance();
        model_inst->setExternalRootTransform(getRoot()->getTransformHandle());
        getRoot()->setTranslation(6, 0, -6);
        getRoot()->setScale(2, 2, 2);
        

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
        anm_idle = loadResource<Animation>("models/ultima_weapon/Idle");
        anm_idle->setHitboxSequence(hitbox_seq);

        // Animator
        animator.reset_acquire();
        animator->setSkeleton(model->getSkeleton());
        animator->addSampler("idle", "Default", anm_idle);

        auto single = new animUnitSingle;
        animator->setRoot(single);
        single->setSampler("idle");
        animator->compile();

        anim_inst = animator->createInstance();
    }
    void onSpawn(WorldSystemRegistry& reg) override {
        auto old_scene_sys = reg.getSystem<scnRenderScene>();
        auto scene_sys = reg.getSystem<SceneSystem>();
        if(old_scene_sys || scene_sys) {
            model_inst->spawnModel(scene_sys, old_scene_sys);
        }
        
        collision_world = reg.getSystem<phyWorld>();
        Actor::onSpawn(reg);
    }
    void onDespawn(WorldSystemRegistry& reg) override {
        auto old_scene_sys = reg.getSystem<scnRenderScene>();
        auto scene_sys = reg.getSystem<SceneSystem>();
        if(old_scene_sys || scene_sys) {
            model_inst->despawnModel(scene_sys, old_scene_sys);
        }
        
        collision_world = 0;
        Actor::onDespawn(reg);
    }
    void onUpdate(float dt) override {
        anim_inst->update(dt);
        anim_inst->getSampleBuffer()->applySamples(model_inst->getSkeletonInstance());
        anim_inst->getHitboxCmdBuffer()->execute(model_inst->getSkeletonInstance(), collision_world);
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

class DoorActor : public Actor {

    RHSHARED<SkeletalModel> model;
    HSHARED<SkeletalModelInstance> model_inst;

    ResourceRef<Animation>  anim_open;
    animSampler    anim_sampler;
    animSampleBuffer    samples;

    phySphereShape     shape_sphere;
    phyRigidBody                 collider_beacon;

    bool is_opening = false;
    float anim_cursor = .0f;
public:
    TYPE_ENABLE();
    DoorActor() {
        setFlags(ACTOR_FLAG_UPDATE);

        model = resGet<SkeletalModel>("models/door/door.skeletal_model");
        model_inst = model->createInstance();
        model_inst->setExternalRootTransform(getRoot()->getTransformHandle());

        anim_open = loadResource<Animation>("models/door/Open");
        anim_sampler = animSampler(model->getSkeleton().get(), anim_open.get());
        samples.init(model->getSkeleton().get());

        setTranslation(gfxm::vec3(1, 0, 6.0f));

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
        //ref_point_front.setTranslation(door_pos + gfxm::vec3(0, 0, 1));
        //ref_point_front.setRotation(gfxm::angle_axis(gfxm::pi, gfxm::vec3(0, 1, 0)));
        //ref_point_back.setTranslation(door_pos + gfxm::vec3(0, 0, -1));
    }

    void onSpawn(WorldSystemRegistry& reg) override {
        auto old_scene_sys = reg.getSystem<scnRenderScene>();
        auto scene_sys = reg.getSystem<SceneSystem>();
        if(old_scene_sys || scene_sys) {
            model_inst->spawnModel(scene_sys, old_scene_sys);
            model_inst->enableTechnique("Outline", false);
        }

        if(auto sys = reg.getSystem<phyWorld>()) {
            sys->addCollider(&collider_beacon);
        }
        Actor::onSpawn(reg);
    }
    void onDespawn(WorldSystemRegistry& reg) override {
        auto old_scene_sys = reg.getSystem<scnRenderScene>();
        auto scene_sys = reg.getSystem<SceneSystem>();
        if(old_scene_sys || scene_sys) {
            model_inst->despawnModel(scene_sys, old_scene_sys);
        }

        if(auto sys = reg.getSystem<phyWorld>()) {
            sys->removeCollider(&collider_beacon);
        }
        Actor::onDespawn(reg);
    }

    void onUpdate(float dt) override {
        collider_beacon.setPosition(getTranslation() + gfxm::vec3(.0f, 1.0f, .0f) + getLeft() * .5f);
        if (is_opening) {            
            anim_sampler.sample(samples.data(), samples.count(), anim_cursor * anim_open->fps);

            for (int i = 1; i < samples.count(); ++i) {
                auto& s = samples[i];
                model_inst->getSkeletonInstance()->getBoneNode(i)->setTranslation(s.t);
                model_inst->getSkeletonInstance()->getBoneNode(i)->setRotation(s.r);
                model_inst->getSkeletonInstance()->getBoneNode(i)->setScale(s.s);
            }
            anim_cursor += dt;
            if (anim_cursor >= anim_open->length) {
                is_opening = false;
            }
        }
    }

    GAME_MESSAGE onMessage(GAME_MESSAGE msg) override {
        if (msg.msg == GAME_MSG::INTERACT) {
            LOG_DBG("Door: Open message received");
            auto m = msg.getPayload<GAME_MSG::INTERACT>();
            is_opening = true;
            anim_cursor = .0f;
            gfxm::vec3 door_pos = getTranslation();
            gfxm::vec3 initiator_pos = m.sender->getTranslation();
            gfxm::vec3 initiator_N = gfxm::normalize(initiator_pos - door_pos);
            gfxm::vec3 front_N = gfxm::normalize(getWorldTransform() * gfxm::vec3(0, 0, -1));
            float d = gfxm::dot(initiator_N, front_N);
            int sync_bone_id = -1;
            if (d > .0f) {
                sync_bone_id = model->getSkeleton()->findBone("SYNC_Front")->getIndex();
            }
            else {
                sync_bone_id = model->getSkeleton()->findBone("SYNC_Back")->getIndex();
            }
            const gfxm::mat4& sync_point_trs = model_inst->getSkeletonInstance()->getBoneNode(sync_bone_id)->getWorldTransform();
            gfxm::vec3 sync_pos = sync_point_trs * gfxm::vec4(0, 0, 0, 1);
            gfxm::quat sync_rot = gfxm::to_quat(gfxm::to_orient_mat3(sync_point_trs));
            return makeGameMessage(PAYLOAD_RESPONSE_DOOR_OPEN{ sync_pos, sync_rot, d > .0f });
        } else if (msg.msg == GAME_MSG::HIGHLIGHT_ON) {
            model_inst->enableTechnique("Outline", true);
            return GAME_MSG::HANDLED;
        } else if (msg.msg == GAME_MSG::HIGHLIGHT_OFF) {
            model_inst->enableTechnique("Outline", false);
            return GAME_MSG::HANDLED;
        }
        return Actor::onMessage(msg);
    }
};


enum class CHARACTER_STATE {
    LOCOMOTION,
    DOOR_OPEN
};

#include "world/node/node_character_capsule.hpp"
#include "world/node/node_skeletal_model.hpp"
#include "world/node/node_decal.hpp"

class actorCharacter : public Actor {

    struct {
        HSHARED<SkeletalModelInstance>  model_inst;
    };

    phyWorld* collision_world = nullptr;

    std::unique_ptr<scnDecal> decal;
    std::unique_ptr<scnTextBillboard> name_caption;
    //scnNode caption_node;
    std::shared_ptr<Font> font;

    // New Anim
    RHSHARED<AnimatorMaster> animator;
    HSHARED<AnimatorInstance> anim_inst;
    ResourceRef<Animation> anm_idle;
    ResourceRef<Animation> anm_run2;
    ResourceRef<Animation> anm_open_door_front;
    ResourceRef<Animation> anm_open_door_back;

    RHSHARED<audioSequence> audio_seq;

    // Gameplay
    Actor* targeted_actor = 0;
    CHARACTER_STATE state = CHARACTER_STATE::LOCOMOTION;

    gfxm::vec3 forward_vec = gfxm::vec3(0, 0, 1);
    gfxm::vec3 loco_vec_tgt;
    gfxm::vec3 loco_vec;
    float velocity = .0f;

    // Collision
    phyCapsuleShape    shape_capsule;
    phyRigidBody                 collider;
    phySphereShape     shape_sphere;
    phyProbe            collider_probe;
public:
    TYPE_ENABLE();
    actorCharacter() {
        setFlags(ACTOR_FLAG_UPDATE);

        auto model = resGet<SkeletalModel>("models/chara_24/chara_24.skeletal_model");
        {
            model_inst = model->createInstance();
            model_inst->setExternalRootTransform(getRoot()->getTransformHandle());
        }
        
        // Audio
        audio_seq.reset_acquire();
        audio_seq->length = 40.0f;
        audio_seq->fps = 60.0f;
        audio_seq->insert(0, loadResource<AudioClip>("audio/sfx/footsteps/asphalt03"));
        audio_seq->insert(20, loadResource<AudioClip>("audio/sfx/footsteps/asphalt04"));
        
        decal.reset(new scnDecal);
        decal->setMaterial(resGet<gpuMaterial>("materials/decals/chara_circle.mat"));
        decal->setBoxSize(1.3f, 1.0f, 1.3f);
        decal->setTransformNode(model_inst->getBoneProxy(0));
        //decal->setSkeletonNode(model_inst->getSkeletonInstance()->getScnSkeleton(), 0);
        
        font = fontGet("fonts/OpenSans-Regular.ttf", 32, 72);
        name_caption.reset(new scnTextBillboard());
        name_caption->setFont(font);
        name_caption->setTransformNode(model_inst->getBoneProxy(16));
        //name_caption->setSkeletonNode(model_inst->getSkeletonInstance()->getScnSkeleton(), 16);
        //caption_node.local_transform = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(.0f, 1.9f, .0f));
        //caption_node.attachToSkeleton(model_inst->getSkeletonInstance()->getScnSkeleton(), 0);
        //name_caption->setNode(&caption_node);
        
        // Animator
        {
            anm_idle = loadResource<Animation>("models/chara_24/Idle");
            anm_run2 = loadResource<Animation>("models/chara_24/Run");
            anm_run2->setAudioSequence(audio_seq);
            anm_open_door_front = loadResource<Animation>("models/chara_24/Falling");
            anm_open_door_back = loadResource<Animation>("models/chara_24/Falling");
            
            animator.reset_acquire();
            animator->setSkeleton(model->getSkeleton());
            // Setup parameters signals and events
            animator->addParam("velocity");
            animator->addParam("is_falling");
            animator->addSignal("sig_door_open");
            animator->addSignal("sig_door_open_back");
            animator->addFeedbackEvent("fevt_door_open_end");
            // Add samplers
            animator
                ->addSampler("idle", "Default", anm_idle)
                .addSampler("run", "Locomotion", anm_run2)
                .addSampler("open_door_front", "Interact", anm_open_door_front)
                .addSampler("open_door_back", "Interact", anm_open_door_back);
            
            // Setup the tree
            auto fsm = new animUnitFsm;
            animator->setRoot(fsm);
            auto st_idle = fsm->addState("Idle");
            auto st_loco = fsm->addState("Locomotion");
            auto st_door_open_front = fsm->addState("DoorOpenFront");
            auto st_door_open_back = fsm->addState("DoorOpenBack");
            auto unitIdle = new animUnitSingle;
            auto unitRun = new animUnitSingle;
            auto unitOpenDoorFront = new animUnitSingle;
            auto unitOpenDoorBack = new animUnitSingle;
            unitIdle->setSampler("idle");
            unitRun->setSampler("run");
            unitOpenDoorFront->setSampler("open_door_front");
            unitOpenDoorBack->setSampler("open_door_back");
            st_idle->setUnit(unitIdle);
            st_loco->setUnit(unitRun);
            st_door_open_front->setUnit(unitOpenDoorFront);
            st_door_open_front->onExit("@fevt_door_open_end");
            st_door_open_back->setUnit(unitOpenDoorBack);
            st_door_open_back->onExit("@fevt_door_open_end");
            fsm->addTransition("Idle", "Locomotion", "velocity > .00001", 0.15f);
            fsm->addTransition("Locomotion", "Idle", "velocity <= .00001", 0.15f);
            fsm->addTransitionAnySource("DoorOpenFront", "sig_door_open", 0.15f);
            fsm->addTransitionAnySource("DoorOpenBack", "sig_door_open_back", 0.15f);
            fsm->addTransition("DoorOpenFront", "Idle", "state_complete", 0.15f);
            fsm->addTransition("DoorOpenBack", "Idle", "state_complete", 0.15f);
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
            GAME_MESSAGE rsp = targeted_actor->sendMessage(PAYLOAD_INTERACT{ this });
            if (rsp.msg == GAME_MSG::RESPONSE_DOOR_OPEN) {
                auto rsp_payload = rsp.getPayload<GAME_MSG::RESPONSE_DOOR_OPEN>();
                collider.setPosition(rsp_payload.sync_pos);
                //setTranslation(trsp->sync_pos);
                setRotation(rsp_payload.sync_rot);

                if (rsp_payload.is_front) {
                    anim_inst->triggerSignal(animator->getSignalId("sig_door_open"));
                }
                else {
                    anim_inst->triggerSignal(animator->getSignalId("sig_door_open_back"));
                }
                state = CHARACTER_STATE::DOOR_OPEN;
                velocity = .0f;
                loco_vec = gfxm::vec3(0, 0, 0);
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
            phyRigidBody* other = collider_probe.getOverlappingCollider(i);
            void* user_ptr = other->user_data.user_ptr;
            if (user_ptr && other->user_data.type == COLLIDER_USER_ACTOR) {
                targeted_actor = (Actor*)user_ptr;
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
    void onUpdate(float dt) override {
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
            float radius = .2f;
            phySphereSweepResult ssr = collision_world->sphereSweep(
                getTranslation() + gfxm::vec3(.0f, .3f, .0f),
                getTranslation() + gfxm::vec3(.0f, .35f, .0f),
                radius,
                COLLISION_LAYER_DEFAULT
            );
            if (ssr.hasHit) {
                gfxm::vec3 pos = collider.getPosition();
                float y_offset = ssr.sphere_pos.y - radius - pos.y;
                collider.translate(gfxm::vec3(.0f, y_offset, .0f));
            }
            /*
            phyRayCastResult r = world->getCollisionWorld()->rayTest(
                getTranslation() + gfxm::vec3(.0f, .3f, .0f),
                getTranslation() - gfxm::vec3(.0f, .35f, .0f),
                COLLISION_LAYER_DEFAULT
            );
            if (r.hasHit) {
                gfxm::vec3 pos = collider.getPosition();
                float y_offset = r.position.y - pos.y;
                collider.translate(gfxm::vec3(.0f, y_offset, .0f));
            }*/
        }

        //collider.position = getTranslation() + gfxm::vec3(0, 1.0f, 0);
        collider.setRotation(getRotation());
        collider_probe.setPosition(getWorldTransform() * gfxm::vec4(0, 0.5f, 0.64f, 1.0f));
        collider_probe.setRotation(gfxm::to_quat(gfxm::to_orient_mat3(getWorldTransform())));
    }

    void onSpawn(WorldSystemRegistry& reg) override {
        auto old_scene_sys = reg.getSystem<scnRenderScene>();
        auto scene_sys = reg.getSystem<SceneSystem>();
        if(old_scene_sys || scene_sys) {
            model_inst->spawnModel(scene_sys, old_scene_sys);
        }
        if(auto sys = reg.getSystem<scnRenderScene>()) {
            sys->addRenderObject(decal.get());
            sys->addRenderObject(name_caption.get());
        }
        if(auto sys = reg.getSystem<phyWorld>()) {
            sys->addCollider(&collider);
            sys->addCollider(&collider_probe);
            collision_world = sys;
        }
        Actor::onSpawn(reg);
    }
    void onDespawn(WorldSystemRegistry& reg) override {
        auto old_scene_sys = reg.getSystem<scnRenderScene>();
        auto scene_sys = reg.getSystem<SceneSystem>();
        if(old_scene_sys || scene_sys) {
            model_inst->despawnModel(scene_sys, old_scene_sys);
        }
        if(auto sys = reg.getSystem<scnRenderScene>()) {
            sys->removeRenderObject(decal.get());
            sys->removeRenderObject(name_caption.get());
        }
        if(auto sys = reg.getSystem<phyWorld>()) {
            sys->removeCollider(&collider);
            sys->removeCollider(&collider_probe);
            collision_world = nullptr;
        }
        Actor::onDespawn(reg);
    }
};


struct cameraState {
    gfxm::mat4 projection;
    gfxm::mat4 transform;
    gfxm::mat4 view;

    const gfxm::mat4& getProjection() { return projection; }
    const gfxm::mat4& getTransform() { return transform; }
    const gfxm::mat4& getView() { return view; }
};

