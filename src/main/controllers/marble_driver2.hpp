#pragma once

#include "marble_driver2.auto.hpp"

#include "world/controller/actor_controller.hpp"
#include "player/player.hpp"
#include "input/input.hpp"
#include "audio/audio.hpp"

#include "world/node/rigid_body_node.hpp"


[[cppi_class]];
class MarbleDriver2 : public ActorDriver {
    int getExecutionPriority() const override { return EXEC_PRIORITY_FIRST; }
    
    phyWorld* phy_world = nullptr;

    ResourceRef<AudioClip> clip_jump;

    ActorNodeView<RigidBodyNode> body;
    gfxm::vec3 desired_dir;

    bool grounded = false;
    gfxm::vec3 Nground = gfxm::vec3(0, 1, 0);
    gfxm::vec3 ang_velo;
    gfxm::vec3 velocity;
    gfxm::vec3 grav_velocity;

    void impulseAtPoint(const gfxm::vec3& impulse, const gfxm::vec3& point);
public:
    TYPE_ENABLE();

    MarbleDriver2() {
        body = registerNodeView<RigidBodyNode>("body", true);
        clip_jump = loadResource<AudioClip>("audio/sfx/swoosh");
    }

    GAME_MESSAGE onMessage(GAME_MESSAGE msg) override {
        switch (msg.msg) {
        case GAME_MSG::PAWN_CMD: {
            auto pld = msg.getPayload<GAME_MSG::PAWN_CMD>();
            switch (pld.cmd) {
            case ePawnInteract:
                if(grounded) {
                    velocity += Nground * 6.f;
                    audioPlayOnce(clip_jump->getBuffer(), .3f);
                    grounded = false;
                }
                return GAME_MSG::HANDLED;
            case ePawnMoveDirection:
                desired_dir = pld.params;
                return GAME_MSG::HANDLED;
            }
            break;
        }
        }
        return GAME_MSG::NOT_HANDLED;
    }

    void onReset() override {}
    void onSpawnActorDriver(WorldSystemRegistry& reg, Actor* actor) override {
        if (auto sys = reg.getSystem<phyWorld>()) {
            phy_world = sys;
        }
    }
    void onDespawnActorDriver(WorldSystemRegistry& reg, Actor* actor) override {
        phy_world = nullptr;
    }
    
    void onUpdate(float dt) override;
};

