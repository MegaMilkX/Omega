#pragma once

#include "math/gfxm.hpp"


bool guiHitTestRect(const gfxm::rect& rc, const gfxm::vec2& pt);
bool guiHitTestCircle(const gfxm::vec2& pos, float radius, const gfxm::vec2& pt);
float guiHitTestLine3d(const gfxm::vec3& a, const gfxm::vec3& b, const gfxm::vec2& cursor, const gfxm::rect& viewport, const gfxm::mat4& transform, float& z);
