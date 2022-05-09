#pragma once

#include "common/render/gpu_pipeline.hpp"
#include "common/render/gpu_text.hpp"


void guiDrawRect(const gfxm::rect& rect, uint32_t col);

void guiDrawRectTextured(const gfxm::rect& rect, gpuTexture2d* texture, uint32_t col);

void guiDrawColorWheel(const gfxm::rect& rect);

void guiDrawRectLine(const gfxm::rect& rect, uint32_t col);

gfxm::vec2 guiCalcTextRect(const char* text, Font* font, float max_width);
void guiDrawText(const gfxm::vec2& pos, const char* text, Font* font, float max_width, uint32_t col);



void guiDrawTitleBar(Font* font, const char* title, const gfxm::rect& rc);