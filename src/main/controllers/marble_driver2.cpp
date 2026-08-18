#include "marble_driver2.hpp"


constexpr float OVERBOUNCE = 1.0f;

static gfxm::vec3 clipVelocity(const gfxm::vec3& V, const gfxm::vec3& N, float overbounce = OVERBOUNCE) {
    float backoff = gfxm::dot(V, N) * overbounce;
    gfxm::vec3 out = V - N * backoff;
    return out;
}

static void impulseAtPoint(
    const gfxm::vec3& COM,
    const gfxm::mat3& inv_inertia,
    float mass,
    const gfxm::vec3& impulse,
    const gfxm::vec3& point,
    gfxm::vec3& out_velo,
    gfxm::vec3& out_ang_velo
) {
    out_velo += impulse * (1.0f / mass);

    gfxm::vec3 wCOM = COM;
    gfxm::vec3 r = point - wCOM;
    gfxm::vec3 torque = gfxm::cross(r, impulse);
    gfxm::mat3 inverse_inertia_tensor_world = inv_inertia;
    out_ang_velo += inverse_inertia_tensor_world * torque;
}

static void solveContact(
    float dt,
    const gfxm::vec3& COM,
    const gfxm::mat3& inv_inertia,
    const gfxm::vec3& CP,
    const gfxm::vec3& N,
    float mass,
    gfxm::vec3& out_velo,
    gfxm::vec3& out_ang_velo
) {
    const gfxm::vec3 rA = CP - COM;
    const gfxm::vec3 V = out_velo + gfxm::cross(out_ang_velo, rA);
    const gfxm::vec3 relV = V;

    const float invMass = 1.f / mass;
    const float invMassSum = invMass;

    gfxm::vec3 t1;
    gfxm::vec3 t2;
    if(fabsf(N.x) < 0.57735f) {
        t1 = gfxm::normalize(gfxm::cross(N, gfxm::vec3(1,0,0)));
    } else {
        t1 = gfxm::normalize(gfxm::cross(N, gfxm::vec3(0,1,0)));
    }
    t2 = gfxm::cross(N, t1);

    gfxm::vec3 rAxt1 = gfxm::cross(rA, t1);
    float denom_t1 = invMassSum
        + gfxm::dot(t1, gfxm::cross(inv_inertia * rAxt1, rA));
    float mass_tangent1 = denom_t1 > .0f ? (1.f / denom_t1) : .0f;

    gfxm::vec3 rAxt2 = gfxm::cross(rA, t2);
    float denom_t2 = invMassSum
        + gfxm::dot(t2, gfxm::cross(inv_inertia * rAxt2, rA));
    float mass_tangent2 = denom_t2 > .0f ? (1.f / denom_t2) : .0f;

    float vt1 = gfxm::dot(relV, t1);
    float vt2 = gfxm::dot(relV, t2);

    float jt1_delta = (-vt1) * mass_tangent1;
    float jt2_delta = (-vt2) * mass_tangent2;

    const gfxm::vec3 rAxN = gfxm::cross(rA, N);
    float denom_n = invMassSum + gfxm::dot(N, gfxm::cross(inv_inertia * rAxN, rA));
    float mass_normal = denom_n > .0f ? (1.f / denom_n) : .0f;
    float vn = gfxm::dot(V, N);
    float jn = fmaxf(0.f, -vn * mass_normal);

    float mu = 0.6f * 0.6f;

    gfxm::vec3 jt_delta_vec = jt1_delta * t1 + jt2_delta * t2;
    float jt_len = gfxm::length(jt_delta_vec);
    float jt_max = mu * jn;
    if (jt_len > jt_max && jt_len > 1e-6f) {
        jt_delta_vec *= (jt_max / jt_len);
    }

    impulseAtPoint(COM, inv_inertia, mass, jt_delta_vec, CP, out_velo, out_ang_velo);
}

static bool slideMoveSphere(
    phyWorld* world, float dt,
    const gfxm::vec3& P0, float radius, const gfxm::mat3& inv_inertia,
    gfxm::vec3& V_out, gfxm::vec3& velo, gfxm::vec3& grav_velo, gfxm::vec3& ang_velo, gfxm::vec3& out_N
) {
    if (V_out.length2() <= .0f) {
        return false;
    }

    out_N = gfxm::vec3(0, 0, 0);

    gfxm::vec3& V = V_out;
    float R = radius;

    gfxm::vec3 C = P0;

    const int MAX_PLANES = 4;
    gfxm::vec3 planes[MAX_PLANES];
    int n_planes = 0;

    const int MAX_SWEEPS = 4; // 3 planes + an extra sweep in case we collide with the same surface twice and hopefully V += N * .001f helps
    int i = 0; // declared outside for hit count
    for(i = 0; i < MAX_SWEEPS; ++i) {
        phySphereSweepResult csr = world->sphereSweep(
            C, C + V, R,
            COLLISION_LAYER_DEFAULT
        );
        if (!csr.hasHit) {
            break;
        }
        const gfxm::vec3& N = csr.normal;
        out_N += N;

        gfxm::vec3 advance = gfxm::normalize(V) * gfxm::_max(.0f, csr.distance - .001f);
        C += advance;
        V -= advance;

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

        solveContact(dt, C, inv_inertia, csr.contact, csr.normal, 1, velo, ang_velo);

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

    V_out = (C - P0) + V; // V here is leftover offset after all the clipping
    if(i > 0) {
        out_N = gfxm::normalize(out_N / float(i));
    }

    return i != 0;
}

void MarbleDriver2::impulseAtPoint(const gfxm::vec3& impulse, const gfxm::vec3& point) {
    float mass = 1.f;
    float radius = .25f;
    float I = .4f * mass * powf(radius, 2.f);
    gfxm::mat3 inertia;
    inertia[0] = gfxm::vec3(I, .0f, .0f);
    inertia[1] = gfxm::vec3(.0f, I, .0f);
    inertia[2] = gfxm::vec3(.0f, .0f, I);
    gfxm::mat3 inv_inertia = gfxm::inverse(inertia);

    const gfxm::vec3 position = getOwner()->getTranslation();
    const gfxm::vec3 mass_center(0, 0, 0);
    const gfxm::mat3 m3_rotation = gfxm::to_mat3(getOwner()->getRotation());

    velocity += impulse * (1.0f / mass);

    gfxm::vec3 wCOM = position + m3_rotation * mass_center;
    gfxm::vec3 r = point - wCOM;
    gfxm::vec3 torque = gfxm::cross(r, impulse);
    ang_velo += inv_inertia * torque;
}

void MarbleDriver2::onUpdate(float dt) {
    gfxm::vec3 world_v = desired_dir;

    gfxm::vec3 pos = getOwner()->getTranslation();

    float mass = 1.f;
    float radius = .25f;
    float I = .4f * mass * powf(radius, 2.f);
    gfxm::mat3 inertia;
    inertia[0] = gfxm::vec3(I, .0f, .0f);
    inertia[1] = gfxm::vec3(.0f, I, .0f);
    inertia[2] = gfxm::vec3(.0f, .0f, I);
    gfxm::mat3 inv_inertia = gfxm::inverse(inertia);

    ang_velo *= powf(.5f, dt);
    /*
    if (!grounded) {
        velocity += (1.f / mass) * world_v * 7.5f * dt;
    }
    gfxm::vec3 torque = gfxm::cross(gfxm::vec3(.0, 1.f, .0f), world_v * 3.f);
    ang_velo += inv_inertia * (torque * dt);
    */
    impulseAtPoint(world_v * 7.5f * dt, pos + Nground * .25f);

    velocity += gfxm::vec3(.0f, -9.8f, .0f) * dt;

    gfxm::vec3 translation
        = velocity * dt
        + grav_velocity * dt;
    if (slideMoveSphere(phy_world, dt, pos, radius, inv_inertia, translation, velocity, grav_velocity, ang_velo, Nground)) {
        grounded = true;
    } else {
        grounded = false;
        Nground = gfxm::vec3(0, 1, 0);
    }
    
    getOwner()->translate(translation);
    getOwner()->rotate(gfxm::angle_axis(ang_velo.length() * dt, gfxm::normalize(ang_velo)));
    if (body.isValid()) {
        body->collider.is_sleeping = false;
        body->collider.setFlags(PHY_COLLIDER_FLAGS::COLLIDER_NO_RESPONSE);
        body->collider.velocity = velocity;
        body->collider.angular_velocity = ang_velo;
        body->collider.setPosition(getOwner()->getTranslation());
        body->collider.setRotation(getOwner()->getRotation());
    }
}

