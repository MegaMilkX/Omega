#pragma once

#include <string>

struct GLX_PP_CONTEXT {
    const char** include_paths = 0;
    int n_include_paths;
};

bool glxPreprocessShaderIncludes(const GLX_PP_CONTEXT* ctx, const char* str, size_t len, std::string& result);
