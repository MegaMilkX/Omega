#pragma once

#include "scn_render_object.hpp"

class scnSkin : public scnRenderObject {

    friend scnRenderScene;

    gpuBuffer* bufVertexSource = 0;
    gpuBuffer* bufNormalSource = 0;
    gpuBuffer* bufBoneIndex4 = 0;
    gpuBuffer* bufBoneWeight4 = 0;

    gpuBuffer   vertexBuffer;
    gpuBuffer   normalBuffer;
    gpuMeshDesc skin_mesh_desc;

    std::vector<int>        bone_indices;
    std::vector<gfxm::mat4> inverse_bind_transforms;
    std::vector<gfxm::mat4> pose_transforms;

    scnSkeleton* skeleton = 0;

    void updatePoseTransforms() {
        for (int i = 0; i < pose_transforms.size(); ++i) {
            int bone_idx = bone_indices[i];
            auto& inverse_bind = inverse_bind_transforms[i];
            auto& world = skeleton->world_transforms[bone_idx];
            pose_transforms[i] = world * inverse_bind;
        }
    }

    void onAdded() override {
        getRenderable(0)->compile();
        pose_transforms.resize(bone_indices.size());
    }
    void onRemoved() override {

    }
public:
    TYPE_ENABLE();
    scnSkin() {
        addRenderable(new gpuRenderable);
        getRenderable(0)->attachUniformBuffer(ubuf_model);
    }
    void setMeshDesc(const gpuMeshDesc* desc) {
        const gpuMeshDesc::AttribDesc* posDesc = desc->getAttribDesc(VFMT::Position_GUID);
        const gpuMeshDesc::AttribDesc* normDesc = desc->getAttribDesc(VFMT::Normal_GUID);
        const gpuMeshDesc::AttribDesc* boneIdxDesc = desc->getAttribDesc(VFMT::BoneIndex4_GUID);
        const gpuMeshDesc::AttribDesc* boneWeightDesc = desc->getAttribDesc(VFMT::BoneWeight4_GUID);
        
        if (!posDesc) {
            assert(false);
            return;
        }

        bufVertexSource = const_cast<gpuBuffer*>(posDesc->buffer);
        bufNormalSource = const_cast<gpuBuffer*>(normDesc->buffer);
        bufBoneIndex4 = const_cast<gpuBuffer*>(boneIdxDesc->buffer);
        bufBoneWeight4 = const_cast<gpuBuffer*>(boneWeightDesc->buffer);

        int vertex_count = desc->getVertexCount();

        vertexBuffer.reserve(vertex_count * sizeof(gfxm::vec3), GL_DYNAMIC_DRAW);
        normalBuffer.reserve(vertex_count * sizeof(gfxm::vec3), GL_DYNAMIC_DRAW);

        skin_mesh_desc.clear();
        skin_mesh_desc.setAttribArray(VFMT::Position_GUID, &vertexBuffer, 0);
        skin_mesh_desc.setAttribArray(VFMT::Normal_GUID, &normalBuffer, 0);

        desc->merge(&skin_mesh_desc, false);
        skin_mesh_desc.setVertexCount(desc->getVertexCount());

        getRenderable(0)->setMeshDesc(&skin_mesh_desc);
    }
    void setSkeleton(scnSkeleton* skeleton) {
        this->skeleton = skeleton;
    }
    void setBoneIndices(int* indices, int count) {
        bone_indices.clear();
        bone_indices.insert(bone_indices.end(), indices, indices + count);        
    }
    void setInverseBindTransforms(const gfxm::mat4* transforms, int count) {
        inverse_bind_transforms.clear();
        inverse_bind_transforms.insert(inverse_bind_transforms.end(), transforms, transforms + count);
    }
    void setMaterial(gpuMaterial* mat) {
        getRenderable(0)->setMaterial(mat);
    }

    static void reflect() {
        type_register<scnSkin>("scnSkin")
            .parent<scnRenderObject>();
    }
};