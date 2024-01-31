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
static auto readMeshData = [](Skeleton* skl, const aiMesh* ai_mesh, MeshData* out) {
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

    if (ai_mesh->GetNumUVChannels() == 0) {
        out->uvs = std::vector<gfxm::vec2>(ai_mesh->mNumVertices);
    } else {
        out->uvs.resize(ai_mesh->mNumVertices);
        for (int iv = 0; iv < ai_mesh->mNumVertices; ++iv) {
            auto ai_uv = ai_mesh->mTextureCoords[0][iv];
            out->uvs[iv] = gfxm::vec2(ai_uv.x, ai_uv.y);
        }
    }
    for (int iv = 0; iv < ai_mesh->mNumVertices; ++iv) {
        auto ai_vert = ai_mesh->mVertices[iv];
        auto ai_norm = ai_mesh->mNormals[iv];
        out->vertices.push_back(gfxm::vec3(ai_vert.x, ai_vert.y, ai_vert.z));
        out->normals.push_back(gfxm::vec3(ai_norm.x, ai_norm.y, ai_norm.z));
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


assimpImporter::assimpImporter()
: skeleton(HANDLE_MGR<Skeleton>::acquire()) {

}
assimpImporter::~assimpImporter() {
    if (ai_scene) {
        aiReleaseImport(ai_scene);
    }
}
bool assimpImporter::loadFile(const char* fname, float customScaleFactor) {
    LOG("Importing model " << fname << "...");;

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
        LOG_WARN("Failed to load " << fname);
        return false;
    }

    ai_scene = aiApplyPostProcessing(ai_scene, aiProcess_CalcTangentSpace);
    if (!ai_scene) {
        LOG_WARN("aiApplyPostProcessing failed");
        return false;
    }

    if (customScaleFactor == .0f) {
        fbxScaleFactor = 1.0f;
        if (ai_scene->mMetaData && ai_scene->mMetaData->Get("UnitScaleFactor", fbxScaleFactor)) {
            if (fbxScaleFactor == .0) fbxScaleFactor = 1.0;
            fbxScaleFactor *= .01;
        }
    } else {
        fbxScaleFactor = customScaleFactor;
    }

    auto ai_root = ai_scene->mRootNode;
    // SKELETON
    {
        aiNode* ai_node = ai_root;
        std::queue<aiNode*> ai_node_q;
        sklBone* skl_bone = skeleton->getRoot();
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
        skeleton->getRoot()->setTranslation(gfxm::vec3(0, 0, 0));
        skeleton->getRoot()->setRotation(gfxm::quat(0, 0, 0, 1));
        skeleton->getRoot()->setScale(gfxm::vec3(1, 1, 1));
        for (int i = 0; i < skeleton->getRoot()->childCount(); ++i) {
            auto bone = skeleton->getRoot()->getChild(i);
            bone->setTranslation(bone->getLclTranslation() * fbxScaleFactor);
            bone->setScale(bone->getLclScale() * fbxScaleFactor);
        }
    }

    return true;
}

#include "config.hpp"
bool assimpImporter::loadSkeletalModel(mdlSkeletalModelMaster* sklm, assimpLoadedResources* resources) {
    assert(ai_scene);
    if (!ai_scene) {
        return false;
    }

    sklm->setSkeleton(skeleton);
    
    assimpLoadedResources local_resources;
    if (!resources) {
        resources = &local_resources;
    }

    auto ai_root = ai_scene->mRootNode;

    // SKELETAL MODEL
    struct MeshObject {
        std::string         name;
        int                 bone_index;
        HSHARED<gpuMesh>    mesh;
        int                 material_id;
    };
    struct SkinObject {
        std::string             name;
        std::vector<int>        bone_indices;
        std::vector<gfxm::mat4> inv_bind_transforms;
        HSHARED<gpuMesh>        mesh;
        int                     material_id;
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
                readMeshData(skeleton.get(), ai_mesh, &md);
                for (int i = 0; i < md.vertices.size(); ++i) {
                    auto& vertex = md.vertices[i];
                    vertex = md.inverse_bind_transforms[0] * gfxm::vec4(vertex, 1.0f);
                }
                for (int i = 0; i < md.normals.size(); ++i) {
                    auto& normal = md.normals[i];
                    normal = md.inverse_bind_transforms[0] * gfxm::vec4(normal, .0f);
                    normal = gfxm::normalize(normal);
                }
                // TODO: tangent and binormal
                // Treat as non-skinned mesh
                MeshObject m;
                m.name = ai_mesh->mName.C_Str();
                m.bone_index = md.bone_transform_source_indices[0];
                m.mesh.reset(HANDLE_MGR<gpuMesh>().acquire());
                md.toGpuMesh(m.mesh.get(), false);
                m.material_id = ai_mesh->mMaterialIndex;
                mesh_objects.push_back(m);
            } else {
                // Process skinned mesh
                MeshData md;
                readMeshData(skeleton.get(), ai_mesh, &md);
                
                SkinObject m;
                m.name = ai_mesh->mName.C_Str();
                m.mesh.reset(HANDLE_MGR<gpuMesh>().acquire());
                md.toGpuMesh(m.mesh.get(), true);
                m.bone_indices = md.bone_transform_source_indices;
                m.inv_bind_transforms = md.inverse_bind_transforms;
                m.material_id = ai_mesh->mMaterialIndex;
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
                readMeshData(skeleton.get(), ai_mesh, &md);
                MeshObject m;
                m.name = ai_mesh->mName.C_Str();
                m.bone_index = skeleton->findBone(ai_node->mName.C_Str())->getIndex();
                m.mesh.reset(HANDLE_MGR<gpuMesh>().acquire());
                md.toGpuMesh(m.mesh.get(), false);
                m.material_id = ai_mesh->mMaterialIndex;
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

    // MATERIALS
    {
        resources->materials.resize(ai_scene->mNumMaterials);
        resources->material_names.resize(ai_scene->mNumMaterials);
        for (int i = 0; i < ai_scene->mNumMaterials; ++i) {
            auto ai_mat = ai_scene->mMaterials[i];
            auto& hmat = resources->materials[i];
            hmat.reset(HANDLE_MGR<gpuMaterial>().acquire());
            {
                auto tech = hmat->addTechnique("Normal");
                auto pass = tech->addPass();
                pass->setShader(resGet<gpuShaderProgram>(build_config::default_import_shader));
            }
            {
                auto tech = hmat->addTechnique("ShadowCubeMap");
                auto pass = tech->addPass();
                pass->setShader(resGet<gpuShaderProgram>("shaders/shadowmap.glsl"));
                pass->cull_faces = true;
            }
            hmat->compile();

            resources->material_names[i] = ai_mat->GetName().C_Str();
        }
    }

    for (int i = 0; i < mesh_objects.size(); ++i) {
        auto& m = mesh_objects[i];
        auto skl_bone = skeleton->getBone(m.bone_index);
        auto c = sklm->addComponent<sklmMeshComponent>(m.name.c_str());
        c->bone_name = skl_bone->getName();
        c->mesh = m.mesh;
        c->material = resources->materials[m.material_id];
    }
    for (int i = 0; i < skin_objects.size(); ++i) {
        auto& m = skin_objects[i];
        auto c = sklm->addComponent<sklmSkinComponent>(m.name.c_str());
        c->inv_bind_transforms = m.inv_bind_transforms;
        for (int j = 0; j < m.bone_indices.size(); ++j) {
            auto bone_index = m.bone_indices[j];
            auto skl_bone = skeleton->getBone(bone_index);
            c->bone_names.push_back(skl_bone->getName());
            c->mesh = m.mesh;
            c->material = resources->materials[m.material_id];
        }
    }

    return true;
}

#include "animation/animation_sample_buffer.hpp"
#include "animation/animation_sampler.hpp"
bool assimpImporter::loadAnimation(Animation* anim, const char* track_name, int frame_start, int frame_end, const char* root_motion_bone) {
    assert(ai_scene);
    if (!ai_scene || !ai_scene->mNumAnimations) {
        return false;
    }

    aiAnimation* ai_anim = ai_scene->mAnimations[0];
    if (track_name != nullptr) {
        for (int i = 0; i < ai_scene->mNumAnimations; ++i) {
            aiAnimation* a = ai_scene->mAnimations[i];
            if (strcmp(a->mName.C_Str(), track_name) == 0) {
                ai_anim = a;
            }
        }
    }

    int fstart = frame_start;
    int fend = (int)ai_anim->mDuration - 1;
    if (frame_end >= 0) {
        fend = frame_end;
    }

    Animation anim_raw;
    anim_raw.length = (float)ai_anim->mDuration;
    anim_raw.fps = (float)ai_anim->mTicksPerSecond;
    for (int j = 0; j < ai_anim->mNumChannels; ++j) {
        aiNodeAnim* ai_node_anim = ai_anim->mChannels[j];

        AnimNode& anim_node = anim_raw.createNode(ai_node_anim->mNodeName.C_Str());
        anim->createNode(ai_node_anim->mNodeName.C_Str());
        float scaleFactorFix = 1.0f;
        sklBone* target_parent = skeleton->findBone(ai_node_anim->mNodeName.C_Str())->getParent();
        if (target_parent == skeleton->getRoot()) {
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
    anim->fps = (float)ai_anim->mTicksPerSecond;
    
    if (root_motion_bone) {
        sklBone* bone = skeleton->findBone(root_motion_bone);
        assert(bone);
        AnimNode rm_node;

        animSampleBuffer sampleBuffer;
        animSampler      sampler;
        HSHARED<SkeletonPose> skl_inst = skeleton->createInstance();
        sampleBuffer.init(skeleton.get());
        sampler = animSampler(skeleton.get(), anim);
        rm_node.t[0] = gfxm::vec3(0,0,0);
        rm_node.r[0] = gfxm::quat(0,0,0,1);
        rm_node.s[0] = gfxm::vec3(0, 0, 0);

        gfxm::quat q_prev = gfxm::quat(0, 0, 0, 1);
        gfxm::vec3 t_world_prev = gfxm::vec3(0, 0, 0);
        gfxm::vec3 t_prev = gfxm::vec3(0, 0, 0);
        for (int i = 0; i < anim->length; ++i) {
            sampler.sample(sampleBuffer.data(), sampleBuffer.count(), i);
            sampleBuffer.applySamples(skl_inst.get());
            skl_inst->calcWorldTransforms();

            gfxm::vec3 t = skl_inst->getWorldTransformsPtr()[bone->getIndex()] * gfxm::vec4(0, 0, 0, 1);
            gfxm::quat r = gfxm::to_quat(gfxm::to_orient_mat3(skl_inst->getWorldTransformsPtr()[bone->getIndex()]));

            
            gfxm::mat4 m = gfxm::to_mat4(r);
            gfxm::vec3 up = gfxm::vec3(0, 1, 0);
            gfxm::vec3 fwd = m[2];
            gfxm::vec3 left = gfxm::cross(up, fwd);
            gfxm::mat3 orient(left, up, fwd);
            gfxm::quat q = gfxm::to_quat(orient);
            rm_node.r[i] = q;

            gfxm::vec3 t_world = t;
            gfxm::vec3 t_world_diff = t_world - t_world_prev;
            gfxm::vec3 t_lcl_diff = gfxm::to_mat4(gfxm::inverse(q_prev)) * gfxm::vec4(t_world_diff, .0f);
            t = t_prev + t_lcl_diff;
            rm_node.t[i] = gfxm::vec4(t, .0f);
            
            q_prev = q;
            t_world_prev = t_world;
            t_prev = t;
        }

        anim->addRootMotionNode(rm_node);
        auto source_node = anim->getNode(root_motion_bone);
        source_node->t.get_keyframes().resize(1);
        source_node->t.get_keyframes()[0] = gfxm::vec3(0, 0, 0); // TODO: Make this optional
        source_node->r.get_keyframes().resize(1);
    }

    return true;
}

static gfxm::mat4 calcNodeWorldTransform(aiNode* ai_node, float scaleFactor) {
    aiVector3D ai_translation;
    aiQuaternion ai_rotation;
    aiVector3D ai_scale;
    ai_node->mTransformation.Decompose(ai_scale, ai_rotation, ai_translation);
    gfxm::vec3 translation(ai_translation.x, ai_translation.y, ai_translation.z);
    gfxm::quat rotation(ai_rotation.x, ai_rotation.y, ai_rotation.z, ai_rotation.w);
    gfxm::vec3 scale(ai_scale.x, ai_scale.y, ai_scale.z);
    gfxm::mat4 lcl_transform 
        = gfxm::translate(gfxm::mat4(1.0f), translation)
        * gfxm::to_mat4(rotation)
        * gfxm::scale(gfxm::mat4(1.0f), scale);
    if (ai_node->mParent == 0) {
        lcl_transform = gfxm::scale(gfxm::mat4(1.0f), gfxm::vec3(scaleFactor, scaleFactor, scaleFactor));
    }
    if (ai_node->mParent) {
        return calcNodeWorldTransform(ai_node->mParent, scaleFactor) * lcl_transform;
    } else {
        return lcl_transform;
    }
}

bool assimpImporter::loadCollisionTriangleMesh(CollisionTriangleMesh* trimesh) {
    assert(ai_scene);
    if (!ai_scene) {
        return false;
    }

    std::vector<gfxm::vec3> vertices;
    std::vector<uint32_t> indices;
    uint32_t base_index = 0;

    auto ai_root = ai_scene->mRootNode;

    aiNode* ai_node = ai_root;
    std::queue<aiNode*> ai_node_q;
    while (ai_node) {
        for (int i = 0; i < ai_node->mNumChildren; ++i) {
            auto ai_child = ai_node->mChildren[i];
            ai_node_q.push(ai_child);;
        }

        gfxm::mat4 transform = gfxm::mat4(1.0f);
        if (ai_node->mNumMeshes > 0) {
            transform = calcNodeWorldTransform(ai_node, (float)fbxScaleFactor);
        }
        int num_degenerate_faces = 0;
        for (int i = 0; i < ai_node->mNumMeshes; ++i) {
            auto ai_mesh_id = ai_node->mMeshes[i];
            auto ai_mesh = ai_scene->mMeshes[ai_mesh_id];

            for (int j = 0; j < ai_mesh->mNumVertices; ++j) {
                gfxm::vec3 v = transform * gfxm::vec4(ai_mesh->mVertices[j].x, ai_mesh->mVertices[j].y, ai_mesh->mVertices[j].z, 1.f);
                vertices.push_back(v);
            }            
            for (int j = 0; j < ai_mesh->mNumFaces; ++j) {
                auto& face = ai_mesh->mFaces[j];
                assert(face.mNumIndices == 3);
                gfxm::vec3 p0 = vertices[base_index + face.mIndices[0]];
                gfxm::vec3 p1 = vertices[base_index + face.mIndices[1]];
                gfxm::vec3 p2 = vertices[base_index + face.mIndices[2]];
                gfxm::vec3 c = gfxm::cross(p1 - p0, p2 - p0);
                float d = c.length();
                if (d <= FLT_EPSILON) {
                    ++num_degenerate_faces;
                    continue;
                }
                indices.push_back(base_index + face.mIndices[0]);
                indices.push_back(base_index + face.mIndices[1]);
                indices.push_back(base_index + face.mIndices[2]);
            }
            base_index += ai_mesh->mNumVertices;
        }
        if (num_degenerate_faces) {
            LOG_WARN(num_degenerate_faces << " degenerate triangles discarded");
        }

        if (ai_node_q.empty()) {
            ai_node = 0;
        } else {
            ai_node = ai_node_q.front();
            ai_node_q.pop();
        }
    }

    trimesh->setData(vertices.data(), vertices.size(), indices.data(), indices.size());
}