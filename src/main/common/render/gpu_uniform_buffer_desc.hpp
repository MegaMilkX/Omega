#pragma once

#include <assert.h>
#include <vector>
#include <map>
#include "common/math/gfxm.hpp"

enum UNIFORM_TYPE {
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
        int offset;
    };

    int id;
    std::string block_name;
    int buffer_size;
    std::vector<Uniform> uniforms;
    std::map<std::string, int> uniform_names;

    gpuUniformBufferDesc& name(const char* name) {
        this->block_name = name;
        return *this;
    }
    gpuUniformBufferDesc& define(const char* name, UNIFORM_TYPE type) {
        uniform_names[name] = uniforms.size();
        uniforms.push_back(Uniform{ type, 0 });
        return *this;
    }
    void compile() {
        assert(!block_name.empty());
        buffer_size = 0;
        if (uniforms.empty()) {
            assert(false);
            return;
        }

        int prev_member_offset = 0;
        int prev_member_size = 0;
        for (int i = 0; i < uniforms.size(); ++i) {
            auto& u = uniforms[i];
            int sz = UNIFORM_SIZES[u.type * 2];
            int base_alignment = UNIFORM_SIZES[u.type * 2 + 1];

            int offset = ((prev_member_offset + prev_member_size) + (base_alignment - 1)) & ~(base_alignment - 1);
            u.offset = offset;

            buffer_size = offset + sz;

            prev_member_offset = offset;
            prev_member_size = sz;
        }
        
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
        assert(i >= 0 && i < uniforms.size());
        return uniforms[i].offset;
    }
};