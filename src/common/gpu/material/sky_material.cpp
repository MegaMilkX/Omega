#include "sky_material.hpp"



SkyMaterial::SkyMaterial() {
    registerShaderKey(nullptr);
    registerFragmentSet(loadResource<gpuShaderSet>("core/shaders/modular/sky.mat.frag"));

    pass_requirement = GPU_MaterialPassReq(GPU_ShadingStyle::ForwardTranslucent);
    //shading_style = GPU_ShadingStyle::ForwardTranslucent;
    is_animated = false;

    sky_map = loadResource<gpuCubeTexture>("cubemaps/hdri/belfast_sunset_puresky_1k");
}

bool SkyMaterial::resolvePass(GPU_RenderDomain domain, PassResolution& out) const {
    switch (domain) {
    case GPU_RenderDomain::Surface:
        out.pass_name = "Default";
        out.blend_mode = GPU_BLEND_MODE::OVERWRITE;
        // TODO: out.draw_flags = 
        out.cast_shadows = true; // ??
        return true;
    }
    return false;
}
void SkyMaterial::applySamplers(gpuShaderProgram* prog, ShaderSamplerSet& out) {
    out.addCubemap(prog, "texSky", sky_map); // TODO: A good skymap fallback
}

void SkyMaterial::makeSnapshot(rtti::PropSnapshot& snap) const {
    snap.type_ = get_type();
    // TODO:
    gpuMaterial::makeSnapshot(snap);
}
void SkyMaterial::applySnapshot(const rtti::PropSnapshot& snap) {
    // TODO:
    gpuMaterial::applySnapshot(snap);
}

