#include "gpu_uniform_buffer_desc.hpp"

#include "gpu.hpp"


gpuUniformBufferDesc::gpuUniformBufferDesc(const char* name)
    : block_name(name) {
    binding_location = gpuGetDevice()->getUniformBlockLocation(name);
}

