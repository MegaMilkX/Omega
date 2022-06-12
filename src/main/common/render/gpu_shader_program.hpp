#ifndef GLX_SHADER_PROGRAM_HPP
#define GLX_SHADER_PROGRAM_HPP

#include <vector>
#include "log/log.hpp"
#include "platform/gl/glextutil.h"


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
#include "common/render/gpu_mesh_desc.hpp"
#include "common/render/gpu_instancing_desc.hpp"

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


class gpuShaderProgram {
    GLuint progid, vid, fid;
    std::unordered_map<VFMT::GUID, int>  attrib_table; // Attrib guid to shader attrib location
    std::unordered_map<std::string, int> sampler_indices;
    std::unordered_map<
        gpuMeshBindingKey, 
        std::unique_ptr<gpuMeshBinding
    >> mesh_bindings;
public:
    gpuShaderProgram(const char* vs, const char* fs);
    ~gpuShaderProgram() {
        glDeleteProgram(progid);
        glDeleteShader(fid);
        glDeleteShader(vid);
    }

    GLuint getId() const {
        return progid;
    }

    GLint getUniformLocation(const char* name) const {
        return glGetUniformLocation(progid, name);
    }

    int getDefaultSamplerSlot(const char* name) const {
        auto it = sampler_indices.find(name);
        if (it == sampler_indices.end()) {
            return -1;
        }
        return it->second;
    }

    const gpuMeshBinding* getMeshBinding(gpuMeshBindingKey key) {
        auto it = mesh_bindings.find(key);
        
        if (it == mesh_bindings.end()) {
            const gpuMeshDesc* desc = key.a;
            const gpuInstancingDesc* inst_desc = key.b;

            auto ptr = new gpuMeshBinding;
            
            ptr->index_buffer = desc->getIndexBuffer();
            for (auto& it : attrib_table) {
                VFMT::GUID attr_guid = it.first;
                int loc = it.second;

                bool is_instance_array = false;
                const gpuBuffer* buffer = 0;
                int stride = 0;
                const VFMT::ATTRIB_DESC* attrDesc = attrDesc = VFMT::getAttribDesc(attr_guid);
                int lcl_attrib_id = desc->getLocalAttribId(attr_guid);
                if (lcl_attrib_id >= 0) {
                    auto& dsc = desc->getLocalAttribDesc(lcl_attrib_id);
                    buffer = dsc.buffer;
                    stride = dsc.stride;
                } else if (inst_desc && (lcl_attrib_id = inst_desc->getLocalInstanceAttribId(attr_guid)) >= 0) {
                    auto& dsc = inst_desc->getLocalInstanceAttribDesc(lcl_attrib_id);
                    buffer = dsc.buffer;
                    stride = dsc.stride;
                    is_instance_array = true;
                } else {
                    LOG_WARN("gpuMeshDesc or gpuInstancingDesc does not have an attribute required by the shader program: " << attr_guid);
                    continue;
                }   

                gpuAttribBinding binding = { 0 };
                binding.buffer = buffer;
                binding.location = loc;
                binding.count = attrDesc->count;
                binding.gl_type = attrDesc->gl_type;
                binding.normalized = attrDesc->normalized;
                binding.stride = stride;
                binding.is_instance_array = is_instance_array;
                ptr->attribs.push_back(binding);
            }

            it = mesh_bindings.insert(
                std::make_pair(key, std::unique_ptr<gpuMeshBinding>(ptr))
            ).first;
        }
        return it->second.get();
    }
};

// NOTE: Basically glBindVertexArray() if there was an actual VAO
// keeping it this way for now
inline void gpuUseMeshBinding(const gpuMeshBinding* binding) {
    for (auto& a : binding->attribs) {
        if (!a.buffer) {
            assert(false);
            continue;
        }
        a.buffer->bindArray();
        glEnableVertexAttribArray(a.location);
        glVertexAttribPointer(
            a.location, a.count, a.gl_type, a.normalized, a.stride, (void*)0
        );
        if (a.is_instance_array) {
            glVertexAttribDivisor(a.location, 1);
        }
    }
    if (binding->index_buffer) {
        binding->index_buffer->bindIndexArray();
    }
}


#endif
