#pragma once

#include "world/agent/agent.hpp"
#include "world/agent/actor_link.hpp"
#include "world/controller/character_controller.hpp"


class TpsSpectator : public IPlayerProxy, public ISpectator, public IActorLink {
    Actor* actor = nullptr;
    LocalPlayer* player = nullptr;

    InputContext input_ctx;
    InputRange* rangeLook = 0;
    InputRange* rangeZoom = 0;
    InputAction* actionLeftClick = 0;
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
    TpsSpectator() {
        rangeLook = input_ctx.createRange("CameraRotation");
        rangeZoom = input_ctx.createRange("Scroll");
        actionLeftClick = input_ctx.createAction("Shoot");
    }
    TpsSpectator(Actor* a)
        : TpsSpectator() {
        linkActor(a);
    }
    ~TpsSpectator() {
        linkActor(nullptr);
    }

    void onSpawn(WorldSystemRegistry& reg) {
        if (auto sys = reg.getSystem<SpectatorSet>()) {
            sys->insert(this);
        }
    }
    void onDespawn(WorldSystemRegistry& reg) {
        if (auto sys = reg.getSystem<SpectatorSet>()) {
            sys->erase(this);
        }
    }

    void onAttachPlayer(IPlayer* p) override {
        LocalPlayer* local = dynamic_cast<LocalPlayer*>(p);
        if (!local) {
            return;
        }
        local->getInputState()->pushContext(&input_ctx);
        player = local;
    }
    void onDetachPlayer(IPlayer* p) override {
        LocalPlayer* local = dynamic_cast<LocalPlayer*>(p);
        if (!local) {
            return;
        }
        local->getInputState()->removeContext(&input_ctx);
        player = nullptr;
    }

    void onAttachActor(Actor* a) override {
        actor = a;
        auto node = a->findNode<EmptyNode>("cam_target");
        if(node) {
            target = node->getTransformHandle();
        } else {
            target = a->getRoot()->getTransformHandle();
        }
    }
    void onDetachActor(Actor* a) override {
        actor = a;
        target = Handle<TransformNode>();
    }

    void onUpdateSpectator(float dt) override {
        if (!player) {
            return;
        }
        if (!actor) {
            // TODO: This should not happen,
            // this role should be destroyed at this point,
            // need to debug
            LOG_ERR("TpsSpectator should be destroyed already");
            return;
        }

        if (!actor->isSpawned()) {
            return;
        }
        auto collision_world = actor->getRegistry()->getSystem<phyWorld>();
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
        look_offs_mul = .0f;

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
        phySphereSweepResult result = collision_world->sphereSweep(
            target_interpolated,
            target_interpolated + gfxm::vec3(orient_trs * gfxm::vec4(back_normal, .0f)) * real_distance,
            sweep_radius, COLLISION_LAYER_DEFAULT
        );
        if (result.hasHit) {
            real_distance = result.distance;
        }
        /*
        phyRayCastResult rayResult = world->getCollisionWorld()->rayTest(
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
        //root->setTranslation(trs * gfxm::vec4(0,0,0,1));
        //root->setRotation(qcam);


        auto vp = player->getViewport();
        auto cam = vp->getCamera();

        gfxm::vec3 pos;
        gfxm::quat rot;

        pos = trs * gfxm::vec4(0, 0, 0, 1);
        rot = qcam;

        cam->setCameraPosition(pos);
        cam->setCameraRotation(rot);

        gfxm::mat4 tr
            = gfxm::translate(gfxm::mat4(1.f), pos)
            * gfxm::to_mat4(rot);
        audioSetListenerTransform(tr);
    }
};

class TpsPlayerController : public IPlayerProxy, public IPlayerController, public IActorLink {
    InputContext input_ctx = InputContext("TpsPlayerAgent");
    InputRange* rangeTranslation = 0;
    InputAction* actionInteract = 0;

    Actor* actor = nullptr;
    LocalPlayer* player = nullptr;
public:
    TpsPlayerController() {
        rangeTranslation = input_ctx.createRange("CharacterLocomotion");
        actionInteract = input_ctx.createAction("CharacterInteract");
    }
    TpsPlayerController(Actor* a)
        : TpsPlayerController() {
        linkActor(a);
    }
    ~TpsPlayerController() {
        linkActor(nullptr);
    }

    void onSpawn(WorldSystemRegistry& reg) {
        if (auto sys = reg.getSystem<PlayerControllerSet>()) {
            sys->insert(this);
        }
    }
    void onDespawn(WorldSystemRegistry& reg) {
        if (auto sys = reg.getSystem<PlayerControllerSet>()) {
            sys->erase(this);
        }
    }

    void onAttachPlayer(IPlayer* p) override {
        LocalPlayer* local = dynamic_cast<LocalPlayer*>(p);
        if (!local) {
            return;
        }
        local->getInputState()->pushContext(&input_ctx);
        player = local;
    }
    void onDetachPlayer(IPlayer* p) override {
        LocalPlayer* local = dynamic_cast<LocalPlayer*>(p);
        if (!local) {
            return;
        }
        local->getInputState()->removeContext(&input_ctx);
        player = nullptr;
    }

    void onAttachActor(Actor* a) override {
        actor = a;
    }
    void onDetachActor(Actor* a) override {
        actor = nullptr;
    }

    void onUpdateController(float dt) override {
        if (!actor) {
            // TODO: This should not happen,
            // this role should be destroyed at this point,
            // need to debug
            LOG_ERR("TpsPlayerController should be destroyed already");
            return;
        }

        gfxm::vec3 input_dir = gfxm::normalize(rangeTranslation->getVec3());
        gfxm::vec3 world_input_dir;
        if(input_dir.length() > FLT_EPSILON) {
            gfxm::mat4 trs(1.0f);            
            if (player && player->getViewport()) {
                trs = gfxm::inverse(player->getViewport()->getViewTransform());
            }
            gfxm::mat3 orient;
            gfxm::vec3 fwd = trs * gfxm::vec4(0, 0, 1, 0);
            fwd.y = .0f;
            fwd = gfxm::normalize(fwd);
            orient[2] = fwd;
            orient[1] = gfxm::vec3(0, 1, 0);
            orient[0] = gfxm::cross(orient[1], orient[2]);

            world_input_dir = orient * input_dir;
        }
        actor->sendMessage(PAYLOAD_PAWN_CMD{
            .cmd = ePawnMoveDirection,
            .params = world_input_dir
        });

        if (actionInteract->isJustPressed()) {
            actor->sendMessage(PAYLOAD_PAWN_CMD{
                .cmd = ePawnInteract
            });
        }
    }
};