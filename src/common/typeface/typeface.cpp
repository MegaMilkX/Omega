#include "typeface.hpp"

#include <assert.h>
#include <fstream>

#include "log/log.hpp"

#include "font.hpp"


static const int DPI = 72;

struct FTContext {
    FT_Library ftlib = nullptr;
    ~FTContext() {
        if (ftlib) {
            FT_Done_FreeType(ftlib);
        }
    }
};

static int initFTContext(FTContext* ctx) {
    FT_Error err = FT_Init_FreeType(&ctx->ftlib);
    if (err) {
        LOG_ERR("FT_Init_FreeType error: " << err);
    }
    return 0;
}
static const FTContext* getFTContext() {
    static FTContext ctx;
    static int once = initFTContext(&ctx);
    return &ctx;
}

// TODO: weak_ptr here might cause unwitting and unnecessary resource recreation
//static std::unordered_map<std::string, std::weak_ptr<Typeface>> typeface_map;
static std::unordered_map<std::string, std::shared_ptr<Typeface>> typeface_map;


std::shared_ptr<Font> Typeface::getFont(int height, int dpi) {
    auto key = font_key{ height, dpi };

    auto it = font_map.find(key);
    if (it != font_map.end()) {
        /*if (auto font = it->second.lock()) {
            return font;
        }*/
        return it->second;
    }

    std::shared_ptr<Font> font(new Font(shared_from_this(), height, dpi));
    font_map[key] = font;
    return font;
}


bool typefaceLoadFromMemory(Typeface* typeface, void* buf, size_t sz) {
    //assert(s_ftlib);

    void* data = buf;

    //const int FACE_SIZE = 24; // TODO

    typeface->typeface_file_buffer = std::vector<char>((char*)buf, (char*)buf + sz);

    auto ftctx = getFTContext();
    FT_Error err = FT_New_Memory_Face(ftctx->ftlib, (FT_Byte*)typeface->typeface_file_buffer.data(), typeface->typeface_file_buffer.size(), 0, &typeface->face);
    if (err) {
        LOG_ERR("FT_New_Memory_Face error: " << err);
        return false;
    }
    err = FT_Select_Charmap(typeface->face, ft_encoding_unicode);
    //err = FT_Set_Char_Size(typeface->face, 0, FACE_SIZE * 64.0f, DPI, DPI);

    std::string postscript_name = FT_Get_Postscript_Name(typeface->face);
    LOG("Loaded typeface name: " << postscript_name);

    return true;
}

bool typefaceLoad(Typeface* typeface, const char* fname) {
    assert(typeface);
    std::ifstream file(fname, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        LOG_ERR("Failed to open typeface file: " << fname);
        return false;
    }
    std::streamsize file_size = file.tellg();

    file.seekg(0, std::ios::beg);
    std::vector<char> file_buffer = std::vector<char>((unsigned int)file_size);
    if (!file.read(file_buffer.data(), file_size)) {
        return false;
    }

    typeface->filename = fname;

    return typefaceLoadFromMemory(typeface, file_buffer.data(), file_size);
}
bool typefaceLoad(Typeface* typeface, void* data, size_t size) {
    return typefaceLoadFromMemory(typeface, data, size);
}


std::shared_ptr<Typeface> typefaceGet(const char* name) {
    auto it = typeface_map.find(name);
    if (it != typeface_map.end()) {
        return it->second;
        /*if (auto typeface = it->second.lock()) {
            return typeface;
        }*/
    }

    std::shared_ptr<Typeface> typeface(new Typeface);
    if (!typefaceLoad(typeface.get(), name)) {
        return 0;
    }
    typeface_map[name] = typeface;
    return typeface;
}