#pragma once

#include <assert.h>
#include <string>
#include <vector>
#include <memory>
#include "math/gfxm.hpp"
#include "gpu/gpu_texture_2d.hpp"
#include "animation/animation.hpp"


struct mimpNode {
    int32_t index = -1;
    std::string name;
    mimpNode* parent = nullptr;
    std::vector<mimpNode*> children;
    gfxm::vec3 translation;
    gfxm::quat rotation;
    gfxm::vec3 scale;

    gfxm::mat4 getLocalTransform() const {
        return gfxm::translate(gfxm::mat4(1.f), translation)
            * gfxm::to_mat4(rotation)
            * gfxm::scale(gfxm::mat4(1.f), scale);
    }
    gfxm::mat4 getWorldTransform() const {
        if (parent) {
            return parent->getWorldTransform() * getLocalTransform();
        } else {
            return getLocalTransform();
        }
    }

    const mimpNode* findNode(const std::string& name) const {
        if (this->name == name) {
            return this;
        }
        for (int i = 0; i < children.size(); ++i) {
            auto node = children[i]->findNode(name);
            if (node) {
                return node;
            }
        }
        return nullptr;
    }
};

struct mimpMaterial {
    int32_t index = -1;
    std::string name;
    RHSHARED<gpuTexture2d> albedo;
    RHSHARED<gpuTexture2d> normalmap;
    RHSHARED<gpuTexture2d> roughness;
    RHSHARED<gpuTexture2d> metallic;
    RHSHARED<gpuTexture2d> emission;
};

struct mimpSkin {
    std::vector<gfxm::ivec4>    bone_indices;
    std::vector<gfxm::vec4>     bone_weights;
    std::vector<gfxm::mat4>     inverse_bind_transforms;
    std::vector<gfxm::mat4>     pose_transforms; // TODO: What's this? Pose at time of import? Useless and redundant if so, remove
    std::vector<int>            bone_transform_source_indices;
};

struct mimpMesh {
    int32_t                     index = -1;
    const mimpMaterial*         material = nullptr;
    gfxm::aabb                  aabb;
    gfxm::vec3                  bounding_sphere_pos;
    float                       bounding_radius;

    std::vector<uint32_t>       indices;

    std::vector<gfxm::vec3>     vertices;
    std::vector<std::vector<uint32_t>> rgba_channels;
    std::vector<std::vector<gfxm::vec2>> uv_channels;
    std::vector<gfxm::vec3>     normals;
    std::vector<gfxm::vec3>     tangents;
    std::vector<gfxm::vec3>     bitangents;

    std::unique_ptr<mimpSkin>   skin;
};

struct mimpMeshInstance {
    const mimpNode* node = nullptr;
    mimpMesh* mesh = nullptr;
};

struct mimpAnimation {
    std::string name;
    ResourceRef<Animation> clip;
};

class ModelImporter {
    std::string source_path;
    std::vector<std::unique_ptr<mimpNode>> nodes;
    std::vector<std::unique_ptr<mimpMaterial>> materials;
    std::vector<std::unique_ptr<mimpMesh>> meshes;
    std::vector<std::unique_ptr<mimpMeshInstance>> mesh_instances;
    std::vector<std::unique_ptr<mimpAnimation>> animations;
    std::vector<RHSHARED<gpuTexture2d>> embedded_textures;
    std::map<std::string, int> embedded_texture_map;
    mimpNode root;
    gfxm::aabb aabb;
    gfxm::vec3 bounding_sphere_origin;
    float bounding_radius = .0f;

    mimpNode* allocNode(const std::string& name);
    mimpMesh* allocMesh();
    mimpNode* createChild(mimpNode* parent, const std::string& name);
    mimpMeshInstance* createMeshInstance(const mimpNode* node, int mesh_idx);
    RHSHARED<gpuTexture2d> findEmbeddedTexture(const std::string& name);
    bool loadAssimp(const std::string& source, float custom_scale_factor = .0f);
public:
    bool optimize_single_bone_skin = true;

    bool load(const std::string& source, float custom_scale_factor = .0f, bool use_cache = true);

    const mimpNode* getRoot() const { return &root; }
    const mimpNode* getNode(int i) const { return nodes[i].get(); }
    size_t              materialCount() const { return materials.size(); }
    const mimpMaterial* getMaterial(int i) const {
        if (i < 0 || i > materials.size()) {
            assert(false);
            return nullptr;
        }
        return materials[i].get();
    }
    size_t meshCount() const { return meshes.size(); }
    const mimpMesh* getMesh(int i) const { return meshes[i].get(); }
    size_t meshInstanceCount() const { return mesh_instances.size(); }
    const mimpMeshInstance* getMeshInstance(int i) const { return mesh_instances[i].get(); }
    size_t animCount() const { return animations.size(); }
    mimpAnimation* getAnimation(int i) const { return animations[i].get(); }

    const gfxm::aabb& getBoundingBox() const { return aabb; }
    const gfxm::vec3& getBoundingSphereOrigin() const { return bounding_sphere_origin; }
    float getBoundingRadius() const { return bounding_radius; }

    const std::string& getSourcePath() const { return source_path; }
};