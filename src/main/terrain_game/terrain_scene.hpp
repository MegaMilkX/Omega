#pragma once

#include "scene/scene.hpp"
#include "world/common_systems/scene_system.hpp"
#include "world/common_systems/player_start_system.hpp"
#include "collision/phy.hpp"
#include "gpu/gpu.hpp"
#include "gpu/param_block/transform_block.hpp"
#include "m3d/m3d_model.hpp"

// TESTING
#include "skeletal_model/skeletal_model.hpp"
// TESTING


class TerrainScene : public IScene, public IVisibilityProvider {
    static constexpr int NSECTORS_X = 10;
    static constexpr int NSECTORS_Z = 10;
    static constexpr float SECTOR_WIDTH = 204.8f;
    static constexpr float SECTOR_DEPTH = 204.8f;

    ResourceRef<gpuMaterial> terrain_material;

    struct Sector {
        gpuMesh terrain_mesh;
        gpuRenderable terrain_renderable;
        phyHeightfieldShape heightfield_shape;
        phyRigidBody terrain_body;
        gfxm::aabb bounding_box;

        // Decorations
        ResourceRef<m3dModel> deco_model;

        std::vector<gfxm::vec4> deco_positions;
        gpuBuffer           deco_inst_pos_buffer;
        gpuBuffer           deco_inst_quat_buffer;
        gpuInstancingDesc   deco_instancing_desc;
        std::vector<std::unique_ptr<gpuGeometryRenderable>> deco_renderables;
        std::vector<gpuTransformBlock*> deco_transform_blocks;
        std::vector<ResourceRef<gpuMaterial>> materials_instancing;
    };
    std::vector<std::unique_ptr<Sector>> sectors;
    /*
    gpuMesh water_mesh;
    ResourceRef<gpuMaterial> water_material;
    std::unique_ptr<gpuGeometryRenderable> water_renderable;
    */
    // TESTING
    ResourceRef<SkeletalModel> model;
    RHSHARED<SkeletalModelInstance> model_instance;
    // TESTING

    std::set<SceneProxy*> proxies;

    void makeSector(
        Sector& sector, ktImage& img,
        const gfxm::vec2& size, const gfxm::vec2& offset,
        const gfxm::vec2& img_min, const gfxm::vec2& img_max
    );

public:
    void onSpawnScene(IWorld& world) override;
    void onDespawnScene(IWorld& world) override;

    void onAddProxy(VisibilityProxyItem*) override;
    void onRemoveProxy(VisibilityProxyItem*) override;
    void updateProxies(VisibilityProxyItem* items, int count) override;
    void collectVisible(const VisibilityQuery& query, gpuRenderBucket* bucket) override;

    bool load(const std::string& path) override;
};



