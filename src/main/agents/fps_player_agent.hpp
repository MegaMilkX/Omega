#pragma once

#include "log/log.hpp"
#include "world/agent/agent.hpp"
#include "world/agent/actor_link.hpp"
#include "input/input.hpp"
#include "render_scene/render_scene.hpp"


class FpsSpectator : public IPlayerProxy, public ISpectator, public IActorLink {
    FpsCharacterDriver* controller = nullptr;
    LocalPlayer* player = nullptr;
    scnRenderScene* render_scene = nullptr;

    bool wpn_visible = false;

    void handleWeaponModel(bool remove = false) {
        if (wpn_visible) {
            if (controller && player && !remove && render_scene) {
                return;
            }
            if (!render_scene) {
                return;
            }
            if (!controller->weapon_model_instance) {
                return;
            }
            auto mdl = controller->weapon_model_instance.get();
            mdl->despawn(render_scene);
            wpn_visible = false;
        } else {
            if (!controller || !player || !isSpawned() || !render_scene) {
                return;
            }
            if (!controller->weapon_model_instance) {
                return;
            }
            auto mdl = controller->weapon_model_instance.get();
            mdl->spawn(render_scene);
            wpn_visible = true;
        }
    }
public:
    FpsSpectator(Actor* a) {
        linkActor(a);
    }
    ~FpsSpectator() {
        linkActor(nullptr);
    }
    
    void onSpawn(WorldSystemRegistry& reg) {
        if (auto sys = reg.getSystem<SpectatorSet>()) {
            sys->insert(this);
        }
        render_scene = reg.getSystem<scnRenderScene>();
        handleWeaponModel();
    }
    void onDespawn(WorldSystemRegistry& reg) {
        if (auto sys = reg.getSystem<SpectatorSet>()) {
            sys->erase(this);
        }
        handleWeaponModel(true);
        render_scene = nullptr;
    }

    void onAttachPlayer(IPlayer* p) override {
        LocalPlayer* local = dynamic_cast<LocalPlayer*>(p);
        if (!local) {
            return;
        }
        player = local;
        handleWeaponModel();
    }
    void onDetachPlayer(IPlayer* p) override {
        LocalPlayer* local = dynamic_cast<LocalPlayer*>(p);
        if (!local) {
            return;
        }
        player = nullptr;
        handleWeaponModel(true);
    }

    void onAttachActor(Actor* a) override {
        FpsCharacterDriver* c = a->getDriver<FpsCharacterDriver>();
        if (!c) {
            return;
        }
        controller = c;
        handleWeaponModel();
    }
    void onDetachActor(Actor* a) override {
        controller = nullptr;
        handleWeaponModel(true);
    }

    void onUpdateSpectator(float dt) override {
        if (player) {
            auto vp = player->getViewport();
            auto cam = vp->getCamera();

            gfxm::vec3 pos;
            gfxm::quat rot;

            pos = controller->getEyePos();
            rot = controller->getEyeQuat();

            cam->setCameraPosition(pos);
            cam->setCameraRotation(rot);

            gfxm::mat4 tr
                = gfxm::translate(gfxm::mat4(1.f), pos)
                * gfxm::to_mat4(rot);
            audioSetListenerTransform(tr);
        }
    }
};


class FpsPlayerController : public IPlayerProxy, public IPlayerController, public IActorLink {
    InputContext input_ctx = InputContext("FpsPlayerAgent");
    InputRange* rangeTranslation = 0;
    InputRange* rangeRotation = 0;
    InputRange* rangeScroll = 0;
    InputAction* inputJump = 0;
    InputAction* inputRecover = 0;
    InputAction* inputShoot = 0;
    InputAction* inputShootAlt = 0;
    InputAction* actionInteract = 0;

    FpsCharacterDriver* controller = nullptr;
    Actor* actor = nullptr;
    LocalPlayer* player = nullptr;

public:
    FpsPlayerController() {
        rangeTranslation = input_ctx.createRange("CharacterLocomotion");
        rangeRotation = input_ctx.createRange("CameraRotation");
        rangeScroll = input_ctx.createRange("Scroll");
        inputJump = input_ctx.createAction("Jump");
        //inputRecover = input_ctx.createAction("Recover");
        inputShoot = input_ctx.createAction("Shoot");
        inputShootAlt = input_ctx.createAction("ShootAlt");
        actionInteract = input_ctx.createAction("CharacterInteract");
    }
    FpsPlayerController(Actor* a)
    : FpsPlayerController() {
        linkActor(a);
    }
    ~FpsPlayerController() {
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
        platformPushMouseState(true, true);
        player = local;
    }
    void onDetachPlayer(IPlayer* p) override {
        LocalPlayer* local = dynamic_cast<LocalPlayer*>(p);
        if (!local) {
            return;
        }
        local->getInputState()->removeContext(&input_ctx);
        platformPopMouseState();
        player = nullptr;
    }

    void onAttachActor(Actor* a) override {
        FpsCharacterDriver* c = a->getDriver<FpsCharacterDriver>();
        if (!c) {
            return;
        }
        controller = c;
        actor = a;
    }
    void onDetachActor(Actor* a) override {
        actor = nullptr;
        controller = nullptr;
    }

    void onUpdateController(float dt) override {
        if (!actor) {
            return;
        }

        gfxm::vec3 dir = rangeTranslation->getVec3();
        gfxm::vec3 angles = rangeRotation->getVec3();
        float scroll = rangeScroll->getValue();

        actor->sendMessage(PAYLOAD_PAWN_CMD{
            .cmd = ePawnMoveDirection,
            .params = { dir.x, dir.y, dir.z }
        });
        actor->sendMessage(PAYLOAD_PAWN_CMD{
            .cmd = ePawnLookOffset,
            .params = { angles.x, angles.y, angles.z }
        });
        actor->sendMessage(PAYLOAD_PAWN_CMD{
            .cmd = ePawnGrabScroll,
            .params = { scroll, .0f, .0f }
        });

        if (inputJump->isPressed()) {
            actor->sendMessage(PAYLOAD_PAWN_CMD{
                .cmd = ePawnJump
            });
        }
        if (inputShoot->isJustPressed()) {
            actor->sendMessage(PAYLOAD_PAWN_CMD{
                .cmd = ePawnGrab
            });
        }
        if (inputShoot->isJustReleased()) {
            actor->sendMessage(PAYLOAD_PAWN_CMD{
                .cmd = ePawnGrabRelease
            });
        }
        if (inputShootAlt->isJustPressed()) {
            actor->sendMessage(PAYLOAD_PAWN_CMD{
                .cmd = ePawnThrow
            });
        }
        if (actionInteract->isJustPressed()) {
            actor->sendMessage(PAYLOAD_PAWN_CMD{
                .cmd = ePawnInteract
            });
        }
    }
};

