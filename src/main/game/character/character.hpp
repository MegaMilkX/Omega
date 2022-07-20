#pragma once

#include "game/actor/actor.hpp"

#include "assimp_load_scene.hpp"
#include "animation/animation.hpp"
#include "common/collision/collision_world.hpp"
#include "gpu/gpu.hpp"
#include "gpu/render/uniform.hpp"
#include "game/animator/animation_sampler.hpp"
#include "game/animator/animator.hpp"
#include "game/world/world.hpp"


class Door : public Actor {
    CollisionSphereShape     shape_sphere;
    Collider                 collider_beacon;
public:
    Actor ref_point_front;
    Actor ref_point_back;

    void init(CollisionWorld* collision_world) {
        setTranslation(gfxm::vec3(1, 1.0f, 6.0f));

        shape_sphere.radius = 0.1f;
        collider_beacon.setShape(&shape_sphere);
        collision_world->addCollider(&collider_beacon);
        collider_beacon.position = getTranslation();
        collider_beacon.setUserPtr(this);

        // Ref points for the character to adjust to for door opening animations
        gfxm::vec3 door_pos = getTranslation();
        door_pos.y = .0f;
        ref_point_front.setTranslation(door_pos + gfxm::vec3(0, 0, 1));
        ref_point_front.setRotation(gfxm::angle_axis(gfxm::pi, gfxm::vec3(0, 1, 0)));
        ref_point_back.setTranslation(door_pos + gfxm::vec3(0, 0, -1));
    }
};


enum class CHARA_ANIM_STATE {
    LOCOMOTION,
    DOOR_OPEN
};
struct CharacterAnimContext {
    float velocity = .0f;
    bool evtDoorOpen = false;
    bool out_evtDoorOpenDone = false;
};
class CharacterAnimState;
class CharacterAnimStateSwitchCondition {
public:
    CharacterAnimState* target = 0;
    virtual ~CharacterAnimStateSwitchCondition() {}
    virtual bool check(CharacterAnimContext& ctx) = 0;
};
class CharacterAnimStateSwitchConditionToDoorOpen : public CharacterAnimStateSwitchCondition {
public:
    bool check(CharacterAnimContext& ctx) override {
        return ctx.evtDoorOpen;
    }
};
class CharacterAnimStateSwitchConditionDoorOpenToLoco : public CharacterAnimStateSwitchCondition {
    bool check(CharacterAnimContext& ctx) override {
        return ctx.out_evtDoorOpenDone;
    }
};

class CharacterAnimState {
public:
    std::vector<CharacterAnimStateSwitchCondition*> conditions;

    virtual ~CharacterAnimState() {}
    virtual void onStart() = 0;
    virtual void onUpdate(CharacterAnimContext& ctx, float dt, AnimSample* out_samples, int max_samples, AnimSample* out_root_motion) = 0;
};
class CharacterAnimStateOneShot : public CharacterAnimState {
public:
    AnimationSampler sampler;
    float cursor_normal = .0f;

    void onStart() override {
        cursor_normal = .0f;
    }
    void onUpdate(CharacterAnimContext& ctx, float dt, AnimSample* out_samples, int max_samples, AnimSample* out_root_motion) override {
        float cursor_normal_prev = cursor_normal;
        cursor_normal += (sampler.getAnimation()->fps / sampler.getAnimation()->length) * dt;
        sampler.sample_normalized(out_samples, max_samples, cursor_normal);

        if (sampler.getAnimation()->hasRootMotion()) {
            sampler.getAnimation()->sample_root_motion(out_root_motion, cursor_normal_prev * sampler.getAnimation()->length, cursor_normal * sampler.getAnimation()->length);
        }

        if (cursor_normal >= 1.0f) {
            ctx.out_evtDoorOpenDone = true;
        }
    }
};
class CharacterAnimStateBlend2 : public CharacterAnimState {
public:
    AnimationSampler sampler_a;
    AnimationSampler sampler_b;
    float cursor_normal = .0f;
    float weight = .0f;

    void onStart() override {
        cursor_normal = .0f;
    }
    void onUpdate(CharacterAnimContext& ctx, float dt, AnimSample* out_samples, int max_samples, AnimSample* out_root_motion) override {
        weight = ctx.velocity;

        std::vector<AnimSample> samples_a
            = std::vector<AnimSample>(max_samples);
        std::vector<AnimSample> samples_b
            = std::vector<AnimSample>(max_samples);

        sampler_a.sample_normalized(samples_a.data(), samples_a.size(), cursor_normal);
        sampler_b.sample_normalized(samples_b.data(), samples_b.size(), cursor_normal);

        for (int i = 1; i < max_samples; ++i) {
            gfxm::vec3 tr = gfxm::lerp(samples_a[i].t, samples_b[i].t, weight);
            gfxm::quat rot = gfxm::slerp(samples_a[i].r, samples_b[i].r, weight);
            gfxm::vec3 scl = gfxm::lerp(samples_a[i].s, samples_b[i].s, weight);
            gfxm::mat4 m = gfxm::translate(gfxm::mat4(1.0f), tr)
                * gfxm::to_mat4(rot)
                * gfxm::scale(gfxm::mat4(1.0f), scl);
            AnimSample sample;
            sample.t = tr;
            sample.r = rot;
            sample.s = scl;
            out_samples[i] = sample;
        }

        float cursor_step_a = sampler_a.getAnimation()->fps / sampler_a.getAnimation()->length;
        float cursor_step_b = sampler_b.getAnimation()->fps / sampler_b.getAnimation()->length;
        float cursor_step = gfxm::lerp(cursor_step_a, cursor_step_b, weight);
        cursor_normal += cursor_step * dt;
        if (cursor_normal > 1.0f) {
            cursor_normal = cursor_normal - 1.0f;
        }
    }
};
#include "skeleton/skeleton_editable.hpp"
#include "skeleton/skeleton_instance.hpp"
class CharacterAnimator {
    float cursor_normal = .0f;
    sklSkeletonEditable* skeleton = 0;

    CHARA_ANIM_STATE state = CHARA_ANIM_STATE::LOCOMOTION;
public:
    AnimationSampler anim_idle;
    AnimationSampler anim_run;
    AnimationSampler anim_door_open;

    CharacterAnimContext context;

    CharacterAnimState* current_state = 0;
    CharacterAnimStateOneShot state_door_open;
    CharacterAnimStateBlend2 state_locomotion;

    CharacterAnimStateSwitchConditionToDoorOpen cond_to_door_open;
    CharacterAnimStateSwitchConditionDoorOpenToLoco cond_door_open_to_loco;

    std::vector<AnimSample> out_samples;
    AnimSample out_root_motion;

    void setSkeleton(sklSkeletonEditable* sk) {
        skeleton = sk;
        out_samples.resize(skeleton->boneCount());
    }

    void init() {
        context.evtDoorOpen = false;
        context.out_evtDoorOpenDone = false;
        context.velocity = .0f;

        cond_to_door_open.target = &state_door_open;
        cond_door_open_to_loco.target = &state_locomotion;

        state_locomotion.sampler_a = anim_idle;
        state_locomotion.sampler_b = anim_run;
        state_door_open.sampler = anim_door_open;

        state_locomotion.conditions.push_back(&cond_to_door_open);
        state_door_open.conditions.push_back(&cond_door_open_to_loco);

        current_state = &state_locomotion;
    }

    void update(float dt) {
        // Clear out-event notifiers
        context.out_evtDoorOpenDone = false;

        // Clear root motion data
        out_root_motion.t = gfxm::vec3(0, 0, 0);
        out_root_motion.r = gfxm::quat(0, 0, 0, 1);
        out_root_motion.s = gfxm::vec3(0, 0, 0);
        
        
        current_state->onUpdate(context, dt, out_samples.data(), out_samples.size(), &out_root_motion);
        for (int i = 0; i < current_state->conditions.size(); ++i) {
            if (current_state->conditions[i]->check(context)) {
                current_state = current_state->conditions[i]->target;
                current_state->onStart();
                break;
            }
        }

        // clear event flags
        context.evtDoorOpen = false;
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


#include "skeletal_model/skeletal_model.hpp"
#include "skeletal_model/skeletal_model_instance.hpp"
#include "import/assimp_load_skeletal_model.hpp"
class Character : public Actor {
    struct {
        RHSHARED<sklmSkeletalModelEditable> model;
        HSHARED<sklmSkeletalModelInstance>  model_inst;
    };

    std::unique_ptr<scnDecal> decal;
    std::unique_ptr<scnTextBillboard> name_caption;
    scnNode caption_node;
    // TEXT STUFF, MUST BE SHARED
    Typeface typeface;
    std::unique_ptr<Font> font;

    // Anim
    CharacterAnimator animator;
    HSHARED<Animation> anim_idle;
    HSHARED<Animation> anim_run;
    HSHARED<Animation> anim_door_open;

    // Gameplay
    Actor* targeted_actor = 0;
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
    void init(CollisionWorld* collision_world, gpuMaterial* material, wWorld* world) {
        {
            model.reset(HANDLE_MGR<sklmSkeletalModelEditable>::acquire());
            assimpLoadSkeletalModel("chara_24.fbx", model.get());
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

        anim_idle = resGet<Animation>("chara_24/Idle.sanim");
        anim_run = resGet<Animation>("chara_24/Run.sanim");
        anim_door_open.reset(HANDLE_MGR<Animation>::acquire());
        //importer.importAnimation(skeleton.get(), "Action_OpenDoor", anim_door_open.get(), "Root");

        animator.anim_idle = AnimationSampler(model->getSkeleton().get(), anim_idle.get());
        animator.anim_run = AnimationSampler(model->getSkeleton().get(), anim_run.get());
        animator.anim_door_open = AnimationSampler(model->getSkeleton().get(), anim_door_open.get());
        animator.setSkeleton(model->getSkeleton().get());
        animator.init();
        
        // Collision
        shape_capsule.radius = 0.3f;
        collider.setShape(&shape_capsule);
        collision_world->addCollider(&collider);

        shape_sphere.radius = 0.85f;
        collider_probe.setShape(&shape_sphere);
        collision_world->addCollider(&collider_probe);
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
            }
        }
    }

    void update_locomotion(float dt) {
        // Handle input
        velocity = gfxm::lerp(velocity, loco_vec.length(), 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f));
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
                targeted_actor = (Actor*)user_ptr;
                break;
            }
        }
    }
    void update_doorOpen(float dt) {
        if (animator.context.out_evtDoorOpenDone) {
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
        animator.context.velocity = velocity;
        animator.update(dt);
        for (int i = 1; i < animator.out_samples.size(); ++i) {
            auto& s = animator.out_samples[i];
            gfxm::mat4 m = gfxm::translate(gfxm::mat4(1.0f), s.t)
                * gfxm::to_mat4(s.r)
                * gfxm::scale(gfxm::mat4(1.0f), s.s);
            model_inst->getSkeletonInstance()->getLocalTransformsPtr()[i] = m;
        }

        // Apply root motion
        translate(gfxm::vec3(getWorldTransform() * gfxm::vec4(animator.out_root_motion.t, .0f)));

        // Update transforms
        model_inst->getSkeletonInstance()->getWorldTransformsPtr()[0] = getWorldTransform();

        collider.position = getTranslation() + gfxm::vec3(0, 1.0f, 0);
        collider.rotation = getRotation();
        collider_probe.position = getWorldTransform() * gfxm::vec4(0, 0.5f, 0.64f, 1.0f);
        collider_probe.rotation = gfxm::to_quat(gfxm::to_orient_mat3(getWorldTransform()));
    }

    void onSpawn(wWorld* world) {
        model_inst->onSpawn(world->getRenderScene());

        world->getRenderScene()->addRenderObject(decal.get());

        world->getRenderScene()->addNode(&caption_node);
        world->getRenderScene()->addRenderObject(name_caption.get());
    }
    void onDespawn(wWorld* world) {
        model_inst->onDespawn(world->getRenderScene());

        world->getRenderScene()->removeRenderObject(decal.get());

        world->getRenderScene()->removeNode(&caption_node);
        world->getRenderScene()->removeRenderObject(name_caption.get());
    }
};