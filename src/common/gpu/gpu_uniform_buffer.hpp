#pragma once

#include "gpu_uniform_buffer_desc.hpp"
#include "gpu/gpu_buffer.hpp"


class gpuUniformBuffer {
    gpuUniformBufferDesc* desc;
    void write(int location, const void* src, size_t elem_size, int count, int index, bool upload_now);
public:
    gpuBuffer gpu_buf;
    std::vector<char> buffer;

    gpuUniformBuffer(gpuUniformBufferDesc* description);
    virtual ~gpuUniformBuffer() = default;

    gpuUniformBufferDesc* getDesc() { return desc; }

    void setValueByOffset(int offset, const void* value, size_t size);
    void setValue(int location, const void* value, size_t size);

    void setInt(int location, int value, int index = 0);
    void setFloat(int location, float value, int index = 0);
    void setVec2(int location, const gfxm::vec2& value, int index = 0);
    void setVec3(int location, const gfxm::vec3& value, int index = 0);
    void setVec4(int location, const gfxm::vec4& value, int index = 0);
    void setMat4(int location, const gfxm::mat4& value, int index = 0);

    // Experimental
    void setIntStaging(int location, const int* value, int count = 1, int start = 0);
    void setFloatStaging(int location, const float* value, int count = 1, int start = 0);
    void setVec2Staging(int location, const gfxm::vec2* value, int count = 1, int start = 0);
    void setVec3Staging(int location, const gfxm::vec3* value, int count = 1, int start = 0);
    void setVec4Staging(int location, const gfxm::vec4* value, int count = 1, int start = 0);
    void setMat4Staging(int location, const gfxm::mat4* value, int count = 1, int start = 0);
    void upload();

    // For serialization
    template<typename T>
    T getValue(int loc) {
        auto& u = desc->uniforms[loc];
        return *(T*)&buffer[u.offset];
    }
};

