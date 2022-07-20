#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include <map>
#include <string>
#include <vector>
#include <set>

#include "gpu/gpu_buffer.hpp"
#include "gpu/gpu_mesh_desc.hpp"
#include "gpu/gpu_material.hpp"
#include "gpu/gpu_renderable.hpp"
#include "gpu/gpu_pipeline.hpp"
#include "gpu/render/uniform.hpp"

#include "mesh3d/mesh3d.hpp"
#include "log/log.hpp"
#include "animation/animation.hpp"
#include "game/animator/animation_sampler.hpp"

#include "gpu/skinning/skinning_compute.hpp"

#include "gpu/gpu.hpp"


struct ImportedScene {
    std::vector<Mesh3d> meshes;
    std::vector<gfxm::mat4> local_transforms;
    std::vector<gfxm::mat4> world_transforms;
    std::vector<int> parents;
    std::map<std::string, int> node_name_to_index;

    struct MeshInstance {
        int transform_id;
        int mesh_id;
    };
    std::vector<MeshInstance> mesh_instances;
};

class AssimpImporter {
    const aiScene* ai_scene = 0;
    double fbxScaleFactor = 1.0f;
    std::set<std::string> second_to_root_nodes;
public:
    ~AssimpImporter() {
        aiReleaseImport(ai_scene);
    }

    bool loadFile(const char* fname) {
        ai_scene = aiImportFile(
            fname,
            aiProcess_GenSmoothNormals |
            aiProcess_GenUVCoords |
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_LimitBoneWeights |
            aiProcess_GlobalScale
        );
        if (!ai_scene) {
            LOG_WARN("Failed to load scene " << fname);
            assert(false);
            return false;
        }

        ai_scene = aiApplyPostProcessing(ai_scene, aiProcess_CalcTangentSpace);
        if (!ai_scene) {
            LOG_WARN("aiApplyPostProcessing failed");
            assert(false);
            return false;
        }
        if (ai_scene->mMetaData && ai_scene->mMetaData->Get("UnitScaleFactor", fbxScaleFactor)) {
            if (fbxScaleFactor == .0) fbxScaleFactor = 1.0;
            fbxScaleFactor *= .01;
        }

        // Keep a list of immediate root children to fix scaling
        auto ai_root = ai_scene->mRootNode;
        for (int i = 0; i < ai_root->mNumChildren; ++i) {
            auto ai_node = ai_root->mChildren[i];
            second_to_root_nodes.insert(ai_node->mName.C_Str());
        }

        return true;
    }
    void loadMemFile(void* data, size_t sz, const char* format_hint) {
        ai_scene = aiImportFileFromMemory(
            (const char*)data, (unsigned int)sz,
            aiProcess_GenSmoothNormals |
            aiProcess_GenUVCoords |
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_LimitBoneWeights |
            aiProcess_GlobalScale,
            format_hint
        );
        // TODO
        assert(false);
    }
    /*
    void importSkeleton(Skeleton* skeleton) {
        assert(ai_scene);
        
        auto ai_root = ai_scene->mRootNode;
        int parent = -1;
        aiNode* ai_node = ai_root;
        std::queue<aiNode*> ai_node_queue;
        std::queue<int> parent_queue;
        while (ai_node) {
            // Ignore any node that only contains skinned meshes
            // If it has at least one non-skinned mesh - keep it
            // If no meshes at all - also keep
            bool skip_node = ai_node->mNumMeshes != 0;
            for (int i = 0; i < ai_node->mNumMeshes; ++i) {
                if (ai_scene->mMeshes[ai_node->mMeshes[i]]->mNumBones == 0) {
                    skip_node = false;
                    break;
                }
            }
            if (skip_node) {
                if (ai_node_queue.empty()) {
                    ai_node = 0;
                } else {
                    ai_node = ai_node_queue.front();
                    ai_node_queue.pop();
                    parent = parent_queue.front();
                    parent_queue.pop();
                }
                continue;
            }

            for (int i = 0; i < ai_node->mNumChildren; ++i) {
                auto ai_child_node = ai_node->mChildren[i];
                ai_node_queue.push(ai_child_node);
                parent_queue.push(skeleton->default_pose.size());
            }

            std::string node_name(ai_node->mName.data, ai_node->mName.length);
            LOG(node_name);

            skeleton->node_name_to_index[node_name] = skeleton->parents.size();

            aiVector3D   ai_translation;
            aiQuaternion ai_rotation;
            aiVector3D   ai_scale;
            ai_node->mTransformation.Decompose(ai_scale, ai_rotation, ai_translation);
            gfxm::vec3 translation(ai_translation.x, ai_translation.y, ai_translation.z);
            gfxm::quat rotation(ai_rotation.x, ai_rotation.y, ai_rotation.z, ai_rotation.w);
            gfxm::vec3 scale(ai_scale.x, ai_scale.y, ai_scale.z);
            if (parent == 0) {
                translation = translation * fbxScaleFactor;
                scale = scale * fbxScaleFactor;
            }
            gfxm::transform transform;
            transform.translate(translation);
            transform.rotation(rotation);
            transform.scale(scale);

            skeleton->parents.push_back(parent);
            if (parent == -1) {
                skeleton->default_pose.push_back(gfxm::scale(gfxm::mat4(1.0f), gfxm::vec3(fbxScaleFactor, fbxScaleFactor, fbxScaleFactor)));
            } else {
                skeleton->default_pose.push_back(transform.matrix());
            }
            if (ai_node_queue.empty()) {
                ai_node = 0;
            } else {
                ai_node = ai_node_queue.front();
                ai_node_queue.pop();
                parent = parent_queue.front();
                parent_queue.pop();
            }
        }
        skeleton->default_pose[0] = gfxm::mat4(1.0f);
    }*/
    /*
    void importSkinModel(const Skeleton* skeleton, gpuSkinModel* out) {
        out->skeleton = const_cast<Skeleton*>(skeleton);

        struct MeshData {
            std::vector<uint32_t>       indices;

            std::vector<gfxm::vec3>     vertices;
            std::vector<unsigned char>  colorsRGB;
            std::vector<gfxm::vec2>     uvs;
            std::vector<gfxm::vec3>     normals;
            
            std::vector<gfxm::ivec4>    bone_indices;
            std::vector<gfxm::vec4>     bone_weights;
            std::vector<gfxm::mat4>     inverse_bind_transforms;
            std::vector<gfxm::mat4>     pose_transforms;
            std::vector<int>            bone_transform_source_indices;
        };
        auto readMeshData = [](const Skeleton* skeleton, const aiMesh* ai_mesh, MeshData* out) {
            std::vector<uint32_t>&       indices = out->indices;

            std::vector<gfxm::vec3>&     vertices = out->vertices;
            std::vector<unsigned char>&  colorsRGB = out->colorsRGB;
            std::vector<gfxm::vec2>&     uvs = out->uvs;
            std::vector<gfxm::vec3>&     normals = out->normals;

            std::vector<gfxm::ivec4>&    bone_indices = out->bone_indices;
            std::vector<gfxm::vec4>&     bone_weights = out->bone_weights;
            std::vector<gfxm::mat4>&     inverse_bind_transforms = out->inverse_bind_transforms;
            std::vector<gfxm::mat4>&     pose_transforms = out->pose_transforms;
            std::vector<int>&            bone_transform_source_indices = out->bone_transform_source_indices;

            for (int iv = 0; iv < ai_mesh->mNumVertices; ++iv) {
                auto ai_vert = ai_mesh->mVertices[iv];
                auto ai_norm = ai_mesh->mNormals[iv];
                auto ai_uv = ai_mesh->mTextureCoords[0][iv];
                out->vertices.push_back(gfxm::vec3(ai_vert.x, ai_vert.y, ai_vert.z));
                out->normals.push_back(gfxm::vec3(ai_norm.x, ai_norm.y, ai_norm.z));
                out->uvs.push_back(gfxm::vec2(ai_uv.x, ai_uv.y));
            }
            if (ai_mesh->GetNumColorChannels() == 0) {
                out->colorsRGB = std::vector<unsigned char>(ai_mesh->mNumVertices * 3, 255);
            } else {
                out->colorsRGB.resize(ai_mesh->mNumVertices * 3);
                for (int iv = 0; iv < ai_mesh->mNumVertices; ++iv) {
                    auto ai_col = ai_mesh->mColors[0][iv];
                    out->colorsRGB[iv * 3] = ai_col.r * 255.0f;
                    out->colorsRGB[iv * 3 + 1] = ai_col.g * 255.0f;
                    out->colorsRGB[iv * 3 + 2] = ai_col.b * 255.0f;
                }
            }

            for (int f = 0; f < ai_mesh->mNumFaces; ++f) {
                auto ai_face = ai_mesh->mFaces[f];
                for (int ind = 0; ind < ai_face.mNumIndices; ++ind) {
                    out->indices.push_back(ai_face.mIndices[ind]);
                }
            }

            if (ai_mesh->HasBones()) {
                bone_indices.resize(vertices.size(), gfxm::ivec4(0, 0, 0, 0));
                bone_weights.resize(vertices.size(), gfxm::vec4(.0f, .0f, .0f, .0f));

                inverse_bind_transforms.resize(ai_mesh->mNumBones);
                pose_transforms.resize(ai_mesh->mNumBones);
                bone_transform_source_indices.resize(ai_mesh->mNumBones);

                for (int j = 0; j < ai_mesh->mNumBones; ++j) {
                    auto& ai_bone_info = ai_mesh->mBones[j];
                    std::string bone_name(ai_bone_info->mName.data, ai_bone_info->mName.length);

                    inverse_bind_transforms[j] = gfxm::transpose(*(gfxm::mat4*)&ai_bone_info->mOffsetMatrix);

                    {
                        int bone_index = skeleton->findBone(bone_name.c_str());
                        if (bone_index < 0) {
                            assert(false);
                        }
                        bone_transform_source_indices[j] = bone_index;
                    }

                    for (int k = 0; k < ai_bone_info->mNumWeights; ++k) {
                        auto ai_vert_weight = ai_bone_info->mWeights[k];
                        auto vert_id = ai_vert_weight.mVertexId;
                        float weight = ai_vert_weight.mWeight;

                        for (int c = 0; c < 4; ++c) {
                            if (bone_weights[vert_id][c] == .0f) {
                                bone_indices[vert_id][c] = j;
                                bone_weights[vert_id][c] = weight;
                                break;
                            }
                        }
                    }
                }
                // Normalize
                for (int j = 0; j < bone_weights.size(); ++j) {
                    float sum =
                        bone_weights[j].x +
                        bone_weights[j].y +
                        bone_weights[j].z +
                        bone_weights[j].w;
                    if (sum == .0f) {
                        //continue;
                    }
                    bone_weights[j] /= sum;
                }
            }
        };
        
        for (int i = 0; i < ai_scene->mNumMeshes; ++i) {
            auto ai_mesh = ai_scene->mMeshes[i];
            int mesh_id = out->meshes.size();
            out->meshes.push_back(std::unique_ptr<gpuMesh>(new gpuMesh));
            gpuMesh* gpu_mesh = out->meshes.back().get();

            MeshData md;
            readMeshData(skeleton, ai_mesh, &md);
            Mesh3d m3d;
            m3d.setIndexArray(md.indices.data(), md.indices.size() * sizeof(md.indices[0]));
            m3d.setAttribArray(VFMT::Position_GUID, md.vertices.data(), md.vertices.size() * sizeof(md.vertices[0]));
            m3d.setAttribArray(VFMT::Normal_GUID, md.normals.data(), md.normals.size() * sizeof(md.normals[0]));
            m3d.setAttribArray(VFMT::ColorRGB_GUID, md.colorsRGB.data(), md.colorsRGB.size() * sizeof(md.colorsRGB[0]));
            m3d.setAttribArray(VFMT::UV_GUID, md.uvs.data(), md.uvs.size() * sizeof(md.uvs[0]));
            if (!md.bone_indices.empty() && !md.bone_weights.empty()) {
                m3d.setAttribArray(VFMT::BoneIndex4_GUID, md.bone_indices.data(), md.bone_indices.size() * sizeof(md.bone_indices[0]));
                m3d.setAttribArray(VFMT::BoneWeight4_GUID, md.bone_weights.data(), md.bone_weights.size() * sizeof(md.bone_weights[0]));

                out->skin_meshes.push_back(SkinMesh());
                auto& m = out->skin_meshes.back();
                m.mesh_id = mesh_id;
                m.bone_indices = md.bone_transform_source_indices;
                m.inverse_bind_transforms = md.inverse_bind_transforms;
            }

            gpu_mesh->setData(&m3d);
            gpu_mesh->setDrawMode(MESH_DRAW_TRIANGLES);
        }
        
        auto ai_root = ai_scene->mRootNode;
        aiNode* ai_node = ai_root;
        std::queue<aiNode*> ai_node_queue;
        while (ai_node) {
            for (int i = 0; i < ai_node->mNumMeshes; ++i) {
                auto ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];
                if (ai_mesh->mNumBones > 0) {
                    continue;
                }   

                std::string mesh_name(ai_mesh->mName.data, ai_mesh->mName.length);
                LOG("Simple mesh: " << mesh_name);

                int bone_index = skeleton->findBone(ai_node->mName.C_Str());
                if (bone_index < 0) {
                    assert(false);
                    bone_index = 0;
                }

                out->simple_meshes.push_back(SimpleMesh());
                auto& out_mesh = out->simple_meshes.back();
                out_mesh.bone_id = bone_index;
                out_mesh.mesh_id = ai_node->mMeshes[i];
            }

            for (int i = 0; i < ai_node->mNumChildren; ++i) {
                auto ai_child_node = ai_node->mChildren[i];
                ai_node_queue.push(ai_child_node);
            }
            
            if (ai_node_queue.empty()) {
                ai_node = 0;
            } else {
                ai_node = ai_node_queue.front();
                ai_node_queue.pop();
            }
        }
    }*/

    int animationCount() const {
        return ai_scene->mNumAnimations;
    }
    void importAnimation(Skeleton* skeleton, const char* name, Animation* anim, const char* root_motion_track = "") {
        for (int j = 0; j < ai_scene->mNumAnimations; ++j) {
            aiAnimation* ai_anim = ai_scene->mAnimations[j];
            if (strcmp(ai_anim->mName.C_Str(), name) == 0) {
                importAnimation(skeleton, j, anim, root_motion_track);
                return;
            }
        }
        LOG_ERR("Animation " << name << " not found");
        assert(false);
        return;
    }
    void importAnimation(Skeleton* skeleton, int id, Animation* anim, const char* root_motion_track_name = "") {
        aiAnimation* ai_anim = ai_scene->mAnimations[id];
        LOG(ai_anim->mName.C_Str());
        anim->length = (float)ai_anim->mDuration;
        anim->fps = (float)ai_anim->mTicksPerSecond;

        for (int j = 0; j < ai_anim->mNumChannels; ++j) {
            aiNodeAnim* ai_node_anim = ai_anim->mChannels[j];

            AnimNode& anim_node = anim->createNode(ai_node_anim->mNodeName.C_Str());
            float scaleFactorFix = 1.0f;
            if (second_to_root_nodes.find(ai_node_anim->mNodeName.C_Str()) != second_to_root_nodes.end()) {
                scaleFactorFix = (float)fbxScaleFactor;
            }
            for (int k = 0; k < ai_node_anim->mNumPositionKeys; ++k) {
                auto& ai_v = ai_node_anim->mPositionKeys[k].mValue;
                float time = (float)ai_node_anim->mPositionKeys[k].mTime;
                gfxm::vec3 v = { ai_v.x, ai_v.y, ai_v.z };
                anim_node.t[time] = v * scaleFactorFix;
            }
            for (int k = 0; k < ai_node_anim->mNumRotationKeys; ++k) {
                auto& ai_v = ai_node_anim->mRotationKeys[k].mValue;
                float time = (float)ai_node_anim->mRotationKeys[k].mTime;
                gfxm::quat v = { ai_v.x, ai_v.y, ai_v.z, ai_v.w };
                anim_node.r[time] = v;
            }
            for (int k = 0; k < ai_node_anim->mNumScalingKeys; ++k) {
                auto& ai_v = ai_node_anim->mScalingKeys[k].mValue;
                float time = (float)ai_node_anim->mScalingKeys[k].mTime;
                gfxm::vec3 v = { ai_v.x, ai_v.y, ai_v.z };
                anim_node.s[time] = v * scaleFactorFix;
            }
        }

        // Sample root motion keyframes
        /*
        if (strlen(root_motion_track_name) > 0) {
            int rm_bone_id = skeleton->findBone(root_motion_track_name);
            int parent = skeleton->parents[rm_bone_id];
            std::vector<int> chain;
            chain.push_back(rm_bone_id);
            while (parent != -1) {
                chain.push_back(parent);
                parent = skeleton->parents[parent];
            }

            std::vector<int32_t> skeleton_to_anim_mapping(skeleton->default_pose.size());
            for (auto& it : skeleton->node_name_to_index) {
                int anim_node_idx = anim->getNodeIndex(it.first);
                skeleton_to_anim_mapping[it.second] = anim_node_idx;
            }
            AnimationSampler sampler(skeleton, anim);
            std::vector<AnimSample> samples(skeleton->default_pose.size());
            AnimNode root_motion_node;
            for (int i = 0; i < (int)anim->length; ++i) {
                sampler.sample(samples.data(), samples.size(), (float)i);
                std::vector<gfxm::mat4> pose = skeleton->default_pose;
                for (int j = 0; j < skeleton_to_anim_mapping.size(); ++j) {
                    if (skeleton_to_anim_mapping[j] < 0) {
                        continue;
                    }
                    auto& s = samples[j];
                    pose[j]
                        = gfxm::translate(gfxm::mat4(1.0f), s.t)
                        * gfxm::to_mat4(s.r)
                        * gfxm::scale(gfxm::mat4(1.0f), s.s);
                }
                
                gfxm::mat4 m(1.0f);
                for (int j = chain.size() - 1; j >= 0; --j) {
                    m = m * pose[chain[j]];
                }

                gfxm::vec3 wt = m * gfxm::vec4(0, 0, 0, 1);
                gfxm::quat wr = gfxm::to_quat(gfxm::to_orient_mat3(m));
                gfxm::vec3 ws = gfxm::vec3(
                    gfxm::length(gfxm::vec3(m[0])),
                    gfxm::length(gfxm::vec3(m[1])),
                    gfxm::length(gfxm::vec3(m[2]))
                );
                root_motion_node.t[(float)i] = wt;
                root_motion_node.r[(float)i] = wr;
                root_motion_node.s[(float)i] = ws;
            }
            anim->addRootMotionNode(root_motion_node);

            // Clear all root motion source keyframes except for the first one
            auto rm_source = anim->getNode(root_motion_track_name);
            rm_source->t.get_keyframes().resize(1);
            rm_source->r.get_keyframes().resize(1);
            rm_source->s.get_keyframes().resize(1);
        }*/
    }
};

inline void assimpLoadScene(const char* fname, ImportedScene& output) {
    LOG("Importing scene " << fname << "...");

    const aiScene* ai_scene = aiImportFile(
        fname,
        aiProcess_GenSmoothNormals |
        aiProcess_GenUVCoords |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_LimitBoneWeights |
        aiProcess_GlobalScale
    );
    if (!ai_scene) {
        LOG_WARN("Failed to load scene " << fname);
        return;
    }
    auto mesh_count = ai_scene->mNumMeshes;
    LOG("Mesh count: " << mesh_count);
    auto material_count = ai_scene->mNumMaterials;
    LOG("Material count: " << material_count);
    auto anim_count = ai_scene->mNumAnimations;
    LOG("Anim count: " << anim_count);

    double fbxScaleFactor = 1.0f;
    ai_scene = aiApplyPostProcessing(ai_scene, aiProcess_CalcTangentSpace);
    if (!ai_scene) {
        LOG_WARN("aiApplyPostProcessing failed");
    }
    if (ai_scene->mMetaData && ai_scene->mMetaData->Get("UnitScaleFactor", fbxScaleFactor)) {
        if (fbxScaleFactor == .0) fbxScaleFactor = 1.0;
        fbxScaleFactor *= .01;
    }

    std::map<std::string, int> node_name_to_index;
    std::vector<void*> nodes; // TODO: void* temporarily
    std::set<std::string> second_to_root_nodes;

    // ===
    // Nodes
    // ===
    auto ai_root = ai_scene->mRootNode;
    int parent = -1;
    aiNode* ai_node = ai_root;
    std::queue<aiNode*> ai_node_queue;
    std::queue<int> parent_queue;
    while (ai_node) {
        for (int i = 0; i < ai_node->mNumChildren; ++i) {
            auto ai_child_node = ai_node->mChildren[i];
            ai_node_queue.push(ai_child_node);
            parent_queue.push(output.world_transforms.size());
        }

        std::string node_name(ai_node->mName.data, ai_node->mName.length);
        //LOG(node_name);
        if (parent == 0) {
            second_to_root_nodes.insert(node_name);
        }

        node_name_to_index[node_name] = nodes.size();
        nodes.push_back(0/* TODO */);

        aiVector3D   ai_translation;
        aiQuaternion ai_rotation;
        aiVector3D   ai_scale;
        ai_node->mTransformation.Decompose(ai_scale, ai_rotation, ai_translation);
        gfxm::vec3 translation(ai_translation.x, ai_translation.y, ai_translation.z);
        gfxm::quat rotation(ai_rotation.x, ai_rotation.y, ai_rotation.z, ai_rotation.w);
        gfxm::vec3 scale(ai_scale.x, ai_scale.y, ai_scale.z);
        if (parent == 0) {
            translation = translation * fbxScaleFactor;
            scale = scale * fbxScaleFactor;
        }
        gfxm::transform transform;
        transform.translate(translation);
        transform.rotation(rotation);
        transform.scale(scale);

        for (int i = 0; i < ai_node->mNumMeshes; ++i) {
            ImportedScene::MeshInstance instance = { 0 };
            instance.mesh_id = ai_node->mMeshes[i];
            instance.transform_id = output.world_transforms.size();
            output.mesh_instances.push_back(instance);
        }

        output.parents.push_back(parent);
        if (parent == -1) {
            output.local_transforms.push_back(gfxm::scale(gfxm::mat4(1.0f), gfxm::vec3(fbxScaleFactor, fbxScaleFactor, fbxScaleFactor)));
            output.world_transforms.push_back(gfxm::mat4(1.0f));
        }
        else {
            output.local_transforms.push_back(transform.matrix());
            output.world_transforms.push_back(gfxm::mat4(1.0f));
        }
        if (ai_node_queue.empty()) {
            ai_node = 0;
        }
        else {
            ai_node = ai_node_queue.front();
            ai_node_queue.pop();
            parent = parent_queue.front();
            parent_queue.pop();
        }
    }
    output.node_name_to_index = node_name_to_index;

    output.world_transforms[0] = gfxm::mat4(1.0f);// gfxm::scale(gfxm::mat4(1.0f), gfxm::vec3(fbxScaleFactor, fbxScaleFactor, fbxScaleFactor));
    for (int i = 1; i < output.world_transforms.size(); ++i) {
        int pid = output.parents[i];
        output.world_transforms[i] = output.world_transforms[pid] * output.local_transforms[i];
    }

    // ===
    // Meshes
    // ===
    output.meshes.resize(ai_scene->mNumMeshes);
    for (int i = 0; i < ai_scene->mNumMeshes; ++i) {
        auto ai_mesh = ai_scene->mMeshes[i];
        std::string mesh_name(ai_mesh->mName.data, ai_mesh->mName.length);
        LOG("Mesh: " << mesh_name);

        auto& out_mesh = output.meshes[i];

        std::vector<gfxm::vec3> vertices;
        std::vector<unsigned char> colorsRGB;
        std::vector<gfxm::vec2> uvs;
        std::vector<gfxm::vec3> normals;
        std::vector<uint32_t> indices;

        for (int iv = 0; iv < ai_mesh->mNumVertices; ++iv) {
            auto ai_vert = ai_mesh->mVertices[iv];
            auto ai_norm = ai_mesh->mNormals[iv];
            auto ai_uv = ai_mesh->mTextureCoords[0][iv];
            vertices.push_back(gfxm::vec3(ai_vert.x, ai_vert.y, ai_vert.z));
            normals.push_back(gfxm::vec3(ai_norm.x, ai_norm.y, ai_norm.z));
            uvs.push_back(gfxm::vec2(ai_uv.x, ai_uv.y));
        }
        colorsRGB.resize(vertices.size() * 3);
        memset(colorsRGB.data(), 255, colorsRGB.size() * sizeof(colorsRGB[0]));
        for (int f = 0; f < ai_mesh->mNumFaces; ++f) {
            auto ai_face = ai_mesh->mFaces[f];
            for (int ind = 0; ind < ai_face.mNumIndices; ++ind) {
                indices.push_back(ai_face.mIndices[ind]);
            }
        }

        out_mesh.setAttribArray(VFMT::Position_GUID, vertices.data(), vertices.size() * sizeof(vertices[0]));
        out_mesh.setAttribArray(VFMT::ColorRGB_GUID, colorsRGB.data(), colorsRGB.size() * sizeof(colorsRGB[0]));
        out_mesh.setAttribArray(VFMT::Normal_GUID, normals.data(), normals.size() * sizeof(normals[0]));
        out_mesh.setAttribArray(VFMT::UV_GUID, uvs.data(), uvs.size() * sizeof(uvs[0]));
        out_mesh.setIndexArray(indices.data(), indices.size() * sizeof(indices[0]));

        if (ai_mesh->HasBones()) {
            std::vector<gfxm::ivec4> bone_indices;
            std::vector<gfxm::vec4> bone_weights;
            std::vector<gfxm::mat4> inverse_bind_transforms;
            std::vector<gfxm::mat4> pose_transforms;
            std::vector<int>        bone_transform_source_indices;
            
            bone_indices.resize(vertices.size(), gfxm::ivec4(0, 0, 0, 0));
            bone_weights.resize(vertices.size(), gfxm::vec4(.0f, .0f, .0f, .0f));

            inverse_bind_transforms.resize(ai_mesh->mNumBones);
            pose_transforms.resize(ai_mesh->mNumBones);
            bone_transform_source_indices.resize(ai_mesh->mNumBones);
            
            for (int j = 0; j < ai_mesh->mNumBones; ++j) {
                auto& ai_bone_info = ai_mesh->mBones[j];
                std::string bone_name(ai_bone_info->mName.data, ai_bone_info->mName.length);
                
                inverse_bind_transforms[j] = gfxm::transpose(*(gfxm::mat4*)&ai_bone_info->mOffsetMatrix);
                
                {
                    auto it = node_name_to_index.find(bone_name);
                    assert(it != node_name_to_index.end());
                    int model_node_index = it->second;
                    bone_transform_source_indices[j] = model_node_index;
                    pose_transforms[j] = output.world_transforms[model_node_index] * inverse_bind_transforms[j];
                }
                
                for (int k = 0; k < ai_bone_info->mNumWeights; ++k) {
                    auto ai_vert_weight = ai_bone_info->mWeights[k];
                    auto vert_id = ai_vert_weight.mVertexId;
                    float weight = ai_vert_weight.mWeight;

                    for (int c = 0; c < 4; ++c) {
                        if (bone_weights[vert_id][c] == .0f) {
                            bone_indices[vert_id][c] = j;
                            bone_weights[vert_id][c] = weight;
                            break;
                        }
                    }
                }
            }
            // Normalize
            for (int j = 0; j < bone_weights.size(); ++j) {
                float sum =
                    bone_weights[j].x +
                    bone_weights[j].y +
                    bone_weights[j].z +
                    bone_weights[j].w;
                if (sum == .0f) {
                    //continue;
                }
                bone_weights[j] /= sum;
            }

            out_mesh.setAttribArray(VFMT::BoneIndex4_GUID, bone_indices.data(), bone_indices.size() * sizeof(bone_indices[0]));
            out_mesh.setAttribArray(VFMT::BoneWeight4_GUID, bone_weights.data(), bone_weights.size() * sizeof(bone_weights[0]));
        }
    }

    // ===
    // Material
    // ===
    for (int i = 0; i < ai_scene->mNumMaterials; ++i) {
        auto ai_material = ai_scene->mMaterials[i];
        aiString path;
        int num_textures = ai_material->GetTextureCount(aiTextureType_DIFFUSE);
        ai_material->GetTexture(aiTextureType_DIFFUSE, 0, &path);
        // TODO:
    }

    // ===
    // Animations
    // ===
    /*
    for (int i = 0; i < ai_scene->mNumAnimations; ++i) {
        aiAnimation* ai_anim = ai_scene->mAnimations[i];
        LOG(ai_anim->mName.C_Str());
        Animation* anim = new Animation();
        anim->length = (float)ai_anim->mDuration;
        anim->fps = (float)ai_anim->mTicksPerSecond;

        for (int j = 0; j < ai_anim->mNumChannels; ++j) {
            aiNodeAnim* ai_node_anim = ai_anim->mChannels[j];

            AnimNode& anim_node = anim->createNode(ai_node_anim->mNodeName.C_Str());
            float scaleFactorFix = 1.0f;
            if (second_to_root_nodes.find(ai_node_anim->mNodeName.C_Str()) != second_to_root_nodes.end()) {
                scaleFactorFix = (float)fbxScaleFactor;
            }
            for (int k = 0; k < ai_node_anim->mNumPositionKeys; ++k) {
                auto& ai_v = ai_node_anim->mPositionKeys[k].mValue;
                float time = (float)ai_node_anim->mPositionKeys[k].mTime;
                gfxm::vec3 v = { ai_v.x, ai_v.y, ai_v.z };
                anim_node.t[time] = v * scaleFactorFix;
            }
            for (int k = 0; k < ai_node_anim->mNumRotationKeys; ++k) {
                auto& ai_v = ai_node_anim->mRotationKeys[k].mValue;
                float time = (float)ai_node_anim->mRotationKeys[k].mTime;
                gfxm::quat v = { ai_v.x, ai_v.y, ai_v.z, ai_v.w };
                anim_node.r[time] = v;
            }
            for (int k = 0; k < ai_node_anim->mNumScalingKeys; ++k) {
                auto& ai_v = ai_node_anim->mScalingKeys[k].mValue;
                float time = (float)ai_node_anim->mScalingKeys[k].mTime;
                gfxm::vec3 v = { ai_v.x, ai_v.y, ai_v.z };
                anim_node.s[time] = v * scaleFactorFix;
            }
        }
        model->animations.push_back(std::unique_ptr<Animation>(anim));
    }*/

    // TODO:

    // Cleanup
    aiReleaseImport(ai_scene);
}