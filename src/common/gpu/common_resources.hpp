#pragma once

#include "platform/gl/glextutil.h"
#include "gpu/gpu_texture_2d.hpp"
#include "gpu/gpu_shader_program.hpp"
#include "handle/hshared.hpp"
#include "math/gfxm.hpp"


extern GLuint vao_screen_triangle;
extern GLuint vao_inverted_cube;
extern GLuint tex_brdf_lut;

enum UNIFORM_BUFFER_SLOT {
    UNIFORM_BUFFER_SLOT_COMMON,
    UNIFORM_BUFFER_SLOT_MODEL
};

struct UniformBufferCommon {
    gfxm::mat4 matProjection;
    gfxm::mat4 matView;
    gfxm::vec3 cameraPosition;
    float time;
    gfxm::vec2 viewportSize;
    float zNear;
    float zFar;
};
static_assert(
    sizeof(UniformBufferCommon)
    == sizeof(gfxm::mat4)
    + sizeof(gfxm::mat4)
    + sizeof(gfxm::vec3)
    + sizeof(float)
    + sizeof(gfxm::vec2)
    + sizeof(float)
    + sizeof(float),
    "UniformBufferCommon misaligned"
);

struct UniformBufferModel {
    gfxm::mat4 matModel;
};
static_assert(
    sizeof(UniformBufferModel)
    == sizeof(gfxm::mat4),
    "UniformBufferModel misaligned"
);


bool initCommonResources();
void cleanupCommonResources();

RHSHARED<gpuTexture2d> getDefaultTexture(const char* name);
RHSHARED<gpuShaderProgram> getWireframeProgram();

