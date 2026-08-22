#include "sky_material.hpp"



SkyMaterial::SkyMaterial() {
    registerShaderKey(nullptr);
    registerFragmentSet(loadResource<gpuShaderSet>("core/shaders/modular/sky.mat.frag"));

    shading_style = GPU_ShadingStyle::ForwardTranslucent;
    is_animated = false;

    sky_map = loadResource<gpuCubeMap>("cubemaps/hdri/belfast_sunset_puresky_1k");
}

void SkyMaterial::applySamplers(gpuShaderProgram* prog, ShaderSamplerSet& out) {
    out.addCubemap(prog, "texSky", sky_map); // TODO: A good skymap fallback
}

void SkyMaterial::makeSnapshot(rtti::PropSnapshot& snap) {
    snap.type_ = get_type();
    // TODO:
    gpuMaterial::makeSnapshot(snap);
}
void SkyMaterial::applySnapshot(rtti::PropSnapshot& snap) {
    // TODO:
    gpuMaterial::applySnapshot(snap);
}

