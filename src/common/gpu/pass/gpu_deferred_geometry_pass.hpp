#pragma once

#include "gpu_geometry_pass.hpp"


class gpuDeferredGeometryPass : public gpuGeometryPass {
public:
    gpuDeferredGeometryPass() {
        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/geo.main.vert"));
        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/geo.main.frag"));
    }
};

class gpuDeferredDecalPass : public gpuGeometryPass {
public:
    gpuDeferredDecalPass() {
        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/geo.main.vert"));
        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/geo.main.frag"));
    }
};

class gpuErrorPass : public gpuGeometryPass {
public:
    gpuErrorPass() {
        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/geo.main.vert"));
        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/error.main.frag"));
    }
};

class gpuDecalPass : public gpuGeometryPass {
public:
    gpuDecalPass() {
        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/geo.main.vert"));
        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/decal.main.frag"));
    }
};

class gpuVFXPass : public gpuGeometryPass {
public:
    gpuVFXPass() {
        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/geo.main.vert"));
        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/vfx.main.frag"));
    }
};

class gpuOutlineColorPass : public gpuGeometryPass {
public:
    gpuOutlineColorPass() {
        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/geo.main.vert"));
        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/solid_color.main.frag"));
    }
};
class gpuOutlineCutoutPass : public gpuGeometryPass {
public:
    gpuOutlineCutoutPass() {
        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/geo.main.vert"));
        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/outline_cutout.main.frag"));
    }
};

class gpuOverlayPass : public gpuGeometryPass {
public:
    gpuOverlayPass() {
        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/geo.main.vert"));
        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/overlay.main.frag"));
    }
};

class gpuShadowmapPass : public gpuGeometryPass {
public:
    gpuShadowmapPass() {
        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/geo.main.vert"));
        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/shadow.frag"));
    }
    void onDraw(gpuPassInstance* inst, gpuRenderTargetMap* target_map, gpuRenderBucket* bucket, pipe_pass_id_t pass_id, const DRAW_PARAMS& params) override {
        gpuGeometryPass::onDraw(inst, target_map, bucket, pass_id, params);
    }
};

