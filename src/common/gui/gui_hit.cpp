#include "gui_hit.hpp"

#include "gui_draw.hpp"


bool guiHitTestRect(const gfxm::rect& rc, const gfxm::vec2& pt) {
    auto& tr = guiGetViewTransform();
    gfxm::vec3 p = gfxm::inverse(tr) * gfxm::vec4(pt.x, pt.y, .0f, 1.f);
    return gfxm::point_in_rect(rc, gfxm::vec2(p.x, p.y));
}

bool guiHitTestCircle(const gfxm::vec2& pos, float radius, const gfxm::vec2& pt) {
    auto& tr = guiGetViewTransform();
    gfxm::vec3 p = gfxm::inverse(tr) * gfxm::vec4(pt.x, pt.y, .0f, 1.f);
    return (p - gfxm::vec3(pos.x, pos.y, .0f)).length() <= radius;
}

float guiHitTestLine3d(
    const gfxm::vec3& a, const gfxm::vec3& b, 
    const gfxm::vec2& cursor, const gfxm::rect& viewport, const gfxm::mat4& transform, float& z
) {
    auto distance = [](const gfxm::vec2& A, const gfxm::vec2& B, const gfxm::vec2& C)->float {
        const gfxm::vec2 AB = B - A;
        const gfxm::vec2 BC = C - B;
        const gfxm::vec2 AC = C - A;
        float dotABBC;
        float dotABAC;
        dotABBC = (AB.x * BC.x + AB.y * BC.y);
        dotABAC = (AB.x * AC.x + AB.y * AC.y);
        float minDist = .0f;
        if (dotABBC > .0f) {
            float y = C.y - B.y;
            float x = C.x - B.x;
            minDist = gfxm::sqrt(x * x + y * y);
        } else if (dotABAC < .0f) {
            float y = C.y - A.y;
            float x = C.x - A.x;
            minDist = gfxm::sqrt(x * x + y * y);
        } else {
            float x1 = AB.x;
            float y1 = AB.y;
            float x2 = AC.x;
            float y2 = AC.y;
            float mod = gfxm::sqrt(x1 * x1 + y1 * y1);
            minDist = fabsf(x1 * y2 - y1 * x2) / mod;
        }
        return minDist;
    };
    gfxm::vec2 vpsz(viewport.max.x - viewport.min.x, viewport.max.y - viewport.min.y);
    gfxm::vec4 A4 = transform * gfxm::vec4(a, 1.f);
    gfxm::vec4 B4 = transform * gfxm::vec4(b, 1.f);
    A4 = gfxm::vec4(A4.x / A4.w, A4.y / A4.w, A4.z, A4.w);
    B4 = gfxm::vec4(B4.x / B4.w, B4.y / B4.w, B4.z, B4.w);
    gfxm::vec2 A2 = gfxm::vec2(A4.x + 1.f, A4.y + 1.f) * .5f * vpsz;
    gfxm::vec2 B2 = gfxm::vec2(B4.x + 1.f, B4.y + 1.f) * .5f * vpsz;
    gfxm::vec2 C(cursor.x, vpsz.y - cursor.y);
    float d = gfxm::dot(gfxm::normalize(B2 - A2), gfxm::normalize(C - A2));
    float dist = distance(A2, B2, C);
    z = gfxm::lerp(A4.z, B4.z, d);
    return dist;
}