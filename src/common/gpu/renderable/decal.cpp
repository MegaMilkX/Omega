#include "decal.hpp"

#include "gpu/gpu.hpp"
#include "resource_manager/resource_manager.hpp"


gpuDecalRenderable::gpuDecalRenderable() {
    auto shared = gpuGetDevice()->getSharedResources();
    setMeshDesc(shared->getDecalUnitCube()->getMeshDesc());

    setRole(GPU_Role_Decal);

    transform_block = gpuGetPipeline()->getParamBlockContext()->createParamBlock<gpuTransformBlock>();
    attachParamBlock(transform_block);

    decal_block = gpuGetPipeline()->getParamBlockContext()->createParamBlock<gpuDecalBlock>();
    attachParamBlock(decal_block);
}
gpuDecalRenderable::gpuDecalRenderable(gpuMaterial* mat, const gpuInstancingDesc* instancing, const char* dbg_name) {
    auto shared = gpuGetDevice()->getSharedResources();

    setMaterial(mat);
    setMeshDesc(shared->getDecalUnitCube()->getMeshDesc());
    setInstancingDesc(instancing);
    this->dbg_name = dbg_name;

    setRole(GPU_Role_Decal);
    
    transform_block = gpuGetPipeline()->getParamBlockContext()->createParamBlock<gpuTransformBlock>();
    attachParamBlock(transform_block);

    decal_block = gpuGetPipeline()->getParamBlockContext()->createParamBlock<gpuDecalBlock>();
    attachParamBlock(decal_block);

    compile();
}
gpuDecalRenderable::~gpuDecalRenderable() {
    gpuGetPipeline()->getParamBlockContext()->destroyParamBlock(decal_block);
    gpuGetPipeline()->getParamBlockContext()->destroyParamBlock(transform_block);
}

