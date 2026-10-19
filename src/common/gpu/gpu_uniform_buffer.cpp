#include "gpu_uniform_buffer.hpp"

#include "gpu/gpu.hpp"


void gpuUniformBuffer::write(int location, const void* src, size_t elem_size, int count, int index, bool upload_now) {
    assert(desc);
    assert(location >= 0 && location < (int)desc->uniforms.size());
    auto& u = desc->uniforms[location];
    assert(index >= 0 && index < u.count);
    if (index < 0 || index >= u.count) return;
    count = gfxm::_min(count, u.count - index);
    if (count <= 0) return;

    const bool is_array = u.count > 1;
    const size_t stride = is_array ? (size_t)u.stride : elem_size;
    const size_t begin = (size_t)u.offset + (size_t)index * stride;
    size_t end;

    if (stride == elem_size) {
        // Non-array, or contiguous array (vec4, mat4): one copy
        const size_t sz = elem_size * count;
        assert(begin + sz <= buffer.size());
        memcpy(&buffer[begin], src, sz);
        end = begin + sz;
    } else {
        // Padded array (float, int, vec2, vec3): element by element
        const char* s = (const char*)src;
        for (int i = 0; i < count; ++i) {
            assert(begin + i * stride + elem_size <= buffer.size());
            memcpy(&buffer[begin + i * stride], s + i * elem_size, elem_size);
        }
        end = begin + (size_t)(count - 1) * stride + elem_size;
    }

    if (upload_now) {
        gpu_buf.setArraySubData(&buffer[begin], end - begin, begin);
    }
}

gpuUniformBuffer::gpuUniformBuffer(gpuUniformBufferDesc* description)
    : desc(description) {
    buffer.resize(desc->buffer_size);
    memset(buffer.data(), 0, buffer.size());
    gpu_buf.setArrayData(buffer.data(), buffer.size());
}


void gpuUniformBuffer::setValueByOffset(int offset, const void* value, size_t size) {
    assert(offset >= 0 && (size_t)offset <= buffer.size());
    size = gfxm::_min(buffer.size() - offset, size);
    memcpy(&buffer[offset], value, size);
    gpu_buf.setArraySubData(value, size, offset);
}
void gpuUniformBuffer::setValue(int location, const void* value, size_t size) {
    assert(desc);
    assert(location >= 0 && location < (int)desc->uniforms.size());
    setValueByOffset(desc->uniforms[location].offset, value, size);
}

void gpuUniformBuffer::setInt(int location, int value, int index) {
    write(location, &value, sizeof(value), 1, index, true);
}
void gpuUniformBuffer::setFloat(int location, float value, int index) {
    write(location, &value, sizeof(value), 1, index, true);
}
void gpuUniformBuffer::setVec2(int location, const gfxm::vec2& value, int index) {
    write(location, &value, sizeof(value), 1, index, true);
}
void gpuUniformBuffer::setVec3(int location, const gfxm::vec3& value, int index) {
    write(location, &value, sizeof(value), 1, index, true);
}
void gpuUniformBuffer::setVec4(int location, const gfxm::vec4& value, int index) {
    write(location, &value, sizeof(value), 1, index, true);
}
void gpuUniformBuffer::setMat4(int location, const gfxm::mat4& value, int index) {
    write(location, &value, sizeof(value), 1, index, true);
}

void gpuUniformBuffer::setIntStaging(int location, const int* value, int count, int start) {
    write(location, value, sizeof(*value), count, start, false);
}
void gpuUniformBuffer::setFloatStaging(int location, const float* value, int count, int start) {
    write(location, value, sizeof(*value), count, start, false);
}
void gpuUniformBuffer::setVec2Staging(int location, const gfxm::vec2* value, int count, int start) {
    write(location, value, sizeof(*value), count, start, false);
}
void gpuUniformBuffer::setVec3Staging(int location, const gfxm::vec3* value, int count, int start) {
    write(location, value, sizeof(*value), count, start, false);
}
void gpuUniformBuffer::setVec4Staging(int location, const gfxm::vec4* value, int count, int start) {
    write(location, value, sizeof(*value), count, start, false);
}
void gpuUniformBuffer::setMat4Staging(int location, const gfxm::mat4* value, int count, int start) {
    write(location, value, sizeof(*value), count, start, false);
}

void gpuUniformBuffer::upload() {
    gpu_buf.setArrayData(&buffer[0], buffer.size());
}

