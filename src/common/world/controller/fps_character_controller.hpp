#pragma once

#include "fps_character_controller.auto.hpp"

#include <random>

#include "world/controller/actor_controller.hpp"
#include "world/agent/pawn.hpp"
#include "player/player.hpp"
#include "input/input.hpp"
#include "audio/audio.hpp"
#include "platform/platform.hpp"

#include "skeletal_model/skeletal_model.hpp"
#include "m3d/skeletal_instance.hpp"

#include "world/node/node_character_capsule.hpp"


[[cppi_class]];
class FpsCharacterDriver : public ActorDriver {
    int getExecutionPriority() const override { return EXEC_PRIORITY_FIRST; }

    phyWorld* collision_world = nullptr;

    ResourceRef<AudioClip> clips_footstep[5];
    ResourceRef<AudioClip> clip_jump;

    const float eye_height = 1.8f - 0.13f;

    const float step_interval = 1.75f;
    float step_delta_distance = .0f;
    float bob_distance = .0f;

    const float walljump_cooldown = .5f;
    float walljump_recovery_time = .0f;

    const float acceleration_ground = 50.f;
    const float acceleration_air = 8.f;
    const float friction_ground = 16.0f;
    const float max_velocity = 5.5f;
    const float max_velocity_air = 8.89f;

    bool is_grounded = false;
    gfxm::vec3 grav_velo = gfxm::vec3(0, 0, 0);
    float rotation_x = .0f;
    float rotation_y = .0f;
    //float velocity = .0f;
    gfxm::vec3 desired_direction;
    gfxm::vec3 desired_direction_world;
    gfxm::vec3 velo;
    bool has_directional_input = false;

    float lean = .0f;

    gfxm::quat cam_q;
    gfxm::vec3 eye_pos;
    gfxm::quat wpn_q = gfxm::quat(0, 0, 0, 1);
    gfxm::vec3 wpn_offs = gfxm::vec3(0, 0, 0);
    float bob_factor = .0f;
    float bob_factor_target = 1.f;

    Actor* targeted_actor = nullptr;
    phyRigidBody* held_collider = nullptr;
    gfxm::vec3 held_collider_lcl_grab_point;
    gfxm::vec3 held_collider_v;
    phyJoint hold_joint;

    COLLISION_SURFACE_MATERIAL surface_mat = COLLISION_SURFACE_NONE;
    std::unordered_map<COLLISION_SURFACE_MATERIAL, std::vector<ResourceRef<AudioClip>>> footstep_lib;

    std::random_device rnd_dev;
    std::mt19937 rnd_rng;
    int footstep_prev_idx = -1;

    void playFootstep(float gain, const gfxm::vec3& at, COLLISION_SURFACE_MATERIAL mat) {
        auto it = footstep_lib.find(mat);
        if (it == footstep_lib.end()) {
            return;
        }
        std::vector<ResourceRef<AudioClip>>& clips = it->second;

        if(!clips.empty()) {
            size_t count = clips.size();
            
            std::uniform_int_distribution<std::mt19937::result_type> dist(0, count - 2);
            int idx_raw = dist(rnd_rng);
            if (idx_raw >= footstep_prev_idx) {
                idx_raw += 1;
            }
            int idx = idx_raw;
            audioPlayOnce3d(
                clips[idx]->getBuffer(),
                at, gain
            );
            footstep_prev_idx = idx;
        }
        //audioPlayOnce3d(clips_footstep[rand() % 5]->getBuffer(), getOwner()->getRoot()->getTranslation(), gain);
    }
    void playFootstep(float gain) {
        playFootstep(gain, getOwner()->getRoot()->getTranslation(), surface_mat);
    }

public:
    TYPE_ENABLE();

    CharacterCapsuleNode* capsule_node = nullptr;
    EmptyNode* head_node = nullptr;
    ResourceRef<m3dModel> wpn_model;
    m3dSkeletalInstance wpn_instance;

    FpsCharacterDriver() {
        rnd_rng = std::mt19937(rnd_dev());

        {
            footstep_lib[COLLISION_SURFACE_NONE] = {
                loadResource<AudioClip>("audio/sfx/footsteps/asphalt00"),
                loadResource<AudioClip>("audio/sfx/footsteps/asphalt01"),
                loadResource<AudioClip>("audio/sfx/footsteps/asphalt02"),
                loadResource<AudioClip>("audio/sfx/footsteps/asphalt03"),
                loadResource<AudioClip>("audio/sfx/footsteps/asphalt04")
            };
            footstep_lib[COLLISION_SURFACE_CHAINLINK] = {
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/chainlink1"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/chainlink2"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/chainlink3"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/chainlink4")
            };
            footstep_lib[COLLISION_SURFACE_CONCRETE] = {
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/concrete1"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/concrete2"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/concrete3"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/concrete4")
            };
            footstep_lib[COLLISION_SURFACE_DIRT] = {
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/dirt1"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/dirt2"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/dirt3"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/dirt4")
            };
            footstep_lib[COLLISION_SURFACE_DUCT] = {
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/duct1"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/duct2"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/duct3"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/duct4")
            };
            footstep_lib[COLLISION_SURFACE_GRASS] = {
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/grass1"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/grass2"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/grass3"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/grass4")
            };
            footstep_lib[COLLISION_SURFACE_GRAVEL] = {
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/gravel1"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/gravel2"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/gravel3"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/gravel4")
            };
            footstep_lib[COLLISION_SURFACE_LADDER] = {
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/ladder1"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/ladder2"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/ladder3"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/ladder4")
            };
            footstep_lib[COLLISION_SURFACE_METAL] = {
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/metal1"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/metal2"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/metal3"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/metal4")
            };
            footstep_lib[COLLISION_SURFACE_METALGRATE] = {
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/metalgrate1"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/metalgrate2"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/metalgrate3"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/metalgrate4")
            };
            footstep_lib[COLLISION_SURFACE_MUD] = {
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/mud1"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/mud2"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/mud3"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/mud4")
            };
            footstep_lib[COLLISION_SURFACE_SAND] = {
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/sand1"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/sand2"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/sand3"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/sand4")
            };
            footstep_lib[COLLISION_SURFACE_WATER] = {
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/slosh1"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/slosh2"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/slosh3"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/slosh4")
            };
            footstep_lib[COLLISION_SURFACE_TILE] = {
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/tile1"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/tile2"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/tile3"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/tile4")
            };
            footstep_lib[COLLISION_SURFACE_WOOD] = {
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/wood1"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/wood2"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/wood3"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/wood4")
            };
            footstep_lib[COLLISION_SURFACE_WOODPANEL] = {
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/woodpanel1"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/woodpanel2"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/woodpanel3"),
                loadResource<AudioClip>("experimental/hl2/sound/player/footsteps/woodpanel4")
            };
        }

        clip_jump = loadResource<AudioClip>("audio/sfx/swoosh");

        //wpn_model = loadResource<m3dModel>("models/fps_q3_rocket_launcher");
        wpn_model = loadResource<m3dModel>("models/fps_arms");
        wpn_instance.init(wpn_model);
        //weapon_model = resGet<SkeletalModel>("models/fps_q3_rocket_launcher/fps_q3_rocket_launcher.skeletal_model");
        //weapon_model_instance = weapon_model->createInstance();
    }

    GAME_MESSAGE onMessage(GAME_MESSAGE msg) override {
        switch (msg.msg) {
        case GAME_MSG::PAWN_CMD: {
            auto pld = msg.getPayload<GAME_MSG::PAWN_CMD>();
            switch (pld.cmd) {
            case ePawnMoveDirection:
                desired_direction = pld.params;
                break;
            case ePawnLookOffset:
                rotation_y += gfxm::radian(pld.params.y) * .15f;
                rotation_x += gfxm::radian(pld.params.x) * .15f;
                break;
            case ePawnJump: jump(); break;
            case ePawnInteract: interact(); break;
            case ePawnGrab: grab(); break;
            case ePawnGrabRelease:
                held_collider = nullptr;
                collision_world->removeJoint(&hold_joint);
                break;
            case ePawnGrabScroll:
                if (pld.params.x > 0) {
                    held_collider_v *=  .8f;
                } else if (pld.params.x < 0) {
                    held_collider_v *=  1.2f;
                }
                break;
            case ePawnThrow:
                push();
                break;
            }
            return GAME_MSG::HANDLED;
        }
        }
        return GAME_MSG::NOT_HANDLED;
    }
    
    void interact() {
        if (targeted_actor) {
            GAME_MESSAGE rsp = targeted_actor->sendMessage(PAYLOAD_INTERACT{ getOwner() });
        }
    }
    void grab() {
        gfxm::vec3 intersection_point;
        gfxm::vec3 ray_from = eye_pos;
        gfxm::vec3 ray_to = eye_pos + -gfxm::to_mat3(cam_q)[2] * 100.f;
        phyRayCastResult rcr = collision_world->rayTest(ray_from, ray_to, COLLISION_LAYER_DEFAULT);
        if (rcr.hasHit && rcr.collider->mass > .0f) {
            intersection_point = rcr.position;
            playFootstep(.25f, rcr.position, rcr.prop.material);

            held_collider = rcr.collider;
            held_collider_lcl_grab_point = gfxm::inverse(rcr.collider->getTransform()) * gfxm::vec4(rcr.position, 1.f);
            held_collider_v = gfxm::to_mat4(gfxm::inverse(cam_q)) * gfxm::vec4(rcr.position - eye_pos, .0f);
            //rcr.collider->impulseAtPoint(gfxm::normalize(ray_to - ray_from) * 2.f, rcr.position);

            collision_world->removeJoint(&hold_joint);
            hold_joint = phyJoint(nullptr, held_collider, intersection_point, gfxm::mat3(1));
            collision_world->addJoint(&hold_joint);
        }
    }
    void push() {
        if (held_collider) {
            held_collider->impulseAtPoint(gfxm::normalize(held_collider->getCOM() - eye_pos) * 100.f, held_collider->getCOM());
            held_collider = nullptr;

            collision_world->removeJoint(&hold_joint);
        } else {
            gfxm::vec3 intersection_point;
            gfxm::vec3 ray_from = eye_pos;
            gfxm::vec3 ray_to = eye_pos + -gfxm::to_mat3(cam_q)[2] * 100.f;
            phyRayCastResult rcr = collision_world->rayTest(ray_from, ray_to, COLLISION_LAYER_DEFAULT);
            if (rcr.hasHit && rcr.collider->mass > .0f) {
                rcr.collider->impulseAtPoint(gfxm::normalize(ray_to - ray_from) * 100.f, rcr.position);
            }
        }
    }
    void jump() {
        auto root = getOwner()->getRoot();

        if (is_grounded) {
            is_grounded = false;
            grav_velo = gfxm::vec3(0, 5, 0);
            walljump_recovery_time = walljump_cooldown;
            audioPlayOnce3d(clip_jump->getBuffer(), root->getTranslation(), .075f);
            //playFootstep(.25f);
        } else {
            float wj_radius = .3f;
            phySphereSweepResult ssr2 = collision_world->sphereSweep(
                root->getTranslation() + gfxm::vec3(.0f, wj_radius + .1f, .0f),
                root->getTranslation() + gfxm::vec3(.0f, wj_radius + .1f, .0f) + desired_direction_world * .3f,
                wj_radius, COLLISION_LAYER_DEFAULT
            );

            float d = gfxm::dot(gfxm::vec3(0, 1, 0), ssr2.normal);
            bool is_wall = fabsf(d) < cosf(gfxm::radian(45.f));
            if (is_wall && ssr2.hasHit && walljump_recovery_time == .0f) {
                walljump_recovery_time = walljump_cooldown;
                grav_velo = gfxm::vec3(0, 4, 0);
                velo += -desired_direction_world * 10.f;
                audioPlayOnce3d(clip_jump->getBuffer(), root->getTranslation(), .075f);
                playFootstep(.25f);
            }
        }
    }

    void onReset() override {}
    void onSpawnActorDriver(WorldSystemRegistry& reg, Actor* actor) override {
        collision_world = reg.getSystem<phyWorld>();
        if(head_node) {
            wpn_instance.getSkeletonInstance()->setExternalRootTransform(head_node->getTransformHandle());
        }
    }
    void onDespawnActorDriver(WorldSystemRegistry& reg, Actor* actor) override {
        collision_world = nullptr;
    }
    void onActorNodeRegister(rtti::type t, ActorNode* node, const std::string& name) override {
        if (t == rtti::type_get<CharacterCapsuleNode>()) {
            capsule_node = static_cast<CharacterCapsuleNode*>(node);
        }
        if (t == rtti::type_get<EmptyNode>() && name == "head") {
            head_node = static_cast<EmptyNode*>(node);
        }
    }
    void onActorNodeUnregister(rtti::type t, ActorNode* component, const std::string& name) override {
        if (t == rtti::type_get<CharacterCapsuleNode>()) {
            capsule_node = nullptr;
        }
        if (t == rtti::type_get<EmptyNode>() && name == "head") {
            head_node = nullptr;
        }
    }

    void setOrientation(const gfxm::vec3& euler) {
        rotation_x = euler.x;
        rotation_y = euler.y;
    }

    const gfxm::vec3& getEyePos() const {
        return eye_pos;
    }
    const gfxm::quat& getEyeQuat() const {
        return cam_q;
    }

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

    void updateLocomotion(float dt);

    void onUpdate(float dt) override {
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

            phyRayCastResult r = collision_world->rayTest(from, from + forward, COLLISION_LAYER_BEACON);
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

        if (walljump_recovery_time > .0f) {
            walljump_recovery_time -= dt;
            if (walljump_recovery_time < .0f) {
                walljump_recovery_time = .0f;
            }
        }

        if (is_grounded) {
            if (step_delta_distance >= step_interval) {
                playFootstep(.25f);
                step_delta_distance = .0f;
            }
        }

        // Camera
        rotation_x = gfxm::clamp(rotation_x, -gfxm::pi * 0.5f, gfxm::pi * 0.5f);

        gfxm::quat qy = gfxm::angle_axis(rotation_y, gfxm::vec3(0, 1, 0));
        gfxm::quat qx = gfxm::angle_axis(rotation_x, gfxm::vec3(1, 0, 0));

        float lean_new = gfxm::lerp(gfxm::radian(3.f), gfxm::radian(-3.f), (desired_direction.x + 1.0f) * .5f);
        desired_direction_world = gfxm::to_mat4(qy) * gfxm::vec4(gfxm::normalize(desired_direction), .0f);
        desired_direction = desired_direction_world;

        lean = gfxm::lerp(lean, lean_new, .05f);
        gfxm::quat qz = gfxm::angle_axis(lean, gfxm::vec3(0, 0, 1));
        gfxm::quat qcam = qy * qz * qx;
        cam_q = qcam;

        gfxm::vec3 eye_pos_rigid =  root->getWorldTransform() * gfxm::vec4(0, eye_height, 0, 1);
        eye_pos.x = eye_pos_rigid.x;
        eye_pos.y = eye_pos_rigid.y;//gfxm::lerp(eye_pos.y, eye_pos_rigid.y, 1.0f - pow(1.0f - 0.1f * 3.0f, dt * 60.0f));
        eye_pos.z = eye_pos_rigid.z;

        wpn_q = gfxm::slerp(wpn_q, cam_q, 1.0f - pow(1.0f - 0.1f * 3.0f, dt * 60.0f));
        bob_factor = gfxm::lerp(bob_factor, bob_factor_target, 1.0f - pow(1.0f - 0.1f * 3.0f, dt * 10.0f));
        float bob_spd = 1.f;
        float bob_hori = cosf(bob_distance * (gfxm::pi / step_interval) * bob_spd - gfxm::pi * .5f);
        float bob_vert = cosf(bob_distance * (gfxm::pi / step_interval) * 2.f * bob_spd - gfxm::pi * .5f);
        wpn_offs = gfxm::vec3(bob_hori * .01f, bob_vert * .005f, 0) * bob_factor;
        wpn_offs = cam_q * wpn_offs;
        //wpn_offs = gfxm::lerp(wpn_offs, gfxm::vec3(0, bob_factor * .02f, 0), 1.0f - pow(1.0f - 0.1f * 3.0f, dt * 60.0f));
        /*
        gfxm::quat wpn_delta_q = gfxm::inverse(cam_q) * wpn_q;
        gfxm::quat wpn_q2 = gfxm::inverse(wpn_delta_q) * cam_q;
        */
        head_node->getTransformHandle()->setInheritFlags(0);
        head_node->setTranslation(eye_pos + wpn_offs);
        head_node->setRotation(wpn_q);

        updateLocomotion(dt);        
    }
};