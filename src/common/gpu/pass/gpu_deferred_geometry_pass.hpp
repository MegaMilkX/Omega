#pragma once

#include "gpu_geometry_pass.hpp"


class gpuDeferredGeometryPass : public gpuGeometryPass {
public:
    gpuDeferredGeometryPass() {
        setColorTarget("Albedo", "Albedo");
        setColorTarget("Position", "Position");
        setColorTarget("Normal", "Normal");
        setColorTarget("Metalness", "Metalness");
        setColorTarget("Roughness", "Roughness");
        setColorTarget("Emission", "Emission");
        setColorTarget("AmbientOcclusion", "AmbientOcclusion");
        setDepthTarget("Depth");
    }
};