#include "assimp_load_skeletal_model.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include "gpu/gpu_material.hpp"


static struct MeshData {
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

    void toGpuMesh(gpuMesh* mesh, bool skin) {
        assert(mesh);
        Mesh3d m3d;
        m3d.setIndexArray(indices.data(), indices.size() * sizeof(indices[0]));
        m3d.setAttribArray(VFMT::Position_GUID, vertices.data(), vertices.size() * sizeof(vertices[0]));
        m3d.setAttribArray(VFMT::Normal_GUID, normals.data(), normals.size() * sizeof(normals[0]));
        m3d.setAttribArray(VFMT::ColorRGB_GUID, colorsRGB.data(), colorsRGB.size() * sizeof(colorsRGB[0]));
        m3d.setAttribArray(VFMT::UV_GUID, uvs.data(), uvs.size() * sizeof(uvs[0]));
        if (skin) {
            assert(!bone_indices.empty() && !bone_weights.empty());
            m3d.setAttribArray(VFMT::BoneIndex4_GUID, bone_indices.data(), bone_indices.size() * sizeof(bone_indices[0]));
            m3d.setAttribArray(VFMT::BoneWeight4_GUID, bone_weights.data(), bone_weights.size() * sizeof(bone_weights[0]));
        }

        mesh->setData(&m3d);
        mesh->setDrawMode(MESH_DRAW_TRIANGLES);
    }
};
static auto readMeshData = [](sklSkeletonEditable* skl, const aiMesh* ai_mesh, MeshData* out) {
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
    }
    else {
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
                auto skl_bone = skl->findBone(bone_name.c_str());
                int bone_index = 0;
                if (skl_bone == nullptr) {
                    assert(false);
                } else {
                    bone_index = skl_bone->getIndex();
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


#include "config.hpp"
bool assimpLoadSkeletalModel(const char* fname, sklmSkeletalModelEditable* sklm) {
    LOG("Importing model " << fname << "...");;

    HSHARED<sklSkeletonEditable> skl = sklm->getSkeleton();

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
        LOG_WARN("Failed to load " << fname);
        return false;
    }

    ai_scene = aiApplyPostProcessing(ai_scene, aiProcess_CalcTangentSpace);
    if (!ai_scene) {
        LOG_WARN("aiApplyPostProcessing failed");
    }

    double fbxScaleFactor = 1.0f;
    if (ai_scene->mMetaData && ai_scene->mMetaData->Get("UnitScaleFactor", fbxScaleFactor)) {
        if (fbxScaleFactor == .0) fbxScaleFactor = 1.0;
        fbxScaleFactor *= .01;
    }

    auto ai_root = ai_scene->mRootNode;

    // SKELETON
    {
        aiNode* ai_node = ai_root;
        std::queue<aiNode*> ai_node_q;
        sklBone* skl_bone = skl->getRoot();
        std::queue<sklBone*> skl_bone_q;
        while (ai_node) {
            for (int i = 0; i < ai_node->mNumChildren; ++i) {
                auto ai_child = ai_node->mChildren[i];
                ai_node_q.push(ai_child);
                auto skl_child = skl_bone->createChild(ai_child->mName.C_Str());
                skl_bone_q.push(skl_child);
            }

            aiVector3D ai_translation;
            aiQuaternion ai_rotation;
            aiVector3D ai_scale;
            ai_node->mTransformation.Decompose(ai_scale, ai_rotation, ai_translation);
            gfxm::vec3 translation(ai_translation.x, ai_translation.y, ai_translation.z);
            gfxm::quat rotation(ai_rotation.x, ai_rotation.y, ai_rotation.z, ai_rotation.w);
            gfxm::vec3 scale(ai_scale.x, ai_scale.y, ai_scale.z);
            skl_bone->setTranslation(translation);
            skl_bone->setRotation(rotation);
            skl_bone->setScale(scale);

            if (ai_node_q.empty()) {
                ai_node = 0;
            } else {
                ai_node = ai_node_q.front();
                ai_node_q.pop();
                skl_bone = skl_bone_q.front();
                skl_bone_q.pop();
            }
        }
        skl->getRoot()->setTranslation(gfxm::vec3(0, 0, 0));
        skl->getRoot()->setRotation(gfxm::quat(0, 0, 0, 1));
        skl->getRoot()->setScale(gfxm::vec3(1, 1, 1));
        for (int i = 0; i < skl->getRoot()->childCount(); ++i) {
            auto bone = skl->getRoot()->getChild(i);
            bone->setTranslation(bone->getLclTranslation() * fbxScaleFactor);
            bone->setScale(bone->getLclScale() * fbxScaleFactor);
        }
    }

    // SKELETAL MODEL
    struct MeshObject {
        std::string         name;
        int                 bone_index;
        HSHARED<gpuMesh>    mesh;
    };
    struct SkinObject {
        std::string             name;
        std::vector<int>        bone_indices;
        std::vector<gfxm::mat4> inv_bind_transforms;
        HSHARED<gpuMesh>        mesh;
    };
    std::vector<MeshObject>    mesh_objects;
    std::vector<SkinObject>    skin_objects;
    {
        for (int i = 0; i < ai_scene->mNumMeshes; ++i) {
            auto ai_mesh = ai_scene->mMeshes[i];
            if (!ai_mesh->HasBones()) {
                continue;
            }
            if (ai_mesh->mNumBones == 1) {
                // Apply inverse bind transform and treat as non-skinned
                MeshData md;
                readMeshData(skl.get(), ai_mesh, &md);
                for (int i = 0; i < md.vertices.size(); ++i) {
                    auto& vertex = md.vertices[i];
                    vertex = md.inverse_bind_transforms[0] * gfxm::vec4(vertex, 1.0f);
                }
                for (int i = 0; i < md.normals.size(); ++i) {
                    auto& normal = md.normals[i];
                    normal = md.inverse_bind_transforms[0] * gfxm::vec4(normal, .0f);
                }
                // TODO: tangent and binormal
                // Treat as non-skinned mesh
                MeshObject m;
                m.name = ai_mesh->mName.C_Str();
                m.bone_index = md.bone_transform_source_indices[0];
                m.mesh.reset(HANDLE_MGR<gpuMesh>().acquire());
                md.toGpuMesh(m.mesh.get(), false);
                mesh_objects.push_back(m);
            } else {
                // Process skinned mesh
                MeshData md;
                readMeshData(skl.get(), ai_mesh, &md);
                
                SkinObject m;
                m.name = ai_mesh->mName.C_Str();
                m.mesh.reset(HANDLE_MGR<gpuMesh>().acquire());
                md.toGpuMesh(m.mesh.get(), true);
                m.bone_indices = md.bone_transform_source_indices;
                m.inv_bind_transforms = md.inverse_bind_transforms;
                skin_objects.push_back(m);
            }
        }
    }
    {
        aiNode* ai_node = ai_root;
        std::queue<aiNode*> ai_node_q;
        while (ai_node) {
            for (int i = 0; i < ai_node->mNumChildren; ++i) {
                auto ai_child = ai_node->mChildren[i];
                ai_node_q.push(ai_child);;
            }

            for (int i = 0; i < ai_node->mNumMeshes; ++i) {
                auto ai_mesh_id = ai_node->mMeshes[i];
                auto ai_mesh = ai_scene->mMeshes[ai_mesh_id];
                if (ai_mesh->HasBones()) {
                    continue;
                }

                MeshData md;
                readMeshData(skl.get(), ai_mesh, &md);
                MeshObject m;
                m.name = ai_mesh->mName.C_Str();
                m.bone_index = skl->findBone(ai_node->mName.C_Str())->getIndex();
                m.mesh.reset(HANDLE_MGR<gpuMesh>().acquire());
                md.toGpuMesh(m.mesh.get(), false);
                mesh_objects.push_back(m);
            }

            if (ai_node_q.empty()) {
                ai_node = 0;
            } else {
                ai_node = ai_node_q.front();
                ai_node_q.pop();
            }
        }
    }

    // MATERIAL (TODO)
    HSHARED<gpuMaterial> material;
    {
        material.reset(HANDLE_MGR<gpuMaterial>().acquire());
        auto tech = material->addTechnique("Normal");
        auto pass = tech->addPass();
        pass->setShader(resGet<gpuShaderProgram>(build_config::default_import_shader));
        material->compile();
    }

    for (int i = 0; i < mesh_objects.size(); ++i) {
        auto& m = mesh_objects[i];
        auto skl_bone = skl->getBone(m.bone_index);
        auto c = sklm->addComponent<sklmMeshComponent>(m.name.c_str());
        c->bone_name = skl_bone->getName();
        c->mesh = m.mesh;
        c->material = material;
    }
    for (int i = 0; i < skin_objects.size(); ++i) {
        auto& m = skin_objects[i];
        auto c = sklm->addComponent<sklmSkinComponent>(m.name.c_str());
        c->inv_bind_transforms = m.inv_bind_transforms;
        for (int j = 0; j < m.bone_indices.size(); ++j) {
            auto bone_index = m.bone_indices[j];
            auto skl_bone = skl->getBone(bone_index);
            c->bone_names.push_back(skl_bone->getName());
            c->mesh = m.mesh;
            c->material = material;
        }
    }

    return true;
}