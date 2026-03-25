#include "model_importer.hpp"

#include <map>
#include <unordered_map>
#include <queue>
#include <filesystem>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <stb_image.h>

#include "log/log.hpp"

static const char* aiMetadataTypeToString(aiMetadataType t) {
    switch (t) {
    case AI_BOOL: return "AI_BOOL";
    case AI_INT32: return "AI_INT32";
    case AI_UINT64: return "AI_UINT64";
    case AI_FLOAT: return "AI_FLOAT";
    case AI_DOUBLE: return "AI_DOUBLE";
    case AI_AISTRING: return "AI_AISTRING";
    case AI_AIVECTOR3D: return "AI_AIVECTOR3D";
    }
    return "UNKNOWN";
}

static void readMesh(const ModelImporter* scn, const aiMesh* ai_mesh, mimpMesh* out) {
    out->material = scn->getMaterial(ai_mesh->mMaterialIndex);
    
    std::vector<uint32_t>&       indices = out->indices;

    std::vector<gfxm::vec3>&     vertices = out->vertices;
    std::vector<std::vector<uint32_t>>&       rgba_channels = out->rgba_channels;
    std::vector<std::vector<gfxm::vec2>>&     uv_channels = out->uv_channels;
    std::vector<gfxm::vec3>&     normals = out->normals;
    std::vector<gfxm::vec3>&     tangents = out->tangents;
    std::vector<gfxm::vec3>&     bitangents = out->bitangents;

    const int n_uv_channels = ai_mesh->GetNumUVChannels();
    uv_channels.resize(n_uv_channels);
    for (int i = 0; i < n_uv_channels; ++i) {
        auto& uv = uv_channels[i];
        uv.resize(ai_mesh->mNumVertices);
        for (int iv = 0; iv < ai_mesh->mNumVertices; ++iv) {
            auto ai_uv = ai_mesh->mTextureCoords[i][iv];
            uv[iv] = gfxm::vec2(ai_uv.x, ai_uv.y);
        }
    }
    if (n_uv_channels == 0) {
        uv_channels.resize(1);
        auto& uv = uv_channels[0];
        uv = std::vector<gfxm::vec2>(ai_mesh->mNumVertices, gfxm::vec2(0, 0));
    }

    vertices.reserve(ai_mesh->mNumVertices);
    normals.reserve(ai_mesh->mNumVertices);
    for (int iv = 0; iv < ai_mesh->mNumVertices; ++iv) {
        auto ai_vert = ai_mesh->mVertices[iv];
        auto ai_norm = ai_mesh->mNormals[iv];
        vertices.push_back(gfxm::vec3(ai_vert.x, ai_vert.y, ai_vert.z));
        normals.push_back(gfxm::vec3(ai_norm.x, ai_norm.y, ai_norm.z));
    }
    tangents.reserve(ai_mesh->mNumVertices);
    bitangents.reserve(ai_mesh->mNumVertices);
    if (ai_mesh->mTangents) {
        for (int iv = 0; iv < ai_mesh->mNumVertices; ++iv) {
            auto ai_tan = ai_mesh->mTangents[iv];
            auto ai_bitan = ai_mesh->mBitangents[iv];
            tangents.push_back(gfxm::vec3(ai_tan.x, ai_tan.y, ai_tan.z));
            bitangents.push_back(gfxm::vec3(ai_bitan.x, ai_bitan.y, ai_bitan.z));
        }
    } else if (ai_mesh->mTangents == nullptr) {
        LOG_ERR("Failed to load tangent vertex data");
        tangents.resize(ai_mesh->mNumVertices);
        bitangents.resize(ai_mesh->mNumVertices);
    }

    const int n_col_channels = ai_mesh->GetNumColorChannels();
    rgba_channels.resize(n_col_channels);
    for (int i = 0; i < n_col_channels; ++i) {
        auto& rgba = rgba_channels[i];
        rgba.resize(ai_mesh->mNumVertices);
        for (int iv = 0; iv < ai_mesh->mNumVertices; ++iv) {
            auto ai_col = ai_mesh->mColors[i][iv];
            rgba[iv] = gfxm::make_rgba32(ai_col.r, ai_col.g, ai_col.b, ai_col.a);
        }
    }
    if (n_col_channels == 0) {
        rgba_channels.resize(1);
        auto& rgba = rgba_channels[0];
        rgba = std::vector<uint32_t>(ai_mesh->mNumVertices, 0xFFFFFFFF);
    }

    for (int f = 0; f < ai_mesh->mNumFaces; ++f) {
        auto ai_face = ai_mesh->mFaces[f];
        for (int ind = 0; ind < ai_face.mNumIndices; ++ind) {
            indices.push_back(ai_face.mIndices[ind]);
        }
    }

    if (ai_mesh->HasBones()) {
        out->skin.reset(new mimpSkin);
        auto pskin = out->skin.get();

        std::vector<gfxm::ivec4>&    bone_indices = pskin->bone_indices;
        std::vector<gfxm::vec4>&     bone_weights = pskin->bone_weights;
        std::vector<gfxm::mat4>&     inverse_bind_transforms = pskin->inverse_bind_transforms;
        std::vector<gfxm::mat4>&     pose_transforms = pskin->pose_transforms;
        std::vector<int>&            bone_transform_source_indices = pskin->bone_transform_source_indices;

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
                auto bone_node = scn->getRoot()->findNode(bone_name.c_str());
                int bone_index = 0;
                if (bone_node == nullptr) {
                    assert(false);
                } else {
                    bone_index = bone_node->index;
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
        // Normalize weights
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
}


mimpNode* ModelImporter::allocNode(const std::string& name) {
    auto& ptr = nodes.emplace_back();
    ptr.reset(new mimpNode);
    ptr->name = name;
    ptr->index = nodes.size() - 1;
    return ptr.get();
}
mimpMesh* ModelImporter::allocMesh() {
    auto& ptr = meshes.emplace_back();
    ptr.reset(new mimpMesh);
    ptr->index = meshes.size() - 1;
    return ptr.get();
}
mimpNode* ModelImporter::createChild(mimpNode* parent, const std::string& name) {
    auto node = allocNode(name);
    parent->children.push_back(node);
    node->parent = parent;
    return node;
}
mimpMeshInstance* ModelImporter::createMeshInstance(const mimpNode* node, int mesh_idx) {
    auto& ptr = mesh_instances.emplace_back();
    ptr.reset(new mimpMeshInstance);
    ptr->node = node;
    ptr->mesh = meshes[mesh_idx].get();
    return ptr.get();
}

static void logAssimpMetadata(aiMetadata* meta) {
    if (!meta) {
        return;
    }
    LOG_DBG("ModelImporter metadata");
    //LOG_DBG("ModelImporter: " << ai_node->mName.C_Str() << " metadata");
    //auto meta = ai_node->mMetaData;
    for (int i = 0; i < meta->mNumProperties; ++i) {
        auto& key = meta->mKeys[i];
        auto& val = meta->mValues[i];

        switch (val.mType) {
        case AI_BOOL:
            LOG_DBG("\t" << key.C_Str() << " " << aiMetadataTypeToString(val.mType) << ": " << *static_cast<bool*>(val.mData));
            break;
        case AI_INT32:
            LOG_DBG("\t" << key.C_Str() << " " << aiMetadataTypeToString(val.mType) << ": " << *static_cast<int32_t*>(val.mData));
            break;
        case AI_UINT64:
            LOG_DBG("\t" << key.C_Str() << " " << aiMetadataTypeToString(val.mType) << ": " << *static_cast<uint64_t*>(val.mData));
            break;
        case AI_FLOAT:
            LOG_DBG("\t" << key.C_Str() << " " << aiMetadataTypeToString(val.mType) << ": " << *static_cast<float*>(val.mData));
            break;
        case AI_DOUBLE:
            LOG_DBG("\t" << key.C_Str() << " " << aiMetadataTypeToString(val.mType) << ": " << *static_cast<double*>(val.mData));
            break;
        case AI_AISTRING:
            LOG_DBG("\t" << key.C_Str() << " " << aiMetadataTypeToString(val.mType) << ": " << static_cast<aiString*>(val.mData)->C_Str());
            break;
        case AI_AIVECTOR3D: {
                const aiVector3D* v3 = static_cast<aiVector3D*>(val.mData);
                LOG_DBG("\t" << key.C_Str() << " " << aiMetadataTypeToString(val.mType) << ": [" << v3->x << ", " << v3->y << ", " << v3->z << "]");
                break;
        }
        default:
            LOG_DBG("\t" << key.C_Str() << ": " << "UNKNOWN TYPE");
            assert(false);
            continue;
        }
    }
}

RHSHARED<gpuTexture2d> ModelImporter::findEmbeddedTexture(const std::string& name) {
    if (name[0] == '*') {
        std::string num(name.begin() + 1, name.end());
        int n = std::atoi(num.c_str());
        return embedded_textures[n];
    } else {
        auto it = embedded_texture_map.find(name);
        if(it != embedded_texture_map.end()) {
            return embedded_textures[it->second];
        }
        return RHSHARED<gpuTexture2d>();
    }    
}

bool ModelImporter::loadAssimp(const std::string& source, float custom_scale_factor) {
    const aiScene* ai_scene = aiImportFile(
        source.c_str(),
        aiProcess_GenSmoothNormals |
        aiProcess_GenUVCoords |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_LimitBoneWeights |
        aiProcess_GlobalScale
    );
    if (!ai_scene) {
        LOG_WARN("ModelImporter: Failed to load '" << source << "'");
        return false;
    }

    ai_scene = aiApplyPostProcessing(ai_scene, aiProcess_CalcTangentSpace);
    if (!ai_scene) {
        LOG_WARN("aiApplyPostProcessing failed");
        return false;
    }

    logAssimpMetadata(ai_scene->mMetaData);
    
    float scale_factor = 1.0f;
    if (custom_scale_factor == .0f) {
        // fbx specific
        double UnitScaleFactor = 1.0f;
        if (ai_scene->mMetaData && ai_scene->mMetaData->Get("UnitScaleFactor", UnitScaleFactor)) {
            if (UnitScaleFactor == .0) UnitScaleFactor = 1.0;
            UnitScaleFactor *= .01;
            scale_factor = (float)UnitScaleFactor;
        }
    } else {
        scale_factor = custom_scale_factor;
    }

    // EMBEDDED TEXTURES
    {
        LOG_DBG("ModelImporter: " << ai_scene->mNumTextures << " embedded textures");
        for (int i = 0; i < ai_scene->mNumTextures; ++i) {
            auto ai_tex = ai_scene->mTextures[i];
            LOG_DBG(ai_tex->mFilename.C_Str());
            if (ai_tex->mHeight == 0) {
                //LOG_DBG("compressed texture, not implemented, skipping");
                std::string fmt_hint = ai_tex->achFormatHint;
                int width = 0;
                int height = 0;
                int channels = 0;
                const int desired_channels = 0; // stbi is clearly bugged, desired_channels == 4 causes garbage results
                stbi_uc* result = stbi_load_from_memory(reinterpret_cast<stbi_uc*>(ai_tex->pcData), ai_tex->mWidth, &width, &height, &channels, desired_channels);
                if (!result) {
                    LOG_WARN("Failed to read compressed texture");
                    continue;
                }
                RHSHARED<gpuTexture2d> tex;
                tex.reset_acquire();
                tex->setData(result, width, height, channels, IMAGE_CHANNEL_UNSIGNED_BYTE);
                embedded_texture_map[ai_tex->mFilename.C_Str()] = (int)embedded_textures.size();
                embedded_textures.push_back(tex);

                stbi_write_bmp("stbi_test.bmp", width, height, channels, result);

                stbi_image_free(result);
                continue;
            }
            RHSHARED<gpuTexture2d> tex;
            tex.reset_acquire();
            tex->setData(ai_tex->pcData, ai_tex->mWidth, ai_tex->mHeight, 4, IMAGE_CHANNEL_UNSIGNED_BYTE);
            embedded_texture_map[ai_tex->mFilename.C_Str()] = (int)embedded_textures.size();
            embedded_textures.push_back(tex);
        }
    }

    // MATERIALS
    {
        LOG_DBG("ModelImporter: " << ai_scene->mNumMaterials << " materials");
        materials.resize(ai_scene->mNumMaterials);
        for (int im = 0; im < ai_scene->mNumMaterials; ++im) {
            auto ai_mat = ai_scene->mMaterials[im];
            LOG_DBG("material " << im << " '" << ai_mat->GetName().C_Str() << "'");
            auto& mat = materials[im];
            mat.reset(new mimpMaterial);
            mat->index = im;
            mat->name = ai_mat->GetName().C_Str();
            /*
            aiTextureType tex_type = aiTextureType::aiTextureType_DIFFUSE;
            for (int itex = 0; itex < ai_mat->GetTextureCount(tex_type); ++itex) {
                aiString ai_path;
                ai_mat->GetTexture(tex_type, itex, &ai_path);
                LOG_DBG(ai_path.C_Str());
            }
            */
            if (ai_mat->GetTextureCount(aiTextureType_DIFFUSE)) {
                aiString ai_path;
                ai_mat->GetTexture(aiTextureType_DIFFUSE, 0, &ai_path);
                LOG_DBG("Diffuse: " << ai_path.C_Str());

                mat->albedo = findEmbeddedTexture(ai_path.C_Str());
            }

            if (ai_mat->GetTextureCount(aiTextureType_NORMALS)) {
                aiString ai_path;
                ai_mat->GetTexture(aiTextureType_NORMALS, 0, &ai_path);
                LOG_DBG("Normal map: " << ai_path.C_Str());

                mat->normalmap = findEmbeddedTexture(ai_path.C_Str());
            }

            if (ai_mat->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS)) {
                aiString ai_path;
                ai_mat->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &ai_path);
                LOG_DBG("Roughness: " << ai_path.C_Str());

                mat->roughness = findEmbeddedTexture(ai_path.C_Str());
            }
            
            if (ai_mat->GetTextureCount(aiTextureType_METALNESS)) {
                aiString ai_path;
                ai_mat->GetTexture(aiTextureType_METALNESS, 0, &ai_path);
                LOG_DBG("Metallic: " << ai_path.C_Str());

                mat->metallic = findEmbeddedTexture(ai_path.C_Str());
            }

            if (ai_mat->GetTextureCount(aiTextureType_EMISSIVE)) {
                aiString ai_path;
                ai_mat->GetTexture(aiTextureType_EMISSIVE, 0, &ai_path);
                LOG_DBG("Emission: " << ai_path.C_Str());

                mat->emission = findEmbeddedTexture(ai_path.C_Str());
            }
        }
    }
    
    std::unordered_map<aiNode*, mimpNode*> node_map;
    node_map[ai_scene->mRootNode] = &root;
    root.name = ai_scene->mRootNode->mName.C_Str();

    // SCENE GRAPH
    {
        auto ai_root = ai_scene->mRootNode;
        aiNode* ai_node = ai_root;
        std::queue<aiNode*> ai_node_q;
        mimpNode* node = &root;
        std::queue<mimpNode*> node_q;
        while (ai_node) {
            logAssimpMetadata(ai_node->mMetaData);

            for (int i = 0; i < ai_node->mNumChildren; ++i) {
                auto ai_child = ai_node->mChildren[i];
                ai_node_q.push(ai_child);
                auto child = createChild(node, ai_child->mName.C_Str());
                node_q.push(child);
                node_map[ai_child] = child;
            }
            
            aiVector3D ai_translation;
            aiQuaternion ai_rotation;
            aiVector3D ai_scale;
            ai_node->mTransformation.Decompose(ai_scale, ai_rotation, ai_translation);
            node->translation = gfxm::vec3(ai_translation.x, ai_translation.y, ai_translation.z);
            node->rotation = gfxm::quat(ai_rotation.x, ai_rotation.y, ai_rotation.z, ai_rotation.w);
            node->scale = gfxm::vec3(ai_scale.x, ai_scale.y, ai_scale.z);

            if (ai_node_q.empty()) {
                ai_node = 0;
            } else {
                ai_node = ai_node_q.front();
                ai_node_q.pop();
                node = node_q.front();
                node_q.pop();
            }
        }

        auto root_node = &root;
        root_node->translation = gfxm::vec3(0, 0, 0);
        root_node->rotation = gfxm::quat(0, 0, 0, 1);
        root_node->scale = gfxm::vec3(1, 1, 1);
        for (int i = 0; i < root_node->children.size(); ++i) {
            auto ch = root_node->children[i];
            ch->translation = ch->translation * scale_factor;
            ch->scale = ch->scale * scale_factor;
        }
    }

    // MESHES
    {
        LOG_DBG("ModelImporter: " << ai_scene->mNumMeshes << " meshes");
        for (int im = 0; im < ai_scene->mNumMeshes; ++im) {
            auto ai_mesh = ai_scene->mMeshes[im];
            LOG_DBG("mNumBones: " << ai_mesh->mNumBones);
            if (optimize_single_bone_skin
                && ai_mesh->HasBones()
                && ai_mesh->mNumBones == 1)
            {
                mimpMesh* md = allocMesh();
                readMesh(this, ai_mesh, md);
                assert(md->skin);
                auto pskin = md->skin.get();

                for (int i = 0; i < md->vertices.size(); ++i) {
                    auto& vertex = md->vertices[i];
                    vertex = pskin->inverse_bind_transforms[0] * gfxm::vec4(vertex, 1.0f);
                }
                for (int i = 0; i < md->normals.size(); ++i) {
                    auto& normal = md->normals[i];
                    normal = pskin->inverse_bind_transforms[0] * gfxm::vec4(normal, .0f);
                    normal = gfxm::normalize(normal);
                }
                for (int i = 0; i < md->tangents.size(); ++i) {
                    auto& tan = md->tangents[i];
                    tan = pskin->inverse_bind_transforms[0] * gfxm::vec4(tan, .0f);
                    tan = gfxm::normalize(tan);
                }
                for (int i = 0; i < md->bitangents.size(); ++i) {
                    auto& bitan = md->bitangents[i];
                    bitan = pskin->inverse_bind_transforms[0] * gfxm::vec4(bitan, .0f);
                    bitan = gfxm::normalize(bitan);
                }
                md->skin.reset();
            } else {
                mimpMesh* md = allocMesh();
                readMesh(this, ai_mesh, md);
            }
        }
    }

    // MESH INSTANCES
    {
        auto ai_root = ai_scene->mRootNode;
        aiNode* ai_node = ai_root;
        std::queue<aiNode*> ai_node_q;
        while (ai_node) {
            for (int i = 0; i < ai_node->mNumChildren; ++i) {
                auto ai_child = ai_node->mChildren[i];
                ai_node_q.push(ai_child);
            }

            for (int i = 0; i < ai_node->mNumMeshes; ++i) {
                const int mesh_index = ai_node->mMeshes[i];
                auto ai_mesh = ai_scene->mMeshes[mesh_index];
                if (optimize_single_bone_skin
                    && ai_mesh->HasBones()
                    && ai_mesh->mNumBones == 1)
                {
                    const mimpNode* node = root.findNode(ai_mesh->mBones[0]->mName.C_Str());
                    if(!node) {
                        assert(node);
                        continue;
                    }
                    createMeshInstance(node, mesh_index);
                } else {
                    auto it = node_map.find(ai_node);
                    if (it == node_map.end()) {
                        continue;
                    }
                    mimpNode* node = it->second;
                    createMeshInstance(node, mesh_index);
                }
            }

            if (ai_node_q.empty()) {
                ai_node = 0;
            } else {
                ai_node = ai_node_q.front();
                ai_node_q.pop();
            }
        }
    }

    // ANIMATIONS
    {
        LOG_DBG("ModelImporter: " << ai_scene->mNumAnimations << " animations");
        for (int ianim = 0; ianim < ai_scene->mNumAnimations; ++ianim) {
            aiAnimation* ai_anim = ai_scene->mAnimations[ianim];
            int fstart = 0;
            int fend = (int)ai_anim->mDuration - 1;
            float length = (float)ai_anim->mDuration;
            float fps = (float)ai_anim->mTicksPerSecond;

            LOG_DBG("\t" << ai_anim->mName.C_Str());

            auto& mimp_anim = animations.emplace_back();
            mimp_anim.reset(new mimpAnimation);
            mimp_anim->name = ai_anim->mName.C_Str();
            mimp_anim->clip = ResourceManager::get()->create<Animation>("");
            Animation* anim = mimp_anim->clip.get();

            Animation anim_raw;
            for (int ich = 0; ich < ai_anim->mNumChannels; ++ich) {
                aiNodeAnim* ai_node_anim = ai_anim->mChannels[ich];
                
                float scale_factor_fix = 1.f;
                auto tgt_parent = getRoot()->findNode(ai_node_anim->mNodeName.C_Str())->parent;
                if (tgt_parent == getRoot()) {
                    scale_factor_fix = scale_factor;
                }

                AnimNode& anim_node = anim_raw.createNode(ai_node_anim->mNodeName.C_Str());
                /*AnimNode& anim_node = */anim->createNode(ai_node_anim->mNodeName.C_Str());

                for (int k = 0; k < ai_node_anim->mNumPositionKeys; ++k) {
                    auto& ai_v = ai_node_anim->mPositionKeys[k].mValue;
                    float time = (float)ai_node_anim->mPositionKeys[k].mTime;
                    gfxm::vec3 v = { ai_v.x, ai_v.y, ai_v.z };
                    anim_node.t[time] = v * scale_factor_fix;
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
                    anim_node.s[time] = v * scale_factor_fix;
                }
            }
            
            // Resample into the final anim clip
            // TODO:
            std::vector<AnimSample> samples(anim_raw.nodeCount());
            for (int frame = fstart; frame < fend + 1; ++frame) {
                anim_raw.sample(samples.data(), samples.size(), frame);
                int tgt_frame = frame - fstart;

                for (int node_id = 0; node_id < anim_raw.nodeCount(); ++node_id) {
                    AnimNode* node = anim->getNode(node_id);
                    node->t[tgt_frame] = samples[node_id].t;
                    node->r[tgt_frame] = samples[node_id].r;
                    node->s[tgt_frame] = samples[node_id].s;
                }
            }
            anim->length = fend - fstart + 1;
            anim->fps = fps;
        }
    }

    return true;
}

static gfxm::aabb makeMeshInstanceAabb(mimpMeshInstance* inst) {
    auto node = inst->node;
    gfxm::mat4 tr = node->getWorldTransform();
    auto mesh = inst->mesh;
    gfxm::aabb inst_aabb;
    gfxm::vec3 prime_vtx = tr * gfxm::vec4(mesh->vertices[0], 1.f);
    inst_aabb.from = prime_vtx;
    inst_aabb.to = prime_vtx;
    for(int j = 1; j < mesh->vertices.size(); ++j) {
        gfxm::vec3 vtx = tr * gfxm::vec4(mesh->vertices[j], 1.f);
        gfxm::expand_aabb(inst_aabb, vtx);
    }
    return inst_aabb;
}

bool ModelImporter::load(const std::string& source, float custom_scale_factor, bool use_cache) {
    source_path = source;
    if (use_cache) {
        // TODO:
    }
    if (!loadAssimp(source, custom_scale_factor)) {
        return false;
    }

    // Per mesh bounding boxes
    for (int i = 0; i < meshes.size(); ++i) {
        auto mesh = meshes[i].get();
        mesh->aabb.from = mesh->vertices[0];
        mesh->aabb.to = mesh->vertices[0];
        for(int j = 1; j < mesh->vertices.size(); ++j) {
            gfxm::expand_aabb(aabb, mesh->vertices[j]);
        }
    }

    // Model bounding box
    if(mesh_instances.size() > 0) {
        this->aabb = makeMeshInstanceAabb(mesh_instances[0].get());
        for (int i = 1; i < mesh_instances.size(); ++i) {
            auto minst = mesh_instances[i].get();
            gfxm::aabb new_box = gfxm::aabb_union(this->aabb, makeMeshInstanceAabb(minst));
            this->aabb = new_box;
        }
    }

    // Per mesh bounding spheres
    // Ritter's algorithm, the less accurate one. TODO: Welzl's algorithm maybe?
    // NOTE: bounds for individual meshes seem useless at the moment,
    // but might come in handy in the future
    for (int i = 0; i < meshes.size(); ++i) {
        auto mesh = meshes[i].get();

        gfxm::vec3 x = mesh->vertices[0];
        gfxm::vec3 y = mesh->vertices[0];
        for (int i = 1; i < mesh->vertices.size(); ++i) {
            if ((mesh->vertices[i] - x).length2() > (y - x).length2()) {
                y = mesh->vertices[i];
            }
        }

        gfxm::vec3 z = y;
        for (int i = 0; i < mesh->vertices.size(); ++i) {
            if ((mesh->vertices[i] - y).length2() > (z - y).length2()) {
                z = mesh->vertices[i];
            }
        }

        gfxm::vec3 origin = (y + z) * 0.5f;
        float radius = gfxm::length(z - y) * .5f;
        for (int i = 0; i < mesh->vertices.size(); ++i) {
            float dist = gfxm::length(mesh->vertices[i] - origin);
            if (dist > radius) {
                float new_radius = (radius + dist) * 0.5f;
                float k = (new_radius - radius) / dist;
                origin = origin + (mesh->vertices[i] - origin) * k;
                radius = new_radius;
            }
        }
        mesh->bounding_radius = radius;
        mesh->bounding_sphere_pos = origin;
    }

    // Model's bounding sphere
    // Ritter's algorithm, the less accurate one. TODO: Welzl's algorithm maybe?
    if(!mesh_instances.empty()) {
        gfxm::vec3 x = mesh_instances[0]->node->getWorldTransform() * gfxm::vec4(mesh_instances[0]->mesh->vertices[0], 1.f);
        gfxm::vec3 y = mesh_instances[0]->node->getWorldTransform() * gfxm::vec4(mesh_instances[0]->mesh->vertices[0], 1.f);

        for(int j = 0; j < mesh_instances.size(); ++j) {
            auto inst = mesh_instances[j].get();
            auto mesh = inst->mesh;
            auto node = inst->node;
            gfxm::mat4 tr = node->getWorldTransform();
            for (int i = 0; i < mesh->vertices.size(); ++i) {
                gfxm::vec3 v = tr * gfxm::vec4(mesh->vertices[i], 1.f);
                if ((v - x).length2() > (y - x).length2()) {
                    y = v;
                }
            }
        }

        gfxm::vec3 z = y;
        for(int j = 0; j < mesh_instances.size(); ++j) {
            auto inst = mesh_instances[j].get();
            auto mesh = inst->mesh;
            auto node = inst->node;
            gfxm::mat4 tr = node->getWorldTransform();
            for (int i = 0; i < mesh->vertices.size(); ++i) {
                gfxm::vec3 v = tr * gfxm::vec4(mesh->vertices[i], 1.f);
                if ((v - y).length2() > (z - y).length2()) {
                    z = v;
                }
            }
        }
        
        gfxm::vec3 origin = (y + z) * 0.5f;
        float radius = gfxm::length(z - y) * .5f;
        for(int j = 0; j < mesh_instances.size(); ++j) {
            auto inst = mesh_instances[j].get();
            auto mesh = inst->mesh;
            auto node = inst->node;
            gfxm::mat4 tr = node->getWorldTransform();
            for (int i = 0; i < mesh->vertices.size(); ++i) {
                gfxm::vec3 v = tr * gfxm::vec4(mesh->vertices[i], 1.f);
                float dist = gfxm::length(v - origin);
                if (dist > radius) {
                    float new_radius = (radius + dist) * 0.5f;
                    float k = (new_radius - radius) / dist;
                    origin = origin + (v - origin) * k;
                    radius = new_radius;
                }
            }
        }
        bounding_radius = radius;
        bounding_sphere_origin = origin;
    }

    return true;
}
