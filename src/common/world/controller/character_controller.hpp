#pragma once

#include "character_controller.auto.hpp"
#include "actor_controller.hpp"
#include "math/gfxm.hpp"
#include "input/input.hpp"
#include "world/actor.hpp"
#include "world/world.hpp"
#include "world/component/components.hpp"
#include "world/node/node_collider.hpp"
#include "world/node/node_probe.hpp"
#include "world/node/node_text_billboard.hpp"
#include "world/node/anim_machine_node.hpp"
#include "world/node/node_character_capsule.hpp"
#include "player/player.hpp"
#include "resource_manager/resource_manager.hpp"

#include "fsm/fsm.hpp"

#include "game/slide_move.hpp"


[[cppi_class]];
class CharacterDriver : public ActorDriver  {
    int getExecutionPriority() const override { return EXEC_PRIORITY_FIRST; }

    phyWorld* collision_world = nullptr;

    //AnimatorComponent* anim_component = 0;
    AnimMachineNode* anim_node = nullptr;
    ProbeNode* probe_node = nullptr;
    Actor* targeted_actor = nullptr;

    ActorFsm<CharacterDriver> fsm;
    /*
    IPlayer* current_player = 0;
    InputContext input_ctx = InputContext("CharacterStateLocomotion");
    InputRange* rangeTranslation = 0;
    InputAction* actionInteract = 0;*/
    bool should_interact = false;

    const float TURN_LERP_SPEED = 0.999f;
    float velocity = .0f;
    gfxm::vec3 velo3;
    gfxm::vec3 desired_dir = gfxm::vec3(0, 0, 0);
    gfxm::vec3 loco_vec = gfxm::vec3(0, 0, 0);

    bool is_grounded = false;
    gfxm::vec3 grav_velo;

    ActorNodeView<CharacterCapsuleNode> capsule_node;

public:
    TYPE_ENABLE();

    [[cppi_decl]]
    float RUN_SPEED = 3.5f;
    [[cppi_decl]]
    gfxm::vec2 test_vec2;
    [[cppi_decl]]
    gfxm::vec3 test_vec3;
    [[cppi_decl]]
    gfxm::vec4 test_vec4;
    [[cppi_decl]]
    std::string my_string = "Hello, World!";
    [[cppi_decl]]
    ResourceRef<ActorPrefab> test_prefab;

    CharacterDriver()
        : fsm(this)
    {
        capsule_node = registerNodeView<CharacterCapsuleNode>("capsule", true);

        ActorFsm<CharacterDriver>::state_t state_locomotion;
        state_locomotion.pfn_on_update = &CharacterDriver::onUpdate_Locomotion;
        state_locomotion.pfn_on_message = &CharacterDriver::onMessage_Locomotion;
        fsm.addState("locomotion", state_locomotion);
        
        ActorFsm<CharacterDriver>::state_t state_interact;
        state_interact.pfn_on_update = &CharacterDriver::onUpdate_Interact;
        fsm.addState("interact", state_interact);
    }
    void onReset() override {

    }
    void onSpawnActorDriver(WorldSystemRegistry& reg, Actor* actor) override {
        collision_world = reg.getSystem<phyWorld>();
        //assert(anim_node);
        assert(collision_world);
    }
    void onDespawnActorDriver(WorldSystemRegistry& reg, Actor* actor) override {
        collision_world = nullptr;
        anim_node = nullptr;
    }
    void onActorNodeRegister(rtti::type t, ActorNode* node, const std::string& name) override {
        if (t == rtti::type_get<AnimMachineNode>()) {
            anim_node = static_cast<AnimMachineNode*>(node);
            return;
        }
        if (name == "probe" && t == rtti::type_get<ProbeNode>()) {
            probe_node = (ProbeNode*)node;
            probe_node->collider.collision_group = COLLISION_LAYER_PROBE;
            probe_node->collider.collision_mask = COLLISION_LAYER_BEACON;
        }

        ActorDriver::onActorNodeRegister(t, node, name);
    }
    void onActorNodeUnregister(rtti::type t, ActorNode* node, const std::string& name) override {
        if (t == rtti::type_get<AnimMachineNode>()) {
            anim_node = nullptr;
            return;
        }
        if (name == "probe" && t == rtti::type_get<ProbeNode>()) {
            probe_node = 0;
        }

        ActorDriver::onActorNodeUnregister(t, node, name);
    }

    GAME_MESSAGE onMessage(GAME_MESSAGE msg) override {
        switch (msg.msg) {
        case GAME_MSG::PAWN_CMD: {
            auto pld = msg.getPayload<GAME_MSG::PAWN_CMD>();
            switch (pld.cmd) {
            case ePawnMoveDirection:
                desired_dir = pld.params;
                return GAME_MSG::HANDLED;
            }
        }
        }
        return fsm.onMessage(msg);
    }
    GAME_MESSAGE onMessage_Locomotion(GAME_MESSAGE msg) {
        switch (msg.msg) {
        case GAME_MSG::PAWN_CMD: {
            auto pld = msg.getPayload<GAME_MSG::PAWN_CMD>();
            switch (pld.cmd) {
            case ePawnInteract: interact(); return GAME_MSG::HANDLED;
            case ePawnJump: jump(); return GAME_MSG::HANDLED;
            }
        }
        }
        return fsm.onMessage(msg);
    }

    void jump() {
        if (!is_grounded) {
            return;
        }
        grav_velo = gfxm::vec3(0, 6, 0);
        is_grounded = false;
    }

    void interact() {
        if (!getOwner()->isSpawned()) {
            return;
        }

        if (test_prefab && playerGetPrimary()) {
            auto vp = playerGetPrimary()->getViewport();
            gfxm::mat4 cam_transform;
            if (vp) {
                cam_transform = gfxm::inverse(vp->getViewTransform());
            }
            gfxm::vec3 cam_pos = cam_transform[3];
            gfxm::vec3 cam_dir = -cam_transform[2];
            gfxm::vec3 at = cam_pos + cam_dir * 4.f;
            auto rc = collision_world->rayTest(cam_pos, at, COLLISION_LAYER_DEFAULT);
            if (rc.hasHit) {
                at = rc.position + rc.normal * 1.1f;
            }
            Actor* a = test_prefab->instantiate();
            getOwner()->getWorld()->spawn(a);
            a->setTranslation(at);
        }

        if (targeted_actor) {
            GAME_MESSAGE rsp = targeted_actor->sendMessage(PAYLOAD_INTERACT{ getOwner() });
            //anim_component->getAnimatorInstance()->triggerSignal(anim_component->getAnimatorMaster()->getSignalId("sig_door_open"));
            /*
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
            }*/
        }
        /*
        if (anim_component) {
            auto anim_inst = anim_component->getAnimatorInstance();
            auto anim_master = anim_component->getAnimatorMaster();
            anim_inst->triggerSignal(anim_master->getSignalId("sig_door_open"));
            getFsm()->setState("interacting");
        }*/
    }

    void onUpdate(float dt) override {
        fsm.update(dt);

        // Choose an actionable object if there are any available
        if (targeted_actor) {
            targeted_actor->sendMessage(PAYLOAD_HIGHLIGHT_OFF{ getOwner() });
        }
        targeted_actor = 0;
        if (probe_node) {
            for (int i = 0; i < probe_node->collider.overlappingColliderCount(); ++i) {
                phyRigidBody* other = probe_node->collider.getOverlappingCollider(i);
                void* user_ptr = other->user_data.user_ptr;
                if (user_ptr && other->user_data.type == COLLIDER_USER_ACTOR) {
                    targeted_actor = (Actor*)user_ptr;
                    targeted_actor->sendMessage(PAYLOAD_HIGHLIGHT_ON{ getOwner() });
                    break;
                }
            }
        }
    }
    gfxm::vec3 applyFriction(float dt, const gfxm::vec3& v, bool is_really_grounded) {
        float speed = v.length();
        float newspeed = .0f;
        float drop = .0f;

        gfxm::vec3 rv = v;

        if (speed <= FLT_EPSILON) {
            return rv;
        }

        const float FRICTION_GROUND = 16.f;
        if (is_really_grounded/* && desired_direction.length() <= FLT_EPSILON*/) {
            drop += FRICTION_GROUND * dt;
        }

        newspeed = speed - drop;
        if (newspeed < .0f) {
            newspeed = .0f;
        }
        newspeed /= speed;

        rv *= newspeed;
        return rv;
    }
    void onUpdate_Locomotion(float dt) {
        auto actor = getOwner();
        auto root = actor->getRoot();

        bool has_dir_input =desired_dir.length() > FLT_EPSILON;

        // Ground test
        {
            float radius = .1f;
            phySphereSweepResult ssr = collision_world->sphereSweep(
                root->getTranslation() + gfxm::vec3(.0f, .3f, .0f),
                root->getTranslation() - gfxm::vec3(.0f, .3f, .0f),
                radius, COLLISION_LAYER_DEFAULT
            );
            if (ssr.hasHit && grav_velo.y <= .0f) {
                gfxm::vec3 pos = root->getTranslation();
                float y_offset = ssr.sphere_pos.y - radius - pos.y;
                // y_offset > .0f if the character is sunk into the ground
                // y_offset < .0f if the character is floating above
                root->translate(gfxm::vec3(.0f, y_offset * 20.f * dt, .0f));
                
                is_grounded = true;
                grav_velo = gfxm::vec3(0, 0, 0);
            } else {
                is_grounded = false;
                grav_velo -= gfxm::vec3(.0f, 9.8f * dt, .0f);
                // 53m/s is the maximum approximate terminal velocity for a human body
                grav_velo.y = gfxm::_min(53.f, grav_velo.y);
            }
        }

        velo3 = applyFriction(dt, velo3, is_grounded);
        const float ACCEL_GROUND = 50.f;
        const float ACCEL_AIR = 8.f;
        if (has_dir_input) {                
            gfxm::vec3 world_input_dir = desired_dir;

            const float ACCEL = is_grounded ? ACCEL_GROUND : ACCEL_AIR;

            velo3 += world_input_dir * ACCEL * dt;
            if (velo3.length() > RUN_SPEED) {
                velo3 = gfxm::normalize(velo3) * RUN_SPEED;
            }
        }

        if(is_grounded) {
            if (desired_dir.length() > velocity) {
                velocity = gfxm::lerp(velocity, desired_dir.length(), 1 - pow(1.f - .999f, dt));
            } else if(!has_dir_input) {
                //velocity = .0f;
                velocity = gfxm::lerp(velocity, desired_dir.length(), 1 - pow(1.f - .999f, dt));
            }
        } else {
            velocity += -velocity * dt;
        }

        if (velocity > FLT_EPSILON) {
            gfxm::mat3 orient;
            if (has_dir_input) {
                loco_vec = gfxm::normalize(desired_dir);
            }

            orient[2] = loco_vec;
            orient[1] = gfxm::vec3(0, 1, 0);
            orient[0] = gfxm::cross(orient[1], orient[2]);
            gfxm::quat tgt_rot = gfxm::to_quat(orient);

            float dot = fabsf(gfxm::dot(root->getRotation(), tgt_rot));
            float angle = 2.f * acosf(gfxm::_min(dot, 1.f));
            float slerp_fix = (1.f - angle / gfxm::pi) * (1.0f - TURN_LERP_SPEED);

            //gfxm::quat cur_rot = gfxm::slerp(root->getRotation(), tgt_rot, 1 - pow(slerp_fix, dt));
            float theta = 1.f;
            {
                float d = gfxm::dot(root->getRotation(), tgt_rot);
                theta = cosf(d);
            }
            float rad_per_sec = 15.f * gfxm::pi * gfxm::smoothstep(.0f, 1.f, (theta / gfxm::pi));
            gfxm::quat cur_rot = gfxm::rotate_to(root->getRotation(), tgt_rot, rad_per_sec * dt);

            //if(is_grounded) {
                root->setRotation(cur_rot);
            //}

        }

        gfxm::vec3 translation = velo3 * dt;
        translation += grav_velo * dt;
        slideMoveYCapsule(
            collision_world, dt,
            root->getTranslation() + capsule_node->collider.getCenterOffset(),
            capsule_node->shape.height, capsule_node->shape.radius,
            translation, velo3, grav_velo
        );
        root->translate(translation);

        if (anim_node) {
            auto anim_inst = anim_node->getAnimatorInstance();
            auto anim_master = anim_node->getAnimatorMaster();
            if(anim_inst && anim_master) {
                anim_inst->setParamValue(anim_master->getParamId("velocity"), velocity);
                anim_inst->setParamValue(anim_master->getParamId("is_falling"), is_grounded ? .0f : 1.f);
            }
        }
    }
    void onUpdate_Interact(float dt) {
        if (anim_node) {
            auto anim_inst = anim_node->getAnimatorInstance();
            auto anim_master = anim_node->getAnimatorMaster();
            if (anim_inst->isFeedbackEventTriggered(anim_master->getFeedbackEventId("fevt_door_open_end"))) {
                fsm.setState("locomotion");
            }
        }
    }
};

