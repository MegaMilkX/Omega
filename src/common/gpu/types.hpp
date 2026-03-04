#pragma once

#include <stdint.h>
#include <string>
#include <map>


#define GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS 8

typedef uint8_t draw_flags_t;
constexpr draw_flags_t GPU_DEPTH_TEST         = 0x01;
constexpr draw_flags_t GPU_DEPTH_WRITE        = 0x02;
constexpr draw_flags_t GPU_BACKFACE_CULLING   = 0x04;
constexpr draw_flags_t GPU_STENCIL_TEST       = 0x08;

enum class GPU_BLEND_MODE {
    INVALID = -1,
    BLEND,
    ADD,
    MULTIPLY,
    OVERWRITE
};

enum RT_OUTPUT {
    RT_OUTPUT_AUTO,
    RT_OUTPUT_RGB,
    RT_OUTPUT_RRR,
    RT_OUTPUT_GGG,
    RT_OUTPUT_BBB,
    RT_OUTPUT_AAA,
    RT_OUTPUT_DEPTH
};

enum MESH_DRAW_MODE {
    MESH_DRAW_POINTS,
    MESH_DRAW_LINES,
    MESH_DRAW_LINE_STRIP,
    MESH_DRAW_LINE_LOOP,
    MESH_DRAW_TRIANGLES,
    MESH_DRAW_TRIANGLE_STRIP,
    MESH_DRAW_TRIANGLE_FAN
};

enum SHADER_TYPE {
    SHADER_UNKNOWN,
    SHADER_VERTEX,
    SHADER_FRAGMENT,
    SHADER_GEOMETRY
};
inline const char* gpuShaderTypeToString(SHADER_TYPE t) {
    switch (t) {
    case SHADER_UNKNOWN: return "UNKNOWN";
    case SHADER_VERTEX: return "VERTEX";
    case SHADER_FRAGMENT: return "FRAGMENT";
    case SHADER_GEOMETRY: return "GEOMETRY";
    };
    return "";
}

typedef uint64_t shader_flags_t;
const shader_flags_t SHADER_FLAG_ENABLE_VERT_EXTENSION = 0x0001;
const shader_flags_t SHADER_FLAG_ENABLE_FRAG_EXTENSION = 0x0002;
const shader_flags_t SHADER_FLAG_ALPHA_TRANSPARENCY    = 0x0004;
const shader_flags_t SHADER_FLAG_ALPHA_ROUGHNESS       = 0x0008;
const shader_flags_t SHADER_FLAG_ALPHA_EMISSION        = 0x0010;
const shader_flags_t SHADER_FLAG_ENABLE_PARALLAX       = 0x0020;

enum GPU_SHADER_DIRECTIVE {
    GPU_SHADER_DIRECTIVE_FIRST = 0,
    GPU_SHADER_ENABLE_VERT_EXTENSION = GPU_SHADER_DIRECTIVE_FIRST,
    GPU_SHADER_ENABLE_FRAG_EXTENSION,
    GPU_SHADER_ALPHA_TRANSPARENCY,
    GPU_SHADER_ALPHA_ROUGHNESS,
    GPU_SHADER_ALPHA_EMISSION,
    GPU_SHADER_ENABLE_PARALLAX,
    GPU_SHADER_DIRECTIVE_COUNT,
};
inline GPU_SHADER_DIRECTIVE& operator++(GPU_SHADER_DIRECTIVE& dir) {
    dir = static_cast<GPU_SHADER_DIRECTIVE>(static_cast<int>(dir) + 1);
    return dir;
}
inline const char* gpuShaderDirectiveToString(GPU_SHADER_DIRECTIVE dir) {
    switch (dir) {
    case GPU_SHADER_ENABLE_VERT_EXTENSION: return "ENABLE_VERT_EXTENSION";
    case GPU_SHADER_ENABLE_FRAG_EXTENSION: return "ENABLE_FRAG_EXTENSION";
    case GPU_SHADER_ALPHA_TRANSPARENCY: return "ALPHA_TRANSPARENCY";
    case GPU_SHADER_ALPHA_ROUGHNESS: return "ALPHA_ROUGHNESS";
    case GPU_SHADER_ALPHA_EMISSION: return "ALPHA_EMISSION";
    case GPU_SHADER_ENABLE_PARALLAX: return "ENABLE_PARALLAX";
    };
    return "";
}

enum GPU_Role {
    GPU_Role_None,
    GPU_Role_Geometry,
    GPU_Role_Decal,
    GPU_Role_Water,
};
inline const char* gpuRoleToString(GPU_Role t) {
    switch (t) {
    case GPU_Role_None: return "none";
    case GPU_Role_Geometry: return "geometry";
    case GPU_Role_Decal: return "decal";
    case GPU_Role_Water: return "water";
    }
    return "UNKNOWN";
}
inline GPU_Role gpuStringToRole(const std::string& str) {
    static const std::map<std::string, GPU_Role> map = {
        { "none", GPU_Role_None },
        { "geometry", GPU_Role_Geometry },
        { "decal", GPU_Role_Decal },
        { "water", GPU_Role_Water },
    };
    auto it = map.find(str);
    if (it == map.end()) {
        return GPU_Role_None;
    }
    return it->second;
}

enum GPU_Effect {
    GPU_Effect_Outline,
    GPU_EFFECT_COUNT
};
inline const char* gpuEffectToString(GPU_Effect e) {
    switch (e) {
    case GPU_Effect_Outline: return "outline";
    }
    return "UNKNOWN";
}

