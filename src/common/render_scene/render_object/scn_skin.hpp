#pragma once

#include "scn_render_object.hpp"
#include "skeleton/skeleton_instance.hpp"

class scnSkin : public scnRenderObject {
    friend scnRenderScene;

    gpuBuffer* bufVertexSource = 0;
    gpuBuffer* bufNormalSource = 0;
    gpuBuffer* bufTangentSource = 0;
    gpuBuffer* bufBitangentSource = 0;
    gpuBuffer* bufBoneIndex4 = 0;
    gpuBuffer* bufBoneWeight4 = 0;

    gpuBuffer   vertexBuffer;
    gpuBuffer   normalBuffer;
    gpuBuffer   tangentBuffer;
    gpuBuffer   bitangentBuffer;
    gpuMeshDesc skin_mesh_desc;

    std::vector<int>        bone_indices;
    std::vector<gfxm::mat4> inverse_bind_transforms;
    std::vector<gfxm::mat4> pose_transforms;

    bool is_valid = true;
    
    SkeletonInstance* skeleton_instance = 0;

    void updatePoseTransforms() {
        for (int i = 0; i < pose_transforms.size(); ++i) {
            int bone_idx = bone_indices[i];
            auto& inverse_bind = inverse_bind_transforms[i];
            auto& world = skeleton_instance->getBoneNode(bone_idx)->getWorldTransform();
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

    bool isValid() const {
        return is_valid;
    }

    bool setSourceMeshDesc(const gpuMeshDesc* desc) {
        const gpuMeshDesc::AttribDesc* posDesc = desc->getAttribDesc(VFMT::Position_GUID);
        const gpuMeshDesc::AttribDesc* normDesc = desc->getAttribDesc(VFMT::Normal_GUID);
        const gpuMeshDesc::AttribDesc* tangentDesc = desc->getAttribDesc(VFMT::Tangent_GUID);
        const gpuMeshDesc::AttribDesc* bitangentDesc = desc->getAttribDesc(VFMT::Bitangent_GUID);
        const gpuMeshDesc::AttribDesc* boneIdxDesc = desc->getAttribDesc(VFMT::BoneIndex4_GUID);
        const gpuMeshDesc::AttribDesc* boneWeightDesc = desc->getAttribDesc(VFMT::BoneWeight4_GUID);
        
        if (!posDesc) {
            LOG_ERR("scnSkin can't be used, missing vertex position data");
            assert(false);
            is_valid = false;
            getRenderable(0)->setMeshDesc(desc);
            return false;
        }
        if (!tangentDesc || !bitangentDesc) {
            LOG_ERR("scnSkin can't be used, missing tangent and/or bitangent data");
            is_valid = false;
            getRenderable(0)->setMeshDesc(desc);
            return false;
        }

        bufVertexSource = const_cast<gpuBuffer*>(posDesc->buffer);
        bufNormalSource = const_cast<gpuBuffer*>(normDesc->buffer);
        bufTangentSource = const_cast<gpuBuffer*>(tangentDesc->buffer);
        bufBitangentSource = const_cast<gpuBuffer*>(bitangentDesc->buffer);
        bufBoneIndex4 = const_cast<gpuBuffer*>(boneIdxDesc->buffer);
        bufBoneWeight4 = const_cast<gpuBuffer*>(boneWeightDesc->buffer);

        int vertex_count = desc->getVertexCount();

        vertexBuffer.reserve(vertex_count * sizeof(gfxm::vec3), GL_DYNAMIC_DRAW);
        normalBuffer.reserve(vertex_count * sizeof(gfxm::vec3), GL_DYNAMIC_DRAW);
        tangentBuffer.reserve(vertex_count * sizeof(gfxm::vec3), GL_DYNAMIC_DRAW);
        bitangentBuffer.reserve(vertex_count * sizeof(gfxm::vec3), GL_DYNAMIC_DRAW);

        skin_mesh_desc.clear();
        skin_mesh_desc.setAttribArray(VFMT::Position_GUID, &vertexBuffer, 0);
        skin_mesh_desc.setAttribArray(VFMT::Normal_GUID, &normalBuffer, 0);
        skin_mesh_desc.setAttribArray(VFMT::Tangent_GUID, &tangentBuffer, 0);
        skin_mesh_desc.setAttribArray(VFMT::Bitangent_GUID, &bitangentBuffer, 0);

        desc->merge(&skin_mesh_desc, false);
        skin_mesh_desc.setVertexCount(desc->getVertexCount());

        getRenderable(0)->setMeshDesc(&skin_mesh_desc);
        return true;
    }
    void setSkeletonInstance(SkeletonInstance* skl) {
        skeleton_instance = skl;
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