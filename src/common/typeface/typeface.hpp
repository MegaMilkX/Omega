#pragma once

#include <stdint.h>
#include <unordered_map>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "image/image.hpp"


class Typeface {
public:
    std::vector<char> typeface_file_buffer;
    FT_Face face;
public:
};


bool typefaceInit();
void typefaceCleanup();

bool typefaceLoad(Typeface* typeface, const char* fname);


Typeface* typefaceGet(const char* name);