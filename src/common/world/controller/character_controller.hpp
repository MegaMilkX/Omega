#pragma once

#include "actor_controller.hpp"
#include "math/gfxm.hpp"
#include "input/input.hpp"
#include "world/actor.hpp"
#include "world/world.hpp"
#include "world/component/components.hpp"

#include "fsm/fsm.hpp"


class CharacterStateLocomotion;
class CharacterStateInteract;

class CharacterController : public ActorControllerT<EXEC_PRIORITY_FIRST>  {
public:
    AnimatorComponent* anim_component = 0;
    FSM_T<CharacterController> fsm;

    CharacterController()
        : fsm(this) {
        fsm.addState<CharacterStateLocomotion>("locomotion");
        fsm.addState<CharacterStateInteract>("interacting");
    }
    void onReset() override {

    }
    void onSpawn(gameActor* actor) override {
        anim_component = actor->getComponent<AnimatorComponent>();
        if (!anim_component) {
            return;
        }
        return;
    }
    void onDespawn(gameActor* actor) override {
        anim_component = 0;
    }
    void onActorNodeRegister(type t, gameActorNode* component, const std::string& name) override {

    }
    void onActorNodeUnregister(type t, gameActorNode* component, const std::string& name) override {

    }
    void onUpdate(gameWorld* world, float dt) override {
        fsm.update(dt);
    }
};


class CharacterStateLocomotion : public FSMState_T<CharacterController> {
    InputRange* rangeTranslation = 0;
    InputAction* actionInteract = 0;

    const float TURN_LERP_SPEED = 0.995f;
    float velocity = .0f;
    gfxm::vec3 desired_dir = gfxm::vec3(0, 0, 1);

    bool is_grounded = true;
    gfxm::vec3 grav_velo;
public:
    CharacterStateLocomotion() {
        rangeTranslation = inputGetRange("CharacterLocomotion");
        actionInteract = inputGetAction("CharacterInteract");
    }

    void onEnter() override {

    }
    void onUpdate(float dt) override {
        auto actor = getFsmOwner()->getOwner();
        auto world = actor->getWorld();
        auto cam_node = world->getCurrentCameraNode();
        auto root = actor->getRoot();
        auto anim_component = getFsmOwner()->anim_component;

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
                getFsm()->setState("interacting");
            }
        }
    }
};
class CharacterStateInteract : public FSMState_T<CharacterController> {
public:
    void onEnter() override {

    }
    void onUpdate(float dt) override {
        auto anim_component = getFsmOwner()->anim_component;
        if (anim_component) {
            auto anim_inst = anim_component->getAnimatorInstance();
            auto anim_master = anim_component->getAnimatorMaster();
            if (anim_inst->isFeedbackEventTriggered(anim_master->getFeedbackEventId("fevt_door_open_end"))) {
                getFsm()->setState("locomotion");
            }
        }
    }
};
