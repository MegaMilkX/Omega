#include "gizmo_hittest.hpp"


static gfxm::ray mouseToRay(
    const gfxm::mat4& proj, const gfxm::mat4& view,
    int viewport_width, int viewport_height,
    int mouse_x, int mouse_y
) {
    gfxm::vec2 vpsz(viewport_width, viewport_height);
    return gfxm::ray_viewport_to_world(
        vpsz, gfxm::vec2(mouse_x, vpsz.y - mouse_y),
        proj, view
    );
}

static float closestPointSegmentRay(
    const gfxm::vec3& SA, const gfxm::vec3& SB,
    const gfxm::vec3& RA, const gfxm::vec3& RB,
    gfxm::vec3& SC, gfxm::vec3& RC,
    float* out_s = 0, float* out_t = 0
) {
    float s = .0f, t = .0f;

    gfxm::vec3 d1 = SB - SA;
    gfxm::vec3 d2 = RB - RA;
    gfxm::vec3 r = SA - RA;
    float a = gfxm::dot(d1, d1);
    float e = gfxm::dot(d2, d2);
    float f = gfxm::dot(d2, r);

    if (a <= FLT_EPSILON && e <= FLT_EPSILON) {
        s = t = .0f;
        SC = SA;
        RC = RA;
        return gfxm::dot(SC - RC, SC - RC);
    }
    if (a <= FLT_EPSILON) {
        s = .0f;
        t = f / e;
        t = gfxm::clamp(t, .0f, 1.f);
    } else {
        float c = gfxm::dot(d1, r);
        if (e <= FLT_EPSILON) {
            t = .0f;
            s = gfxm::clamp(-c / a, .0f, 1.f);
        } else {
            float b = gfxm::dot(d1, d2);
            float denom = a * e - b * b;
            if (denom != .0f) {
                s = gfxm::clamp((b * f - c * e) / denom, .0f, 1.f);
            } else {
                s = .0f;
            }
            t = (b * s + f) / e;
            if (t < .0f) {
                t = .0f;
                s = gfxm::clamp(-c / a, .0f, 1.f);
            } else if(t > 1.f) {
                t = 1.f;
                s = gfxm::clamp((b - c) / a, .0f, 1.f);
            }
        }
    }
    SC = SA + d1 * s;
    RC = RA + d2 * t;
    if (out_s) *out_s = s;
    if (out_t) *out_t = t;
    return gfxm::dot(SC - RC, SC - RC);
}

bool gizmoHitTranslate(
    GIZMO_TRANSFORM_STATE& state,
    int viewport_width, int viewport_height,
    int mouse_x, int mouse_y
) {
    if (state.is_active) {
        return true;
    }

    const float SHAFT_LEN = .7f;
    const float CONE_LEN = 1.f - SHAFT_LEN;

    const gfxm::mat4& proj = state.projection;
    const gfxm::mat4& view = state.view;
    const gfxm::mat4& model = state.transform;

    // Figure out scale modifier necessary to keep gizmo the same size on screen at any distance
    const float target_size = .2f; // screen ratio
    float scale = 1.f;
    {
        gfxm::vec4 ref4 = model[3];
        ref4 = proj * view * gfxm::vec4(ref4, 1.f);
        scale = target_size * ref4.w;
    }

    gfxm::ray ray = mouseToRay(
        state.projection, state.view,
        viewport_width, viewport_height,
        mouse_x, mouse_y
    );

    gfxm::vec3 SC, RC;
    float dist_x = closestPointSegmentRay(
        model[3], model[3] + gfxm::normalize(model[0]) * scale * (SHAFT_LEN + CONE_LEN),
        ray.origin, ray.origin + ray.direction * 1000.f,
        SC, RC
    );

    float dist_y = closestPointSegmentRay(
        model[3], model[3] + gfxm::normalize(model[1]) * scale * (SHAFT_LEN + CONE_LEN),
        ray.origin, ray.origin + ray.direction * 1000.f,
        SC, RC
    );

    float dist_z = closestPointSegmentRay(
        model[3], model[3] + gfxm::normalize(model[2]) * scale * (SHAFT_LEN + CONE_LEN),
        ray.origin, ray.origin + ray.direction * 1000.f,
        SC, RC
    );

    if (dist_x < dist_y && dist_x < dist_z && dist_x <= .01f * scale) {
        state.hovered_axis = 1;
        return true;
    }
    if (dist_y < dist_x && dist_y < dist_z && dist_y <= .01f * scale) {
        state.hovered_axis = 2;
        return true;
    }
    if (dist_z < dist_y && dist_z < dist_x && dist_z <= .01f * scale) {
        state.hovered_axis = 3;
        return true;
    }

    // TODO: PLANES

    state.hovered_axis = 0;
    return false;
}

static bool intersectRotator(
    const gfxm::mat4& proj, const gfxm::mat4& view, const gfxm::mat4& model,
    const gfxm::ivec2& vp_size, const gfxm::ivec2& mouse
) {
    const float THICKNESS = 6.f;
    const int N_SEGMENTS = 32;
    const float target_size = .2f; // screen ratio
    float scale = 1.f;
    {
        gfxm::vec4 ref4 = model[3];
        ref4 = proj * view * gfxm::vec4(ref4, 1.f);
        scale = target_size * ref4.w;
    }
    const float radius = 1.f * scale;

    gfxm::vec4 vertices[N_SEGMENTS];
    for(int i = 0; i < N_SEGMENTS; ++i) {
        const float a = i / float(N_SEGMENTS) * gfxm::pi * 2.f;
        gfxm::vec4 P = gfxm::vec4(gfxm::vec3(cosf(a), sinf(a), .0f) * radius, 1.f);
        P = proj * view * model * P;
        P /= P.w;
        P.x = (P.x + 1.f) * .5f;
        P.y = 1.f - (P.y + 1.f) * .5f;
        P.x *= vp_size.x;
        P.y *= vp_size.y;
        vertices[i] = P;
    }

    gfxm::vec2 P(mouse.x, mouse.y);
    for (int i = 0; i < N_SEGMENTS; ++i) {
        gfxm::vec4 A4 = vertices[i];
        gfxm::vec4 B4 = vertices[(i + 1) % N_SEGMENTS];
        gfxm::vec2 A = gfxm::vec2(A4.x, A4.y);
        gfxm::vec2 B = gfxm::vec2(B4.x, B4.y);
        gfxm::vec2 AB = B - A;
        gfxm::vec2 AP = P - A;
        float ab2 = gfxm::dot(AB, AB);
        float d = gfxm::dot(AB, AP) / ab2;
        d = gfxm::clamp(d, .0f, 1.f);
        gfxm::vec2 C = A + AB * d;
        if ((P - C).length2() < THICKNESS * THICKNESS) {
            return true;
        }
    }
    return false;
}
bool gizmoHitRotate(
    GIZMO_TRANSFORM_STATE& state,
    int viewport_width, int viewport_height,
    int mouse_x, int mouse_y
) {
    const gfxm::mat4& proj = state.projection;
    const gfxm::mat4& view = state.view;
    const gfxm::mat4& model = state.transform;
    const gfxm::ivec2 p(mouse_x, mouse_y);
    const gfxm::ivec2 vp_size(viewport_width, viewport_height);

    if (intersectRotator(
        proj, view, model * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(-90.0f), gfxm::vec3(.0f, 1.f, .0f))),
        vp_size, p)
    ) {
        state.hovered_axis = 1;
        return true;
    }
    if (intersectRotator(
        proj, view, model * gfxm::to_mat4(gfxm::angle_axis(gfxm::radian(90.0f), gfxm::vec3(1.f, .0f, .0f))),
        vp_size, p)
    ) {
        state.hovered_axis = 2;
        return true;
    }
    if (intersectRotator(
        proj, view, model,
        vp_size, p)
    ) {
        state.hovered_axis = 3;
        return true;
    }
    /*
    gfxm::ray R = getMouseRay(gfxm::vec2(x, y));
    gfxm::vec3 planeN = gfxm::normalize(gfxm::vec3(gfxm::inverse(view)[2]) - gfxm::vec3(model[3]));
    if (gfxm::intersect_line_plane_point(R.origin, R.direction, planeN, gfxm::dot(gfxm::vec3(model[3]), planeN), pt)) {
        gfxm::vec2 pt2d = gfxm::project_point_xy(gfxm::to_mat3(model), model[3], pt);
        float len = pt2d.length();
        if (len < 1.25f && len > 1.05f) {
            spin_axis_id_hovered = 4;
        }
    }*/
    state.hovered_axis = 0;
    return false;
}

