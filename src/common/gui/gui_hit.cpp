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
