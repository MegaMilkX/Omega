#pragma once

#include <string>
#include <vector>

struct GLX_PP_DEFINITION {
    std::string identifier;
    std::string value;
};

struct GLX_PP_CONTEXT {
    const char** include_paths = 0;
    int n_include_paths;
    std::vector<GLX_PP_DEFINITION> definitions;

    void define(const char* identifier, const char* value) {
        definitions.push_back(GLX_PP_DEFINITION{ identifier, value });
    }
};

bool glxPreprocessShaderIncludes(const GLX_PP_CONTEXT* ctx, const char* str, size_t len, std::string& result, int first_line = 1);

