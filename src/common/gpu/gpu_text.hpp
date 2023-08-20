#pragma once

#include "math/gfxm.hpp"
#include "typeface/font.hpp"
#include "gpu_mesh.hpp"


class gpuText {
    Font* font = 0;
    std::string str;

    gpuBuffer vertices_buf;
    gpuBuffer uv_buf;
    gpuBuffer rgb_buf;
    gpuBuffer text_uv_lookup_buf;
    gpuBuffer index_buf;
    gpuMeshDesc mesh_desc;

    gfxm::vec2 bounding_size;
public:
    gpuText() {}
    gpuText(Font* font);
    ~gpuText();

    void setFont(Font* fnt);
    void setString(const char* str);
    void commit(float max_width = .0f, float scale = .01f);

    gpuMeshDesc* getMeshDesc();
    const gfxm::vec2& getBoundingSize() const { return bounding_size; }
};