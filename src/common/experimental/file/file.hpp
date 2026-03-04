#pragma once

#include <stdint.h>


struct HFILE;

enum FILE_SEEK {
    FILE_SEEK_SET,
    FILE_SEEK_CUR,
    FILE_SEEK_END
};

HFILE*  fileOpen(const char* path, const char* options);
HFILE*  fileOpenMemory(const void* data, uint64_t size);
HFILE*  fileOpenSubFile(HFILE* file, uint64_t size);

void    fileClose(HFILE* file);

void    fileSeek(int64_t offset, FILE_SEEK seek_type);

void    fileRead(void* dst, uint64_t size);

void    fileGetSize(const char* path);
void    fileGetSize(HFILE* file);

// TODO:
/*

void    fileSlurp()

*/