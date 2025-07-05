#ifndef GLX_SHADER_PROGRAM_HPP
#define GLX_SHADER_PROGRAM_HPP

#include <vector>
#include "log/log.hpp"
#include "platform/gl/glextutil.h"
#include "reflection/reflection.hpp"


inline void glxShaderSource(GLuint shader, const char* string, int len = 0) {
    glShaderSource(shader, 1, &string, len == 0 ? 0 : &len);
}
inline bool glxCompileShader(GLuint shader) {
    glCompileShader(shader);
    GLint res = GL_FALSE;
    int infoLogLen;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);
    if(infoLogLen > 1)
    {
        std::vector<char> errMsg(infoLogLen + 1);
        glGetShaderInfoLog(shader, infoLogLen, NULL, &errMsg[0]);
        LOG_ERR("GLSL compile: " << &errMsg[0]);
    }
    if(res == GL_FALSE)
        return false;
    return true;
}
#include "gpu/vertex_format.hpp"
inline void glxProgramBindVertexFormat(GLuint program, const VFMT::VERTEX_DESC* const vertex_desc) {
    for(int i = 0; i < vertex_desc->attribCount; ++i) {
        glBindAttribLocation(program, i, vertex_desc->attribs[i]->in_name);
    }
}
#include "frame_format.hpp"
inline void glxProgramBindFrameFormat(GLuint program, const FFMT::FRAME_DESC* const frame_desc) {
    for(int i = 0; i < frame_desc->layerCount; ++i) {
        glBindFragDataLocation(program, i, frame_desc->layers[i].out_name);    
    }
}
#include "texture_layout.hpp"
inline void glxProgramSetTextureUniforms(GLuint program, const TL::LAYOUT_DESC* const texture_desc) {
    glUseProgram(program);
    for(int i = 0; i < texture_desc->layerCount; ++i) {
        auto& layer = texture_desc->layers[i];
        glUniform1i(glGetUniformLocation(program, layer.shader_name), i);
    }
    glUseProgram(0);
}

#include <unordered_map>
#include <memory>
#include "gpu/gpu_mesh_desc.hpp"
#include "gpu/gpu_instancing_desc.hpp"

struct gpuMeshBindingKey {
    const gpuMeshDesc* a;
    const gpuInstancingDesc* b;

    bool operator==(const gpuMeshBindingKey& other) const {
        return a == other.a && b == other.b;
    }
};
template<>
struct std::hash<gpuMeshBindingKey> {
    size_t operator()(const gpuMeshBindingKey& k) const {
        size_t ha = std::hash<const gpuMeshDesc*>()(k.a);
        size_t hb = std::hash<const gpuInstancingDesc*>()(k.b);
        if (ha != hb) {
            ha = ha ^ hb;
        }
        return ha;
    }
};

struct UNIFORM_INFO {
    std::string name;
    int location = 0;
    GLenum type = 0;
    bool auto_upload = false;
};

struct gpuUniformBufferDesc;

enum SHADER_TYPE {
    SHADER_UNKNOWN,
    SHADER_VERTEX,
    SHADER_FRAGMENT,
    SHADER_GEOMETRY
};

class gpuShaderProgram {
    struct SHADER {
        SHADER_TYPE type;
        GLuint id;
    };

    GLuint progid = 0;
    std::vector<SHADER> shaders;
    std::unordered_map<VFMT::GUID, int>  attrib_table; // Attrib guid to shader attrib location
    std::unordered_map<std::string, int> sampler_indices;
    std::vector<std::string> sampler_names;

    int sampler_count = 0;
    std::vector<std::string> outputs;

    std::vector<UNIFORM_INFO> uniforms;
    std::vector<const gpuUniformBufferDesc*> uniform_blocks;

    bool compileAndAttach();
    void bindAttributeLocations();
    void bindFragmentOutputLocations();
    bool link();
    void setSamplerIndices();
    void getVertexAttributes();
    void setUniformBlockBindings();
    void enumerateUniforms();

public:
    TYPE_ENABLE();

    gpuShaderProgram() {}
    gpuShaderProgram(const char* vs, const char* fs);
    ~gpuShaderProgram() {
        glDeleteProgram(progid);
        clearShaders();
    }

    void clearShaders();

    void setShaders(const char* vs, const char* fs);
    void addShader(SHADER_TYPE type, const char* source);

    void init();
    void initForLightmapSampling();

    GLuint getId() const {
        return progid;
    }

    int uniformCount();
    int getUniformIndex(const std::string& name) const; // Not the same as location
    const UNIFORM_INFO& getUniformInfo(int i) const;
    UNIFORM_INFO& getUniformInfo(int i);

    int uniformBlockCount();
    const gpuUniformBufferDesc* getUniformBlockDesc(int i) const;

    GLint getUniformLocation(const char* name) const;
    bool setUniform1i(const char* name, int i);
    bool setUniform1f(const char* name, float f);
    bool setUniform4f(const char* name, const gfxm::vec4& f4);
    bool setUniform3f(const char* name, const gfxm::vec3& f3);
    bool setUniformMatrix4(const char* name, const gfxm::mat4& m4);

    const std::unordered_map<VFMT::GUID, int>& getAttribTable() const { return attrib_table; }

    int getSamplerCount() const {
        return sampler_count;
    }
    const std::string& getSamplerName(int i) const {
        return sampler_names[i];
    }
    int getDefaultSamplerSlot(const char* name) const {
        auto it = sampler_indices.find(name);
        if (it == sampler_indices.end()) {
            return -1;
        }
        return it->second;
    }

    size_t outputCount() const { return outputs.size(); }
    const std::string& getOutputName(int i) const { return outputs[i]; }
};


RHSHARED<gpuShaderProgram> loadShaderProgram(const char* path);
RHSHARED<gpuShaderProgram> loadShaderProgramForLightmapSampling(const char* path);

Handle<gpuShaderProgram> createProgram(const char* filepath, const char* str, size_t len);

#endif
