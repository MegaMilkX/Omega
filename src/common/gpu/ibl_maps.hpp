#pragma once

#include "platform/gl/glextutil.h"


struct IBLMaps {
    GLuint environment;
    GLuint irradiance;
    GLuint specular;
};

// TODO: Utility shaders are leaking

IBLMaps loadIBLMapsFromHDRI(const char* path);
IBLMaps loadIBLMapsFromCubeSides(
    const char* posx, const char* negx,
    const char* posy, const char* negy,
    const char* posz, const char* negz
);