#include "fps_character_controller.hpp"

constexpr float OVERBOUNCE = 1.0f;

static gfxm::vec3 clipVelocity (const gfxm::vec3& V, const gfxm::vec3& N, float overbounce = OVERBOUNCE) {
    float backoff = gfxm::dot(V, N) * overbounce;
    gfxm::vec3 out = V - N * backoff;
    return out;
}

static bool moveAndSlideYCapsule(
    phyWorld* world, float dt,
    const gfxm::vec3& P0, float capHeight, float capRadius,
    gfxm::vec3& V_out, gfxm::vec3& velo, gfxm::vec3& grav_velo
) {
    if (V_out.length2() <= .0f) {
        return false;
    }

    gfxm::vec3& V = V_out;
    float R = capRadius;
    float H = capHeight;

    gfxm::vec3 C = P0;

    const int MAX_PLANES = 4;
    gfxm::vec3 planes[MAX_PLANES];
    int n_planes = 0;

    // TODO: Adds a plane behind us, forbidding backwards adjustment, idk how necessary this is
    //planes[n_planes++] = gfxm::normalize(V);

    const int MAX_SWEEPS = 4; // 3 planes + an extra sweep in case we collide with the same surface twice and hopefully V += N * .001f helps
    int i = 0; // declared outside for the sweep count
    for(i = 0; i < MAX_SWEEPS; ++i) {
        phyCapsuleSweepResult csr = world->capsuleSweep(
            C, C + V,
            H, R
        );
        if (!csr.hasHit) {
            ++i; // for correct sweep count reporting
            break;
        }
        const gfxm::vec3& N = csr.normal;

        //dbgDrawLine(csr.contact - gfxm::vec3(0, 1, 0), csr.contact + gfxm::vec3(0, 1, 0), DBG_COLOR_GREEN);

        C += gfxm::normalize(V) * gfxm::_max(.0f, fabsf(csr.distance) - .001f);

        bool is_old_plane = false;
        for (int j = 0; j < n_planes; ++j) {
            if (gfxm::dot(N, planes[j]) > 0.99f) {
                V += N * .001f;
                is_old_plane = true;
                break;
            }
        }
        if (is_old_plane) {
            continue;
        }
        if (n_planes < MAX_PLANES) {
            planes[n_planes++] = N;
        }

        for (int j = 0; j < n_planes; ++j) {
            const gfxm::vec3& Nj = planes[j];
            float d = gfxm::dot(V, Nj);
            if (d >= .0f) {
                continue;
            }

            V = clipVelocity(V, Nj, OVERBOUNCE);
            velo = clipVelocity(velo, Nj);
            grav_velo = clipVelocity(grav_velo, Nj);

            for (int k = 0; k < n_planes; ++k) {
                if (k == j) {
                    continue;
                }
                const gfxm::vec3& Nk = planes[k];
                float d = gfxm::dot(V, Nk);
                if (d >= .0f) {
                    continue;
                }                    

                V = clipVelocity(V, Nk, OVERBOUNCE);
                velo = clipVelocity(velo, Nk);
                grav_velo = clipVelocity(grav_velo, Nk);

                if (gfxm::dot(V, Nj) >= .0f) {
                    continue;
                }

                gfxm::vec3 dir = gfxm::cross(Nj, Nk);
                dir = gfxm::normalize(dir);
                d = gfxm::dot(dir, V);
                V = dir * d;

                d = gfxm::dot(dir, velo);
                velo = dir * d;

                d = gfxm::dot(dir, grav_velo);
                grav_velo = dir * d;

                for (int l = 0; l < n_planes; ++l) {
                    if (l == k || l == j) {
                        continue;
                    }
                    if (gfxm::dot(V, planes[l]) >= .0f) {
                        continue;
                    }

                    V_out = gfxm::vec3(0, 0, 0);
                    return true;
                }
            }

            break;
        }
    }

    //V_out = V;
    V_out = (C - P0) + V; // V here is leftover offset after all the clipping

    //dbgDrawLine(C, C + V / dt, DBG_COLOR_BLUE | DBG_COLOR_GREEN);
    //dbgDrawText(P0, std::format("sweeps: {}", i), 0xFFFFFFFF);
    return i != 0;
}

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
            /*
            const float slop = .5f;
            const float bias_factor = .2f;
            const float inv_dt = dt > .0f ? (1.f / dt) : .0f;
            float bias = -bias_factor * inv_dt * gfxm::_min(.0f, -vlen + slop);
            */
            held_collider->angular_velocity = gfxm::vec3(0,0,0);
            held_collider->velocity = V * 50.f;

            //held_collider->impulseAtPoint(N * bias, collider_hold_p);
            //held_collider->is_sleeping = false;
            //held_collider->impulseAtPoint(1.f * V * 400.f * dt, collider_hold_p);
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
    moveAndSlideYCapsule(
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

