#include "skin_instance.hpp"


gpuSkinInstance::~gpuSkinInstance() {
    if (skin_task) {
        gpuDestroySkinTask(skin_task);
    }
}
bool gpuSkinInstance::build(const m3dMesh* m3d_mesh, const SkeletonInstance* skl_inst) {
    assert(m3d_mesh->skin);
    if (!m3d_mesh->skin) {
        return false;
    }

    const gpuMesh* gpu_mesh = m3d_mesh->mesh.get();
    const gpuMeshDesc* src_mesh_desc = gpu_mesh->getMeshDesc();
    int vertex_count = src_mesh_desc->getVertexCount();

    {
        m3dSkin* m3d_skin = m3d_mesh->skin.get();
        bone_indices = std::vector<int>(m3d_skin->bone_list.size(), 0);
        for (int i = 0; i < m3d_skin->bone_list.size(); ++i) {
            auto& bone_name = m3d_skin->bone_list[i];
            bone_indices[i] = skl_inst->findBoneIndex(bone_name.c_str());
        }
        pose_transforms.resize(bone_indices.size());
        inv_bind_transforms = m3d_skin->inv_bind_transforms.data();
    }

    // Setup buffers and final mesh desc
    const gpuMeshDesc::AttribDesc* posDesc = src_mesh_desc->getAttribDesc(VFMT::Position_GUID);
    const gpuMeshDesc::AttribDesc* normDesc = src_mesh_desc->getAttribDesc(VFMT::Normal_GUID);
    const gpuMeshDesc::AttribDesc* tangentDesc = src_mesh_desc->getAttribDesc(VFMT::Tangent_GUID);
    const gpuMeshDesc::AttribDesc* bitangentDesc = src_mesh_desc->getAttribDesc(VFMT::Bitangent_GUID);

    const gpuMeshDesc::AttribDesc* boneIdxDesc = src_mesh_desc->getAttribDesc(VFMT::BoneIndex4_GUID);
    const gpuMeshDesc::AttribDesc* boneWeightDesc = src_mesh_desc->getAttribDesc(VFMT::BoneWeight4_GUID);

    vertexBuffer.reserve(vertex_count * sizeof(gfxm::vec3), GL_DYNAMIC_DRAW);
    normalBuffer.reserve(vertex_count * sizeof(gfxm::vec3), GL_DYNAMIC_DRAW);
    tangentBuffer.reserve(vertex_count * sizeof(gfxm::vec3), GL_DYNAMIC_DRAW);
    bitangentBuffer.reserve(vertex_count * sizeof(gfxm::vec3), GL_DYNAMIC_DRAW);

    mesh_desc.clear();
    mesh_desc.setAttribArray(VFMT::Position_GUID, &vertexBuffer, 0);
    mesh_desc.setAttribArray(VFMT::Normal_GUID, &normalBuffer, 0);
    mesh_desc.setAttribArray(VFMT::Tangent_GUID, &tangentBuffer, 0);
    mesh_desc.setAttribArray(VFMT::Bitangent_GUID, &bitangentBuffer, 0);

    src_mesh_desc->merge(&mesh_desc, false);
    mesh_desc.setVertexCount(vertex_count);

    // Setup the skinning task
    skin_task = gpuCreateSkinTask();
    gpuSkinTask* st = skin_task;

    st->vertex_count = vertex_count;

    st->pose_transforms = &pose_transforms[0];
    st->pose_count = pose_transforms.size();

    st->bufVerticesSource = const_cast<gpuBuffer*>(posDesc->buffer);
    st->bufNormalsSource = const_cast<gpuBuffer*>(normDesc->buffer);
    st->bufTangentsSource = const_cast<gpuBuffer*>(tangentDesc->buffer);
    st->bufBitangentsSource = const_cast<gpuBuffer*>(bitangentDesc->buffer);
    st->bufBoneIndices = const_cast<gpuBuffer*>(boneIdxDesc->buffer);
    st->bufBoneWeights = const_cast<gpuBuffer*>(boneWeightDesc->buffer);

    st->bufVerticesOut = &vertexBuffer;
    st->bufNormalsOut = &normalBuffer;
    st->bufTangentsOut = &tangentBuffer;
    st->bufBitangentsOut = &bitangentBuffer;

    st->is_valid = true; // TODO: remove this field, useless
    return true;
}
// Pose matrices are in WORLD space, not MODEL space
// The result of a skinning operation gives a mesh in WORLD space
// Therefore the TransformBlock used while rendering it must be IDENTITY
void gpuSkinInstance::updatePose(SkeletonInstance* skl_inst) {
    for (int i = 0; i < pose_transforms.size(); ++i) {
        int bone_idx = bone_indices[i];
        auto& inverse_bind = inv_bind_transforms[i];
        auto& world = skl_inst->getBoneNode(bone_idx)->getWorldTransform();
        pose_transforms[i] = world * inverse_bind;
    }
}

