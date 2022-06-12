#include "assimp_load_model.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include <string>
#include <queue>

#include "log/log.hpp"
#include "common/mesh3d/mesh3d.hpp"
#include "common/render/gpu_mesh.hpp"

#include "game/world/render_scene/model/components/mdl_mesh_component.hpp"
#include "game/world/render_scene/model/components/mdl_skin_component.hpp"

#include "common/render/render.hpp"
#include "game/resource/resource.hpp"


void logMdlNodes(mdlNode* n) {
    LOG("*" << n->name);
    for (int i = 0; i < n->childCount(); ++i) {
        auto ch = n->getChild(i);
        logMdlNodes(ch);
    }
}


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
auto readMeshData = [](const std::unordered_map<std::string, int>& name_to_id, const aiMesh* ai_mesh, MeshData* out) {
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
                auto it = name_to_id.find(bone_name);
                int bone_index = 0;
                if (it == name_to_id.end()) {
                    assert(false);
                } else {
                    bone_index = it->second;
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


bool assimpLoadModel(const char* fname, mdlModelMutable* model) {
    LOG("Importing model " << fname << "...");;

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

    // NODES
    std::vector<aiNode*> nodes;
    std::vector<mdlNode*> mdl_nodes;
    std::unordered_map<std::string, int> name_to_id;

    auto ai_root = ai_scene->mRootNode;
    int parent = -1;
    aiNode* ai_node = ai_root;
    std::queue<aiNode*> ai_node_q;
    std::queue<int> parent_q;
    mdlNode* mdl_node = model->getRoot();
    std::queue<mdlNode*> mdl_node_q;
    while (ai_node) {
        for (int i = 0; i < ai_node->mNumChildren; ++i) {
            auto ai_child = ai_node->mChildren[i];
            ai_node_q.push(ai_child);
            parent_q.push(nodes.size());
            auto mdl_child = mdl_node->createChild("");
            mdl_node_q.push(mdl_child);
        }

        std::string node_name(ai_node->mName.data, ai_node->mName.length);
        name_to_id[node_name] = nodes.size();

        aiVector3D ai_translation;
        aiQuaternion ai_rotation;
        aiVector3D ai_scale;
        ai_node->mTransformation.Decompose(ai_scale, ai_rotation, ai_translation);
        gfxm::vec3 translation(ai_translation.x, ai_translation.y, ai_translation.z);
        gfxm::quat rotation(ai_rotation.x, ai_rotation.y, ai_rotation.z, ai_rotation.w);
        gfxm::vec3 scale(ai_scale.x, ai_scale.y, ai_scale.z);
        if (parent == 0) {
            translation = translation * fbxScaleFactor;
            scale = scale * fbxScaleFactor;
        }

        mdl_node->name = node_name;
        mdl_node->translation = translation;
        mdl_node->rotation = rotation;
        mdl_node->scale = scale;

        nodes.push_back(ai_node);
        mdl_nodes.push_back(mdl_node);
        if (ai_node_q.empty()) {
            ai_node = 0;
        } else {
            ai_node = ai_node_q.front();
            ai_node_q.pop();
            parent = parent_q.front();
            parent_q.pop();
            mdl_node = mdl_node_q.front();
            mdl_node_q.pop();
        }
    }

    std::vector<gpuMaterial*> materials;
    materials.resize(ai_scene->mNumMaterials);
    for (int i = 0; i < ai_scene->mNumMaterials; ++i) {
        auto ai_mat = ai_scene->mMaterials[i];
        materials[i] = gpuGetPipeline()->createMaterial();
        auto tech = materials[i]->addTechnique("Normal");
        auto pass = tech->addPass();
        pass->setShader(resGet<gpuShaderProgram>("shaders/vertex_color.glsl"));
        materials[i]->compile();
    }

    // MESHES
    struct tmpMesh {
        gpuMesh* gpu_mesh = 0;
        std::vector<int> bone_indices;
        std::vector<gfxm::mat4> inverse_bind_transforms;
        int material_id = 0;
    };
    std::vector<tmpMesh> gpu_meshes;
    gpu_meshes.resize(ai_scene->mNumMeshes);
    for (int i = 0; i < ai_scene->mNumMeshes; ++i) {
        auto ai_mesh = ai_scene->mMeshes[i];
        MeshData md;
        readMeshData(name_to_id, ai_mesh, &md);

        Mesh3d m3d;
        m3d.setIndexArray(md.indices.data(), md.indices.size() * sizeof(md.indices[0]));
        m3d.setAttribArray(VFMT::Position_GUID, md.vertices.data(), md.vertices.size() * sizeof(md.vertices[0]));
        m3d.setAttribArray(VFMT::Normal_GUID, md.normals.data(), md.normals.size() * sizeof(md.normals[0]));
        m3d.setAttribArray(VFMT::ColorRGB_GUID, md.colorsRGB.data(), md.colorsRGB.size() * sizeof(md.colorsRGB[0]));
        m3d.setAttribArray(VFMT::UV_GUID, md.uvs.data(), md.uvs.size() * sizeof(md.uvs[0]));
        if (!md.bone_indices.empty() && !md.bone_weights.empty()) {
            m3d.setAttribArray(VFMT::BoneIndex4_GUID, md.bone_indices.data(), md.bone_indices.size() * sizeof(md.bone_indices[0]));
            m3d.setAttribArray(VFMT::BoneWeight4_GUID, md.bone_weights.data(), md.bone_weights.size() * sizeof(md.bone_weights[0]));
            gpu_meshes[i].bone_indices = md.bone_transform_source_indices;
            gpu_meshes[i].inverse_bind_transforms = md.inverse_bind_transforms;
        }

        gpu_meshes[i].gpu_mesh = new gpuMesh;
        gpu_meshes[i].gpu_mesh->setData(&m3d);
        gpu_meshes[i].gpu_mesh->setDrawMode(MESH_DRAW_TRIANGLES);
        gpu_meshes[i].material_id = ai_mesh->mMaterialIndex;
    }

    for (int i = 0; i < nodes.size(); ++i) {
        auto ai_node = nodes[i];
        auto mdl_node = mdl_nodes[i];
        for (int j = 0; j < ai_node->mNumMeshes; ++j) {
            auto mesh_id = ai_node->mMeshes[j];
            auto& tmp_mesh = gpu_meshes[mesh_id];
            if (tmp_mesh.bone_indices.empty()) {
                auto cmesh = mdl_node->createComponent<mdlMeshComponent>();
                cmesh->mesh = tmp_mesh.gpu_mesh;
                cmesh->material = materials[tmp_mesh.material_id];
            } else {
                auto cskin = mdl_node->createComponent<mdlSkinComponent>();
                cskin->mesh = tmp_mesh.gpu_mesh;
                cskin->bones.resize(tmp_mesh.bone_indices.size());
                for (int k = 0; k < tmp_mesh.bone_indices.size(); ++k) {
                    cskin->bones[k] = mdl_nodes[tmp_mesh.bone_indices[k]];
                }
                cskin->inverse_bind_transforms = tmp_mesh.inverse_bind_transforms;
                cskin->material = materials[tmp_mesh.material_id];
            }

            // TODO: Support more than one component per node or what?
            break;
        }
    }

    return true;
}