#pragma once

#include "common/render/gpu_pipeline.hpp"
#include "common/render/gpu_text.hpp"


void guiDrawRect(const gfxm::rect& rect, uint32_t col);

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
void guiDrawText(const gfxm::vec2& pos, const char* text, Font* font, float max_width, uint32_t col);


class GuiElement;
void guiDrawTitleBar(GuiElement* elem, Font* font, const char* title, const gfxm::rect& rc);