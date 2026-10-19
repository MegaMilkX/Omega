#pragma once

#include "gpu/param_block/param_block.hpp"


constexpr int N_CASCADES = 6;
constexpr int SHADOW_RESOLUTION = 1024;
class gpuDirectLightBlock : public gpuParamBlock {
    friend class gpuDirectLightBlockMgr;
    gfxm::mat4 cascadeViewProj[N_CASCADES];
    gfxm::vec4 cascadeParams[N_CASCADES];
    gfxm::vec4 lightDir;
public:
    void setViewProj(const gfxm::mat4& view, const gfxm::mat4& proj, int cascade) {
        cascadeViewProj[cascade] = proj * view;
        markDirty();
    }
    void setParams(float split, float one_over_ortho_width, float ortho_depth_range, int cascade) {
        cascadeParams[cascade].x = split;
        cascadeParams[cascade].y = one_over_ortho_width;
        cascadeParams[cascade].z = ortho_depth_range;
        markDirty();
    }
    void setLightDir(const gfxm::vec3& dir) {
        lightDir = gfxm::vec4(dir, .0f);
        markDirty();
    }
    /*
    const gfxm::mat4& getView() const { return view; }
    const gfxm::mat4& getProjection() const { return proj; }*/
};