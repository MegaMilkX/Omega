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

bool gizmoHitRotate(
    GIZMO_TRANSFORM_STATE& state,
    int viewport_width, int viewport_height,
    int mouse_x, int mouse_y
) {
    return false;
}

