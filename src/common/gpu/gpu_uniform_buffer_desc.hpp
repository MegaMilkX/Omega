#pragma once

#include <assert.h>
#include <vector>
#include <map>
#include <string>
#include "math/gfxm.hpp"


enum UNIFORM_TYPE {
    UNIFORM_UNKNOWN,

    UNIFORM_BOOL,
    UNIFORM_INT,
    UNIFORM_UINT,
    UNIFORM_FLOAT,
    UNIFORM_DOUBLE,

    UNIFORM_BVEC2,
    UNIFORM_BVEC3,
    UNIFORM_BVEC4,
    UNIFORM_IVEC2,
    UNIFORM_IVEC3,
    UNIFORM_IVEC4,
    UNIFORM_UVEC2,
    UNIFORM_UVEC3,
    UNIFORM_UVEC4,
    UNIFORM_VEC2,
    UNIFORM_VEC3,
    UNIFORM_VEC4,
    UNIFORM_DVEC2,
    UNIFORM_DVEC3,
    UNIFORM_DVEC4,

    UNIFORM_MAT2,
    UNIFORM_MAT2X3,
    UNIFORM_MAT2X4,
    UNIFORM_MAT3,
    UNIFORM_MAT3X2,
    UNIFORM_MAT3X4,
    UNIFORM_MAT4,
    UNIFORM_MAT4X2,
    UNIFORM_MAT4X3
};
constexpr int UNIFORM_SIZES[] = {
    // SIZE, BASE_ALIGNMENT
    0, 0, // UNKNOWN
    4, 4, // BOOL
    4, 4, // INT
    4, 4, // UINT
    4, 4, // FLOAT
    8, 8, // DOUBLE

    8, 8, //UNIFORM_BVEC2
    12, 16, //UNIFORM_BVEC3
    16, 16, //UNIFORM_BVEC4
    8, 8, //UNIFORM_IVEC2
    12, 16, //UNIFORM_IVEC3
    16, 16, //UNIFORM_IVEC4
    8, 8, //UNIFORM_UVEC2
    12, 16, //UNIFORM_UVEC3
    16, 16, //UNIFORM_UVEC4
    8, 8, //UNIFORM_VEC2
    12, 16, //UNIFORM_VEC3
    16, 16, //UNIFORM_VEC4
    16, 16, //UNIFORM_DVEC2
    24, 32, //UNIFORM_DVEC3
    32, 32, //UNIFORM_DVEC4

    16 * 2, 16, //UNIFORM_MAT2
    16 * 3, 16, //UNIFORM_MAT2X3
    16 * 4, 16, //UNIFORM_MAT2X4
    16 * 3, 16, //UNIFORM_MAT3
    16 * 2, 16, //UNIFORM_MAT3X2
    16 * 4, 16, //UNIFORM_MAT3X4
    16 * 4, 16, //UNIFORM_MAT4
    16 * 2, 16, //UNIFORM_MAT4X2
    16 * 3, 16  //UNIFORM_MAT4X3
};


struct gpuUniformBufferDesc {
    struct Uniform {
        UNIFORM_TYPE type;
        int count;
        int offset;
        int stride;
    };

    int binding_location = 0;
    std::string block_name;
    int buffer_size;
    std::vector<Uniform> uniforms;
    std::map<std::string, int> uniform_names;

    gpuUniformBufferDesc(const char* name);

    gpuUniformBufferDesc& define(const char* name, UNIFORM_TYPE type, int count = 1) {
        uniform_names[name] = uniforms.size();
        uniforms.push_back(Uniform{ type, count, 0, 0 });
        return *this;
    }
    void compile() {
        assert(!block_name.empty());
        buffer_size = 0;
        if (uniforms.empty()) {
            assert(false);
            return;
        }
        
        auto roundUp = [](int v, int a) { return (v + (a - 1)) & ~(a - 1); };

        int end = 0;
        for (int i = 0; i < int(uniforms.size()); ++i) {
            auto& u = uniforms[i];
            int elem_sz = UNIFORM_SIZES[u.type * 2];
            int base_align = UNIFORM_SIZES[u.type * 2 + 1];

            int total_size = 0;
            if (u.count > 1) {
                base_align = roundUp(base_align, 16);
                u.stride = roundUp(elem_sz, 16);
                total_size = u.stride * u.count;
            } else {
                u.stride = 0;
                total_size = elem_sz;
            }

            u.offset = roundUp(end, base_align);
            end = u.offset + total_size;
        }
        buffer_size = roundUp(end, 16);
    }

    const char* getName() const {
        return block_name.c_str();
    }

    int uniformCount() const {
        return uniforms.size();
    }
    int getUniform(const char* name) const {
        auto it = uniform_names.find(name);
        if (it == uniform_names.end()) {
            return -1;
        }
        return it->second;
    }
    const char* getUniformName(int i) const {
        for (auto& it : uniform_names) {
            if (it.second == i) {
                return it.first.c_str();
            }
        }
        assert(false);
        return 0;
    }
    int getUniformByteOffset(int i) const {
        assert(i >= 0 && i < int(uniforms.size()));
        return uniforms[i].offset;
    }
    int getUniformStride(int i) const {
        assert(i >= 0 && i < int(uniforms.size()));
        return uniforms[i].stride;
    }
    int getUniformCount(int i) const {
        assert(i >= 0 && i < int(uniforms.size()));
        return uniforms[i].count;
    }
};