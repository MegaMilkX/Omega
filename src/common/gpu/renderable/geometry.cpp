#include "geometry.hpp"

#include "gpu/gpu.hpp"
#include "resource_manager/resource_manager.hpp"

#include "gpu/param_block/transform_block.hpp"


gpuGeoRenderable::gpuGeoRenderable() {
    setRole(GPU_Role_Geometry);

    // TODO:
    transform_block = gpuGetDevice()->createParamBlock<gpuTransformBlock>();
    attachParamBlock(transform_block);
}
gpuGeoRenderable::gpuGeoRenderable(gpuMaterial* mat, const gpuMeshDesc* mesh, const gpuInstancingDesc* instancing, const char* dbg_name) {
    setMaterial(mat);
    setMeshDesc(mesh);
    setInstancingDesc(instancing);
    this->dbg_name = dbg_name;

    setRole(GPU_Role_Geometry);

    transform_block = gpuGetDevice()->createParamBlock<gpuTransformBlock>();
    attachParamBlock(transform_block);

    compile();
}
gpuGeoRenderable::~gpuGeoRenderable() {
    gpuGetDevice()->destroyParamBlock(transform_block);
}

