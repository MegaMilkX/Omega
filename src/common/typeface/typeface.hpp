#pragma once

#include <stdint.h>
#include <memory>
#include <unordered_map>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "image/image.hpp"


struct font_key {
    int height;
    int dpi;
    bool operator==(const font_key& other) const {
        return height == other.height && dpi == other.dpi;
    }
};
template<>
struct std::hash<font_key> {
    std::size_t operator()(const font_key& k) const {
        return (std::hash<int>()(k.height) << 4)
            ^ std::hash<int>()(k.dpi);
    }
};

class Font;
class Typeface : public std::enable_shared_from_this<Typeface> {
public:
    std::vector<char> typeface_file_buffer;
    FT_Face face;

    std::unordered_map<font_key, std::weak_ptr<Font>> font_map;
public:
    std::shared_ptr<Font> getFont(int height, int dpi);
};


bool typefaceInit();
void typefaceCleanup();

bool typefaceLoad(Typeface* typeface, const char* fname);
bool typefaceLoad(Typeface* typeface, void* data, size_t size);


std::shared_ptr<Typeface> typefaceGet(const char* name);