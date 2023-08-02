#pragma once

#include "actor_controller.hpp"
#include "world/component/components.hpp"

#include "input/input.hpp"
#include "world/actor.hpp"
#include "world/world.hpp"

#include "world/node/node_skeletal_model.hpp"


class ctrlCharacterPlayerInput
    : public ActorControllerT<EXEC_PRIORITY_PRE_ANIMATION> {
    InputRange* rangeTranslation = 0;
    InputAction* actionInteract = 0;

    AnimatorComponent* animComponent = 0;
public:
    ctrlCharacterPlayerInput() {
        rangeTranslation = inputGetRange("CharacterLocomotion");
        actionInteract = inputGetAction("CharacterInteract");
    }
    void onReset() override {}
    void onSpawn(gameActor* actor) override {
        animComponent = actor->getComponent<AnimatorComponent>();
    }
    void onDespawn(gameActor* actor) override {}
    void onActorNodeRegister(type t, gameActorNode* component, const std::string& name) override {}
    void onActorNodeUnregister(type t, gameActorNode* component, const std::string& name) override {}
    void onUpdate(gameWorld* world, float dt) override {
        auto actor = getOwner();
        auto cam_node = world->getCurrentCameraNode();
        
        auto root = actor->getRoot();
        if (rangeTranslation->getVec3().length() > FLT_EPSILON) {
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
            gfxm::vec3 loco_vec = orient * rangeTranslation->getVec3();

            root->translate(gfxm::normalize(loco_vec) * dt * 5.f);

            orient[2] = loco_vec;
            orient[1] = gfxm::vec3(0, 1, 0);
            orient[0] = gfxm::cross(orient[1], orient[2]);
            root->setRotation(gfxm::slerp(root->getRotation(), gfxm::to_quat(orient), 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f)));
        }

        if (animComponent) {
            auto anim_inst = animComponent->getAnimatorInstance();
            auto anim_master = animComponent->getAnimatorMaster();
            anim_inst->setParamValue(anim_master->getParamId("velocity"), rangeTranslation->getVec3().length());
        }
    }
};

class AnimatorController
    : public ActorControllerT<EXEC_PRIORITY_PRE_ANIMATION> {

    AnimatorComponent* animComponent = 0;
    std::set<SkeletalModelNode*> skeletal_models;
public:
    void onReset() override {}
    void onSpawn(gameActor* actor) override {
        animComponent = actor->getComponent<AnimatorComponent>();
    }
    void onDespawn(gameActor* actor) override {}
    void onActorNodeRegister(type t, gameActorNode* component, const std::string& name) override {
        if (t == type_get<SkeletalModelNode>()) {
            skeletal_models.insert((SkeletalModelNode*)component);
        }
    }
    void onActorNodeUnregister(type t, gameActorNode* component, const std::string& name) override {
        if (t == type_get<SkeletalModelNode>()) {
            skeletal_models.erase((SkeletalModelNode*)component);
        }
    }
    void onUpdate(gameWorld* world, float dt) override {
        auto root = getOwner()->getRoot();
        if (animComponent && root) {
            auto anim_inst = animComponent->getAnimatorInstance();
            anim_inst->update(dt);

            for (auto& sm : skeletal_models) {
                auto anim_inst_skel = anim_inst->getSkeletonMaster();
                auto model_inst_skel = sm->getModelInstance()->getSkeletonMaster();
                if (anim_inst_skel != model_inst_skel) {
                    continue;
                }
                anim_inst->getSampleBuffer()->applySamples(sm->getModelInstance()->getSkeletonInstance());
                anim_inst->getAudioCmdBuffer()->execute(sm->getModelInstance()->getSkeletonInstance());
            }

            // Apply root motion
            gfxm::vec3 rm_t = gfxm::vec3(root->getWorldTransform() * gfxm::vec4(anim_inst->getSampleBuffer()->getRootMotionSample().t, .0f));
            rm_t.y = .0f;
            root->translate(rm_t);
            root->rotate(anim_inst->getSampleBuffer()->getRootMotionSample().r);
        }
    }
};

class CameraTpsController
    : public ActorControllerT<EXEC_PRIORITY_CAMERA> {
    InputRange* rangeLook = 0;
    InputRange* rangeZoom = 0;
    InputAction* actionLeftClick = 0;

    gfxm::vec3 target_desired = gfxm::vec3(0,1.6f,0);
    gfxm::vec3 target_interpolated;

    float rotation_y = 0;
    float rotation_x = 0;
    float target_distance = 2.0f;
    float smooth_distance = target_distance;
    gfxm::quat qcam;

    Handle<TransformNode> target;
public:
    CameraTpsController() {
        rangeLook = inputGetRange("CameraRotation");
        rangeZoom = inputGetRange("Scroll");
        actionLeftClick = inputGetAction("Shoot");
    }

    void setTarget(Handle<TransformNode> target) {
        this->target = target;
    }

    void onReset() override {}
    void onSpawn(gameActor* actor) override {
        assert(actor->getRoot());
    }
    void onDespawn(gameActor* actor) override {}
    void onActorNodeRegister(type t, gameActorNode* component, const std::string& name) override {}
    void onActorNodeUnregister(type t, gameActorNode* component, const std::string& name) override {}
    void onUpdate(gameWorld* world, float dt) override {
        auto actor = getOwner();
        auto root = actor->getRoot();
        assert(root);

        if (target.isValid()) {
            target_desired = target->getWorldTranslation();
        }

        gfxm::vec3 cam_lcl_delta_rotation;
        cam_lcl_delta_rotation = rangeLook->getVec3();
        if (actionLeftClick->isPressed()) {
            rotation_y += cam_lcl_delta_rotation.y * (1.0f / 60.f) * 0.5f; // don't use actual frame time here
            rotation_x += cam_lcl_delta_rotation.x * (1.0f / 60.f) * 0.5f;
        }

        float scroll = rangeZoom->getVec3().x;
        float mul = scroll > .0f ? (target_distance / 3.0f) : (target_distance / 4.0f);
        target_distance += scroll * mul;
        target_distance = gfxm::_min(3.f, gfxm::_max(0.5f, target_distance));
        smooth_distance = gfxm::lerp(smooth_distance, target_distance, 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f));

        float look_offs_mul = 1.0f - (smooth_distance - .5f) / (3.f - .5f);
        look_offs_mul = gfxm::sqrt(look_offs_mul);

        rotation_x = gfxm::clamp(rotation_x, -gfxm::pi * 0.48f, gfxm::pi * 0.25f);
        gfxm::quat qy = gfxm::angle_axis(rotation_y, gfxm::vec3(0, 1, 0));
        gfxm::quat qx = gfxm::angle_axis(rotation_x, gfxm::vec3(1, 0, 0));
        qcam = gfxm::slerp(qcam, qy * qx, 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f));

        gfxm::mat4 orient_trs = gfxm::to_mat4(qcam);
        gfxm::vec3 back_normal = gfxm::normalize(gfxm::vec3(.5f * look_offs_mul, .0f, 1.f));

        target_interpolated = target_desired;// gfxm::lerp(target_interpolated, target_desired, 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f));
        gfxm::vec3 target_pos = target_interpolated;


        float real_distance = smooth_distance;
        RayCastResult rayResult = world->getCollisionWorld()->rayTest(
            target_interpolated,
            target_interpolated + gfxm::vec3(orient_trs * gfxm::vec4(back_normal, .0f)) * real_distance,
            COLLISION_LAYER_DEFAULT
        );
        if (rayResult.hasHit) {
            real_distance = rayResult.distance;
        }

        gfxm::mat4 trs
            = gfxm::translate(gfxm::mat4(1.0f), target_pos)
            * orient_trs
            * gfxm::translate(gfxm::mat4(1.0f), back_normal * real_distance);
        root->setTranslation(trs * gfxm::vec4(0,0,0,1));
        root->setRotation(qcam);
    }
};


class FsmController;
class ctrlFsmState {
public:
    virtual ~ctrlFsmState() {}

    virtual void onReset() = 0;
    virtual void onActorNodeRegister(type t, gameActorNode* component, const std::string& name) = 0;
    virtual bool onSpawn(gameActor* actor) { return true; }
    virtual void onDespawn(gameActor* actor) {}

    virtual void onEnter() {}
    virtual void onUpdate(gameWorld* world, gameActor* actor, FsmController* fsm, float dt) = 0;
};
class FsmController : public ActorControllerT<EXEC_PRIORITY_FIRST> {
    ctrlFsmState* initial_state = 0;
    ctrlFsmState* current_state = 0;
    ctrlFsmState* next_state = 0;
    std::map<std::string, std::unique_ptr<ctrlFsmState>> states;
public:
    void addState(const char* name, ctrlFsmState* state) {
        auto it = states.find(name);
        if (it != states.end()) {
            assert(false);
            LOG_ERR("State " << name << " already exists");
            return;
        }
        if (states.empty()) {
            initial_state = state;
            next_state = initial_state;
        }
        states.insert(std::make_pair(std::string(name), std::unique_ptr<ctrlFsmState>(state)));
    }

    void setState(const char* name) {
        auto it = states.find(name);
        if (it == states.end()) {
            assert(false);
            LOG_ERR("State " << name << " does not exist");
            return;
        }
        next_state = it->second.get();
    }

    void onReset() override {
        next_state = initial_state;
        for (auto& kv : states) {
            kv.second->onReset();
        }
    }
    void onSpawn(gameActor* actor) override {
        for (auto& kv : states) {
            kv.second->onSpawn(actor);
        }
    }
    void onDespawn(gameActor* actor) override {
        for (auto& kv : states) {
            kv.second->onDespawn(actor);
        }
    }
    void onActorNodeRegister(type t, gameActorNode* component, const std::string& name) override {
        for (auto& kv : states) {
            kv.second->onActorNodeRegister(t, component, name);
        }
    }
    void onActorNodeUnregister(type t, gameActorNode* component, const std::string& name) override {
        // TODO:?
    }
    void onUpdate(gameWorld* world, float dt) override {
        auto actor = getOwner();
        
        if (next_state) {
            current_state = next_state;
            next_state = 0;
            current_state->onEnter();
        }
        assert(current_state);
        current_state->onUpdate(world, actor, this, dt);
    }
};