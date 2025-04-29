#include "gpu_renderable.hpp"

#include "gpu.hpp"
#include "gpu_pipeline.hpp"


void gpuRenderable::enableMaterialTechnique(const char* path, bool value) {
    auto node = gpuGetPipeline()->findNode(path);
    if (!node) {
        assert(false);
        return;
    }
    int pass_count = node->getPassCount();
    gpuPass* passes[32];
    pass_count = node->getPassList(passes, 32);
    for (int i = 0; i < pass_count; ++i) {
        int pass_pipe_idx = passes[i]->getId();
        int pass_mat_idx = material->getPassMaterialIdx(pass_pipe_idx);
        if (pass_mat_idx < 0) {
            continue;
        }
        pass_states[pass_mat_idx] = value;
    }
}

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