#ifndef UTIL_FILESYSTEM_H
#define UTIL_FILESYSTEM_H

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iterator>
#include <cctype>

class fs_path {
    std::vector<std::string> stack;
    std::string str;
public:
    fs_path();
    fs_path(const char* path);
    fs_path(const std::string& path);

    // Get path relative to this (other must be absolute)
    fs_path relative(const fs_path& other) const;

    const std::string& string() const;
    const char*        c_str() const;

};

std::string fsGetExtension(const std::string& path);

std::string fsReplaceReservedChars(const std::string& path, char r);

std::string fsCutDirPath(const std::string& path);

std::string fsGetModulePath();
std::string fsGetModuleDir();

bool fsSlurpFile(const std::string& path, std::vector<uint8_t>& data);

bool fsFileCopy(const std::string& from, const std::string& to);

void                        fsFindFiles(const std::string& dir, const std::string& filter, std::vector<std::string>& out);
std::vector<std::string>    fsFindAllFiles(const std::string& dir, const std::string& filter);

void fsCreateDirRecursive(const std::string& p);

std::string fsMakeRelativePath(const std::string& root, const std::string& path);
fs_path fsMakeRelativePath(const fs_path& root, const fs_path& path);

fs_path fsGetCurrentDirectory();

#endif
