#include "vfx_material.hpp"


void VFXMaterial::updateShaderFlags() {
    pass_requirement = GPU_MaterialPassReq(GPU_ShadingStyle::VFX);
    //shading_style = GPU_ShadingStyle::VFX;
    
    is_animated
        = uv_scroll_mode != GPU_UVScrollMode::None
        || uv2_scroll_mode != GPU_UVScrollMode::None;

    setBlendingMode(GPU_BLEND_MODE::ADD);
}

VFXMaterial::VFXMaterial()
: ubdesc("ubMaterial") {
    registerShaderKey(&key);

    registerVertexSet(loadResource<gpuShaderSet>("core/shaders/modular/vfx.mat.vert"));
    registerFragmentSet(loadResource<gpuShaderSet>("core/shaders/modular/vfx.mat.frag"));

    ubdesc.define("rgba", UNIFORM_TYPE::UNIFORM_VEC4);
    ubdesc.define("uv_scale", UNIFORM_TYPE::UNIFORM_VEC2);
    ubdesc.define("uv_offset", UNIFORM_TYPE::UNIFORM_VEC2);
    ubdesc.define("uv2_scale", UNIFORM_TYPE::UNIFORM_VEC2);
    ubdesc.define("uv2_offset", UNIFORM_TYPE::UNIFORM_VEC2);
    ubdesc.define("depth_bias", UNIFORM_TYPE::UNIFORM_FLOAT);
    ubdesc.define("soft_clip_dist", UNIFORM_TYPE::UNIFORM_FLOAT);
    ubdesc.compile();

    ubuf.reset(new gpuUniformBuffer(&ubdesc));
    ubuf->setVec4(ubuf->getDesc()->getUniform("rgba"), rgba);
    ubuf->setVec2(ubuf->getDesc()->getUniform("uv_scale"), gfxm::vec2(1, 1));
    ubuf->setVec2(ubuf->getDesc()->getUniform("uv_offset"), gfxm::vec2(0, 0));
    ubuf->setVec2(ubuf->getDesc()->getUniform("uv2_scale"), gfxm::vec2(1, 1));
    ubuf->setVec2(ubuf->getDesc()->getUniform("uv2_offset"), gfxm::vec2(0, 0));
    ubuf->setFloat(ubuf->getDesc()->getUniform("depth_bias"), depth_bias);
    ubuf->setFloat(ubuf->getDesc()->getUniform("soft_clip_dist"), soft_clip_dist);
    addUniformBuffer(ubuf.get());

    updateShaderFlags();
}

bool VFXMaterial::resolvePass(GPU_RenderDomain domain, PassResolution& out) const {
    switch (domain) {
    case GPU_RenderDomain::Surface:
        out.pass_name = "VFX";
        out.blend_mode = getBlendingMode();
        // TODO: out.draw_flags = 
        out.cast_shadows = false; // ??
        return true;
    case GPU_RenderDomain::Decal:
        out.pass_name = "Decals";
        out.blend_mode = getBlendingMode();
        // TODO: out.draw_flags = 
        out.cast_shadows = false; // ??
        return true;
    }
    return false;
}
void VFXMaterial::applySamplers(gpuShaderProgram* prog, ShaderSamplerSet& out) {
    out.addTexture2dRef(prog, "texAlbedo", albedo_map ? albedo_map : getDefaultTexture("WHITE"));
    out.addTexture2dRef(prog, "texAlbedo2", texture2 ? texture2 : getDefaultTexture("WHITE"));
}
void VFXMaterial::onTick(float dt) {    
    switch (uv_scroll_mode) {
    case GPU_UVScrollMode::Smooth: {
        gfxm::vec2 offs = ubuf->getValue<gfxm::vec2>(ubuf->getDesc()->getUniform("uv_offset"));
        offs += uv_velocity * dt;
        ubuf->setVec2(ubuf->getDesc()->getUniform("uv_offset"), offs);
        break;
    }
    case GPU_UVScrollMode::Step: {
        gfxm::vec2 offs;
        float t = uv_step_interval > 0.0f
            ? std::floor(time / uv_step_interval) * uv_step_interval
            : time;
        offs.x = uv_velocity.x * t;
        offs.y = uv_velocity.y * t;
        ubuf->setVec2(ubuf->getDesc()->getUniform("uv_offset"), offs);
        break;
    }
    case GPU_UVScrollMode::Flipbook: {
        gfxm::vec2 scale;
        gfxm::vec2 offs;

        int total = uv_flipbook_cols * uv_flipbook_rows;
        int frame = static_cast<int>(time * uv_flipbook_fps);
        frame = (frame % total);
        float sx = 1.0f / uv_flipbook_cols;
        float sy = 1.0f / uv_flipbook_rows;
        scale = gfxm::vec2(sx, sy);
        offs = gfxm::vec2((frame % uv_flipbook_cols) * sx, 1.0f - (frame / uv_flipbook_cols) * sy);

        ubuf->setVec2(ubuf->getDesc()->getUniform("uv_scale"), scale);
        ubuf->setVec2(ubuf->getDesc()->getUniform("uv_offset"), offs);
        break;
    }
    }
    
    switch (uv2_scroll_mode) {
    case GPU_UVScrollMode::Smooth: {
        gfxm::vec2 offs = ubuf->getValue<gfxm::vec2>(ubuf->getDesc()->getUniform("uv2_offset"));
        offs += uv2_velocity * dt;
        ubuf->setVec2(ubuf->getDesc()->getUniform("uv2_offset"), offs);
        break;
    }
    case GPU_UVScrollMode::Step: {
        gfxm::vec2 offs;
        float t = uv2_step_interval > 0.0f
            ? std::floor(time / uv2_step_interval) * uv2_step_interval
            : time;
        offs.x = uv2_velocity.x * t;
        offs.y = uv2_velocity.y * t;
        ubuf->setVec2(ubuf->getDesc()->getUniform("uv2_offset"), offs);
        break;
    }
    case GPU_UVScrollMode::Flipbook: {
        gfxm::vec2 scale;
        gfxm::vec2 offs;

        int total = uv2_flipbook_cols * uv2_flipbook_rows;
        int frame = static_cast<int>(time * uv2_flipbook_fps);
        frame = (frame % total);
        float sx = 1.0f / uv2_flipbook_cols;
        float sy = 1.0f / uv2_flipbook_rows;
        scale = gfxm::vec2(sx, sy);
        offs = gfxm::vec2((frame % uv2_flipbook_cols) * sx, 1.0f - (frame / uv2_flipbook_cols) * sy);

        ubuf->setVec2(ubuf->getDesc()->getUniform("uv2_scale"), scale);
        ubuf->setVec2(ubuf->getDesc()->getUniform("uv2_offset"), offs);
        break;
    }
    }
    time += dt;
}

void VFXMaterial::makeSnapshot(rtti::PropSnapshot& snap) const {
    snap.type_ = get_type();

    snap.add("albedo_map", rtti::varying::make(albedo_map), "VFXMaterial");
    snap.add("texture2", rtti::varying::make(texture2), "VFXMaterial");
    snap.add("rgba", rtti::varying::make(rgba), "VFXMaterial");
    snap.add("depth_test", rtti::varying::make(key.depth_test), "VFXMaterial");
    snap.add("depth_bias", rtti::varying::make(depth_bias), "VFXMaterial");
    snap.add("soft_clipping", rtti::varying::make(key.soft_clipping), "VFXMaterial");
    snap.add("soft_clip_dist", rtti::varying::make(soft_clip_dist), "VFXMaterial");

    snap.add("uv_scroll", rtti::varying::make(uv_scroll_mode), "VFXMaterial");
    snap.add("uv_velocity", rtti::varying::make(uv_velocity), "VFXMaterial");
    snap.add("uv_interval", rtti::varying::make(uv_step_interval), "VFXMaterial");
    snap.add("flipbook_columns", rtti::varying::make(uv_flipbook_cols), "VFXMaterial");
    snap.add("flipbook_rows", rtti::varying::make(uv_flipbook_rows), "VFXMaterial");
    snap.add("flipbook_fps", rtti::varying::make(uv_flipbook_fps), "VFXMaterial");

    snap.add("uv2_scroll", rtti::varying::make(uv2_scroll_mode), "VFXMaterial");
    snap.add("uv2_velocity", rtti::varying::make(uv2_velocity), "VFXMaterial");
    snap.add("uv2_interval", rtti::varying::make(uv2_step_interval), "VFXMaterial");
    snap.add("flipbook2_columns", rtti::varying::make(uv2_flipbook_cols), "VFXMaterial");
    snap.add("flipbook2_rows", rtti::varying::make(uv2_flipbook_rows), "VFXMaterial");
    snap.add("flipbook2_fps", rtti::varying::make(uv2_flipbook_fps), "VFXMaterial");

    gpuMaterial::makeSnapshot(snap);
}
void VFXMaterial::applySnapshot(const rtti::PropSnapshot& snap) {
    if (auto map = snap.get<ResourceRef<gpuTexture2d>>("albedo_map")) {
        albedo_map = *map;
    }
    if (auto map = snap.get<ResourceRef<gpuTexture2d>>("texture2")) {
        texture2 = *map;
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

    if (auto val = snap.get<GPU_UVScrollMode>("uv_scroll")) {
        uv_scroll_mode = *val;
    }
    if (auto val = snap.get<gfxm::vec2>("uv_velocity")) {
        uv_velocity = *val;
    }
    if (auto val = snap.get<float>("uv_interval")) {
        uv_step_interval = *val;
    }
    if (auto val = snap.get<int>("flipbook_columns")) {
        uv_flipbook_cols = *val;
    }
    if (auto val = snap.get<int>("flipbook_rows")) {
        uv_flipbook_rows = *val;
    }
    if (auto val = snap.get<float>("flipbook_fps")) {
        uv_flipbook_fps = *val;
    }

    if (auto val = snap.get<GPU_UVScrollMode>("uv2_scroll")) {
        uv2_scroll_mode = *val;
    }
    if (auto val = snap.get<gfxm::vec2>("uv2_velocity")) {
        uv2_velocity = *val;
    }
    if (auto val = snap.get<float>("uv2_interval")) {
        uv2_step_interval = *val;
    }
    if (auto val = snap.get<int>("flipbook2_columns")) {
        uv2_flipbook_cols = *val;
    }
    if (auto val = snap.get<int>("flipbook2_rows")) {
        uv2_flipbook_rows = *val;
    }
    if (auto val = snap.get<float>("flipbook2_fps")) {
        uv2_flipbook_fps = *val;
    }

    updateShaderFlags();

    gpuMaterial::applySnapshot(snap);
}

