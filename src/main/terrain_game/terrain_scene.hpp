#pragma once

#include "scene/scene.hpp"
#include "world/common_systems/scene_system.hpp"
#include "world/common_systems/player_start_system.hpp"
#include "collision/phy.hpp"
#include "gpu/gpu.hpp"
#include "gpu/param_block/transform_block.hpp"
#include "m3d/m3d_model.hpp"
#include "gpu/default_instancing_desc.hpp"

// TESTING
#include "skeletal_model/skeletal_model.hpp"
// TESTING


class TerrainScene : public IScene, public IVisibilityProvider {
    static constexpr int NSECTORS_X = 10;
    static constexpr int NSECTORS_Z = 10;
    static constexpr float SECTOR_WIDTH = 204.8f;
    static constexpr float SECTOR_DEPTH = 204.8f;
    const float MAX_DEPTH = 60.f;
    const int SEGMENTS_W = 200;
    const int SEGMENTS_H = 200;
    float CELL_W = 0;
    float CELL_H = 0;

    ktImage img_heightmap;
    ktImage img_slopemap;
    ktImage img_inv_slopemap;
    ktImage img_watermask;
    ktImage img_shoremap;
    ktImage img_forestmap;

    ResourceRef<gpuMaterial> terrain_material;

    struct Decoration {
        ResourceRef<m3dModel> model;

        struct Subset {
            gpuDefaultInstancingDesc inst_desc;
            std::vector<std::unique_ptr<gpuRenderable>> renderables;
            std::vector<gpuTransformBlock*> transform_blocks;
            int instance_count = 0;
        };
        std::vector<std::unique_ptr<Subset>> subsets;
    };
    struct Sector {
        gpuMesh terrain_mesh;
        gpuRenderable terrain_renderable;
        phyHeightfieldShape heightfield_shape;
        phyRigidBody terrain_body;
        gfxm::aabb bounding_box;
        gfxm::vec2 img_min;
        gfxm::vec2 img_max;
        gfxm::vec2 offset;

        // Decorations
        std::vector<std::unique_ptr<Decoration>> decorations;
    };
    std::vector<std::unique_ptr<Sector>> sectors;
    
    struct DistribData {
        std::vector<gfxm::vec4> pos;
        std::vector<gfxm::quat> quat;
        int count = 0;
    };

    gpuMesh water_mesh;
    ResourceRef<gpuMaterial> water_material;
    std::unique_ptr<gpuGeometryRenderable> water_renderable;
    
    // TESTING
    ResourceRef<SkeletalModel> model;
    RHSHARED<SkeletalModelInstance> model_instance;
    // TESTING

    std::set<SceneProxy*> proxies;

    void makeSector(
        Sector& sector, const gfxm::vec2& offset,
        const gfxm::vec2& img_min, const gfxm::vec2& img_max
    );
    void makeDistribution(
        Sector& sector, ktImage& img, ktImage& img_slopemap,
        DistribData& out, float scale, int budget, float threshold = .0f
    );
    void makeDistributionSimple(
        Sector& sector, ktImage& img, ktImage& img_slopemap,
        DistribData& out, float scale, int budget, float threshold = .0f
    );
    void addDecorations(
        Sector& sector, const DistribData& distrib,
        ResourceRef<m3dModel> m3d
    );

public:
    void onSpawnScene(IWorld& world) override;
    void onDespawnScene(IWorld& world) override;

    void onAddProxy(VisibilityProxyItem*) override;
    void onRemoveProxy(VisibilityProxyItem*) override;
    void updateProxies(VisibilityProxyItem* items, int count) override;
    void query(const GeometryQuery& q);
    void collectVisible(const VisibilityQuery& query, gpuRenderBucket* bucket) override;

    bool load(const std::string& path) override;
};



