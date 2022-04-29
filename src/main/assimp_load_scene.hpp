#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include <map>
#include <string>
#include <vector>
#include <set>

#include "common/render/gpu_buffer.hpp"
#include "common/render/gpu_mesh_desc.hpp"
#include "common/render/gpu_material.hpp"
#include "common/render/gpu_renderable.hpp"
#include "common/render/gpu_pipeline.hpp"
#include "game/render/uniform.hpp"

#include "common/mesh3d/mesh3d.hpp"
#include "common/log/log.hpp"
#include "common/animation/animation.hpp"
#include "game/animator/animation_sampler.hpp"

#include "game/skinning/skinning_compute.hpp"


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

struct Skeleton {
    std::vector<gfxm::mat4> default_pose;
    std::vector<int>        parents;
    std::map<std::string, int> node_name_to_index;

    int findBone(const char* bone_name) const {
        auto it = node_name_to_index.find(bone_name);
        if (it == node_name_to_index.end()) {
            return -1;
        }
        return it->second;
    }

    gfxm::mat4 getDefaultParentWorldTransform(int child) const {
        int parent = parents[child];
        std::vector<int> chain;
        while (parent != -1) {
            chain.push_back(parent);
            parent = parents[parent];
        }
        gfxm::mat4 m(1.0f);
        for (int i = chain.size() - 1; i >= 0; --i) {
            m = default_pose[chain[i]] * m;
        }
        return m;
    }
};
struct SkeletonInstance {
    Skeleton* skeleton;
    std::vector<gfxm::mat4> local_transforms;
    std::vector<gfxm::mat4> world_transforms;

    void init(Skeleton* skel) {
        skeleton = skel;
        local_transforms = skel->default_pose;
        world_transforms = local_transforms;

        world_transforms[0] = gfxm::mat4(1.0f);
        for (int i = 1; i < world_transforms.size(); ++i) {
            int pid = skeleton->parents[i];
            world_transforms[i] = world_transforms[pid] * local_transforms[i];
        }
    }
    void updateBones() {
        for (int i = 1; i < world_transforms.size(); ++i) {
            int pid = skeleton->parents[i];
            world_transforms[i] = world_transforms[pid] * local_transforms[i];
        }
    }
};

// TODO
struct SimpleMesh {
    int bone_id;
    gpuBuffer gpuVertices;
    gpuBuffer gpuNormals;
    gpuBuffer gpuUV;
    gpuBuffer gpuRGB;
    gpuBuffer gpuIndices;

    gpuMeshDesc mesh_desc;
};
struct SimpleMeshInstance {
    SimpleMesh* mesh = 0;
    SkeletonInstance* skel_instance = 0;
    gpuUniformBuffer* ubufModel = 0;
};
// ^ TODO

struct SkinMesh {
    std::vector<gfxm::mat4> inverse_bind_transforms;
    std::vector<int> bone_ids;

    int vertex_count = 0;

    gpuBuffer gpuVertices;
    gpuBuffer gpuNormals;
    gpuBuffer gpuBoneIndices;
    gpuBuffer gpuBoneWeights;

    gpuBuffer gpuUV;
    gpuBuffer gpuRGB;
    gpuBuffer gpuIndices;
};
struct SkinMeshInstance {
    SkinMesh* mesh = 0;
    SkeletonInstance* skel_instance = 0;
    std::vector<gfxm::mat4> pose_transforms;

    gpuBuffer gpuVertices;
    gpuBuffer gpuNormals;

    gpuMeshDesc mesh_desc;

    void init(SkinMesh* mesh, SkeletonInstance* skel_instance) {
        this->mesh = mesh;
        this->skel_instance = skel_instance;
        gpuVertices.reserve(mesh->vertex_count * sizeof(gfxm::vec3), GL_DYNAMIC_DRAW);
        gpuNormals.reserve(mesh->vertex_count * sizeof(gfxm::vec3), GL_DYNAMIC_DRAW);
        pose_transforms.resize(mesh->inverse_bind_transforms.size());

        updateBoneData();

        mesh_desc.setIndexArray(&mesh->gpuIndices);
        mesh_desc.setAttribArray(VFMT::Position_GUID, &gpuVertices, 0);
        mesh_desc.setAttribArray(VFMT::Normal_GUID, &gpuNormals, 0);
        mesh_desc.setAttribArray(VFMT::UV_GUID, &mesh->gpuUV, 0);
        mesh_desc.setAttribArray(VFMT::ColorRGB_GUID, &mesh->gpuRGB, 0);
    }
    void updateBoneData() {
        for (int j = 0; j < pose_transforms.size(); ++j) {
            int model_node_idx = mesh->bone_ids[j];
            auto& inverse_bind = mesh->inverse_bind_transforms[j];
            auto& world = skel_instance->world_transforms[model_node_idx];
            pose_transforms[j] = world * inverse_bind;
        }
    }
};
struct SkinModel {
    std::vector<std::unique_ptr<SkinMesh>> skin_meshes;
    std::vector<std::unique_ptr<SimpleMesh>> simple_meshes;
};
struct SkinModelInstance {
    std::unique_ptr<SkeletonInstance> skeleton_instance;
    std::vector<std::unique_ptr<SkinMeshInstance>> skin_mesh_instances;
    std::vector<std::unique_ptr<SimpleMeshInstance>> simple_mesh_instances;
    std::vector<gpuRenderMaterial*> materials;

    std::vector<std::unique_ptr<gpuRenderable>> renderables;
    gpuUniformBuffer* ubufModel;

    int uniform_loc_model_transform = 0; // should be a "constant"

    void init(gpuPipeline* gpu_pipeline, SkinModel* model, Skeleton* skeleton) {
        ubufModel = gpu_pipeline->createUniformBuffer(UNIFORM_BUFFER_MODEL);
        uniform_loc_model_transform = gpu_pipeline->getUniformBufferDesc(UNIFORM_BUFFER_MODEL)->getUniform(UNIFORM_MODEL_TRANSFORM);
        ubufModel->setMat4(uniform_loc_model_transform, gfxm::mat4(1.0f));

        skeleton_instance.reset(new SkeletonInstance);
        skeleton_instance->init(skeleton);

        for (int i = 0; i < model->skin_meshes.size(); ++i) {
            SkinMeshInstance* skin = new SkinMeshInstance;
            skin->init(model->skin_meshes[i].get(), skeleton_instance.get());
            skin_mesh_instances.push_back(std::unique_ptr<SkinMeshInstance>(skin));
        }

        //
        for (int i = 0; i < model->simple_meshes.size(); ++i) {
            SimpleMeshInstance* mesh = new SimpleMeshInstance;
            mesh->mesh = model->simple_meshes[i].get();
            mesh->skel_instance = skeleton_instance.get();
            mesh->ubufModel = gpu_pipeline->createUniformBuffer(UNIFORM_BUFFER_MODEL);
            mesh->ubufModel->setMat4(uniform_loc_model_transform, gfxm::mat4(1.0f));
            simple_mesh_instances.push_back(std::unique_ptr<SimpleMeshInstance>(mesh));
        }
    }
    void setMaterial(int id, gpuRenderMaterial* mat) {
        assert(id >= 0);
        if (materials.size() <= id) {
            materials.resize(id + 1);
        }
        materials[id] = mat;
    }
    void compileRenderables() {
        renderables.clear();
        for (int i = 0; i < skin_mesh_instances.size() && i < materials.size(); ++i) {
            auto mat = materials[i];
            if (!mat) {
                continue;
            }
            gpuRenderable* renderable = new gpuRenderable;
            renderable->setMaterial(mat);
            renderable->setMeshDesc(&skin_mesh_instances[i].get()->mesh_desc);
            renderable->attachUniformBuffer(ubufModel);
            renderable->compile();
            renderables.push_back(std::unique_ptr<gpuRenderable>(renderable));
        }
        for (int i = 0; i < simple_mesh_instances.size(); ++i) {
            int material_idx = skin_mesh_instances.size();
            auto mat = materials[0];
            if (!mat) {
                continue;
            }
            gpuRenderable* renderable = new gpuRenderable;
            renderable->setMaterial(mat);
            renderable->setMeshDesc(&simple_mesh_instances[i]->mesh->mesh_desc);
            renderable->attachUniformBuffer(simple_mesh_instances[i]->ubufModel);
            renderable->compile();
            renderables.push_back(std::unique_ptr<gpuRenderable>(renderable));
        }
    }

    void update() {
        skeleton_instance->updateBones();

        for (int i = 0; i < skin_mesh_instances.size(); ++i) {
            skin_mesh_instances[i]->updateBoneData();
        }

        std::vector<SkinUpdateData> skin_data(skin_mesh_instances.size());
        for (int i = 0; i < skin_mesh_instances.size(); ++i) {
            SkinUpdateData& d = skin_data[i];
            SkinMeshInstance* inst = skin_mesh_instances[i].get();
            d.vertex_count = inst->mesh->vertex_count;
            d.pose_transforms = inst->pose_transforms.data();
            d.pose_count = inst->pose_transforms.size();
            d.bufVerticesSource = &inst->mesh->gpuVertices;
            d.bufNormalsSource = &inst->mesh->gpuNormals;
            d.bufBoneIndices = &inst->mesh->gpuBoneIndices;
            d.bufBoneWeights = &inst->mesh->gpuBoneWeights;
            d.bufVerticesOut = &inst->gpuVertices;
            d.bufNormalsOut = &inst->gpuNormals;
        }
        updateSkinVertexDataCompute(skin_data.data(), skin_data.size());
    }

    void updateWorldTransform(const gfxm::mat4& t) {
        ubufModel->setMat4(uniform_loc_model_transform, t);
        for (int i = 0; i < simple_mesh_instances.size(); ++i) {
            auto mesh = simple_mesh_instances[i].get();
            const gfxm::mat4& model_space_transform = skeleton_instance->world_transforms[mesh->mesh->bone_id];
            mesh->ubufModel->setMat4(uniform_loc_model_transform, t * model_space_transform);
        }
    }
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
    }
    void importSkinModel(const Skeleton* skeleton, SkinModel* out) {
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
                
                out->simple_meshes.push_back(std::unique_ptr<SimpleMesh>(new SimpleMesh));
                auto out_mesh = out->simple_meshes.back().get();

                MeshData mesh_data;
                readMeshData(skeleton, ai_mesh, &mesh_data);
                out_mesh->gpuVertices.setArrayData(mesh_data.vertices.data(), mesh_data.vertices.size() * sizeof(mesh_data.vertices[0]));
                out_mesh->gpuNormals.setArrayData(mesh_data.normals.data(), mesh_data.normals.size() * sizeof(mesh_data.normals[0]));
                out_mesh->gpuUV.setArrayData(mesh_data.uvs.data(), mesh_data.uvs.size() * sizeof(mesh_data.uvs[0]));
                out_mesh->gpuRGB.setArrayData(mesh_data.colorsRGB.data(), mesh_data.colorsRGB.size() * sizeof(mesh_data.colorsRGB[0]));
                out_mesh->gpuIndices.setArrayData(mesh_data.indices.data(), mesh_data.indices.size() * sizeof(mesh_data.indices[0]));
                int bone_index = skeleton->findBone(ai_node->mName.C_Str());
                if (bone_index < 0) {
                    assert(false);
                    bone_index = 0;
                }
                out_mesh->bone_id = bone_index;

                out_mesh->mesh_desc.setIndexArray(&out_mesh->gpuIndices);
                out_mesh->mesh_desc.setAttribArray(VFMT::Position_GUID, &out_mesh->gpuVertices, 0);
                out_mesh->mesh_desc.setAttribArray(VFMT::Normal_GUID, &out_mesh->gpuNormals, 0);
                out_mesh->mesh_desc.setAttribArray(VFMT::UV_GUID, &out_mesh->gpuUV, 0);
                out_mesh->mesh_desc.setAttribArray(VFMT::ColorRGB_GUID, &out_mesh->gpuRGB, 0);
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

        for (int i = 0; i < ai_scene->mNumMeshes; ++i) {
            auto ai_mesh = ai_scene->mMeshes[i];
            if (ai_mesh->mNumBones == 0) {
                continue;
            }
            std::string mesh_name(ai_mesh->mName.data, ai_mesh->mName.length);
            LOG("Skinned mesh: " << mesh_name);
            LOG("Num bones: " << ai_mesh->mNumBones);

            out->skin_meshes.push_back(std::unique_ptr<SkinMesh>(new SkinMesh));
            auto out_mesh = out->skin_meshes.back().get();

            MeshData mesh_data;
            readMeshData(skeleton, ai_mesh, &mesh_data);
            out_mesh->vertex_count = mesh_data.vertices.size();

            out_mesh->gpuVertices.setArrayData(mesh_data.vertices.data(), mesh_data.vertices.size() * sizeof(mesh_data.vertices[0]));
            out_mesh->gpuNormals.setArrayData(mesh_data.normals.data(), mesh_data.normals.size() * sizeof(mesh_data.normals[0]));
            out_mesh->gpuUV.setArrayData(mesh_data.uvs.data(), mesh_data.uvs.size() * sizeof(mesh_data.uvs[0]));
            out_mesh->gpuRGB.setArrayData(mesh_data.colorsRGB.data(), mesh_data.colorsRGB.size() * sizeof(mesh_data.colorsRGB[0]));
            out_mesh->gpuIndices.setArrayData(mesh_data.indices.data(), mesh_data.indices.size() * sizeof(mesh_data.indices[0]));

            out_mesh->gpuBoneIndices.setArrayData(mesh_data.bone_indices.data(), mesh_data.bone_indices.size() * sizeof(mesh_data.bone_indices[0]));
            out_mesh->gpuBoneWeights.setArrayData(mesh_data.bone_weights.data(), mesh_data.bone_weights.size() * sizeof(mesh_data.bone_weights[0]));

            out_mesh->inverse_bind_transforms = mesh_data.inverse_bind_transforms;
            out_mesh->bone_ids = mesh_data.bone_transform_source_indices;

            /*
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
            if (ai_mesh->GetNumColorChannels() == 0) {
                colorsRGB = std::vector<unsigned char>(ai_mesh->mNumVertices * 3, 255);
            } else {
                colorsRGB.resize(ai_mesh->mNumVertices * 3);
                for (int iv = 0; iv < ai_mesh->mNumVertices; ++iv) {
                    auto ai_col = ai_mesh->mColors[0][iv];
                    colorsRGB[iv * 3]       = ai_col.r * 255.0f;
                    colorsRGB[iv * 3 + 1]   = ai_col.g * 255.0f;
                    colorsRGB[iv * 3 + 2]   = ai_col.b * 255.0f;
                }
            }

            for (int f = 0; f < ai_mesh->mNumFaces; ++f) {
                auto ai_face = ai_mesh->mFaces[f];
                for (int ind = 0; ind < ai_face.mNumIndices; ++ind) {
                    indices.push_back(ai_face.mIndices[ind]);
                }
            }

            out_mesh->vertices = vertices;
            out_mesh->normals = normals;
            out_mesh->gpuUV.setArrayData(uvs.data(), uvs.size() * sizeof(uvs[0]));
            out_mesh->gpuRGB.setArrayData(colorsRGB.data(), colorsRGB.size() * sizeof(colorsRGB[0]));
            out_mesh->gpuIndices.setArrayData(indices.data(), indices.size() * sizeof(indices[0]));

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

                out_mesh->bone_indices = bone_indices;
                out_mesh->bone_weights = bone_weights;

                out_mesh->inverse_bind_transforms = inverse_bind_transforms;
                out_mesh->bone_ids = bone_transform_source_indices;
            }*/
        }
    }

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
        }
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
    for (int i = 0; i < ai_scene->mNumMeshes; ++i) {
        auto ai_mesh = ai_scene->mMeshes[i];
        std::string mesh_name(ai_mesh->mName.data, ai_mesh->mName.length);
        LOG("Mesh: " << mesh_name);

        output.meshes.push_back(Mesh3d());
        auto& out_mesh = output.meshes.back();

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
        /*
        skin_mesh->vertices = vertices;
        skin_mesh->normals = normals;
        skin_mesh->bone_indices = bone_indices;
        skin_mesh->bone_weights = bone_weights;

        skin_mesh->gpuIndices.setArrayData(indices.data(), indices.size() * sizeof(indices[0]));
        skin_mesh->gpuVertices.setArrayData(vertices.data(), vertices.size() * sizeof(vertices[0]));
        skin_mesh->gpuUV.setArrayData(uvs.data(), uvs.size() * sizeof(uvs[0]));
        skin_mesh->gpuNormals.setArrayData(normals.data(), normals.size() * sizeof(normals[0]));
        skin_mesh->mesh_desc.setIndexArray(&skin_mesh->gpuIndices);
        skin_mesh->mesh_desc.setAttribArray(VFMT::Position_GID, &skin_mesh->gpuVertices, 0);
        skin_mesh->mesh_desc.setAttribArray(VFMT::UV_GID, &skin_mesh->gpuUV, 0);
        skin_mesh->mesh_desc.setAttribArray(VFMT::Normal_GID, &skin_mesh->gpuNormals, 0);*/
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