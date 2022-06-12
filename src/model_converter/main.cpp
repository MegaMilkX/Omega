
#include <assert.h>
#include <fstream>
#include <string>
#include <set>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include <nlohmann/json.hpp>

#include "gpu/vertex_format.hpp"

#include "log/log.hpp"

#include "serialization/serialization.hpp"

struct MeshFileData {
    std::vector<gfxm::vec3>                 vertices;
    std::vector<gfxm::vec3>                 normals;
    std::vector<gfxm::vec3>                 tangents;
    std::vector<gfxm::vec3>                 bitangents;
    std::vector<std::vector<gfxm::vec2>>    uv_layers;
    std::vector<std::vector<uint32_t>>      rgba_layers;
    std::vector<uint32_t>                   indices;

    MeshFileData(int vertex_count = 0)
    : vertices(vertex_count, gfxm::vec3(.0f, .0f, .0f)),
    normals(vertex_count) {

    }
};
void serializeMeshFileData_v0(FILE* f, const MeshFileData& data) {
    const uint32_t tag = *(uint32_t*)"MESH";
    const uint32_t version = 0;

    const uint32_t FLAG_HAS_TANGENTS_AND_BITANGENTS = 0x0001;
    const uint32_t FLAG_HAS_UV                      = 0x0002;
    const uint32_t FLAG_HAS_RGBA                    = 0x0004;
    uint32_t flags = 0x0;
    if (!data.tangents.empty() && !data.bitangents.empty()) {
        flags |= FLAG_HAS_TANGENTS_AND_BITANGENTS;
    }
    if (!data.uv_layers.empty()) {
        flags |= FLAG_HAS_UV;
    }
    if (!data.rgba_layers.empty()) {
        flags |= FLAG_HAS_RGBA;
    }

    fwrite(&tag, sizeof(tag), 1, f);
    fwrite(&version, sizeof(version), 1, f);

    fwrite(&flags, sizeof(flags), 1, f);

    fwrite_vector(data.vertices, f);
    fwrite_vector(data.normals, f, false);
    if (flags & FLAG_HAS_TANGENTS_AND_BITANGENTS) {
        fwrite_vector(data.tangents, f, false);
        fwrite_vector(data.bitangents, f, false);
    }
    if (flags & FLAG_HAS_UV) {
        fwrite_length(data.uv_layers, f);
        for (int i = 0; i < data.uv_layers.size(); ++i) {
            auto& uvs = data.uv_layers[i];
            fwrite_vector(uvs, f, false);
        }
    }
    if (flags & FLAG_HAS_RGBA) {
        fwrite_length(data.rgba_layers, f);
        for (int i = 0; i < data.rgba_layers.size(); ++i) {
            auto& colors = data.rgba_layers[i];
            fwrite_vector(colors, f, false);
        }
    }
    fwrite_vector(data.indices, f);
}
void serializeMeshFileData(const char* path, const MeshFileData& data) {
    std::string fname = MKSTR(path << ".mesh");
    FILE* f = fopen(fname.c_str(), "wb");
    if (!f) {
        LOG_ERR("Failed to create file '" << fname << "'");
        return;
    }

    serializeMeshFileData_v0(f, data);

    fclose(f);
}

struct SkinFileData {
    MeshFileData* mesh;
    std::vector<uint32_t>       skeleton_bone_indices;
    std::vector<gfxm::mat4>     inverse_bind_transforms;
    std::vector<gfxm::ivec4>    bone_indices;
    std::vector<gfxm::vec4>     bone_weights;
};
void serializeSkinFileData_v0(FILE* f, const SkinFileData& data) {
    const uint32_t tag = *(uint32_t*)"SKIN";
    const uint32_t version = 0;

    fwrite(&tag, sizeof(tag), 1, f);
    fwrite(&version, sizeof(version), 1, f);
    
    fwrite_vector(data.skeleton_bone_indices, f);
    fwrite_vector(data.inverse_bind_transforms, f);
    fwrite_vector(data.bone_indices, f);
    fwrite_vector(data.bone_weights, f, false);

    serializeMeshFileData_v0(f, *data.mesh);
}
void serializeSkinFileData(const char* path, const SkinFileData& data) {
    std::string fname = MKSTR(path << ".skin");
    FILE* f = fopen(fname.c_str(), "wb");
    if (!f) {
        LOG_ERR("Failed to create file '" << fname << "'");
        return;
    }

    serializeSkinFileData_v0(f, data);

    fclose(f);
}

struct SkeletonAnimationFileData {
    struct KeyframeV3 {
        float time;
        gfxm::vec3 value;
    };
    struct KeyframeQ {
        float time;
        gfxm::quat value;
    };
    struct Track {
        std::string name;
        std::vector<KeyframeV3> frames_pos;
        std::vector<KeyframeQ>  frames_rot;
        std::vector<KeyframeV3> frames_scl;
    };

    float length;
    float fps;
    std::vector<Track> tracks;
};
void serializeSkeletonAnimationFileData_v0(FILE* f, const SkeletonAnimationFileData& data) {
    const uint32_t tag = *(uint32_t*)"SANM";
    const uint32_t version = 0;

    fwrite(&tag, sizeof(tag), 1, f);
    fwrite(&version, sizeof(version), 1, f);

    fwrite(&data.length, sizeof(data.length), 1, f);
    fwrite(&data.fps, sizeof(data.fps), 1, f);
    
    fwrite_length(data.tracks, f);
    for (int i = 0; i < data.tracks.size(); ++i) {
        auto& track = data.tracks[i];
        fwrite_string(track.name, f);
        fwrite_vector(track.frames_pos, f);
        fwrite_vector(track.frames_rot, f);
        fwrite_vector(track.frames_scl, f);
    }
}
void serializeSkeletonAnimationFileData(const char* path, const SkeletonAnimationFileData& data) {
    std::string fname = MKSTR(path << ".sanim");
    FILE* f = fopen(fname.c_str(), "wb");
    if (!f) {
        LOG_ERR("Failed to create file '" << fname << "'");
        return;
    }

    serializeSkeletonAnimationFileData_v0(f, data);

    fclose(f);
}

struct SkeletonFileData {
    std::vector<uint32_t>           parents;
    std::vector<gfxm::mat4>         default_pose;    
    std::map<std::string, uint32_t> node_name_to_index;

    int findBone(const char* bone_name) const {
        auto it = node_name_to_index.find(bone_name);
        if (it == node_name_to_index.end()) {
            return -1;
        }
        return it->second;
    }
};
void serializeSkeletonFileData_v0(FILE* f, const SkeletonFileData& data) {
    const uint32_t tag = *(uint32_t*)"SKEL";
    const uint32_t version = 0;

    fwrite(&tag, sizeof(tag), 1, f);
    fwrite(&version, sizeof(version), 1, f);

    fwrite_vector(data.parents, f);
    fwrite_vector(data.default_pose, f, false);
    for (auto& kv : data.node_name_to_index) {
        fwrite_string(kv.first, f);
        fwrite(&kv.second, sizeof(kv.second), 1, f);
    }
}
void serializeSkeletonFileData(const char* path, const SkeletonFileData& data) {
    std::string fname = MKSTR(path << ".skeleton");
    FILE* f = fopen(fname.c_str(), "wb");
    if (!f) {
        LOG_ERR("Failed to create file '" << fname << "'");
        return;
    }

    serializeSkeletonFileData_v0(f, data);

    fclose(f);
}


bool loadFile(const char* path, std::vector<char>& bytes) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        LOG_ERR("Failed to open file: " << path);
        return false;
    }
    std::streamsize file_size = file.tellg();

    file.seekg(0, std::ios::beg);
    bytes.resize(file_size);
    file.read(&bytes[0], file_size);

    return true;
}


bool loadModel(const char* bytes, size_t sz, const char* format_hint) {
    LOG("aiImportFileFromMemory()...");
    const aiScene* ai_scene = aiImportFileFromMemory(
        bytes, sz,
        aiProcess_GenSmoothNormals |
        aiProcess_GenUVCoords |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_LimitBoneWeights |
        aiProcess_GlobalScale,
        format_hint
    );
    if (!ai_scene) {
        LOG_ERR("aiImportFileFromMemory() failed");
        return false;
    }

    LOG("aiApplyPostProcessing()...");
    ai_scene = aiApplyPostProcessing(ai_scene, aiProcess_CalcTangentSpace);
    if (!ai_scene) {
        LOG_ERR("aiApplyPostProcessing() failed");
        return false;
    }

    double fbxScaleFactor = 1.0;
    if (ai_scene->mMetaData && ai_scene->mMetaData->Get("UnitScaleFactor", fbxScaleFactor)) {
        if (fbxScaleFactor == .0) fbxScaleFactor = 1.0;
        fbxScaleFactor *= .01;
    }
    LOG("fbx scale factor: " << fbxScaleFactor);

    // Keep a list of immediate root children to fix scaling
    std::set<std::string> second_to_root_nodes;
    auto ai_root = ai_scene->mRootNode;
    for (int i = 0; i < ai_root->mNumChildren; ++i) {
        auto ai_node = ai_root->mChildren[i];
        second_to_root_nodes.insert(ai_node->mName.C_Str());
    }

    LOG("Skeleton");
    SkeletonFileData skeleton;
    {
        auto ai_root = ai_scene->mRootNode;
        int parent = -1;
        aiNode* ai_node = ai_root;
        std::queue<aiNode*> ai_node_queue;
        std::queue<int>     parent_queue;
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
                parent_queue.push(skeleton.default_pose.size());
            }

            std::string node_name(ai_node->mName.data, ai_node->mName.length);
            LOG(node_name);
            skeleton.node_name_to_index[node_name] = skeleton.parents.size();

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

            skeleton.parents.push_back(parent);
            if (parent == -1) {
                skeleton.default_pose.push_back(gfxm::scale(gfxm::mat4(1.0f), gfxm::vec3(fbxScaleFactor, fbxScaleFactor, fbxScaleFactor)));
            } else {
                skeleton.default_pose.push_back(transform.matrix());
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
        skeleton.default_pose[0] = gfxm::mat4(1.0f);
        serializeSkeletonFileData("skeleton", skeleton);
    }

    LOG("Meshes");
    for (int i = 0; i < ai_scene->mNumMeshes; ++i) {
        auto ai_mesh = ai_scene->mMeshes[i];
        aiString mesh_name = ai_mesh->mName;
        LOG("\tMesh " << i << ": " << mesh_name.C_Str());

        int vertex_count = ai_mesh->mNumVertices;

        MeshFileData mesh(vertex_count);
        for (int f = 0; f < ai_mesh->mNumFaces; ++f) {
            auto ai_face = ai_mesh->mFaces[f];
            for (int ind = 0; ind < ai_face.mNumIndices; ++ind) {
                mesh.indices.push_back(ai_face.mIndices[ind]);
            }
        }

        for (int iv = 0; iv < ai_mesh->mNumVertices; ++iv) {
            auto ai_vert    = ai_mesh->mVertices[iv];
            auto ai_norm    = ai_mesh->mNormals[iv];
            mesh.vertices[iv] = gfxm::vec3(ai_vert.x, ai_vert.y, ai_vert.z);
            mesh.normals[iv] = gfxm::vec3(ai_norm.x, ai_norm.y, ai_norm.z);
        }
        if (ai_mesh->HasTangentsAndBitangents()) {
            mesh.tangents.resize(vertex_count);
            mesh.bitangents.resize(vertex_count);
            for (int iv = 0; iv < ai_mesh->mNumVertices; ++iv) {
                auto ai_tan = ai_mesh->mTangents[iv];
                auto ai_bitan = ai_mesh->mBitangents[iv];
                mesh.tangents[iv] = gfxm::vec3(ai_tan.x, ai_tan.y, ai_tan.z);
                mesh.bitangents[iv] = gfxm::vec3(ai_bitan.x, ai_bitan.y, ai_bitan.z);
            }
        }
        for (int uv_layer = 0; uv_layer < AI_MAX_NUMBER_OF_TEXTURECOORDS && ai_mesh->HasTextureCoords(uv_layer); ++uv_layer) {
            mesh.uv_layers.push_back(std::vector<gfxm::vec2>(vertex_count, gfxm::vec2(.0f, .0f)));
            for (int iv = 0; iv < ai_mesh->mNumVertices; ++iv) {
                auto ai_uv = ai_mesh->mTextureCoords[uv_layer][iv];
                mesh.uv_layers[uv_layer][iv] = gfxm::vec2(ai_uv.x, ai_uv.y);
            }
        }
        for (int col_layer = 0; col_layer < AI_MAX_NUMBER_OF_COLOR_SETS && ai_mesh->HasVertexColors(col_layer); ++col_layer) {
            mesh.rgba_layers.push_back(std::vector<uint32_t>(vertex_count, 0xFFFFFF));
            for (int iv = 0; iv < ai_mesh->mNumVertices; ++iv) {
                auto ai_col = ai_mesh->mColors[col_layer][iv];
                uint32_t c = 0;
                c |= 0x000000FF & (long)(ai_col.r * 255.0f);
                c |= 0x0000FF00 & ((long)(ai_col.g * 255.0f) << 8);
                c |= 0x00FF0000 & ((long)(ai_col.b * 255.0f) << 16);
                c |= 0xFF000000 & ((long)(ai_col.a * 255.0f) << 24);
                mesh.rgba_layers[col_layer][iv] = c;
            }
        }


        if (ai_mesh->HasBones()) {
            SkinFileData skin;
            skin.mesh = &mesh;
            skin.skeleton_bone_indices.resize(ai_mesh->mNumBones);
            skin.inverse_bind_transforms.resize(ai_mesh->mNumBones);
            skin.bone_indices.resize(vertex_count);
            skin.bone_weights.resize(vertex_count);

            for (int j = 0; j < ai_mesh->mNumBones; ++j) {
                auto& ai_bone = ai_mesh->mBones[j];
                std::string bone_name(ai_bone->mName.data, ai_bone->mName.length);

                skin.inverse_bind_transforms[j] = gfxm::transpose(*(gfxm::mat4*)&ai_bone->mOffsetMatrix);

                {
                    int bone_index = skeleton.findBone(bone_name.c_str());
                    if (bone_index < 0) {
                        assert(false);
                    }
                    skin.skeleton_bone_indices[j] = bone_index;
                }

                for (int k = 0; k < ai_bone->mNumWeights; ++k) {
                    auto ai_weight = ai_bone->mWeights[k];
                    unsigned int    vert_id = ai_weight.mVertexId;
                    float           weight  = ai_weight.mWeight;

                    for (int c = 0; c < 4; ++c) {
                        if (skin.bone_weights[vert_id][c] == .0f) {
                            skin.bone_indices[vert_id][c] = j;
                            skin.bone_weights[vert_id][c] = weight;
                            break;
                        }
                    }
                }
            }

            // Normalize weights
            for (int j = 0; j < skin.bone_weights.size(); ++j) {
                float sum = skin.bone_weights[j].x + skin.bone_weights[j].y + skin.bone_weights[j].z + skin.bone_weights[j].w;
                skin.bone_weights[j] /= sum;
            }

            serializeSkinFileData(mesh_name.C_Str(), skin);
        } else {
            serializeMeshFileData(mesh_name.C_Str(), mesh);
        }
    }

    LOG("Materials");
    for (int i = 0; i < ai_scene->mNumMaterials; ++i) {
        auto ai_material = ai_scene->mMaterials[i];
        aiString material_name = ai_material->GetName();
        LOG("\tMaterial " << i << ": " << material_name.C_Str());
        int num_textures = ai_material->GetTextureCount(aiTextureType_DIFFUSE);
        for (int j = 0; j < num_textures; ++j) {
            aiString path;
            ai_material->GetTexture(aiTextureType_DIFFUSE, j, &path);
            LOG("\t\tDiffuse " << j << ": " << path.C_Str());
        }

        nlohmann::json json = nlohmann::json::object();
        nlohmann::json j_techniques = nlohmann::json::object();
        json["techniques"] = j_techniques;

        // TODO: 

        std::string jstr = json.dump(4, ' ');

        std::ofstream file(MKSTR(material_name.C_Str() << ".mat").c_str());
        file.write(&jstr[0], jstr.size());
        file.close();
    }
    
    LOG("Animations");
    for (int i = 0; i < ai_scene->mNumAnimations; ++i) {
        SkeletonAnimationFileData anim;

        aiAnimation* ai_anim = ai_scene->mAnimations[i];
        LOG("\tAnim " << i << ": " << ai_anim->mName.C_Str());

        anim.length    = ai_anim->mDuration;
        anim.fps       = ai_anim->mTicksPerSecond;
        anim.tracks.resize(ai_anim->mNumChannels);

        for (int j = 0; j < ai_anim->mNumChannels; ++j) {
            aiNodeAnim* ai_node_anim = ai_anim->mChannels[j];
            
            float scaleFactorFix = 1.0f;
            if (second_to_root_nodes.find(ai_node_anim->mNodeName.C_Str()) != second_to_root_nodes.end()) {
                scaleFactorFix = (float)fbxScaleFactor;
            }

            anim.tracks[j].frames_pos.resize(ai_node_anim->mNumPositionKeys);
            for (int k = 0; k < ai_node_anim->mNumPositionKeys; ++k) {
                auto& ai_v = ai_node_anim->mPositionKeys[k].mValue;
                float time = (float)ai_node_anim->mPositionKeys[k].mTime;
                gfxm::vec3 v = { ai_v.x, ai_v.y, ai_v.z };
                anim.tracks[j].frames_pos[k] = SkeletonAnimationFileData::KeyframeV3 { time, v * scaleFactorFix };
            }
            anim.tracks[j].frames_rot.resize(ai_node_anim->mNumRotationKeys);
            for (int k = 0; k < ai_node_anim->mNumRotationKeys; ++k) {
                auto& ai_v = ai_node_anim->mRotationKeys[k].mValue;
                float time = (float)ai_node_anim->mRotationKeys[k].mTime;
                gfxm::quat v = { ai_v.x, ai_v.y, ai_v.z, ai_v.w };
                anim.tracks[j].frames_rot[k] = SkeletonAnimationFileData::KeyframeQ { time, v };
            }
            anim.tracks[j].frames_scl.resize(ai_node_anim->mNumScalingKeys);
            for (int k = 0; k < ai_node_anim->mNumScalingKeys; ++k) {
                auto& ai_v = ai_node_anim->mScalingKeys[k].mValue;
                float time = (float)ai_node_anim->mScalingKeys[k].mTime;
                gfxm::vec3 v = { ai_v.x, ai_v.y, ai_v.z };
                anim.tracks[j].frames_scl[k] = SkeletonAnimationFileData::KeyframeV3 { time, v * scaleFactorFix };
            }
        }

        // TODO: ROOT MOTION

        serializeSkeletonAnimationFileData(ai_anim->mName.C_Str(), anim);
    }

    return true;
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Not enough arguments");
        assert(false);
        return -1;
    }
    for (int i = 0; i < argc; ++i) {
        printf("%s\n", argv[i]);
    }

    std::string model_path = argv[1];
    
    std::vector<char> bytes;
    if (!loadFile(model_path.c_str(), bytes)) {
        return -2;
    }

    if (!loadModel(&bytes[0], bytes.size(), model_path.c_str())) {
        return -3;
    }

    return 0;
}