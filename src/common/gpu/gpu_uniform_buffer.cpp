#include "gpu_uniform_buffer.hpp"

#include "gpu/gpu.hpp"


gpuDecalUniformBuffer::gpuDecalUniformBuffer()
    : gpuUniformBuffer(gpuGetPipeline()->getUniformBufferDesc(UNIFORM_BUFFER_DECAL))
{
    loc_color = getDesc()->getUniform("RGBA");
    loc_size = getDesc()->getUniform("boxSize");
}