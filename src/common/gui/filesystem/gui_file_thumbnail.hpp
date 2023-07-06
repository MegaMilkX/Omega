#pragma once

#include <string>
#include "math/gfxm.hpp"


struct guiFileThumbnail;

bool guiFileThumbnailInit();
void guiFileThumbnailCleanup();


const guiFileThumbnail* guiFileThumbnailLoad(const char* path_dir, const char* path_file_rel, int size_hint = 64);
const guiFileThumbnail* guiFileThumbnailLoad(const std::string& path_absolute, int size_hint = 64);
void                    guiFileThumbnailRelease(const guiFileThumbnail* ptr);

void                    guiFileThumbnailClearCache();

void                    guiFileThumbnailDraw(const gfxm::rect& rc, const guiFileThumbnail* ptr);

bool guiFileThumbnailThreadInit();
void guiFileThumbnailThreadCleanup();