#include "gpu_renderable.hpp"

#include "gpu.hpp"


gpuGeometryRenderable::gpuGeometryRenderable(gpuMaterial* mat, const gpuMeshDesc* mesh, const gpuInstancingDesc* instancing, const char* dbg_name)
    : gpuRenderable(mat, mesh, instancing, dbg_name) {
    ubuf_model = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_MODEL);
    loc_transform = ubuf_model->getDesc()->getUniform(UNIFORM_MODEL_TRANSFORM);
    attachUniformBuffer(ubuf_model);
    compile();
}
gpuGeometryRenderable::~gpuGeometryRenderable() {
    gpuGetPipeline()->destroyUniformBuffer(ubuf_model);
}