#pragma once

#include "gpu_geometry_pass.hpp"


class gpuDeferredGeometryPass : public gpuGeometryPass {
public:
    gpuDeferredGeometryPass() {
        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/geo.main.vert"));
        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/geo.main.frag"));
    }
};

class gpuDecalPass : public gpuGeometryPass {
public:
    gpuDecalPass() {
        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/decal.main.vert"));
        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/modular/decal.main.frag"));
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