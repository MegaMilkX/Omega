#pragma once

#include <memory>
#include "typeface/font.hpp"
#include "gpu/gpu_texture_2d.hpp"



void guiFontInit(const std::shared_ptr<Font>& default_font);
void guiFontCleanup();
//void guiPushFont(Font* font);
//void guiPopFont();
//Font* guiGetCurrentFont();
Font* guiGetDefaultFont();
