#pragma once

#include "gpu_uniform_buffer_desc.hpp"
#include "gpu/gpu_buffer.hpp"


class gpuUniformBuffer {
    gpuUniformBufferDesc* desc;
public:
    gpuBuffer gpu_buf;
    std::vector<char> buffer;

    gpuUniformBuffer(gpuUniformBufferDesc* description);
    virtual ~gpuUniformBuffer() = default;

    gpuUniformBufferDesc* getDesc() { return desc; }

    void setValueByOffset(int offset, const void* value, size_t size);
    void setValue(int location, const void* value, size_t size);
    void setInt(int location, int value);
    void setFloat(int location, float value);
    void setVec2(int location, const gfxm::vec2& value);
    void setVec3(int location, const gfxm::vec3& value);
    void setVec4(int location, const gfxm::vec4& value);
    void setMat4(int location, const gfxm::mat4& value);

    // Experimental
    void setIntStaging(int location, int value);
    void setFloatStaging(int location, float value);
    void setVec2Staging(int location, const gfxm::vec2& value);
    void setVec3Staging(int location, const gfxm::vec3& value);
    void setVec4Staging(int location, const gfxm::vec4& value);
    void setMat4Staging(int location, const gfxm::mat4& value);
    void upload();

    // For serialization
    template<typename T>
    T getValue(int loc) {
        auto& u = desc->uniforms[loc];
        return *(T*)&buffer[u.offset];
    }
};

