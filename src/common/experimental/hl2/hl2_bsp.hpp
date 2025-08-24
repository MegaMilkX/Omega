#pragma once

#include "hl2_mdl.hpp"
#include "gpu/gpu_mesh.hpp"
#include "gpu/gpu_renderable.hpp"
#include "gpu/render_bucket.hpp"
#include "world/world.hpp"
#include "world/actor.hpp"
#include "collision/collision_world.hpp"


struct hl2BSPPart {
    std::unique_ptr<gpuMesh> mesh;
    std::unique_ptr<gpuGeometryRenderable> renderable;
    RHSHARED<gpuMaterial> material;

    std::unique_ptr<CollisionTriangleMesh> col_trimesh;
    std::unique_ptr<CollisionTriangleMeshShape> col_shape;
    std::unique_ptr<Collider> collider;
};

struct hl2SkyCamera {
    gfxm::vec3 origin;
    gfxm::vec3 angles;
    float scale;
};

struct hl2Scene {
    std::vector<std::unique_ptr<hl2BSPPart>> parts;

    std::vector<std::unique_ptr<gpuGeometryRenderable>> renderables;
    std::vector<std::unique_ptr<MDLModel>> models;
    std::map<std::string, RHSHARED<gpuMaterial>> static_prop_materials;

    std::vector<std::unique_ptr<CollisionShape>> collider_shapes;
    std::vector<std::unique_ptr<Collider>> static_colliders;

    std::vector<std::unique_ptr<Actor>> actors;

    RHSHARED<gpuTexture2d> lm_texture;

    gfxm::vec3 player_origin;
    gfxm::vec3 player_orientation;
    std::vector<gfxm::vec3> info_player_start_array;

    hl2SkyCamera sky_camera;

    void draw(gpuRenderBucket* bucket) {
        for (int i = 0; i < parts.size(); ++i) {
            bucket->add(parts[i]->renderable.get());
        }
        for (int i = 0; i < renderables.size(); ++i) {
            bucket->add(renderables[i].get());
        }
    }
    void addCollisionShapes(CollisionWorld* world) {
        for (int i = 0; i < parts.size(); ++i) {
            world->addCollider(parts[i]->collider.get());
        }
        for (int i = 0; i < static_colliders.size(); ++i) {
            world->addCollider(static_colliders[i].get());
        }
    }
    void spawnActors(RuntimeWorld* world) {
        for (int i = 0; i < actors.size(); ++i) {
            world->spawnActor(actors[i].get());
        }
    }
};

bool hl2LoadBSP(const char* path, hl2Scene* model);

