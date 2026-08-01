#include "gpu/gpu_material.hpp"

#include "gpu/gpu_pipeline.hpp"

#include "gpu/gpu.hpp"
#include "gpu/readwrite/rw_gpu_material.hpp"
#include "resource_manager/byte_writer/file_writer.hpp"


int glTypeToSize(GLenum type) {
    switch (type) {
    case GL_FLOAT:
    case GL_INT:
    case GL_UNSIGNED_INT:
        return 4;
    case GL_FLOAT_VEC2:
    case GL_INT_VEC2:
    case GL_UNSIGNED_INT_VEC2:
        return 8;
    case GL_FLOAT_VEC3:
    case GL_INT_VEC3:
    case GL_UNSIGNED_INT_VEC3:
        return 12;
    case GL_FLOAT_VEC4:
    case GL_INT_VEC4:
    case GL_UNSIGNED_INT_VEC4:
        return 16;
    case GL_DOUBLE:
        return 8;
    case GL_DOUBLE_VEC2:
        return 16;
    case GL_DOUBLE_VEC3:
        return 24;
    case GL_DOUBLE_VEC4:
        return 32;
     /*
     case GL_BOOL: {  //  bool
     break;
     }
     case GL_BOOL_VEC2: {   // bvec2
     break;
     }
     case GL_BOOL_VEC3: {   // bvec3
     break;
     }
     case GL_BOOL_VEC4: {   // bvec4
     break;
     }*/
    case GL_FLOAT_MAT2:
        return 16;
    case GL_FLOAT_MAT3:
        return 36;
    case GL_FLOAT_MAT4:
        return 64;
    case GL_FLOAT_MAT2x3:
        return 24;
    case GL_FLOAT_MAT2x4:
        return 32;
    case GL_FLOAT_MAT3x2:
        return 24;
    case GL_FLOAT_MAT3x4:
        return 48;
    case GL_FLOAT_MAT4x2:
        return 32;
    case GL_FLOAT_MAT4x3:
        return 48;
        /*
     case GL_DOUBLE_MAT2: {       // dmat2
     break;
     }
     case GL_DOUBLE_MAT3: {       // dmat3
     break;
     }
     case GL_DOUBLE_MAT4: {       // dmat4
     break;
     }
     case GL_DOUBLE_MAT2x3: {   // dmat2x3
     break;
     }
     case GL_DOUBLE_MAT2x4: {   // dmat2x4
     break;
     }
     case GL_DOUBLE_MAT3x2: {   // dmat3x2
     break;
     }
     case GL_DOUBLE_MAT3x4: {   // dmat3x4
     break;
     }
     case GL_DOUBLE_MAT4x2: {   // dmat4x2
     break;
     }
     case GL_DOUBLE_MAT4x3: {   // dmat4x3
     break;
     }*//*
     case GL_SAMPLER_1D: {   // sampler1D
     break;
     }
     case GL_SAMPLER_2D: {   // sampler2D
     break;
     }
     case GL_SAMPLER_3D: {   // sampler3D
     break;
     }
     case GL_SAMPLER_CUBE: {   // samplerCube
     break;
     }
     case GL_SAMPLER_1D_SHADOW: {   // sampler1DShadow
     break;
     }
     case GL_SAMPLER_2D_SHADOW: {   // sampler2DShadow
     break;
     }
     case GL_SAMPLER_1D_ARRAY: {   // sampler1DArray
     break;
     }
     case GL_SAMPLER_2D_ARRAY: {   // sampler2DArray
     break;
     }
     case GL_SAMPLER_1D_ARRAY_SHADOW: {       // sampler1DArrayShadow
     break;
     }
     case GL_SAMPLER_2D_ARRAY_SHADOW: {       // sampler2DArrayShadow
     break;
     }
     case GL_SAMPLER_2D_MULTISAMPLE: {   // sampler2DMS
     break;
     }
     case GL_SAMPLER_2D_MULTISAMPLE_ARRAY: {   // sampler2DMSArray
     break;
     }
     case GL_SAMPLER_CUBE_SHADOW: {       // samplerCubeShadow
     break;
     }
     case GL_SAMPLER_BUFFER: {   // samplerBuffer
     break;
     }
     case GL_SAMPLER_2D_RECT: {       // sampler2DRect
     break;
     }
     case GL_SAMPLER_2D_RECT_SHADOW: {   // sampler2DRectShadow
     break;
     }
     case GL_INT_SAMPLER_1D: {   // isampler1D
     break;
     }
     case GL_INT_SAMPLER_2D: {   // isampler2D
     break;
     }
     case GL_INT_SAMPLER_3D: {   // isampler3D
     break;
     }
     case GL_INT_SAMPLER_CUBE: {   // isamplerCube
     break;
     }
     case GL_INT_SAMPLER_1D_ARRAY: {   // isampler1DArray
     break;
     }
     case GL_INT_SAMPLER_2D_ARRAY: {   // isampler2DArray
     break;
     }
     case GL_INT_SAMPLER_2D_MULTISAMPLE: {   // isampler2DMS
     break;
     }
     case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY: {   // isampler2DMSArray
     break;
     }
     case GL_INT_SAMPLER_BUFFER: {   // isamplerBuffer
     break;
     }
     case GL_INT_SAMPLER_2D_RECT: {       // isampler2DRect
     break;
     }
     case GL_UNSIGNED_INT_SAMPLER_1D: {       // usampler1D
     break;
     }
     case GL_UNSIGNED_INT_SAMPLER_2D: {       // usampler2D
     break;
     }
     case GL_UNSIGNED_INT_SAMPLER_3D: {       // usampler3D
     break;
     }
     case GL_UNSIGNED_INT_SAMPLER_CUBE: {   // usamplerCube
     break;
     }
     case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY: {   // usampler2DArray
     break;
     }
     case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY: {   // usampler2DArray
     break;
     }
     case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE: {       // usampler2DMS
     break;
     }
     case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY: { 	// usampler2DMSArray
     break;
     }
     case GL_UNSIGNED_INT_SAMPLER_BUFFER: {       // usamplerBuffer
     break;
     }
     case GL_UNSIGNED_INT_SAMPLER_2D_RECT: {    // usampler2DRect
     break;
     }*/
    default: {
        LOG_ERR("glTypeToSize: unknown type: " << type);
        assert(false);
    }
    }
    return 0;
}

void gpuMaterial::registerVertexSet(const ResourceRef<gpuShaderSet>& shaders) {
    vertex_set = shaders;
}

void gpuMaterial::registerFragmentSet(const ResourceRef<gpuShaderSet>& shaders) {
    fragment_set = shaders;
}

gpuMaterial::gpuMaterial() {
    registerVertexSet(loadResource<gpuShaderSet>("core/shaders/modular/basic.vert"));
    registerFragmentSet(loadResource<gpuShaderSet>("core/shaders/modular/basic.frag"));
}

void gpuMaterial::setParam(const std::string& name, GLenum type, const void* data) {
    PARAMETER param = PARAMETER(type);
    memcpy(param.data, data, glTypeToSize(type));
    params[name] = param;
}
void gpuMaterial::setParamFloat(const std::string& name, float value) {
    setParam(name, GL_FLOAT, &value);
}
void gpuMaterial::setParamVec2(const std::string& name, const gfxm::vec2& v) {
    setParam(name, GL_FLOAT_VEC2, &v);
}
void gpuMaterial::setParamVec3(const std::string& name, const gfxm::vec3& v) {
    setParam(name, GL_FLOAT_VEC3, &v);
}
void gpuMaterial::setParamVec4(const std::string& name, const gfxm::vec4& v) {
    setParam(name, GL_FLOAT_VEC4, &v);
}
void gpuMaterial::setParamInt(const std::string& name, int value) {
    setParam(name, GL_INT, &value);
}
void gpuMaterial::setParamVec2i(const std::string& name, const gfxm::ivec2& v) {
    setParam(name, GL_INT_VEC2, &v);
}
void gpuMaterial::setParamVec3i(const std::string& name, const gfxm::ivec3& v) {
    setParam(name, GL_INT_VEC3, &v);

}
void gpuMaterial::setParamVec4i(const std::string& name, const gfxm::ivec4& v) {
    setParam(name, GL_INT_VEC4, &v);
}
void gpuMaterial::setParamMat2(const std::string& name, float* pvalue) {
    setParam(name, GL_FLOAT_MAT2, pvalue);
}
void gpuMaterial::setParamMat3(const std::string& name, float* pvalue) {
    setParam(name, GL_FLOAT_MAT3, pvalue);
}
void gpuMaterial::setParamMat4(const std::string& name, float* pvalue) {
    setParam(name, GL_FLOAT_MAT4, pvalue);
}
void gpuMaterial::setParamMat2x3(const std::string& name, float* pvalue) {
    setParam(name, GL_FLOAT_MAT2x3, pvalue);
}
void gpuMaterial::setParamMat2x4(const std::string& name, float* pvalue) {
    setParam(name, GL_FLOAT_MAT2x4, pvalue);
}
void gpuMaterial::setParamMat3x2(const std::string& name, float* pvalue) {
    setParam(name, GL_FLOAT_MAT3x2, pvalue);
}
void gpuMaterial::setParamMat3x4(const std::string& name, float* pvalue) {
    setParam(name, GL_FLOAT_MAT3x4, pvalue);
}
void gpuMaterial::setParamMat4x2(const std::string& name, float* pvalue) {
    setParam(name, GL_FLOAT_MAT4x2, pvalue);
}
void gpuMaterial::setParamMat4x3(const std::string& name, float* pvalue) {
    setParam(name, GL_FLOAT_MAT4x3, pvalue);
}

gpuMaterial::PARAMETER* gpuMaterial::getParam(const std::string& name) {
    auto it = params.find(name);
    if (it == params.end()) {
        return nullptr;
    }
    return &it->second;
}

void gpuMaterial::compile() {
    auto pipeline = gpuGetPipeline();

    for (int i = 0; i < passes.size(); ++i) {
        auto mat_pass = passes[i].get();
        auto pip_pass = pipeline->findPass(mat_pass->getPath().c_str());
        if (!pip_pass) {
            LOG_ERR("Pass '" << mat_pass->getPath() << "' required by a material does not exist");
            continue;
        }

        mat_pass->pipeline_idx = pip_pass->getId();
        // TODO: Can move this to load stage
    }
}

void gpuMaterial::makeSnapshot(rtti::PropSnapshot& snap) {
    rtti::MetaObject::makeSnapshot(snap);
    snap.add("transparent", rtti::varying::make(transparent), "state");
    snap.add("depth test", rtti::varying::make(depth_test), "state");
    snap.add("stencil test", rtti::varying::make(stencil_test), "state");
    snap.add("cull faces", rtti::varying::make(cull_faces), "state");
    snap.add("depth write", rtti::varying::make(depth_write), "state");
    snap.add("blend mode", rtti::varying::make<GPU_BLEND_MODE>(blend_mode), "state");
    snap.add("sort bias", rtti::varying::make(sort_bias), "state");

    //snap.add("vertex", rtti::varying::make(vertex_extension_set), "extensions");
    //snap.add("fragment", rtti::varying::make(fragment_extension_set), "extensions");
}

void gpuMaterial::applySnapshot(rtti::PropSnapshot& snap) {
    rtti::MetaObject::applySnapshot(snap);

    if(auto p = snap.get<bool>("transparent")) {
        transparent = *p;
    }
    if(auto p = snap.get<bool>("depth test")) {
        depth_test = *p;
    }
    if(auto p = snap.get<bool>("stencil test")) {
        stencil_test = *p;
    }
    if(auto p = snap.get<bool>("cull faces")) {
        cull_faces = *p;
    }
    if(auto p = snap.get<bool>("depth write")) {
        depth_write = *p;
    }
    if(auto p = snap.get<GPU_BLEND_MODE>("blend mode")) {
        blend_mode = *p;
    }
    if(auto p = snap.get<int>("sort bias")) {
        sort_bias = *p;
    }
    /*
    if(auto p = snap.get<ResourceRef<gpuShaderSet>>("vertex")) {
        vertex_extension_set = *p;
    }
    if(auto p = snap.get<ResourceRef<gpuShaderSet>>("fragment")) {
        fragment_extension_set = *p;
    }*/
    
    compile();
    ++version;
}

bool gpuMaterial::fromJson(const nlohmann::json& json) {
    return readGpuMaterialJson(json, this);
}

bool gpuMaterial::load(byte_reader& in) {
    auto view = in.try_slurp();
    if (!view) {
        return false;
    }

    std::string str(view.data, view.data + view.size);
    nlohmann::json json = nlohmann::json::parse(str);

    return readGpuMaterialJson(json, this);
}

#include "filesystem/filesystem.hpp"
void gpuMaterial::write(const std::string& path) const {
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) {
        assert(false);
        LOG_ERR("Failed to open file for writing: '" << path << "'");
        return;
    }
    file_writer out(f);
    write(out);
    fclose(f);
}
void gpuMaterial::write(byte_writer& out) const {
    nlohmann::json json;
    if (!writeGpuMaterialJson(json, const_cast<gpuMaterial*>(this))) {
        LOG_ERR("gpuMaterial::write failed");
        assert(false);
        return;
    }
    std::string str = json.dump(2);
    out.write(str.data(), str.size());
}

