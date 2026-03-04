#pragma once

#include "scene/scene.hpp"
#include "hl2_mdl.hpp"
#include "gpu/gpu_mesh.hpp"
#include "gpu/gpu_renderable.hpp"
#include "gpu/render_bucket.hpp"
#include "world/world.hpp"
#include "world/actor.hpp"
#include "collision/collision_world.hpp"
#include "collision/shape/triangle_mesh.hpp"
#include "render_scene/render_scene.hpp"
#include "world/common_systems/player_start_system.hpp"


struct hl2BSPPart {
    std::unique_ptr<gpuMesh> mesh;
    //std::unique_ptr<gpuGeometryRenderable> renderable;
    RHSHARED<gpuMaterial> material;
    std::unique_ptr<scnMeshObject> render_object;
    HSHARED<TransformNode> transform_node;

    std::unique_ptr<CollisionTriangleMesh> col_trimesh;
    std::unique_ptr<phyTriangleMeshShape> col_shape;
    std::unique_ptr<phyRigidBody> collider;
};

struct hl2StaticProp {
    std::vector<std::unique_ptr<scnMeshObject>> render_objects;
    HSHARED<TransformNode> transform_node;
};

struct hl2SkyCamera {
    gfxm::vec3 origin;
    gfxm::vec3 angles;
    float scale;
};

struct HL2Scene;
bool hl2LoadBSP(const char* path, HL2Scene* model);

struct HL2Scene : public IScene {
    std::vector<std::unique_ptr<hl2BSPPart>> parts;

    //std::vector<std::unique_ptr<gpuGeometryRenderable>> renderables;
    //std::vector<std::unique_ptr<scnMeshObject>> render_objects;
    std::vector<std::unique_ptr<hl2StaticProp>> static_props;
    
    std::vector<std::unique_ptr<MDLModel>> models;
    std::map<std::string, RHSHARED<gpuMaterial>> static_prop_materials;

    std::vector<std::unique_ptr<phyShape>> collider_shapes;
    std::vector<std::unique_ptr<phyRigidBody>> static_colliders;

    std::vector<std::unique_ptr<Actor>> actors;

    RHSHARED<gpuTexture2d> lm_texture;

    gfxm::vec3 player_origin;
    gfxm::vec3 player_orientation;
    std::vector<gfxm::vec3> info_player_start_array;

    hl2SkyCamera sky_camera;

    bool load(const std::string& path) override {
        return hl2LoadBSP(path.c_str(), this);
    }

    void onSpawnScene(WorldSystemRegistry& reg) override {
        if (auto sys = reg.getSystem<scnRenderScene>()) {
            for (int i = 0; i < parts.size(); ++i) {
                sys->addRenderObject(parts[i]->render_object.get());
            }
            for (int i = 0; i < static_props.size(); ++i) {
                auto& prop = static_props[i];
                for (int j = 0; j < prop->render_objects.size(); ++j) {
                    sys->addRenderObject(prop->render_objects[j].get());
                }
            }
        }
        if (auto sys = reg.getSystem<phyWorld>()) {
            for (int i = 0; i < parts.size(); ++i) {
                sys->addCollider(parts[i]->collider.get());
            }
            for (int i = 0; i < static_colliders.size(); ++i) {
                sys->addCollider(static_colliders[i].get());
            }
        }
        for (int i = 0; i < actors.size(); ++i) {
            reg.spawn(actors[i].get());
        }
        if (auto sys = reg.getSystem<PlayerStartSystem>()) {
            sys->points.clear();
            sys->points.push_back(
                PlayerStartSystem::Location{
                    .origin = player_origin,
                    .rotation = player_orientation
                }
            );
            /*for (int i = 0; i < info_player_start_array.size(); ++i) {
                const auto& ips = info_player_start_array[i];
                sys->points.push_back(
                    PlayerStartSystem::Location{
                        .origin = ips,
                        .rotation = gfxm::vec3(0,0,0)
                    }
                );
            }*/
        } else {
            LOG_ERR("HL2Scene: PlayerStartSystem not found");
        }
    }
    void onDespawnScene(WorldSystemRegistry& reg) override {
        if (auto sys = reg.getSystem<PlayerStartSystem>()) {
            sys->points.clear();
        }
        for (int i = 0; i < actors.size(); ++i) {
            reg.despawn(actors[i].get());
        }
        if (auto sys = reg.getSystem<phyWorld>()) {
            for (int i = 0; i < parts.size(); ++i) {
                sys->removeCollider(parts[i]->collider.get());
            }
            for (int i = 0; i < static_colliders.size(); ++i) {
                sys->removeCollider(static_colliders[i].get());
            }
        }
        if (auto sys = reg.getSystem<scnRenderScene>()) {
            for (int i = 0; i < parts.size(); ++i) {
                sys->removeRenderObject(parts[i]->render_object.get());
            }
            for (int i = 0; i < static_props.size(); ++i) {
                auto& prop = static_props[i];
                for (int j = 0; j < prop->render_objects.size(); ++j) {
                    sys->removeRenderObject(prop->render_objects[i].get());
                }
            }
        }
    }
    /*
    void draw(gpuRenderBucket* bucket) {
        for (int i = 0; i < parts.size(); ++i) {
            bucket->add(parts[i]->renderable.get());
        }
        for (int i = 0; i < renderables.size(); ++i) {
            bucket->add(renderables[i].get());
        }
    }*/
};


