#pragma once

#include "gpu_uniform_buffer_desc.hpp"
#include "gpu/gpu_buffer.hpp"


class gpuUniformBuffer {
    gpuUniformBufferDesc* desc;
public:
    gpuBuffer gpu_buf;
    std::vector<char> buffer;

    gpuUniformBuffer(gpuUniformBufferDesc* description)
    : desc(description) {
        buffer.resize(desc->buffer_size);
        memset(buffer.data(), 0, buffer.size());
        gpu_buf.setArrayData(buffer.data(), buffer.size());
    }

    gpuUniformBufferDesc* getDesc() {
        return desc;
    }

    void setInt(int location, int value) {
        auto& u = desc->uniforms[location];
        memcpy(&buffer[u.offset], &value, gfxm::_min(buffer.size() - u.offset, sizeof(value)));
        gpu_buf.setArraySubData(&value, sizeof(value), u.offset);
    }
    void setFloat(int location, float value) {
        auto& u = desc->uniforms[location];
        memcpy(&buffer[u.offset], &value, gfxm::_min(buffer.size() - u.offset, sizeof(value)));
        gpu_buf.setArraySubData(&value, sizeof(value), u.offset);
    }
    void setVec2(int location, const gfxm::vec2& value) {
        auto& u = desc->uniforms[location];
        memcpy(&buffer[u.offset], &value, gfxm::_min(buffer.size() - u.offset, sizeof(value)));
        gpu_buf.setArraySubData(&value, sizeof(value), u.offset);
    }
    void setVec3(int location, const gfxm::vec3& value) {
        auto& u = desc->uniforms[location];
        memcpy(&buffer[u.offset], &value, gfxm::_min(buffer.size() - u.offset, sizeof(value)));
        gpu_buf.setArraySubData(&value, sizeof(value), u.offset);
    }
    void setMat4(int location, const gfxm::mat4& value) {
        auto& u = desc->uniforms[location];
        memcpy(&buffer[u.offset], &value, gfxm::_min(buffer.size() - u.offset, sizeof(value)));
        gpu_buf.setArraySubData(&value, sizeof(value), u.offset);
    }
};