#pragma once

#include "common/typeface/font.hpp"
#include "common/render/gpu_mesh.hpp"


class gpuText {
    Font* font = 0;
    std::string str;

    gpuBuffer vertices_buf;
    gpuBuffer uv_buf;
    gpuBuffer rgb_buf;
    gpuBuffer text_uv_lookup_buf;
    gpuBuffer index_buf;
    gpuMeshDesc mesh_desc;
public:
    gpuText() {}
    gpuText(Font* font);
    ~gpuText();

    void setString(const char* str);
    void commit();

    gpuMeshDesc* getMeshDesc();
};