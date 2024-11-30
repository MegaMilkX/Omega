#pragma once

#include <string>


bool glxPreprocessShaderIncludes(const char* path, const char* str, size_t len, std::string& result);
