#pragma once

#include "math/gfxm.hpp"


bool guiHitTestRect(const gfxm::rect& rc, const gfxm::vec2& pt);
bool guiHitTestCircle(const gfxm::vec2& pos, float radius, const gfxm::vec2& pt);
