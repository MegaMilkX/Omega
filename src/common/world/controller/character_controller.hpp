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
#include "player/player.hpp"

#include "fsm/fsm.hpp"


[[cppi_class]];
class CharacterController : public ActorController  {
    int getExecutionPriority() const override { return EXEC_PRIORITY_FIRST; }

    AnimatorComponent* anim_component = 0;
    ProbeNode* probe_node = 0;
    Actor* targeted_actor = 0;

    ActorFsm<CharacterController> fsm;

    IPlayer* current_player = 0;
    InputContext input_ctx = InputContext("CharacterStateLocomotion");
    InputRange* rangeTranslation = 0;
    InputAction* actionInteract = 0;

    constexpr static float RUN_SPEED = 3.f;
    const float TURN_LERP_SPEED = 0.997f;
    float velocity = .0f;
    gfxm::vec3 desired_dir = gfxm::vec3(0, 0, 1);
    gfxm::vec3 loco_vec = gfxm::vec3(0, 0, 1);

    bool is_grounded = true;
    gfxm::vec3 grav_velo;

public:
    TYPE_ENABLE();

    CharacterController()
        : fsm(this)
    {
        ActorFsm<CharacterController>::state_t state_locomotion;
        state_locomotion.pfn_on_update = &CharacterController::onUpdate_Locomotion;
        fsm.addState("locomotion", state_locomotion);
        
        ActorFsm<CharacterController>::state_t state_interact;
        state_interact.pfn_on_update = &CharacterController::onUpdate_Interact;
        fsm.addState("interact", state_interact);

        rangeTranslation = input_ctx.createRange("CharacterLocomotion");
        actionInteract = input_ctx.createAction("CharacterInteract");
    }
    void onReset() override {

    }
    void onSpawn(Actor* actor) override {
        anim_component = actor->getComponent<AnimatorComponent>();
        if (!anim_component) {
            return;
        }
        return;
    }
    void onDespawn(Actor* actor) override {
        anim_component = 0;
    }
    void onActorNodeRegister(type t, ActorNode* component, const std::string& name) override {
        if (name == "probe" && t == type_get<ProbeNode>()) {
            probe_node = (ProbeNode*)component;
            probe_node->collider.collision_group = COLLISION_LAYER_PROBE;
            probe_node->collider.collision_mask = COLLISION_LAYER_BEACON;
        }
    }
    void onActorNodeUnregister(type t, ActorNode* component, const std::string& name) override {
        if (name == "probe" && t == type_get<ProbeNode>()) {
            probe_node = 0;
        }
    }

    GAME_MESSAGE onMessage(GAME_MESSAGE msg) override {
        switch (msg.msg) {
        case GAME_MSG::PLAYER_ATTACH: {
            auto player = msg.getPayload<GAME_MSG::PLAYER_ATTACH>().player;
            getOwner()->forEachNode<TextBillboardNode>([player](TextBillboardNode* n) {
                n->setText(player->getName().c_str());
            });
            current_player = msg.getPayload<GAME_MSG::PLAYER_ATTACH>().player;
            current_player->getInputState()->pushContext(&input_ctx);
            return GAME_MSG::HANDLED;
        }
        case GAME_MSG::PLAYER_DETACH: {
            getOwner()->forEachNode<TextBillboardNode>([](TextBillboardNode* n) {
                n->setText("Unknown");
            });
            auto player = msg.getPayload<GAME_MSG::PLAYER_DETACH>().player;
            player->getInputState()->removeContext(&input_ctx);
            current_player = 0;
            return GAME_MSG::HANDLED;
        }
        }
        return fsm.onMessage(msg);
    }
    void onUpdate(RuntimeWorld* world, float dt) override {
        fsm.update(dt);

        // Choose an actionable object if there are any available
        targeted_actor = 0;
        if (probe_node) {
            for (int i = 0; i < probe_node->collider.overlappingColliderCount(); ++i) {
                Collider* other = probe_node->collider.getOverlappingCollider(i);
                void* user_ptr = other->user_data.user_ptr;
                if (user_ptr && other->user_data.type == COLLIDER_USER_ACTOR) {
                    targeted_actor = (Actor*)user_ptr;
                    break;
                }
            }
        }
    }
    void onUpdate_Locomotion(float dt) {
        auto actor = getOwner();
        auto world = actor->getWorld();
        auto root = actor->getRoot();

        bool has_dir_input = rangeTranslation->getVec3().length() > FLT_EPSILON;
        gfxm::vec3 input_dir = gfxm::normalize(rangeTranslation->getVec3());

        // Ground test
        if (!is_grounded) {
            root->translate(grav_velo * dt);
        }
        {
            float radius = .1f;
            SphereSweepResult ssr = world->getCollisionWorld()->sphereSweep(
                root->getTranslation() + gfxm::vec3(.0f, .3f, .0f),
                root->getTranslation() - gfxm::vec3(.0f, .3f, .0f),
                radius, COLLISION_LAYER_DEFAULT
            );
            if (ssr.hasHit) {
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

        if (has_dir_input) {
            desired_dir = input_dir;
        }
        if(is_grounded) {
            if (input_dir.length() > velocity) {
                velocity = gfxm::lerp(velocity, input_dir.length(), 1 - pow(1.f - .995f, dt));
            } else if(!has_dir_input) {
                //velocity = .0f;
                velocity = gfxm::lerp(velocity, input_dir.length(), 1 - pow(1.f - .995f, dt));
            }
        } else {
            velocity += -velocity * dt;
        }

        if (velocity > FLT_EPSILON) {
            gfxm::mat4 trs(1.0f);            
            if (current_player && current_player->getViewport()) {
                trs = gfxm::inverse(current_player->getViewport()->getViewTransform());
            }
            gfxm::mat3 orient;
            gfxm::vec3 fwd = trs * gfxm::vec4(0, 0, 1, 0);
            fwd.y = .0f;
            fwd = gfxm::normalize(fwd);
            orient[2] = fwd;
            orient[1] = gfxm::vec3(0, 1, 0);
            orient[0] = gfxm::cross(orient[1], orient[2]);
            if (has_dir_input) {
                loco_vec = orient * desired_dir;
            }

            orient[2] = loco_vec;
            orient[1] = gfxm::vec3(0, 1, 0);
            orient[0] = gfxm::cross(orient[1], orient[2]);
            gfxm::quat tgt_rot = gfxm::to_quat(orient);

            float dot = fabsf(gfxm::dot(root->getRotation(), tgt_rot));
            float angle = 2.f * acosf(gfxm::_min(dot, 1.f));
            float slerp_fix = (1.f - angle / gfxm::pi) * (1.0f - TURN_LERP_SPEED);

            gfxm::quat cur_rot = gfxm::slerp(root->getRotation(), tgt_rot, 1 - pow(slerp_fix, dt));
            if (has_dir_input) {
                root->setRotation(cur_rot);
            }
            root->translate((gfxm::to_mat4(cur_rot) * gfxm::vec3(0,0,1)) * dt * RUN_SPEED * velocity);
        }

        if (anim_component) {
            auto anim_inst = anim_component->getAnimatorInstance();
            auto anim_master = anim_component->getAnimatorMaster();
            anim_inst->setParamValue(anim_master->getParamId("velocity"), velocity);
            anim_inst->setParamValue(anim_master->getParamId("is_falling"), is_grounded ? .0f : 1.f);
        }
        if (actionInteract->isJustPressed()) {
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
    }
    void onUpdate_Interact(float dt) {
        if (anim_component) {
            auto anim_inst = anim_component->getAnimatorInstance();
            auto anim_master = anim_component->getAnimatorMaster();
            if (anim_inst->isFeedbackEventTriggered(anim_master->getFeedbackEventId("fevt_door_open_end"))) {
                fsm.setState("locomotion");
            }
        }
    }
};

