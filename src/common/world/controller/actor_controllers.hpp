#pragma once

#include "actor_controller.hpp"
#include "world/component/components.hpp"

#include "input/input.hpp"
#include "world/actor.hpp"
#include "world/world.hpp"
#include "player/player.hpp"

#include "world/node/node_skeletal_model.hpp"


class AnimatorController
    : public ActorControllerT<EXEC_PRIORITY_PRE_ANIMATION> {

    AnimatorComponent* animComponent = 0;
    std::set<SkeletalModelNode*> skeletal_models;
public:
    void onReset() override {}
    void onSpawn(Actor* actor) override {
        animComponent = actor->getComponent<AnimatorComponent>();
    }
    void onDespawn(Actor* actor) override {}
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
    void onUpdate(RuntimeWorld* world, float dt) override {
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
    InputContext input_ctx;
    InputRange* rangeLook = 0;
    InputRange* rangeZoom = 0;
    InputAction* actionLeftClick = 0;

    IPlayer* current_player = 0;

    gfxm::vec3 target_desired = gfxm::vec3(0,1.6f,0);
    gfxm::vec3 target_interpolated;

    constexpr static float DISTANCE_MIN = .5f;
    constexpr static float DISTANCE_MAX = 3.f;

    float rotation_y = 0;
    float rotation_x = 0;
    float target_distance = 2.0f;
    float smooth_distance = target_distance;
    gfxm::quat qcam;

    Handle<TransformNode> target;
public:
    CameraTpsController() {
        rangeLook = input_ctx.createRange("CameraRotation");
        rangeZoom = input_ctx.createRange("Scroll");
        actionLeftClick = input_ctx.createAction("Shoot");
    }

    void setTarget(Handle<TransformNode> target) {
        this->target = target;
    }

    void onReset() override {}
    void onSpawn(Actor* actor) override {
        assert(actor->getRoot());
    }
    void onDespawn(Actor* actor) override {}
    void onActorNodeRegister(type t, gameActorNode* component, const std::string& name) override {}
    void onActorNodeUnregister(type t, gameActorNode* component, const std::string& name) override {}
    GAME_MESSAGE onMessage(GAME_MESSAGE msg) override {
        switch (msg.msg) {
        case GAME_MSG::PLAYER_ATTACH: {
            current_player = msg.getPayload<GAME_MSG::PLAYER_ATTACH>().player;
            current_player->getInputState()->pushContext(&input_ctx);
            if (current_player->getViewport()) {
                gfxm::mat4 trs = gfxm::inverse(current_player->getViewport()->getViewTransform());
                gfxm::vec2 euler = gfxm::to_euler_xy(trs);
                rotation_x = euler.x;
                rotation_y = euler.y;
            }
            return GAME_MSG::HANDLED;
        }
        case GAME_MSG::PLAYER_DETACH: {
            auto player = msg.getPayload<GAME_MSG::PLAYER_DETACH>().player;
            player->getInputState()->removeContext(&input_ctx);
            current_player = 0;
            return GAME_MSG::HANDLED;
        }
        }
        return GAME_MSG::NOT_HANDLED;
    }
    void onUpdate(RuntimeWorld* world, float dt) override {
        if (!current_player) {
            return;
        }

        auto actor = getOwner();
        auto root = actor->getRoot();
        assert(root);

        if (target.isValid()) {
            target_desired = target->getWorldTranslation();
        }

        gfxm::vec3 cam_lcl_delta_rotation;
        cam_lcl_delta_rotation = rangeLook->getVec3();
        if (actionLeftClick->isPressed()) {
            rotation_y += cam_lcl_delta_rotation.y * (1.0f / 60.f) * 0.3f; // don't use actual frame time here
            rotation_x += cam_lcl_delta_rotation.x * (1.0f / 60.f) * 0.3f;
        }

        float scroll = rangeZoom->getVec3().x;
        float mul = scroll > .0f ? (target_distance / 3.0f) : (target_distance / 4.0f);
        target_distance += scroll * mul;
        target_distance = gfxm::_min(DISTANCE_MAX, gfxm::_max(DISTANCE_MIN, target_distance));
        smooth_distance = gfxm::lerp(smooth_distance, target_distance, 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f));

        float follow_strength = (1.f - .9f * (smooth_distance - DISTANCE_MIN) / (DISTANCE_MAX - DISTANCE_MIN));

        float look_offs_mul = 1.0f - (smooth_distance - .5f) / (3.f - .5f);
        look_offs_mul = gfxm::sqrt(look_offs_mul);

        rotation_x = gfxm::clamp(rotation_x, -gfxm::pi * 0.48f, gfxm::pi * 0.25f);
        gfxm::quat qy = gfxm::angle_axis(rotation_y, gfxm::vec3(0, 1, 0));
        gfxm::quat qx = gfxm::angle_axis(rotation_x, gfxm::vec3(1, 0, 0));
        qcam = qy * qx;//gfxm::slerp(qcam, qy * qx, 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f));

        gfxm::mat4 orient_trs = gfxm::to_mat4(qcam);
        gfxm::vec3 back_normal = gfxm::normalize(gfxm::vec3(1.f * look_offs_mul, .0f, 1.f));

        target_interpolated = gfxm::lerp(target_interpolated, target_desired, 1 - pow(1 - follow_strength, dt * 60.0f));
        gfxm::vec3 target_pos = target_interpolated;


        float real_distance = smooth_distance;
        float sweep_radius = .25f;
        SphereSweepResult result = world->getCollisionWorld()->sphereSweep(
            target_interpolated,
            target_interpolated + gfxm::vec3(orient_trs * gfxm::vec4(back_normal, .0f)) * real_distance,
            sweep_radius, COLLISION_LAYER_DEFAULT
        );
        if (result.hasHit) {
            real_distance = result.distance;
        }
        /*
        RayCastResult rayResult = world->getCollisionWorld()->rayTest(
            target_interpolated,
            target_interpolated + gfxm::vec3(orient_trs * gfxm::vec4(back_normal, .0f)) * real_distance,
            COLLISION_LAYER_DEFAULT
        );
        if (rayResult.hasHit) {
            real_distance = rayResult.distance;
        }*/

        gfxm::mat4 trs
            = gfxm::translate(gfxm::mat4(1.0f), target_pos)
            * orient_trs
            * gfxm::translate(gfxm::mat4(1.0f), back_normal * real_distance);
        root->setTranslation(trs * gfxm::vec4(0,0,0,1));
        root->setRotation(qcam);
        
        auto viewport = current_player->getViewport();
        if (!viewport) {
            return;
        }
        viewport->setFov(65.f);
        viewport->setCameraPosition(trs * gfxm::vec4(0, 0, 0, 1));
        viewport->setCameraRotation(qcam);
        viewport->setZFar(1000.f);
        viewport->setZNear(.01f);

        // TODO: ?
        audioSetListenerTransform(root->getWorldTransform());
    }
};


class FsmController;
class ctrlFsmState {
public:
    virtual ~ctrlFsmState() {}

    virtual void onReset() = 0;
    virtual void onActorNodeRegister(type t, gameActorNode* component, const std::string& name) = 0;
    virtual bool onSpawn(Actor* actor) { return true; }
    virtual void onDespawn(Actor* actor) {}

    virtual void onEnter() {}
    virtual void onUpdate(RuntimeWorld* world, Actor* actor, FsmController* fsm, float dt) = 0;
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
    void onSpawn(Actor* actor) override {
        for (auto& kv : states) {
            kv.second->onSpawn(actor);
        }
    }
    void onDespawn(Actor* actor) override {
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
    void onUpdate(RuntimeWorld* world, float dt) override {
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