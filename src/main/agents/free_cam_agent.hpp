#pragma once

#include "world/agent/agent.hpp"
#include "world/world_system_registry.hpp"
#include "input/input.hpp"
#include "player/player.hpp"
#include "audio/audio.hpp"


class FreeCamAgent : public IPlayerProxy, public ISpectator {
    InputContext input_ctx;
    InputRange* rangeLook = 0;
    InputRange* rangeMove = 0;
    InputAction* actionLeftClick = 0;
    InputAction* actionJump = 0;
    InputAction* actionCrouch = 0;
    InputAction* actionSprint = 0;

    LocalPlayer* player = nullptr;

    gfxm::vec3 world_pos;
    float rotation_y = 0;
    float rotation_x = 0;
public:
    FreeCamAgent() {    
        rangeLook = input_ctx.createRange("CameraRotation");
        rangeMove = input_ctx.createRange("CharacterLocomotion");
        actionLeftClick = input_ctx.createAction("Shoot");
        actionCrouch = input_ctx.createAction("C");
        actionSprint = input_ctx.createAction("Sprint");
        actionJump = input_ctx.createAction("Jump");
    }
    
    void onAttachPlayer(IPlayer* p) override {
        LocalPlayer* local = dynamic_cast<LocalPlayer*>(p);
        if (!local) {
            return;
        }
        local->getInputState()->pushContext(&input_ctx);
        player = local;
        gfxm::mat4 tr = gfxm::inverse(local->getViewport()->getViewTransform());
        world_pos = tr[3];
        gfxm::vec2 euler = gfxm::to_euler_xy(tr);
        rotation_x = euler.x;
        rotation_y = euler.y;
    }
    void onDetachPlayer(IPlayer* p) override {
        LocalPlayer* local = dynamic_cast<LocalPlayer*>(p);
        if (!local) {
            return;
        }
        local->getInputState()->removeContext(&input_ctx);
        player = nullptr;
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

    void onUpdateSpectator(float dt) override {
        if (!player) {
            return;
        }

        gfxm::vec3 cam_lcl_delta_rotation;
        cam_lcl_delta_rotation = rangeLook->getVec3();
        if (actionLeftClick->isPressed()) {
            rotation_y += cam_lcl_delta_rotation.y * (1.0f / 60.f) * 0.5f; // don't use actual frame time here
            rotation_x += cam_lcl_delta_rotation.x * (1.0f / 60.f) * 0.5f;
        }

        gfxm::vec3 cam_lcl_delta_pos = rangeMove->getVec3();

        rotation_x = gfxm::clamp(rotation_x, -gfxm::pi * 0.48f, gfxm::pi * 0.25f);
        gfxm::quat qy = gfxm::angle_axis(rotation_y, gfxm::vec3(0, 1, 0));
        gfxm::quat qx = gfxm::angle_axis(rotation_x, gfxm::vec3(1, 0, 0));
        gfxm::quat qcam = qy * qx;// gfxm::slerp(qcam, qy * qx, 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f));

        gfxm::mat4 orient_trs = gfxm::to_mat4(qcam);

        gfxm::vec3 world_delta_pos = orient_trs * gfxm::vec4(gfxm::normalize(cam_lcl_delta_pos), 1.f);
        
        if (actionJump->isPressed()) {
            world_delta_pos += gfxm::vec3(0, 1, 0);
        }
        if (actionCrouch->isPressed()) {
            world_delta_pos += gfxm::vec3(0, -1, 0);
        }

        const float SPEED = 10.f;
        float SPRINT_FACTOR = 1.f;
        if (actionSprint->isPressed()) {
            SPRINT_FACTOR = 3.f;
        }
        
        world_pos += world_delta_pos * SPEED * SPRINT_FACTOR * dt;
        
        auto viewport = player->getViewport();
        if (!viewport) {
            return;
        }
        auto cam = viewport->getCamera();
        if (!cam) {
            return;
        }
        cam->setCameraPosition(world_pos);
        cam->setCameraRotation(qcam);

        gfxm::mat4 tr
            = gfxm::translate(gfxm::mat4(1.f), world_pos)
            * gfxm::to_mat4(qcam);
        audioSetListenerTransform(tr);
    }
};

