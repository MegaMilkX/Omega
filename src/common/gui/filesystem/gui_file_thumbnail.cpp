#include "gui_file_thumbnail.hpp"

#include <stdint.h>
#include <unordered_map>
#include "gpu/gpu_texture_2d.hpp"

#include <shellapi.h>
#include <ShObjIdl.h>

#include <filesystem>

enum GUI_FILE_THUMB_TYPE {
    GUI_FILE_THUMB_UNKNOWN,
    GUI_FILE_THUMB_PLATFORM,
    GUI_FILE_THUMB_ENGINE // Unused at the moment
};

struct guiFileThumbnail {
    uint64_t key;
    int ref_count;
    int width;
    int height;
    GUI_FILE_THUMB_TYPE type;
    std::shared_ptr<gpuTexture2d> texture;
};

typedef std::unordered_map<std::string, const guiFileThumbnail*>    hash_table_str;
typedef std::unordered_map<uint64_t, const guiFileThumbnail*>       hash_table_platform;

static hash_table_str thumbnails;
static hash_table_platform thumbnails_platform;


bool guiFileThumbnailInit() {
    CoInitialize(0);
    return true;
}
void guiFileThumbnailCleanup() {
    guiFileThumbnailClearCache();

    CoUninitialize();
}


const guiFileThumbnail* guiFileThumbnailGetCached(const char* path_absolute) {
    auto it = thumbnails.find(path_absolute);
    if (it == thumbnails.end()) {
        return 0;
    }
    return it->second;
}
const guiFileThumbnail* guiFileThumbnailGetCachedPlatform(uint64_t key) {
    auto it = thumbnails_platform.find(key);
    if (it == thumbnails_platform.end()) {
        return 0;
    }
    return it->second;
}


const guiFileThumbnail* guiFileThumbnailLoad(const char* path_dir, const char* path_file_rel, int size_hint) {
    std::filesystem::path path_absolute(path_dir);
    if (path_file_rel) {
        path_absolute /= path_file_rel;
    }
    path_absolute = std::filesystem::canonical(path_absolute);
    auto ptr = guiFileThumbnailGetCached(path_absolute.string().c_str());
    if (ptr) {
        const_cast<guiFileThumbnail*>(ptr)->ref_count++;
        return ptr;
    }

    HRESULT hres;
    IShellItem* dirItem = 0;
    IShellItemImageFactory* imageFactory = 0;
    if (path_file_rel != 0) {
        std::wstring wsdirname;
        wsdirname.resize(MAX_PATH);
        size_t dirlen = strnlen_s(path_dir, MAX_PATH);
        wsdirname = std::wstring(path_dir, path_dir + dirlen);

        hres = SHCreateItemFromParsingName(wsdirname.c_str(), 0, IID_PPV_ARGS(&dirItem));
        if (hres != S_OK) {
            LOG_ERR("SHCreateItemFromParsingName failed to get dir shell item: " << std::hex << hres);
            return 0;
        }

        std::wstring wsfname;
        wsfname.resize(MAX_PATH);
        size_t len = strnlen_s(path_file_rel, MAX_PATH);
        wsfname = std::wstring(path_file_rel, path_file_rel + len);
        hres = SHCreateItemFromRelativeName(dirItem, wsfname.c_str(), 0, IID_PPV_ARGS(&imageFactory));
        if (hres != S_OK) {
            LOG_ERR("SHCreateItemFromRelativeName failed: " << std::hex << hres);
            dirItem->Release();
            return 0;
        }
    } else {
        std::wstring wsfname;
        wsfname.resize(MAX_PATH);
        size_t len = strnlen_s(path_dir, MAX_PATH);
        wsfname = std::wstring(path_dir, path_dir + len);
        hres = SHCreateItemFromParsingName(wsfname.c_str(), 0, IID_PPV_ARGS(&imageFactory));
        if (hres != S_OK) {
            LOG_ERR("SHCreateItemFromParsingName failed: " << std::hex << hres);
            return 0;
        }
    }
    SIZE sz;
    sz.cx = size_hint;
    sz.cy = size_hint;
    HBITMAP hbmp = 0;
    hres = imageFactory->GetImage(sz, SIIGBF_BIGGERSIZEOK, &hbmp);
    if (hres != S_OK) {
        imageFactory->Release();
        if (dirItem) {
            dirItem->Release();
        }
        assert(false);
        return 0;
    }
    imageFactory->Release();
    if (dirItem) {
        dirItem->Release();
    }

    ptr = guiFileThumbnailGetCachedPlatform((uint64_t)hbmp);
    if (ptr) {
        thumbnails[path_absolute.string().c_str()] = ptr;
        const_cast<guiFileThumbnail*>(ptr)->ref_count++;
        return ptr;
    }
    
    HDC hdc = GetDC(0);
    BITMAPINFO bmpinfo = { 0 };
    bmpinfo.bmiHeader.biSize = sizeof(bmpinfo.bmiHeader);
    if (GetDIBits(hdc, hbmp, 0, 0, 0, &bmpinfo, DIB_RGB_COLORS) == 0) {
        assert(false);
        return 0;
    }
    std::vector<uint32_t> colors;
    int w = bmpinfo.bmiHeader.biWidth;
    int h = bmpinfo.bmiHeader.biHeight;
    colors.resize(w * h);
    bmpinfo.bmiHeader.biBitCount = 32;
    bmpinfo.bmiHeader.biCompression = BI_RGB;
    if (GetDIBits(hdc, hbmp, 0, h, colors.data(), &bmpinfo, DIB_RGB_COLORS) == 0) {
        assert(false);
        return 0;
    }

    gpuTexture2d* tex = new gpuTexture2d();
    tex->setData(colors.data(), w, h, 4, IMAGE_CHANNEL_UNSIGNED_BYTE, true);

    guiFileThumbnail* th = new guiFileThumbnail();
    th->key = (uint64_t)hbmp;
    th->ref_count = 1;
    th->width = w;
    th->height = h;
    th->type = GUI_FILE_THUMB_PLATFORM;
    th->texture.reset(tex);

    thumbnails_platform[(uint64_t)hbmp] = th;
    thumbnails[path_absolute.string().c_str()] = th;

    return th;
}
const guiFileThumbnail* guiFileThumbnailLoad(const std::string& path_absolute, int size_hint) {
    return guiFileThumbnailLoad(path_absolute.c_str(), size_hint);
}
void                    guiFileThumbnailRelease(const guiFileThumbnail* ptr) {
    if (ptr == 0) {
        return;
    }
    auto th = const_cast<guiFileThumbnail*>(ptr);
    assert(th->ref_count > 0);
    th->ref_count--;
    if (th->ref_count == 0) {
        for (auto t : thumbnails) {
            if (t.second == th) {
                thumbnails.erase(t.first);
                break;
            }
        }
        if (th->type == GUI_FILE_THUMB_PLATFORM) {
            thumbnails_platform.erase(th->key);
            delete th;
        }
    }
}

void                    guiFileThumbnailClearCache() {
    for (auto t : thumbnails) {
        if (t.second->type != GUI_FILE_THUMB_PLATFORM) {
            delete t.second;
        }
    }
    thumbnails.clear();

    for (auto t : thumbnails_platform) {
        delete t.second;
    }
    thumbnails_platform.clear();
}

#include "gui/gui_draw.hpp"
void                    guiFileThumbnailDraw(const gfxm::rect& rc, const guiFileThumbnail* ptr) {
    if (ptr->width / ptr->height == 1) {
        guiDrawRectTextured(rc, ptr->texture.get(), GUI_COL_WHITE);
    } else if(ptr->width < ptr->height) {
        gfxm::rect rc_ = rc;
        float coef = ptr->width / (float)ptr->height;
        float width_adjusted = (rc.max.x - rc.min.x) * coef;
        float x_center = rc.min.x + (rc.max.x - rc.min.x) * .5f;
        rc_.min.x = x_center - width_adjusted * .5f;
        rc_.max.x = x_center + width_adjusted * .5f;
        guiDrawRectTextured(rc_, ptr->texture.get(), GUI_COL_WHITE);
    } else if(ptr->width > ptr->height) {
        gfxm::rect rc_ = rc;
        float coef = ptr->height / (float)ptr->width;
        float height_adjusted = (rc.max.y - rc.min.y) * coef;
        float y_center = rc.min.y + (rc.max.y - rc.min.y) * .5f;
        rc_.min.y = y_center - height_adjusted * .5f;
        rc_.max.y = y_center + height_adjusted * .5f;
        guiDrawRectTextured(rc_, ptr->texture.get(), GUI_COL_WHITE);
    }
}


bool guiFileThumbnailThreadInit() {
    // TODO
    return true;
}
void guiFileThumbnailThreadCleanup() {

}