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

void                guiPushViewTransform(const gfxm::mat4& tr);
void                guiPopViewTransform();
void                guiClearViewTransform();
const gfxm::mat4&   guiGetViewTransform();

void guiDrawPushScissorRect(const gfxm::rect& rect);
void guiDrawPushScissorRect(float minx, float miny, float maxx, float maxy);
void guiDrawPopScissorRect();

void guiDrawCurveSimple(const gfxm::vec2& from, const gfxm::vec2& to, float thickness, uint32_t col = GUI_COL_WHITE);
void guiDrawCircle(const gfxm::vec2& pos, float radius, bool is_filled = true, uint32_t col = GUI_COL_WHITE);
void guiDrawRectShadow(const gfxm::rect& rc, uint32_t col = 0xAA000000);
void guiDrawRect(const gfxm::rect& rect, uint32_t col);
void guiDrawRectRound(const gfxm::rect& rc, float radius, uint32_t col = GUI_COL_WHITE, uint8_t corner_flags = GUI_DRAW_CORNER_ALL);
void guiDrawRectRoundBorder(const gfxm::rect& rc, float radius, float thickness, uint32_t col_a, uint32_t col_b, uint8_t corner_flags = GUI_DRAW_CORNER_ALL);
void guiDrawRectTextured(const gfxm::rect& rect, gpuTexture2d* texture, uint32_t col);

void guiDrawColorWheel(const gfxm::rect& rect);

void guiDrawRectLine(const gfxm::rect& rect, uint32_t col);

void guiDrawLine(const gfxm::rect& rc, uint32_t col);

gfxm::vec2 guiCalcTextRect(const char* text, Font* font, float max_width);
const int GUI_ALIGN_LEFT    = 0x0000;
const int GUI_ALIGN_HMID    = 0x1000;
const int GUI_ALIGN_RIGHT   = 0x0100;
const int GUI_ALIGN_TOP     = 0x0000;
const int GUI_ALIGN_VMID    = 0x0010;
const int GUI_ALIGN_BOTTOM  = 0x0001;
gfxm::vec2 guiCalcTextPosInRect(const gfxm::rect& rc_text, const gfxm::rect& rc, int alignment, const gfxm::rect& margin, Font* font);
void guiDrawText(const gfxm::vec2& pos, const char* text, GuiFont* font, float max_width, uint32_t col);

class GuiElement;
// NOTE: Do not use! too slow
void guiDrawTitleBar(GuiElement* elem, GuiTextBuffer* buf, const gfxm::rect& rc);