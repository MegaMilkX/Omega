#include "gpu_uniform_buffer.hpp"

#include "gpu/gpu.hpp"


gpuUniformBuffer::gpuUniformBuffer(gpuUniformBufferDesc* description)
    : desc(description) {
    buffer.resize(desc->buffer_size);
    memset(buffer.data(), 0, buffer.size());
    gpu_buf.setArrayData(buffer.data(), buffer.size());
}


void gpuUniformBuffer::setValueByOffset(int offset, const void* value, size_t size) {
    memcpy(&buffer[offset], value, gfxm::_min(buffer.size() - offset, size));
    gpu_buf.setArraySubData(value, size, offset);
}
void gpuUniformBuffer::setValue(int location, const void* value, size_t size) {
    assert(desc);
    assert(location >= 0 && location < desc->uniforms.size());
    auto& u = desc->uniforms[location];
    memcpy(&buffer[u.offset], value, gfxm::_min(buffer.size() - u.offset, size));
    gpu_buf.setArraySubData(value, size, u.offset);
}
void gpuUniformBuffer::setInt(int location, int value) {
    assert(desc);
    assert(location >= 0 && location < desc->uniforms.size());
    auto& u = desc->uniforms[location];
    memcpy(&buffer[u.offset], &value, gfxm::_min(buffer.size() - u.offset, sizeof(value)));
    gpu_buf.setArraySubData(&value, sizeof(value), u.offset);
}
void gpuUniformBuffer::setFloat(int location, float value) {
    assert(desc);
    assert(location >= 0 && location < desc->uniforms.size());
    auto& u = desc->uniforms[location];
    memcpy(&buffer[u.offset], &value, gfxm::_min(buffer.size() - u.offset, sizeof(value)));
    gpu_buf.setArraySubData(&value, sizeof(value), u.offset);
}
void gpuUniformBuffer::setVec2(int location, const gfxm::vec2& value) {
    assert(desc);
    assert(location >= 0 && location < desc->uniforms.size());
    auto& u = desc->uniforms[location];
    memcpy(&buffer[u.offset], &value, gfxm::_min(buffer.size() - u.offset, sizeof(value)));
    gpu_buf.setArraySubData(&value, sizeof(value), u.offset);
}
void gpuUniformBuffer::setVec3(int location, const gfxm::vec3& value) {
    assert(desc);
    assert(location >= 0 && location < desc->uniforms.size());
    auto& u = desc->uniforms[location];
    memcpy(&buffer[u.offset], &value, gfxm::_min(buffer.size() - u.offset, sizeof(value)));
    gpu_buf.setArraySubData(&value, sizeof(value), u.offset);
}
void gpuUniformBuffer::setVec4(int location, const gfxm::vec4& value) {
    assert(desc);
    assert(location >= 0 && location < desc->uniforms.size());
    auto& u = desc->uniforms[location];
    memcpy(&buffer[u.offset], &value, gfxm::_min(buffer.size() - u.offset, sizeof(value)));
    gpu_buf.setArraySubData(&value, sizeof(value), u.offset);
}
void gpuUniformBuffer::setMat4(int location, const gfxm::mat4& value) {
    assert(desc);
    assert(location >= 0 && location < desc->uniforms.size());
    auto& u = desc->uniforms[location];
    memcpy(&buffer[u.offset], &value, gfxm::_min(buffer.size() - u.offset, sizeof(value)));
    gpu_buf.setArraySubData(&value, sizeof(value), u.offset);
}

void gpuUniformBuffer::setIntStaging(int location, int value) {
    assert(desc);
    assert(location >= 0 && location < desc->uniforms.size());
    auto& u = desc->uniforms[location];
    memcpy(&buffer[u.offset], &value, gfxm::_min(buffer.size() - u.offset, sizeof(value)));
}
void gpuUniformBuffer::setFloatStaging(int location, float value) {
    assert(desc);
    assert(location >= 0 && location < desc->uniforms.size());
    auto& u = desc->uniforms[location];
    memcpy(&buffer[u.offset], &value, gfxm::_min(buffer.size() - u.offset, sizeof(value)));
}
void gpuUniformBuffer::setVec2Staging(int location, const gfxm::vec2& value) {
    assert(desc);
    assert(location >= 0 && location < desc->uniforms.size());
    auto& u = desc->uniforms[location];
    memcpy(&buffer[u.offset], &value, gfxm::_min(buffer.size() - u.offset, sizeof(value)));
}
void gpuUniformBuffer::setVec3Staging(int location, const gfxm::vec3& value) {
    assert(desc);
    assert(location >= 0 && location < desc->uniforms.size());
    auto& u = desc->uniforms[location];
    memcpy(&buffer[u.offset], &value, gfxm::_min(buffer.size() - u.offset, sizeof(value)));
}
void gpuUniformBuffer::setVec4Staging(int location, const gfxm::vec4& value) {
    assert(desc);
    assert(location >= 0 && location < desc->uniforms.size());
    auto& u = desc->uniforms[location];
    memcpy(&buffer[u.offset], &value, gfxm::_min(buffer.size() - u.offset, sizeof(value)));
}
void gpuUniformBuffer::setMat4Staging(int location, const gfxm::mat4& value) {
    assert(desc);
    assert(location >= 0 && location < desc->uniforms.size());
    auto& u = desc->uniforms[location];
    memcpy(&buffer[u.offset], &value, gfxm::_min(buffer.size() - u.offset, sizeof(value)));
}
void gpuUniformBuffer::upload() {
    gpu_buf.setArrayData(&buffer[0], buffer.size());
}

