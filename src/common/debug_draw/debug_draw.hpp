#pragma once

#include <string>
#include "math/gfxm.hpp"

constexpr uint32_t DBG_COLOR_WHITE = 0xFFFFFFFF;
constexpr uint32_t DBG_COLOR_BLACK = 0xFF000000;
constexpr uint32_t DBG_COLOR_RED = 0xFF0000FF;
constexpr uint32_t DBG_COLOR_GREEN = 0xFF00FF00;
constexpr uint32_t DBG_COLOR_BLUE = 0xFFFF0000;


void dbgDrawClearBuffers();
void dbgDrawDraw(const gfxm::mat4& projection, const gfxm::mat4& view, int vp_x, int vp_y, int vp_w, int vp_h);

void dbgDrawText(const gfxm::vec3& at, const std::string& text, uint32_t color = DBG_COLOR_WHITE, float time = .0f);
void dbgDrawLine(const gfxm::vec3& from, const gfxm::vec3& to, uint32_t color, float time = .0f);
void dbgDrawCross(const gfxm::vec3& pos, float radius, uint32_t color, float time = .0f);
void dbgDrawCross(const gfxm::mat4& tr, float radius, uint32_t color, float time = .0f);
void dbgDrawRay(const gfxm::vec3& origin, const gfxm::vec3& dir, uint32_t color, float time = .0f);
void dbgDrawArrow(const gfxm::vec3& origin, const gfxm::vec3& dir, uint32_t color, float time = .0f);
void dbgDrawSphere(const gfxm::vec3& pos, float radius, uint32_t color, float time = .0f);
void dbgDrawSphere(const gfxm::mat4& tr, float radius, uint32_t color, float time = .0f);
void dbgDrawDome(const gfxm::vec3& pos, float radius, uint32_t color, float time = .0f);
void dbgDrawDome(const gfxm::mat4& tr, float radius, uint32_t color, float time = .0f);
void dbgDrawCapsule(const gfxm::vec3& pos, float height, float radius, uint32_t color, float time = .0f);
void dbgDrawCapsule(const gfxm::mat4& tr, float height, float radius, uint32_t color, float time = .0f);
void dbgDrawBox(const gfxm::mat4& transform, const gfxm::vec3& half_extents, uint32_t color, float time = .0f);
void dbgDrawAabb(const gfxm::aabb& aabb, uint32_t color, float time = .0f);
void dbgDrawFrustum(const gfxm::frustum& frust, uint32_t color, float time = .0f);
void dbgDrawPortal(const gfxm::mat4& transform, const gfxm::vec2& half_extents, uint32_t color, float time = .0f);