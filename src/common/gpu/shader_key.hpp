#pragma once

#include <stdint.h>
#include <string>
#include <format>
#include "gpu/types.hpp"


struct ShaderKey {
    virtual uint64_t hash() const = 0;
    virtual std::string makePrefix() const = 0;
};

struct NullShaderKey : public ShaderKey {
    uint64_t hash() const override { return 0; }
    std::string makePrefix() const override { return ""; }
};

struct PassShaderKey : public ShaderKey {
    //bool use_instancing = false;
    bool use_attrib_shaders = false;
    bool use_material_vertex = false;
    bool use_material_fragment = false;

    uint64_t hash() const override {
        uint64_t k = 0;
        //k |= uint64_t(use_instancing) << 0;
        k |= uint64_t(use_attrib_shaders) << 1;
        k |= uint64_t(use_material_vertex) << 2;
        k |= uint64_t(use_material_fragment) << 3;
        return k;
    }

    std::string makePrefix() const override {
        std::string prefix;
        //if(use_instancing) prefix += "#define ENABLE_INSTANCING\n";
        if(use_attrib_shaders) prefix += "#define ENABLE_ATTRIBS\n";
        if(use_material_vertex) prefix += "#define ENABLE_VERT_EXTENSION\n";
        if(use_material_fragment) prefix += "#define ENABLE_FRAG_EXTENSION\n";
        return prefix;
    }
};


struct TransformShaderKey : public ShaderKey {
    GPU_TransformMode mode = GPU_TransformMode::World;

    uint64_t hash() const override {
        return uint64_t(mode);
    }

    std::string makePrefix() const override {
        std::string out;
        out += std::format(
            "#define TRANSFORM_WORLD {}\n"
            "#define TRANSFORM_BILLBOARD {}\n"
            "#define TRANSFORM_BILLBOARD_Y {}\n"
            "#define TRANSFORM_MODE {}\n",
            int(GPU_TransformMode::World),
            int(GPU_TransformMode::Billboard),
            int(GPU_TransformMode::BillboardY),
            int(mode)
        );
        return out;
    }
};

