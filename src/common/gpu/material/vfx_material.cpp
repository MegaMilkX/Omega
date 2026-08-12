#include "vfx_material.hpp"


void VFXMaterial::updateShaderFlags() {
    shading_style = GPU_ShadingStyle::VFX;
    is_animated = false;
    setBlendingMode(GPU_BLEND_MODE::ADD);
}

VFXMaterial::VFXMaterial()
: ubdesc("ubMaterial") {
    registerShaderKey(&key);

    registerVertexSet(loadResource<gpuShaderSet>("core/shaders/modular/vfx.mat.vert"));
    registerFragmentSet(loadResource<gpuShaderSet>("core/shaders/modular/vfx.mat.frag"));

    ubdesc.define("rgba", UNIFORM_TYPE::UNIFORM_VEC4);
    ubdesc.define("depth_bias", UNIFORM_TYPE::UNIFORM_FLOAT);
    ubdesc.define("soft_clip_dist", UNIFORM_TYPE::UNIFORM_FLOAT);
    ubdesc.compile();

    ubuf.reset(new gpuUniformBuffer(&ubdesc));
    ubuf->setVec4(ubuf->getDesc()->getUniform("rgba"), rgba);
    ubuf->setFloat(ubuf->getDesc()->getUniform("depth_bias"), depth_bias);
    ubuf->setFloat(ubuf->getDesc()->getUniform("soft_clip_dist"), soft_clip_dist);
    addUniformBuffer(ubuf.get());

    updateShaderFlags();
}

void VFXMaterial::applySamplers(gpuShaderProgram* prog, ShaderSamplerSet& out) {
    out.addTexture2d(prog, "texAlbedo", albedo_map ? albedo_map : getDefaultTexture("WHITE"));
}
void VFXMaterial::onTick(float dt) {

}

void VFXMaterial::makeSnapshot(rtti::PropSnapshot& snap) {
    snap.type_ = get_type();

    snap.add("albedo_map", rtti::varying::make(albedo_map), "VFXMaterial");
    snap.add("rgba", rtti::varying::make(rgba), "VFXMaterial");
    snap.add("depth_test", rtti::varying::make(key.depth_test), "VFXMaterial");
    snap.add("depth_bias", rtti::varying::make(depth_bias), "VFXMaterial");
    snap.add("soft_clipping", rtti::varying::make(key.soft_clipping), "VFXMaterial");
    snap.add("soft_clip_dist", rtti::varying::make(soft_clip_dist), "VFXMaterial");

    gpuMaterial::makeSnapshot(snap);
}
void VFXMaterial::applySnapshot(rtti::PropSnapshot& snap) {
    if (auto map = snap.get<ResourceRef<gpuTexture2d>>("albedo_map")) {
        albedo_map = *map;
    }

    if (auto val = snap.get<gfxm::vec4>("rgba")) {
        rgba = *val;
        ubuf->setVec4(ubuf->getDesc()->getUniform("rgba"), rgba);
    }
    if (auto val = snap.get<bool>("depth_test")) {
        key.depth_test = *val;
    }
    if (auto val = snap.get<float>("depth_bias")) {
        depth_bias = *val;
        ubuf->setFloat(ubuf->getDesc()->getUniform("depth_bias"), depth_bias);
    }
    if (auto val = snap.get<bool>("soft_clipping")) {
        key.soft_clipping = *val;
    }
    if (auto val = snap.get<float>("soft_clip_dist")) {
        soft_clip_dist = *val;
        ubuf->setFloat(ubuf->getDesc()->getUniform("soft_clip_dist"), soft_clip_dist);
    }

    updateShaderFlags();

    gpuMaterial::applySnapshot(snap);
}

