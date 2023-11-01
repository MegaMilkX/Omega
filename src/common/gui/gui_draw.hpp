#pragma once

#include "gui/gui_text_buffer.hpp"
#include "gpu/gpu_pipeline.hpp"
#include "gpu/gpu_text.hpp"

constexpr uint8_t GUI_DRAW_CORNER_NW = 0b0001;
constexpr uint8_t GUI_DRAW_CORNER_NE = 0b0010;
constexpr uint8_t GUI_DRAW_CORNER_SE = 0b0100;
constexpr uint8_t GUI_DRAW_CORNER_SW = 0b1000;
constexpr uint8_t GUI_DRAW_CORNER_LEFT = 0b1001;
constexpr uint8_t GUI_DRAW_CORNER_RIGHT = 0b0110;
constexpr uint8_t GUI_DRAW_CORNER_TOP = 0b0011;
constexpr uint8_t GUI_DRAW_CORNER_BOTTOM = 0b1100;
constexpr uint8_t GUI_DRAW_CORNER_ALL = 0b1111;

enum GUI_DRAW_CMD {
    GUI_DRAW_LINE_STRIP,
    GUI_DRAW_LINES,
    GUI_DRAW_TRIANGLE_STRIP,
    GUI_DRAW_TRIANGLE_FAN,
    GUI_DRAW_TRIANGLES,
    GUI_DRAW_TRIANGLES_INDEXED,
    GUI_DRAW_TEXT,
    GUI_DRAW_TEXT_HIGHLIGHT
};
struct GuiDrawCmd {
    GUI_DRAW_CMD cmd;
    gfxm::mat4 projection;
    gfxm::mat4 view_transform;
    gfxm::mat4 model_transform;
    gfxm::rect viewport_rect;
    gfxm::rect scissor_rect;
    int vertex_count;
    int vertex_first;
    int index_count;
    int index_first;
    uint32_t color;
    GLuint tex0;
    GLuint tex1;
    float usr0;
};

GuiDrawCmd& guiDrawTextHighlight(
    const gfxm::vec3* vertices,
    int vertex_count,
    uint32_t* indices,
    int index_count,
    uint32_t color
);
GuiDrawCmd& _guiDrawText(
    const gfxm::vec3* vertices,
    const gfxm::vec2* uv,
    const uint32_t* colors,
    const float* uv_lookup,
    int vertex_count,
    uint32_t* indices,
    int index_count,
    uint32_t color,
    GLuint atlas,
    GLuint lut,
    int lut_width
);

void                guiPushTransform(const gfxm::vec3& pos);
void                guiPushTransform(const gfxm::mat4& tr);
void                guiPopTransform();
const gfxm::mat4&   guiGetCurrentTransform();

void                guiPushViewTransform(const gfxm::mat4& tr);
void                guiPopViewTransform();
void                guiClearViewTransform();
const gfxm::mat4&   guiGetViewTransform();

void                guiPushProjection(const gfxm::mat4& p);
void                guiPushProjectionOrthographic(float left, float right, float bottom, float top);
void                guiPushProjectionPerspective(float fov_deg, float width, float height, float znear, float zfar);
void                guiPopProjection();
void                guiClearProjection();
const gfxm::mat4&   guiGetCurrentProjection();
void                guiSetDefaultProjection(const gfxm::mat4& p);
const gfxm::mat4&   guiGetDefaultProjection();

void guiPushViewportRect(const gfxm::rect& rc);
void guiPushViewportRect(float minx, float miny, float maxx, float maxy);
void guiPopViewportRect();
void guiClearViewportRectStack();
const gfxm::rect& guiGetCurrentViewportRect();
void guiSetDefaultViewportRect(const gfxm::rect& rc);
const gfxm::rect& guiGetDefaultViewportRect();

void guiDrawPushScissorRect(const gfxm::rect& rect);
void guiDrawPushScissorRect(float minx, float miny, float maxx, float maxy);
void guiDrawPopScissorRect();
const gfxm::rect& guiDrawGetCurrentScissor();

GuiDrawCmd& guiDrawTriangleStrip(
    const gfxm::vec3* vertices,
    int vertex_count,
    uint32_t color
);
GuiDrawCmd& guiDrawTrianglesIndexed(
    const gfxm::vec3* vertices,
    int vertex_count,
    uint32_t* indices,
    int index_count,
    uint32_t color
);

void guiDrawBezierCurve(const gfxm::vec2& a, const gfxm::vec2& b, const gfxm::vec2& c, const gfxm::vec2& d, float thickness, uint32_t col = GUI_COL_WHITE, const gfxm::vec2& zoom_factor = gfxm::vec2(1.f, 1.f));
void guiDrawCurveSimple(const gfxm::vec2& from, const gfxm::vec2& to, float thickness, uint32_t col = GUI_COL_WHITE);
void guiDrawLineWithArrow(const gfxm::vec2& from, const gfxm::vec2& to, float thickness, uint32_t col = GUI_COL_WHITE);
void guiDrawCircle(const gfxm::vec2& pos, float radius, bool is_filled = true, uint32_t col = GUI_COL_WHITE);
void guiDrawDiamond(const gfxm::vec2& POS, float radius, uint32_t col0, const gfxm::vec2& zoom_factor = gfxm::vec2(1.f, 1.f));
void guiDrawDiamond(const gfxm::vec2& POS, float radius, uint32_t col0, uint32_t col1, uint32_t col2, const gfxm::vec2& zoom_factor = gfxm::vec2(1.f, 1.f));
void guiDrawRectShadow(const gfxm::rect& rc, uint32_t col = 0x77000000);
void guiDrawRect(const gfxm::rect& rect, uint32_t col);
void guiDrawRectGradient(const gfxm::rect& rect, uint32_t col_lt, uint32_t col_rt, uint32_t col_lb, uint32_t col_rb);
void guiDrawRectRound(const gfxm::rect& rc, float radius, uint32_t col = GUI_COL_WHITE, uint8_t corner_flags = GUI_DRAW_CORNER_ALL);
void guiDrawRectRound(
    const gfxm::rect& rc, 
    float radius_nw, float radius_ne, float radius_sw, float radius_se,
    uint32_t col = GUI_COL_WHITE, uint8_t corner_flags = GUI_DRAW_CORNER_ALL
);
void guiDrawRectRoundBorder(const gfxm::rect& rc, float radius, float thickness, uint32_t col_a, uint32_t col_b, uint8_t corner_flags = GUI_DRAW_CORNER_ALL);
void guiDrawRectRoundBorder(
    const gfxm::rect& rc_,
    float radius_top_left, float radius_top_right,
    float radius_bottom_left, float radius_bottom_right,
    float thickness_left, float thickness_top, float thickness_right, float thickness_bottom,
    uint32_t col_left, uint32_t col_top, uint32_t col_right, uint32_t col_bottom, uint8_t corner_flags = GUI_DRAW_CORNER_ALL
);
void guiDrawRectTextured(const gfxm::rect& rect, gpuTexture2d* texture, uint32_t col);

void guiDrawColorWheel(const gfxm::rect& rect);
void guiDrawCheckBox(const gfxm::rect& rc, bool is_checked, bool is_hovered);

void guiDrawRectLine(const gfxm::rect& rect, uint32_t col);

void guiDrawLine(const gfxm::vec2& a, const gfxm::vec2& b, float thickness, uint32_t col, const gfxm::vec2& zoom_factor = gfxm::vec2(1.f, 1.f));
void guiDrawLine(const gfxm::rect& rc, float thickness, uint32_t col, const gfxm::vec2& zoom_factor = gfxm::vec2(1.f, 1.f));
GuiDrawCmd& guiDrawLine3(const gfxm::vec3& a, const gfxm::vec3& b, uint32_t col);
GuiDrawCmd& guiDrawLine3d2(const gfxm::vec3& a, const gfxm::vec3& b, uint32_t col0, uint32_t col1);
GuiDrawCmd& guiDrawCircle3(float radius, uint32_t col);
GuiDrawCmd& guiDrawAABB(const gfxm::aabb& aabb, const gfxm::mat4& transform, uint32_t col);
GuiDrawCmd& guiDrawCone(float radius, float height, uint32_t color);
GuiDrawCmd& guiDrawQuad3d(const gfxm::vec3& a, const gfxm::vec3& b, const gfxm::vec3& c, const gfxm::vec3& d, uint32_t col);
GuiDrawCmd& guiDrawPointSquare3d(const gfxm::vec3& pt, float side, uint32_t col);
GuiDrawCmd& guiDrawPolyConvex3d(const gfxm::vec3* vertices, size_t count, uint32_t col);

gfxm::vec2 guiCalcTextPosInRect(const gfxm::rect& rc_text, const gfxm::rect& rc, int alignment, const gfxm::rect& margin, Font* font);
void guiDrawText(const gfxm::vec2& pos, const char* text, Font* font, float max_width, uint32_t col, const gfxm::vec2& zoom_factor = gfxm::vec2(1.f, 1.f));
void guiDrawText(const gfxm::rect& rc, const char* text, Font* font, GUI_ALIGNMENT align, uint32_t col);

class GuiElement;

void guiRender(bool clear = true);
void guiRenderToCurrentFramebuffer(int screen_w, int screen_h);