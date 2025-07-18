#pragma once

#include "fps_character_controller.auto.hpp"

#include "world/controller/actor_controller.hpp"
#include "player/player.hpp"
#include "input/input.hpp"
#include "platform/platform.hpp"


[[cppi_class]];
class FpsCharacterController : public ActorController {
    int getExecutionPriority() const override { return EXEC_PRIORITY_FIRST; }

    IPlayer* current_player = 0;

    InputContext input_ctx = InputContext("FpsCharacterController");
    InputRange* rangeTranslation = 0;
    InputRange* rangeRotation = 0;
    InputAction* inputJump = 0;
    InputAction* inputRecover = 0;
    InputAction* inputShoot = 0;
    InputAction* actionInteract = 0;

    RHSHARED<AudioClip> clips_footstep[5];
    RHSHARED<AudioClip> clip_jump;

    const float eye_height = 1.8f - 0.13f;

    const float step_interval = 1.75f;
    float step_delta_distance = .0f;

    const float walljump_cooldown = .5f;
    float walljump_recovery_time = .0f;

    const float acceleration_ground = 50.f;
    const float acceleration_air = 8.f;
    const float friction_ground = 16.0f;
    const float max_velocity = 5.0f;
    const float max_velocity_air = 8.89f;

    bool is_grounded = false;
    gfxm::vec3 grav_velo = gfxm::vec3(0, 0, 0);
    float rotation_x = .0f;
    float rotation_y = .0f;
    //float velocity = .0f;
    gfxm::vec3 desired_direction;
    gfxm::vec3 velo;

    float lean = .0f;

    bool is_climbing = false;
    gfxm::vec3 climb_target;

    gfxm::quat cam_q;

    Actor* targeted_actor = nullptr;

    void playFootstep(float gain) {
        audioPlayOnce3d(clips_footstep[rand() % 5]->getBuffer(), getOwner()->getRoot()->getTranslation(), gain);
    }

public:
    TYPE_ENABLE();

    FpsCharacterController() {
        rangeTranslation = input_ctx.createRange("CharacterLocomotion");
        rangeRotation = input_ctx.createRange("CameraRotation");
        inputJump = input_ctx.createAction("Jump");
        inputRecover = input_ctx.createAction("Recover");
        inputShoot = input_ctx.createAction("Shoot");
        actionInteract = input_ctx.createAction("CharacterInteract");

        clips_footstep[0] = getAudioClip("audio/sfx/footsteps/asphalt00.ogg");
        clips_footstep[1] = getAudioClip("audio/sfx/footsteps/asphalt01.ogg");
        clips_footstep[2] = getAudioClip("audio/sfx/footsteps/asphalt02.ogg");
        clips_footstep[3] = getAudioClip("audio/sfx/footsteps/asphalt03.ogg");
        clips_footstep[4] = getAudioClip("audio/sfx/footsteps/asphalt04.ogg");
        clip_jump = getAudioClip("audio/sfx/swoosh.ogg");
    }

    GAME_MESSAGE onMessage(GAME_MESSAGE msg) override {
        switch (msg.msg) {
        case GAME_MSG::PLAYER_ATTACH: {
            current_player = msg.getPayload<GAME_MSG::PLAYER_ATTACH>().player;
            current_player->getInputState()->pushContext(&input_ctx);
            platformPushMouseState(true, true);
            return GAME_MSG::HANDLED;
        }
        case GAME_MSG::PLAYER_DETACH: {
            auto player = msg.getPayload<GAME_MSG::PLAYER_DETACH>().player;
            player->getInputState()->removeContext(&input_ctx);
            current_player = 0;
            platformPopMouseState();
            return GAME_MSG::HANDLED;
        }
        }
        return GAME_MSG::NOT_HANDLED;
    }

    void onReset() override {}
    void onSpawn(Actor* actor) override {}
    void onDespawn(Actor* actor) override {}
    void onActorNodeRegister(type t, ActorNode* component, const std::string& name) override {}
    void onActorNodeUnregister(type t, ActorNode* component, const std::string& name) override {}

    gfxm::vec3 applyFriction(float dt, const gfxm::vec3& v, bool is_really_grounded) {
        float speed = v.length();
        float newspeed = .0f;
        float drop = .0f;
        
        gfxm::vec3 rv = v;

        if (speed <= FLT_EPSILON) {
            return rv;
        }

        if (is_really_grounded/* && desired_direction.length() <= FLT_EPSILON*/) {
            drop += friction_ground * dt;
        }

        newspeed = speed - drop;
        if (newspeed < .0f) {
            newspeed = .0f;
        }
        newspeed /= speed;

        rv *= newspeed;
        return rv;
    }

    void updateLocomotion(RuntimeWorld* world, float dt) {
        auto root = getOwner()->getRoot();
        if (!root) {
            assert(false);
            return;
        }

        if (inputShoot->isJustPressed()) {
            MSGPLD_MISSILE_SPAWN pld;
            pld.translation = root->getTranslation() + gfxm::vec3(0, eye_height - .25f, 0);
            pld.orientation = cam_q;
            world->postMessage((int)MSGID_MISSILE_SPAWN, pld);
        }

        bool is_really_grounded = false;
        //if (!is_grounded) {
            root->translate(grav_velo * dt);
        //}
        float radius = .1f;
        SphereSweepResult ssr = world->getCollisionWorld()->sphereSweep(
            root->getTranslation() + gfxm::vec3(.0f, .3f, .0f),
            root->getTranslation() - gfxm::vec3(.0f, .3f, .0f),
            radius, COLLISION_LAYER_DEFAULT
        );
        if (ssr.hasHit && grav_velo.y <= .0f) {
            gfxm::vec3 pos = root->getTranslation();
            float y_offset = ssr.sphere_pos.y - radius - pos.y;
            // y_offset > .0f if the character is sunk into the ground
            // y_offset < .0f if the character is floating above
            
            if (y_offset >= .0f) {
                if (!is_grounded) {
                    playFootstep(.5f);
                    step_delta_distance = .0f;
                }
                is_grounded = true;
                is_really_grounded = true;
                grav_velo = gfxm::vec3(0, 0, 0);
                velo.y = .0f; // -_-
                root->translate(gfxm::vec3(.0f, y_offset * 10.f * dt, .0f));
            } else {
                grav_velo -= gfxm::vec3(.0f, 9.8f * dt, .0f);
                // 53m/s is the maximum approximate terminal velocity for a human body
                grav_velo.y = gfxm::_min(53.f, grav_velo.y);
            }
        } else {
            is_grounded = false;
            grav_velo -= gfxm::vec3(.0f, 9.8f * dt, .0f);
            // 53m/s is the maximum approximate terminal velocity for a human body
            grav_velo.y = gfxm::_min(53.f, grav_velo.y);
        }

        velo = applyFriction(dt, velo, is_really_grounded);

        if (is_really_grounded) {
            velo += desired_direction * acceleration_ground * dt;
        } else {
            velo += desired_direction * acceleration_air * dt;
        }

        if (velo.length() > max_velocity) {
            velo = gfxm::normalize(velo) * max_velocity;
        }

        gfxm::vec3 translation = velo * dt;
        
        root->translate(translation);
        if (is_really_grounded) {
            step_delta_distance += translation.length();
        }
    }
    void updateClimb(RuntimeWorld* world, float dt) {
        auto root = getOwner()->getRoot();
        if (!root) {
            assert(false);
            return;
        }

        gfxm::vec3 pos = gfxm::lerp(root->getTranslation(), climb_target, .05);
        root->setTranslation(pos);
        if (gfxm::length(climb_target - pos) < .05f) {
            is_climbing = false;
        }
    }

    void onUpdate(RuntimeWorld* world, float dt) override {
        auto root = getOwner()->getRoot();
        if (!root) {
            assert(false);
            return;
        }

        // Interaction check
        {
            if (targeted_actor) {
                targeted_actor->sendMessage(PAYLOAD_HIGHLIGHT_OFF{ getOwner() });
            }
            targeted_actor = nullptr;

            gfxm::vec3 from = root->getTranslation() + gfxm::vec3(0, eye_height, 0);
            gfxm::vec3 forward = gfxm::to_mat4(cam_q) * gfxm::vec4(0,0,-1.5,0);

            RayCastResult r = world->getCollisionWorld()->rayTest(from, from + forward, COLLISION_LAYER_BEACON);
            if (r.hasHit) {
                if (r.collider->user_data.type == COLLIDER_USER_NODE) {
                    ActorNode* node = (ActorNode*)r.collider->user_data.user_ptr;
                    assert(node);
                    // TODO: get owning actor?
                } else if (r.collider->user_data.type == COLLIDER_USER_ACTOR) {
                    Actor* actor = (Actor*)r.collider->user_data.user_ptr;
                    assert(actor);
                    targeted_actor = actor;
                }
            }
        }
        if (targeted_actor) {
            targeted_actor->sendMessage(PAYLOAD_HIGHLIGHT_ON{ getOwner() });
        }
        // Interaction action
        if (actionInteract->isJustPressed()) {
            if (targeted_actor) {
                GAME_MESSAGE rsp = targeted_actor->sendMessage(PAYLOAD_INTERACT{ getOwner() });
            }
        }
        
        // Ledge grab check
        bool ledge_available = false;
        gfxm::vec3 ledge_contact_pos;
        {
            const float forward_check_radius = .35f;
            const float platform_check_radius = .35f;
            const gfxm::vec3 y_offset = gfxm::vec3(0, eye_height + .7f, 0);
            const gfxm::vec3 from = root->getTranslation() + y_offset;
            const gfxm::quat qy = gfxm::angle_axis(rotation_y, gfxm::vec3(0, 1, 0));
            const gfxm::vec3 forward = gfxm::to_mat4(qy) * gfxm::vec4(0, 0, -1, 0);
            SphereSweepResult ssr_fwd = world->getCollisionWorld()->sphereSweep(
                from,
                from + forward,
                forward_check_radius,
                COLLISION_LAYER_DEFAULT
            );
            if (!ssr_fwd.hasHit) {
                const gfxm::vec3 y_offset = gfxm::vec3(0, eye_height + .5f, 0);
                const gfxm::vec3 from = root->getTranslation() + y_offset + forward;
                const gfxm::vec3 to = from + gfxm::vec3(0, -1.f, 0);
                SphereSweepResult ssr_platform = world->getCollisionWorld()->sphereSweep(
                    from, to, platform_check_radius, COLLISION_LAYER_DEFAULT
                );
                if (ssr_platform.hasHit) {
                    float d = gfxm::dot(ssr_platform.normal, gfxm::vec3(0, 1, 0));
                    if (d > .7f) {
                        ledge_available = true;
                        ledge_contact_pos = ssr_platform.contact;
                    }
                }
            }
        }

        // Wall jump
        float wj_radius = .3f;
        SphereSweepResult ssr2 = world->getCollisionWorld()->sphereSweep(
            root->getTranslation() + gfxm::vec3(.0f, wj_radius + .1f, .0f),
            root->getTranslation() + gfxm::vec3(.0f, wj_radius + .1f, .0f) + desired_direction * .3f,
            wj_radius, COLLISION_LAYER_DEFAULT
        );
        if (!is_grounded) {
            float d = gfxm::dot(gfxm::vec3(0, 1, 0), ssr2.normal);
            bool is_wall = fabsf(d) < cosf(gfxm::radian(45.f));
            if (is_wall && ssr2.hasHit && inputJump->isJustPressed() && walljump_recovery_time == .0f) {
                walljump_recovery_time = walljump_cooldown;
                grav_velo = gfxm::vec3(0, 4, 0);
                velo += ssr2.normal * 10.f;
                audioPlayOnce3d(clip_jump->getBuffer(), root->getTranslation(), .075f);
                playFootstep(.5f);
            }
        }
        if (walljump_recovery_time > .0f) {
            walljump_recovery_time -= dt;
            if (walljump_recovery_time < .0f) {
                walljump_recovery_time = .0f;
            }
        }

        if (inputRecover->isJustPressed()) {
            root->setTranslation(0, 10, 0);
            grav_velo = gfxm::vec3(0, 0, 0);
        }
        // Normal jump
        if (inputJump->isPressed() && is_grounded && !ledge_available) {
            is_grounded = false;
            grav_velo = gfxm::vec3(0, 4, 0);
            audioPlayOnce3d(clip_jump->getBuffer(), root->getTranslation(), .075f);
        }
        // Ledge grab
        if (inputJump->isJustPressed() && ledge_available && !is_climbing) {
            //root->setTranslation(ledge_contact_pos);
            is_climbing = true;
            climb_target = ledge_contact_pos;
            grav_velo = gfxm::vec3(0, 0, 0);
            velo = gfxm::vec3(0, 0, 0);
            audioPlayOnce3d(clip_jump->getBuffer(), root->getTranslation(), .075f);
        }

        if (is_grounded) {
            if (step_delta_distance >= step_interval) {
                playFootstep(.3f);
                step_delta_distance = .0f;
            }
        }

        if(is_climbing) {
            updateClimb(world, dt);
        } else {
            updateLocomotion(world, dt);
        }

        // Camera
        rotation_y += gfxm::radian(rangeRotation->getVec3().y) * .15f;
        rotation_x += gfxm::radian(rangeRotation->getVec3().x) * .15f;
        rotation_x = gfxm::clamp(rotation_x, -gfxm::pi * 0.5f, gfxm::pi * 0.5f);

        gfxm::quat qy = gfxm::angle_axis(rotation_y, gfxm::vec3(0, 1, 0));
        gfxm::quat qx = gfxm::angle_axis(rotation_x, gfxm::vec3(1, 0, 0));

        desired_direction = rangeTranslation->getVec3();
        float lean_new = gfxm::lerp(gfxm::radian(3.f), gfxm::radian(-3.f), (desired_direction.x + 1.0f) * .5f);
        desired_direction = gfxm::to_mat4(qy) * gfxm::vec4(gfxm::normalize(desired_direction), .0f);

        lean = gfxm::lerp(lean, lean_new, .05f);
        gfxm::quat qz = gfxm::angle_axis(lean, gfxm::vec3(0, 0, 1));
        gfxm::quat qcam = qy * qz * qx;
        cam_q = qcam;

        if (current_player) {
            auto viewport = current_player->getViewport();

            viewport->setFov(80.f);
            viewport->setCameraPosition(root->getWorldTransform() * gfxm::vec4(0, eye_height, 0, 1));
            viewport->setCameraRotation(qcam);
            viewport->setZFar(1000.f);
            viewport->setZNear(.01f);

            audioSetListenerTransform(root->getWorldTransform() * gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(0, eye_height, 0)));
        }
    }
};