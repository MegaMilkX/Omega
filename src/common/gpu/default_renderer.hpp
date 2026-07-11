#pragma once

#include "gpu/renderer.hpp"
#include "gpu/render_sequence.hpp"


class gpuDefaultRenderer : public gpuRenderer {
    gpuRenderSequence rseq_clear;
    gpuRenderSequence rseq_world;
    gpuRenderSequence rseq_blit_depth;
    gpuRenderSequence rseq_post;

    gpuRenderTarget rt2;
    gpuRenderTargetMap rt2_clear_map;
    gpuRenderTargetMap rt2_map;
public:
    gpuDefaultRenderer();

    void initView(EngineRenderView* rv) override;
    void draw(gpuRenderBucket* bucket, EngineRenderView* rv, DRAW_PARAMS& params) override;

    // TMP ========
    gpuRenderTarget* getRt2() { return &rt2; }
};

