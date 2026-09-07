#include "fps_character_controller.hpp"

#include "game/slide_move.hpp"


void FpsCharacterDriver::updateLocomotion(float dt) {
    auto root = getOwner()->getRoot();
    if (!root) {
        assert(false);
        return;
    }

    gfxm::vec3 ground_correction;

    if (held_collider) {
        dbgDrawText(held_collider->getPosition(), std::format("Mass: {:.2f}", held_collider->mass).c_str(), 0xFFFFFFFF);

        gfxm::vec3 hold_v_world = gfxm::to_mat4(cam_q) * gfxm::vec4(held_collider_v, .0f);
        gfxm::vec3 hold_p_world = eye_pos + hold_v_world;
        gfxm::vec3 collider_hold_p = held_collider->getTransform() * gfxm::vec4(held_collider_lcl_grab_point, 1.f);
        gfxm::vec3 V = hold_p_world - collider_hold_p;
        if (hold_p_world.length2() > FLT_EPSILON) {
            //const gfxm::vec3 N = gfxm::normalize(V);
            float vlen = V.length();
            if(vlen > 1.0f) {
                V *= 1.0f / vlen;
            }
            
            hold_joint.lcl_anchor_a = hold_p_world;
            held_collider->is_sleeping = false;
            /*
            held_collider->angular_velocity = gfxm::vec3(0,0,0);
            held_collider->velocity = V * 50.f;
            */

            /*
            held_collider->is_sleeping = false;
            held_collider->impulseAtPoint(1.f * V * 40.f * held_collider->mass * dt, collider_hold_p);
            */
        }
        dbgDrawSphere(collider_hold_p, .1f, 0xFF00FF00);
        dbgDrawLine(collider_hold_p, hold_p_world, 0xFF00FF00);
    }

    bool had_directional_input = has_directional_input;
    has_directional_input = desired_direction.length2() > FLT_EPSILON;
    if (is_grounded && had_directional_input && !has_directional_input) {
        playFootstep(.25f);
        step_delta_distance = .0f;
        bob_factor_target = .0f;
    }
    if (is_grounded && !had_directional_input && has_directional_input) {
        bob_distance = .0f;
        bob_factor_target = 1.f;
    }

    bool was_grounded = is_grounded;

    bool is_really_grounded = false;
    float radius = .1f;
    phySphereSweepResult ssr = collision_world->sphereSweep(
        root->getTranslation() + gfxm::vec3(.0f, .3f, .0f),
        root->getTranslation() - gfxm::vec3(.0f, .3f, .0f),
        radius, COLLISION_LAYER_DEFAULT
    );
    if (ssr.hasHit) {
        surface_mat = ssr.prop.material;

        if(grav_velo.y <= .0f) {
            gfxm::vec3 pos = root->getTranslation();
            float y_offset = ssr.sphere_pos.y - radius - pos.y;
            // y_offset > .0f if the character is sunk into the ground
            // y_offset < .0f if the character is floating above

            if (y_offset >= .0f) {
                grav_velo = gfxm::vec3(0, 0, 0);
                velo.y = .0f; // -_-
                //root->translate(gfxm::vec3(.0f, y_offset * 10.f * dt, .0f));
                ground_correction = gfxm::vec3(.0f, y_offset, .0f);

                is_grounded = true;
                is_really_grounded = true;
            } else {
                //root->translate(gfxm::vec3(.0f, y_offset * 10.f * dt, .0f));
                //float factor = fabsf(y_offset / .2f);
                //root->translate(gfxm::vec3(.0f, y_offset * factor * 15.f * dt, .0f));
                if(was_grounded) {
                    ground_correction = gfxm::vec3(.0f, y_offset, .0f);
                }
                /*
                grav_velo -= gfxm::vec3(.0f, 9.8f * dt, .0f);
                // 53m/s is the maximum approximate terminal velocity for a human body
                grav_velo.y = gfxm::_min(53.f, grav_velo.y);*/
                if(y_offset > -.05f) {
                    is_grounded = true;
                    is_really_grounded = true;
                    ground_correction = gfxm::vec3(.0f, y_offset, .0f);
                }
            }
        }
    } else {
        is_grounded = false;
    }

    if (was_grounded && !is_grounded) {
        LOG_DBG("left ground");
        bob_factor_target = .0f;
    }

    if (!was_grounded && is_grounded) {
        playFootstep(.25f);
        step_delta_distance = .0f;
        bob_factor_target = 1.f;
    }

    if (!is_grounded) {
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
    translation += grav_velo * dt;
    translation += ground_correction;
    slideMoveYCapsule(
        collision_world, dt,
        root->getTranslation() + capsule_node->collider.getCenterOffset(),
        capsule_node->shape.height, capsule_node->shape.radius,
        translation, velo, grav_velo
    );

    //dbgDrawText(root->getTranslation() + gfxm::vec3(0, 0, 1), std::format("velo: {:.4f}", velo.length()), 0xFFFFFFFF);
    //dbgDrawText(root->getTranslation() + gfxm::vec3(0, 0, .5f), std::format("translation: {:.4f}", translation.length()), 0xFFFFFFFF);

    root->translate(translation);
    if (is_really_grounded) {
        step_delta_distance += translation.length();
        bob_distance += translation.length();
    }
}

