#pragma once

#include "types.auto.hpp"
#include <stdint.h>
#include <string>
#include <map>
#include "math/gfxm.hpp"


#define GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS 8

enum class GPU_MESH_DESC_TYPE {
    NONE = 0,
    GENERIC,
    DECAL,
    SPRITE,
    TEXT,

    COUNT
};

typedef uint8_t draw_flags_t;
constexpr draw_flags_t GPU_DEPTH_TEST         = 0x01;
constexpr draw_flags_t GPU_DEPTH_WRITE        = 0x02;
constexpr draw_flags_t GPU_BACKFACE_CULLING   = 0x04;
constexpr draw_flags_t GPU_STENCIL_TEST       = 0x08;

[[cppi_enum]];
enum class GPU_TransformMode {
    World,
    Billboard,
    BillboardY
};

[[cppi_enum]];
enum class GPU_AlphaMode {
    Opaque = 0,
    Blend = 1,
    Discard = 2,

    COUNT
};

[[cppi_enum]];
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

struct DRAW_PARAMS {
    gfxm::mat4 view = gfxm::mat4(1.f);
    gfxm::mat4 view_prev = gfxm::mat4(1.f);
    gfxm::mat4 projection = gfxm::mat4(1.f);
    gfxm::rect vp_rect_ratio;
    int viewport_x = 0;
    int viewport_y = 0;
    int viewport_width = 0;
    int viewport_height = 0;
    int layer = -1; // -1 means all layers
    float time = .0f;
};

enum class GPU_SORT_MODE {
    NONE,
    STATE_CHANGE,
    BACK_TO_FRONT,
    FRONT_TO_BACK,
};

