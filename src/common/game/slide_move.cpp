#include "slide_move.hpp"


gfxm::vec3 clipVelocity(const gfxm::vec3& V, const gfxm::vec3& N, float overbounce) {
    float backoff = gfxm::dot(V, N) * overbounce;
    gfxm::vec3 out = V - N * backoff;
    return out;
}

bool slideMoveYCapsule(
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
        phyCapsuleSweepResult csr = world->capsuleYSweep(
            C, C + V,
            H, R
        );
        if (!csr.hasHit) {
            break;
        }
        const gfxm::vec3& N = csr.normal;

        //dbgDrawLine(csr.contact - gfxm::vec3(0, 1, 0), csr.contact + gfxm::vec3(0, 1, 0), DBG_COLOR_GREEN);

        //C += gfxm::normalize(V) * gfxm::_max(.0f, fabsf(csr.distance) - .001f);
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

