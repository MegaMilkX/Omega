#pragma once

// TODO: move viewport to gpu related stuff
#include "viewport/viewport.hpp"
#include "gpu/render_bucket.hpp"


class gpuRenderer {
public:
    virtual ~gpuRenderer() {}
    virtual void initView(EngineRenderView* rv) = 0;
    virtual void draw(gpuRenderBucket* bucket, EngineRenderView* rv, DRAW_PARAMS& params) = 0;
};

